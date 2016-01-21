/*************************************************************************************
 
	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2015 Janus Lynggaard Thorborg [LightBridge Studios]
	 
	 This program is free software: you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation, either version 3 of the License, or
	 (at your option) any later version.
	 
	 This program is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.
	 
	 You should have received a copy of the GNU General Public License
	 along with this program.  If not, see <http://www.gnu.org/licenses/>.
	 
	 See \licenses\ for additional details on licenses associated with this program.
 
 **************************************************************************************

	file:CAudioStream.h
		
		A bridge system allowing efficient audio data processing between threads that mustn't
		suffer from priority inversion, but still be able to handle locking.
		TODO: Ensure ordering of listener queue (so it works like a FIFO)
			see: tidyListenerQueue
		Other stuff: create a scoped lock that can be returned (the audioBufferLock).

*************************************************************************************/

#ifndef _CAUDIOSTREAM_H
	#define _CAUDIOSTREAM_H
	#include <thread>
	#include "Utility.h"
	#include <vector>
	#include <mutex>
	#include "ConcurrentServices.h"
	#include "lib/BlockingLockFreeQueue.h"
	#include "lib/CLIFOStream.h"
	#include "CProcessorTimer.h"
	#include <deque>
	#include <algorithm>
	#include <numeric>
	#include "Protected.h"
	#define CPL_DEBUG_CAUDIOSTREAM

	namespace cpl
	{
		namespace
		{
			void ASDbg(const char * msg, ...)
			{
				va_list args;
				va_start(args, msg);
				vfprintf(stderr, msg, args);
				va_end(args);
			}
		};
		template<typename T, std::size_t>
			class CAudioStream;

		#ifdef __MSVC__
			#pragma pack(push, 1)
		#endif
		/// <summary>
		/// A simple blob of audio channel data transmitted,
		/// used for transmitting data from real time threads
		/// to worker threads.
		/// </summary>
		template<typename T, std::size_t bufsize>
			PACKED struct AudioPacket
			{
			public:
				AudioPacket(std::uint16_t elementsUsed)
					: size(elementsUsed)
				{
					static_assert(sizeof(AudioPacket<T, bufsize>) == bufsize, "Wrong packing");
				}

				AudioPacket(std::uint16_t elementsUsed, std::uint16_t channelConfiguration)
					: size(elementsUsed), utility((util_t)channelConfiguration)
				{
					static_assert(sizeof(AudioPacket<T, bufsize>) == bufsize, "Wrong packing");
				}

				AudioPacket() : size() {}

				/// <summary>
				/// The total number of samples.
				/// </summary>
				std::uint16_t size;

				/// <summary>
				/// Channel configuration of incoming data
				/// </summary>
				enum util_t : std::uint16_t
				{
					/// <summary>
					/// All samples are mono, and belongs to left
					/// </summary>
					left,
					/// <summary>
					/// Same as mono
					/// </summary>
					mono = left,
					/// <summary>
					/// All samples are mono, and belongs to right
					/// </summary>
					right,
					/// <summary>
					/// Every 2N sample is left, every 2N+1 sample is right
					/// </summary>
					interleaved,
					/// <summary>
					/// First half is left, second half is right
					/// </summary>
					separate
				} utility;

				static std::size_t getChannelCount(util_t channelType) noexcept
				{
					return channelType > right ? 2 : 1;
				}

				static const std::uint32_t element_size = sizeof(T);
				static const std::uint32_t capacity = (bufsize - sizeof(std::uint32_t)) / element_size;

				T * begin() noexcept { return buffer; }
				T * end() noexcept { return buffer + size; }
				T buffer[capacity];
			};
		#ifdef __MSVC__
			#pragma pack(pop)
		#endif

		template<typename T, std::size_t PacketSize = 64>
		class CAudioStream : Utility::CNoncopyable
		{
		public:

			struct PerformanceMeasurements
			{
				PerformanceMeasurements()
				:
					asyncOverhead(0), rtOverhead(0), asyncUsage(0), rtUsage(0), droppedAudioFrames(0)
				{
					
				}
				std::atomic<double> asyncOverhead;
				std::atomic<double> rtOverhead;
				std::atomic<double> asyncUsage;
				std::atomic<double> rtUsage;
				/// <summary>
				/// Returns the number of dropped frames from the audio thread,
				/// due to the FIFO being filled up as the async thread hasn't
				/// catched up (due to being blocked or simply have too much work).
				/// </summary>
				std::atomic<std::uint64_t> droppedAudioFrames;
			};

			/// <summary>
			/// Holds info about the audio stream. Wow.
			/// </summary>
			struct AudioStreamInfo
			{
				// yayyy no copy constructor for atomics. 
				// remember to update this function!
				AudioStreamInfo(const AudioStreamInfo & other)
				{
					*this = other;
				}

				AudioStreamInfo & operator = (const AudioStreamInfo & other) noexcept
				{
					std::atomic_thread_fence(std::memory_order_acquire);

					copyA(sampleRate, other.sampleRate);
					copyA(anticipatedSize, other.anticipatedSize);
					copyA(anticipatedChannels, other.anticipatedChannels);
					copyA(audioHistorySize, other.audioHistorySize);
					copyA(audioHistoryCapacity, other.audioHistoryCapacity);
					copyA(isFrozen, other.isFrozen);
					copyA(isSuspended, other.isSuspended);
					copyA(callAsyncListeners, other.callAsyncListeners);
					copyA(callRTListeners, other.callRTListeners);
					copyA(storeAudioHistory, other.storeAudioHistory);
					copyA(blockOnHistoryBuffer, other.blockOnHistoryBuffer);

					std::atomic_thread_fence(std::memory_order_release);

					return *this;
				}

				AudioStreamInfo()
				{
					std::atomic_thread_fence(std::memory_order_acquire);

					sampleRate.store(0, std::memory_order_relaxed);
					anticipatedSize.store(0, std::memory_order_relaxed);
					anticipatedChannels.store(0, std::memory_order_relaxed);
					audioHistorySize.store(0, std::memory_order_relaxed);
					audioHistoryCapacity.store(0, std::memory_order_relaxed);
					isFrozen.store(0, std::memory_order_relaxed);
					isSuspended.store(0, std::memory_order_relaxed);
					callAsyncListeners.store(0, std::memory_order_relaxed);
					callRTListeners.store(0, std::memory_order_relaxed);
					storeAudioHistory.store(0, std::memory_order_relaxed);
					blockOnHistoryBuffer.store(0, std::memory_order_relaxed);

					std::atomic_thread_fence(std::memory_order_release);

				}

				std::atomic<double> sampleRate;
				
				std::atomic<std::size_t> 
					anticipatedSize,
					anticipatedChannels;

				std::atomic<std::size_t>
					
					audioHistorySize,
					audioHistoryCapacity;

				std::atomic<bool>
					isFrozen,
					isSuspended,
					/// <summary>
					/// if false, removes one mutex from the async subsystem and 
					/// improves performance
					/// See Listener::onAsyncAudio
					/// </summary>
					callAsyncListeners,
					/// <summary>
					/// If false, removes a lot of locking complexity and improves performance.
					/// See Listener::onRTAudio
					/// </summary>
					callRTListeners,
					/// <summary>
					/// If true, stores the last audioHistorySize samples in a circular buffer.
					/// </summary>
					storeAudioHistory,
					/// <summary>
					/// If set, the async subsystem will block on the audio history buffers until
					/// they are released back into the stream - this blocks async audio updates,
					/// listener updates etc. as well
					/// </summary>
					blockOnHistoryBuffer;
			private:
				template<typename X, typename Y>
				void copyA(X & dest, const Y & source)
				{
					dest.store(source.load(std::memory_order_relaxed), std::memory_order_relaxed);
				}
			};

			class Listener;

			static const std::size_t packetSize = PacketSize;
			static const std::size_t storageAlignment = 32;

			typedef AudioPacket<T, PacketSize> AudioFrame;
			// atomic vector of pointers, atomic because we have unsynchronized read-access between threads (UB without atomic access)
			typedef std::vector<std::atomic<Listener *>> ListenerQueue;
			typedef typename cpl::CLIFOStream<T, storageAlignment> AudioBuffer;
			typedef typename cpl::CLIFOStream<T, storageAlignment>::ProxyView AudioBufferView;
			typedef T DataType;
			typedef typename AudioBuffer::IteratorBase::iterator BufferIterator;
			typedef typename AudioBuffer::IteratorBase::const_iterator CBufferIterator;
			typedef CAudioStream<T, PacketSize> StreamType;
			/// <summary>
			/// When you iterate over a audio buffer using ended iterators,
			/// there will be this number of iterator iterations.
			/// </summary>
			static const std::size_t bufferIndices = AudioBuffer::IteratorBase::iterator_indices;


			/// <summary>
			/// Provides a constant view of the internal audio buffers, synchronized.
			/// The interface is built on RAII, the data is valid as long as this struct is in scope.
			/// Same principle for AudioBufferViews.
			/// </summary>
			struct AudioBufferAccess
			{
			public:

				typedef typename AudioBuffer::IteratorBase::iterator iterator;
				typedef typename AudioBuffer::IteratorBase::const_iterator const_iterator;

				AudioBufferAccess(std::mutex & m, const std::vector<AudioBuffer> & audioChannels)
					: lock(m), audioChannels(audioChannels)
				{

				}

				AudioBufferAccess(const AudioBufferAccess &) = delete;
				AudioBufferAccess & operator = (const AudioBufferAccess &) = delete;
				AudioBufferAccess & operator = (AudioBufferAccess &&) = delete;
				AudioBufferAccess(AudioBufferAccess &&) = default;

				AudioBufferView getView(std::size_t channel) const
				{
					return audioChannels.at(channel).createProxyView();
				}

				std::size_t getNumChannels() const noexcept
				{
					return audioChannels.size();
				}

				std::size_t getNumSamples() const noexcept
				{
					return getNumChannels() ? audioChannels[0].getSize() : 0;
				}

				/// <summary>
				/// Iterate over the samples in the buffers. If biased is set, the samples arrive in chronological order.
				/// The functor shall accept the following parameters:
				///		(std::size_t sampleFrame, AudioStream::Data channel0, AudioStream::Data channelN ...)
				/// </summary>
				template<std::size_t Channels, bool biased, typename Functor>
				inline void iterate(const Functor & f)
				{
					ChannelIterator<Channels, biased>::run(*this, f);
				}

				template<std::size_t Channels, bool biased, typename Functor>
				inline void iterate(const Functor & f) const
				{
					ChannelIterator<Channels, biased>::run(*this, f);
				}

			private:

				template<std::size_t channels, bool biased>
					struct ChannelIterator;

				template<>
				struct ChannelIterator<2, true>
				{
					template<typename Functor>
					static void run(AudioBufferAccess & access, const Functor & f)
					{
						StreamType::AudioBufferView views[2] = { access.getView(0), access.getView(1) };

						for (std::size_t indice = 0, n = 0; indice < StreamType::bufferIndices; ++indice)
						{
							BufferIterator
								left = views[0].getItIndex(indice),
								right = views[1].getItIndex(indice);

							std::size_t range = views[0].getItRange(indice);

							while (range--)
							{
								f(n++, *left++, *right++);
							}
						}
					}

					template<typename Functor>
					static void run(const AudioBufferAccess & access, const Functor & f)
					{
						StreamType::AudioBufferView views[2] = { access.getView(0), access.getView(1) };

						for (std::size_t indice = 0, n = 0; indice < StreamType::bufferIndices; ++indice)
						{
							BufferIterator
								left = views[0].getItIndex(indice),
								right = views[1].getItIndex(indice);

							std::size_t range = views[0].getItRange(indice);

							while (range--)
							{
								f(n++, *left++, *right++);
							}
						}
					}
				};

				template<>
				struct ChannelIterator<1, true>
				{
					template<typename Functor>
					static void run(const AudioBufferAccess & access, const Functor & f)
					{
						StreamType::AudioBufferView view = access.getView(0);

						for (std::size_t indice = 0, n = 0; indice < StreamType::bufferIndices; ++indice)
						{
							BufferIterator left = views[0].getItIndex(indice);

							std::size_t range = views[0].getItRange(indice);

							while (range--)
							{
								f(n++, *left++, *right++);
							}
						}
					}
				};

				const std::vector<AudioBuffer> & audioChannels;
				std::unique_lock<std::mutex> lock;
			};

			/// <summary>
			/// A class that enables listening callbacks on both real-time and async audio
			/// channels from a CAudioStream
			/// </summary>
			class Listener : Utility::CNoncopyable
			{
			public:

				typedef CAudioStream<T, PacketSize> Stream;

				Listener()
					: internalSource(nullptr)
				{

				}
				/// <summary>
				/// Called when certain properties are changed in the stream. 
				/// Called from a real-time thread!
				/// </summary>
				virtual void onRTChangedProperties(const Stream & changedSource, const typename Stream::AudioStreamInfo & before) {}
				/// <summary>
				/// Called when certain properties are changed in the stream. 
				/// Called from a non-real time thread!
				/// Be very careful with whatever locks you obtain here, as you can easily deadlock something.
				/// </summary>
				virtual void onAsyncChangedProperties(const Stream & changedSource, const typename Stream::AudioStreamInfo & before) {}
				/// <summary>
				/// Called from a real-time thread.
				/// </summary>
				virtual bool onRTAudio(const Stream & source, T ** buffer, std::size_t numChannels, std::size_t numSamples) { return false; }
				/// <summary>
				/// Called from a non real-time thread.
				/// Be very careful with whatever locks you obtain here, as you can easily deadlock something.
				/// </summary>
				virtual bool onAsyncAudio(const Stream & source, T ** buffer, std::size_t numChannels, std::size_t numSamples) { return false; }
				
				/// <summary>
				/// May fail if the streams listener buffer is temporarily filled.
				/// Callable from any thread (but may block, depending on parameters).
				/// </summary>
				/// <param name="force">
				/// If set, guarantees the listener is added but the operation may block this thread.
				/// </param>
				/// <param name="milisecondsToTryFor">
				/// How long to try to add the listener. If zero, will only try once and is 
				/// guaranteed to be deterministic (if force is false).
				/// </param>
				bool listenToSource(Stream & audioSource, bool force = false, int milisecondsToTryFor = 2000)
				{
					if (internalSource.load(std::memory_order_acquire))
					{
						crash("Already listening to one source!");
					}
					// set the source already now, so we can ensure that internalSource is updated
					// should other threads use this object concurrently before this function is done.
					internalSource.store(&audioSource, std::memory_order_release);

					//ASDbg("Stored 0x%p in 0x%p\n", &audioSource, this);

					auto firstTry = audioSource.addListener(this, force);
					bool tryMore = milisecondsToTryFor != 0;

					auto functor = [&]()
					{
						return audioSource.addListener(this, force).second;
					};

					if (firstTry.second || (tryMore && Misc::WaitOnCondition(milisecondsToTryFor, functor, 100)))
					{
						return true;
					}
					// we didn't succeed, store null.
					internalSource.store(nullptr, std::memory_order_release);
					return false;
				}

				bool isListening() const noexcept
				{
					return internalSource.load(std::memory_order_acquire);
				}

				bool onIncomingRTAudio(Stream & source, T ** buffer, std::size_t numChannels, std::size_t numSamples)
				{
					auto s1 = internalSource.load(std::memory_order_acquire);
					if (!s1)
					{
						crash("Internal audio stream is null");
						return false;
					}
					auto s2 = &source;
					if (s1 != s2)
					{
						crash("Inconsistency between argument CAudioStream and internal source; corrupt listener chain.");
						return false;
					}
					return onRTAudio(source, buffer, numChannels, numSamples);
				}
				void sourceIsDying(Stream & dyingSource)
				{
					internalSource.store(nullptr, std::memory_order_seq_cst);
					onSourceDied(dyingSource);
				}

				/// <summary>
				/// Called when the current source being listened to died.
				/// May be called from any thread.
				/// </summary>
				virtual void onSourceDied(Stream & dyingSource) {};

				/// <summary>
				/// Detaches from currently attached source.
				/// Do not call from real-time threads.
				/// </summary>
				/// <param name="source">Reserved.</param>
				/// <returns></returns>
				bool detachFromSource(Stream * source = nullptr)
				{
					auto s1 = internalSource.load(std::memory_order_acquire);
					if (!source && s1)
					{
						std::pair<bool, bool> res;
						bool success = cpl::Misc::WaitOnCondition(10000, 
							[&]() 
							{ 
								res = s1->removeListener(this);
								return res.first || res.second; // whether the listener was removed.
							}, 
							100 // 100 ms retries
						);
						
						if (!success) // we never succeded in 10 seconds; listener may be present
						{
							crash("Listener not removed after 10 seconds of trying!");
							return true;
						}
						else if (res.first && !res.second) // we succeded looking through listeners, and we weren't present (bug)
						{
							crash("Listener not present in stream!");
							return false;
						}
						else // res.x && res.y == true
						{
							internalSource = nullptr;
							return true;
						}

					}
					return false;
				}

				/// <summary>
				/// Tries to remove the listener for 10 seconds. 
				/// </summary>
				virtual ~Listener()
				{
					detachFromSource();
				}

				void sourcePropertiesChangedRT(const Stream & changedSource, const typename Stream::AudioStreamInfo & before)
				{
					auto s1 = internalSource.load(std::memory_order_acquire);
					if (&changedSource != s1)
						return crash("Inconsistency between argument CAudioStream and internal source; corrupt listener chain.");

					onRTChangedProperties(changedSource, before);
				}

				void sourcePropertiesChangedAsync(const Stream & changedSource, const typename Stream::AudioStreamInfo & before)
				{
					auto s1 = internalSource.load(std::memory_order_acquire);
					if (&changedSource != s1)
						return crash("Inconsistency between argument CAudioStream and internal source; corrupt listener chain.");

					onAsyncChangedProperties(changedSource, before);
				}

				/// <summary>
				/// Tries to gracefully crash and disable this listener.
				/// Implements a breakpoint, if debugged.
				/// Callable from any thread.
				/// </summary>
				/// <param name="why"></param>
				void crash(const char * why)
				{
					ASDbg(why);
					BreakIfDebugged();
					auto s1 = internalSource.load(std::memory_order_acquire);
					if (s1)
						s1->removeListener(this, true);
				}

			private:

				std::atomic<Stream *>  internalSource;
			};

			/// <summary>
			/// The async subsystem enables access to a callback on a background thread, similar
			/// to audio callbacks, however, it is not real-time and can be blocked.
			/// Integrity of audio stream is not guaranteed, especially if you block it for longer times,
			/// however it should run almost as fast and synced as the audio thread, with a minimal overhead.
			/// It subsystem also continuously updates a circular buffer which you can lock.
			/// 
			/// Fifo sizes refer to the buffer size of the lock free fifo. The fifo stores AudioFrames.
			/// </summary>
			/// <param name="enableAsyncSubsystem"></param>
			CAudioStream(std::size_t defaultListenerBankSize = 16, bool enableAsyncSubsystem = false, size_t initialFifoSize = 20, std::size_t maxFifoSize = 1000)
			: 
				audioFifo(initialFifoSize, maxFifoSize), 
				objectIsDead(false)
			{
				auto newListeners = std::make_unique<ListenerQueue>(defaultListenerBankSize);
				if (!audioListeners.tryReplace(newListeners.get()))
				{
					CPL_RUNTIME_EXCEPTION("Unable to initiate listener queue!");
				}
				else
				{
					newListeners.release();
				}

				if (enableAsyncSubsystem)
				{
					asyncAudioThread = std::thread(&CAudioStream::asyncAudioSystem, this);
					asyncAudioThreadCreated.store(true);
					
				}

			}

			/// <summary>
			/// This must be called at least once, before streaming starts.
			/// It is not safe to call this function concurrently
			/// - decide on one thread, controlling it.
			/// </summary>
			void initializeInfo(const AudioStreamInfo & info)
			{
				AudioStreamInfo oldInfo = internalInfo;
				internalInfo = info;
				
				audioSignalChange = true;
				asyncSignalChange = true;
			}

			/// <summary>
			/// Should only be called from the audio thread.
			/// Deterministic ( O(N) ), wait free and lock free, so long as listeners are as well.
			/// </summary>
			/// <returns>True if incoming audio was changed</returns>
			bool processIncomingRTAudio(T ** buffer, std::size_t numChannels, std::size_t numSamples)
			{
				cpl::CProcessorTimer overhead, all;
				overhead.start(); all.start();

				auto const timeFraction = double(numSamples) / internalInfo.sampleRate.load(std::memory_order_relaxed);

				auto id = std::this_thread::get_id();
				if (audioRTThreadID != std::thread::id() && id != audioRTThreadID)
				{
					// so, another realtime thread is calling this.
					// this should be interesting to debug.
					BreakIfDebugged();
					audioRTThreadID = id;
				}
				else
				{
					audioRTThreadID = id;
				}
				// may catch dangling pointers.
				if (objectIsDead)
					CPL_RUNTIME_EXCEPTION("This object was deleted somewhere!!");

				// listeners can only be tidied up in here,
				// as no mutual exclusion occurs further below in this function.
				// we, however, will only try to lock the mutex.
				if (tidyListeners)
				{
					std::unique_lock<std::mutex> lock(aListenerMutex, std::try_to_lock);
					if (lock.owns_lock())
					{
						tidyListenerQueue();
						tidyListeners.cas();
					}
				}


				if (internalInfo.isSuspended || internalInfo.isFrozen)
					return false;

				// publish all data to listeners
				unsigned mask(0);

				bool signalChange = audioSignalChange.cas();

				auto & listeners = *audioListeners.getObject();
				if (internalInfo.callRTListeners)
				{
					overhead.pause();
					for (auto & listener : listeners)
					{
						auto rawListener = listener.load(std::memory_order_acquire);
						if (rawListener)
						{
							if(signalChange)
								rawListener->sourcePropertiesChangedRT(*this, oldInfo);

							// TODO: exception handling?
							mask |= (unsigned)rawListener->onIncomingRTAudio(*this, buffer, numChannels, numSamples);
						}
					}
					overhead.resume();
				}


				// publish all data to audio consumer thread

				// remaining samples
				std::int64_t n = numSamples;

				switch (numChannels)
				{
				case 1:
					while (n > 0)
					{
						auto const aSamples = std::min(std::int64_t(AudioFrame::capacity), n - std::max(std::int64_t(0), n - std::int64_t(AudioFrame::capacity)));

						if (aSamples > 0)
						{
							// TODO: ensure aSamples < std::uint16_T::max()
							AudioFrame af(static_cast<std::uint16_t>(aSamples), AudioFrame::mono);
							std::memcpy(af.buffer, (buffer[0] + numSamples - n), static_cast<std::size_t>(aSamples * AudioFrame::element_size));

							if (!audioFifo.pushElement(af))
								measures.droppedAudioFrames++;
						}

						n -= aSamples;
					}
					break;
				case 2:
					while (n > 0)
					{
						auto const aSamples = std::min(std::int64_t(AudioFrame::capacity / 2), n - std::max(std::int64_t(0), n - std::int64_t(AudioFrame::capacity / 2)));

						if (aSamples > 0)
						{
							// TODO: ensure aSamples < std::uint16_T::max()
							AudioFrame af(static_cast<std::uint16_t>(aSamples * 2), AudioFrame::separate);
							std::memcpy(af.buffer, (buffer[0] + numSamples - n), static_cast<std::size_t>(aSamples * AudioFrame::element_size));
							std::memcpy(af.buffer + aSamples, (buffer[1] + numSamples - n), static_cast<std::size_t>(aSamples * AudioFrame::element_size));
							if (!audioFifo.pushElement(af))
								measures.droppedAudioFrames++;
						}

						n -= aSamples;
					}

					break;
				default:
					// TODO: what to do?
					BreakIfDebugged();
					break;
				}

				// post new measures

				lpFilterTimeToMeasurement(measures.rtOverhead, overhead.clocksToCoreUsage(overhead.getTime()), timeFraction);
				lpFilterTimeToMeasurement(measures.rtUsage, all.clocksToCoreUsage(all.getTime()), timeFraction);

				// return whether any data was changed.
				return mask ? true : false;
			}

			/// <summary>
			/// Returns a view of the audio history for all channels the last N samples (see setAudioHistorySize)
			/// References are only guaranteed to be valid while AudioBufferAccess is in scope.
			/// May acquire a lock in the returned class, so don't call it from real-time threads.
			/// 
			/// Ensures exclusive access while it is hold.
			/// </summary>
			AudioBufferAccess getAudioBufferViews()
			{
				return{ aBufferMutex, audioHistoryBuffers };
			}

			/// <summary>
			/// Safe to call from any thread (wait free).
			/// </summary>
			const AudioStreamInfo & getInfo() const noexcept
			{
				return internalInfo;
			}

			~CAudioStream() throw(...)
			{
				// the audio thread is created inside this flag.
				// and that flag is set by this thread. 
				if (asyncAudioThreadCreated.load())
				{
					// so the thread has been created; wait for it to enter function space.
					cpl::Misc::WaitOnCondition(10000,
						[&]()
						{
							return asyncAudioThreadInitiated.load(std::memory_order_relaxed);
						}
					);

					// TODO: consider 'success'

					// signal the consumer audio thread and join it:
					audioFifo.releaseConsumer();

					// weird error-checking
					if (asyncAudioThread.get_id() != std::thread::id())
					{
						// try to join it. if it isn't joinable at this point, something very bad has happened.
						if (asyncAudioThread.joinable())
							asyncAudioThread.join();
						else
							CPL_RUNTIME_EXCEPTION("CAudioStream's audio thread crashed.");
					}
				}

				// async thread should be gone.
				// just signal the audio entry function that it should blow up in case anything calls it
				// (the user forgot to delete a listener that calls this object whilst asynchrounously deleting it)
				// it may catch some bugs.
				objectIsDead.store(true);

				// signal to listeners we died.
				auto & listeners = *audioListeners.getObject();

				for (auto & listener : listeners)
				{
					auto rawListener = listener.load(std::memory_order_acquire);
					if (rawListener)
					{
						rawListener->sourceIsDying(*this);
					}
				}
			}

			/// <summary>
			/// Safe to call from any thread (wait free).
			/// Only valid if initialize() has been called previously.
			/// </summary>
			bool isAudioThread() noexcept
			{
				return std::this_thread::get_id() == audioRTThreadID;
			}

			/// <summary>
			/// Safe to call from any thread (wait free).
			/// </summary>
			bool isAsyncThread() noexcept
			{
				return std::this_thread::get_id() == asyncAudioThread.get_id();
			}

			/// <summary>
			/// Safe to call from any thread (wait free).
			/// Returns the number of AudioFrames in the async FIFO
			/// </summary>
			std::size_t getASyncBufferSize() const noexcept
			{
				return audioFifo.size();
			}

			/// <summary>
			/// Safe to call from any thread (wait free).
			/// The actual current value, may not equal whatever set through setAudioHistorySize()
			/// </summary>
			std::size_t getAudioHistorySize() const noexcept
			{
				return audioHistoryBuffers.size() ? audioHistoryBuffers[0].getSize() : 0;
			}

			/// <summary>
			/// Safe to call from any thread (wait free).
			/// The actual current value, may not equal whatever set through setAudioHistoryCapacity()
			/// </summary>
			std::size_t getAudioHistoryCapacity() const noexcept
			{
				return audioHistoryBuffers.size() ? audioHistoryBuffers[0].getCapacity() : 0;
			}

			/// <summary>
			/// Safe to call from any thread (wait free), however it may not take effect immediately
			/// </summary>
			void setAudioHistorySize(std::size_t newSize) noexcept
			{
				//OutputDebugString("1. Sat audio history size\n");
				internalInfo.audioHistorySize.store(newSize);
				audioSignalChange = true;
				asyncSignalChange = true;
			}

			/// <summary>
			/// Safe to call from any thread (wait free), however it may not take effect immediately
			/// </summary>
			void setAudioHistoryCapacity(std::size_t newCapacity) noexcept
			{
				internalInfo.audioHistorySize.store(std::min(internalInfo.audioHistorySize.load(), newCapacity));
				internalInfo.audioHistoryCapacity.store(newCapacity);
				audioSignalChange = true;
				asyncSignalChange = true;
			}

			/// <summary>
			/// May block. Ensure both fields are update before submitting.
			/// May still not take effect immediately.
			/// </summary>
			void setAudioHistorySizeAndCapacity(std::size_t newSize, std::size_t newCapacity) noexcept
			{
				std::lock_guard<std::mutex> lk(aBufferMutex);
				internalInfo.audioHistorySize.store(newSize, std::memory_order_relaxed);
				internalInfo.audioHistoryCapacity.store(newCapacity, std::memory_order_relaxed);
				audioSignalChange = true;
				asyncSignalChange = true;
			}

			void setSuspendedState(bool newValue)
			{
				internalInfo.isSuspended.store(!!newValue, std::memory_order_release);
				audioSignalChange = true;
				asyncSignalChange = true;
			}

			const PerformanceMeasurements & getPerfMeasures() const noexcept
			{
				return measures;
			}

			/// <summary>
			/// Returns the current number of async samples that 'has' happened asynchronously, but still haven't been posted
			/// into the audio buffers, since the current AUdioStreamInfo::blockOnHistoryBuffers is set to false and something has blocked the audio buffers meanwhile. 
			/// Safe and wait-free to call from any thread, guaranteed not to change in asynchronous callbacks.
			/// </summary>
			/// <returns></returns>
			std::size_t getNumDeferredSamples() const noexcept
			{
				return numDeferredAsyncSamples.load(std::memory_order_acquire);
			}

		protected:
			template<std::memory_order order = std::memory_order_relaxed>
				inline void lpFilterTimeToMeasurement(std::atomic<double> & old, double newTime, double timeFraction)
				{
					const double coeff = std::pow(0.3, timeFraction);
					newTime /= timeFraction;
					old.store(newTime + coeff * (old.load(order) - newTime), order);
				}

			/// <summary>
			/// Tries to add an audio listener into the stream.
			/// May fail if the internal audio listener buffer is full, 
			/// in which case it will be resized asynchronously. 
			/// Try to call this function at a later point.
			/// Safe to call from any thread, and wait + lockfree as default.
			/// tryForceSuccess claims the mutex and may block, however it can
			/// still fail (much less likely though).
			/// ret.first determines whether lock got acquired.
			/// ret.second determines whether the listener was found and removed.
			/// </summary>
			/// <param name="newListener"></param>
			/// <returns></returns>
			std::pair<bool, bool> addListener(Listener * newListener, bool tryForceSuccess = false)
			{
				std::unique_lock<std::mutex> lock(aListenerMutex, std::defer_lock);

				tryForceSuccess ? lock.lock() : lock.try_lock();

				if (!lock.owns_lock())
					return{ false, false };

				if (insertIntoListenerQueue(newListener))
				{
					return{ true, true };
				}

				else if ( tryForceSuccess)
				{
					return { true, expandListenerQueue() && insertIntoListenerQueue(newListener) };
				}
				else
				{
					// request more listener slots.
					resizeListeners = true;
					return{ true, false };
				}

			}

			/// <summary>
			/// Safe to call from any thread, but may not succeed (it only tries to lock a mutex)
			/// wait + lockfree as default.
			/// ret.first determines whether lock got acquired.
			/// ret.second determines whether the listener was found and removed.
			/// </summary>
			/// <param name=""></param>
			/// <returns></returns>
			std::pair<bool, bool> removeListener(Listener * listenerToBeRemoved, bool forceSuccess = false)
			{

				std::unique_lock<std::mutex> lock(aListenerMutex, std::defer_lock);

				forceSuccess ? lock.lock() : lock.try_lock();

				if (!lock.owns_lock())
					return { false, false };

				return { true, removeFromListenerQueue(listenerToBeRemoved) };

			}

			/// <summary>
			/// Assumes you have appropriate locks!!
			/// </summary>
			bool insertIntoListenerQueue(Listener * newListener)
			{
				auto & listeners = getListeners();

				for (auto & listener : listeners)
				{
					// we have exclusive write-access to the atomic pointer,
					// so we dont need a cas-loop
					if (!listener.load(std::memory_order_relaxed)) // relaxed: noone else can write to it anyway
					{
						listener.store(newListener, std::memory_order_release);
						return true;
					}

				}
				return false;
			}
			/// <summary>
			/// Assumes you have appropriate locks!!
			/// </summary>
			bool removeFromListenerQueue(Listener * listenerToBeRemoved)
			{
				auto & listeners = getListeners();

				for (auto & listener : listeners)
				{
					if (listener.load(std::memory_order_relaxed) == listenerToBeRemoved)
					{
						listener.store(nullptr); // can be relaxed?
						tidyListeners = true;
						return true;
					}
				}

				return false;
			}

			/// <summary>
			/// Gets the current listener list with appropriate signaling mechanisms
			/// based on what thread is calling.
			/// </summary>
			/// <returns></returns>
			ListenerQueue & getListeners()
			{
				if (isAudioThread())
				{
					return *audioListeners.getObject();
				}
				else
				{
					return *audioListeners.getObjectWithoutSignaling();
				}
			}
			/// <summary>
			/// Assumes you have appropriate locks!!
			/// Compresses all listeners such that they are not scattered.
			/// TODO: figure out if this is needed at all
			/// </summary>
			bool tidyListenerQueue()
			{
				auto & listeners = getListeners();
				for (std::size_t i = 0, lastSlot = 0; i < listeners.size(); ++i)
				{
					if (listeners[i].load(std::memory_order_relaxed)) // again, we have exclusive access
					{
						if (lastSlot != i)
						{
							listeners[lastSlot].store(listeners[i], std::memory_order_relaxed);
							listeners[i].store(nullptr, std::memory_order_relaxed);
							lastSlot = i;
						}
						else
						{
							lastSlot = i + 1;
						}

					}
				}
				// fixes all the relaxed stores
				std::atomic_thread_fence(std::memory_order::memory_order_release);
				return false;
			}
			/// <summary>
			/// Assumes you have appropriate locks!!
			/// Non-deterministic
			/// </summary>
			bool expandListenerQueue()
			{
				const std::size_t listenerIncreaseFactor = 2;

				auto & curListeners = getListeners();
				// create a larger object, and ensure its scope
				auto newListeners = std::make_unique<ListenerQueue>(curListeners.size() * listenerIncreaseFactor);
				// cant use std::copy because of deleted copy constructor
				for (std::size_t i = 0; i < curListeners.size(); ++i)
				{
					// simply copy.
					newListeners->operator[](i).store(curListeners[i].load(std::memory_order_relaxed), std::memory_order_relaxed);
				}

				std::atomic_thread_fence(std::memory_order_release);

				if (audioListeners.tryReplace(newListeners.get()))
				{
					newListeners.release();
					return true;
				}
				return false;
			}

		private:

			void protectedAsyncSystemEntry()
			{
				CProtected::runProtectedCodeErrorHandling
				(
					[this]()
					{
						asyncAudioSystem();
					}
				);
			}

			/// <summary>
			/// Asynchronous subsystem.
			/// </summary>
			void asyncAudioSystem()
			{
				_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
				asyncAudioThreadInitiated.store(true);

				AudioFrame recv;
				int pops(20);
				// TODO: support more channels
				std::vector<T> audioInput[2];
				std::vector<T> deferredAudioInput[2];

				typedef decltype(cpl::Misc::ClockCounter()) ctime_t;

				// when it returns false, its time to quit this thread.
				while (audioFifo.popElementBlocking(recv))
				{
					CProcessorTimer overhead, all;

					overhead.start(); all.start();
					// each time we get into here, it's very likely there's a bunch of messages waiting.
					auto numExtraEntries = audioFifo.enqueuededElements();

					// always resize queue before emptying
					if (pops++ > 10)
					{
						audioFifo.grow(0, true, 0.3f, 2);
						pops = 0;
					}

					// resize each channel
					for (auto & ch : audioInput)
						ensureVSize(ch, AudioFrame::capacity * (1 + numExtraEntries)); // close to worst case possible

					std::size_t numSamples[2] = { 0, 0 };

					// insert the frame we already captured at the top of the loop
					insertFrameIntoBuffer(audioInput, numSamples, recv);

					auto currentType = recv.utility;

					for (size_t i = 0; i < numExtraEntries; i++)
					{
						if (!audioFifo.popElementBlocking(recv))
							return;

						// TODO: fix by posting audio here, and then start a new batch with the new configuration.
						if (recv.utility != currentType)
							CPL_RUNTIME_EXCEPTION("Runtime switch of channel configuration without notification!");

						insertFrameIntoBuffer(audioInput, numSamples, recv);
						
					}

					auto channels = AudioFrame::getChannelCount(recv.utility);

					bool signalChange = false;
					{

						// dont lock unless it's necessary
						std::unique_lock<std::mutex> llock(aListenerMutex, std::defer_lock);

						if (resizeListeners)
						{
							if (!llock.owns_lock())
								llock.lock();
							do
							{
								audioListeners.tryRemoveOld();

								if (!expandListenerQueue())
								{
									// failed to replace the queue, there may have been another in queue.
									// maybe refactor this to not be a loop.
									break;
								}


								// there may have been more requests meanwhile.
							} while (resizeListeners.cas());

						}

						signalChange = asyncSignalChange.cas();
						auto localAudioHistorySize = internalInfo.audioHistorySize.load(std::memory_order_acquire);
						auto localAudioHistoryCapacity = internalInfo.audioHistoryCapacity.load(std::memory_order_acquire);
						bool somethingWasDone = false;


						// resize audio history buffer here, so it takes effect, 
						// before any async callers are notified.
						if (signalChange && (internalInfo.storeAudioHistory &&
							localAudioHistorySize != getAudioHistorySize() ||
							localAudioHistoryCapacity != getAudioHistoryCapacity()))
						{
							std::lock_guard<std::mutex> bufferLock(aBufferMutex);
							//OutputDebugString(("1. Changed size to: " + std::to_string(localAudioHistorySize)).c_str());
							ensureAudioHistoryStorage
							(
								channels,
								localAudioHistorySize,
								localAudioHistoryCapacity
							);
							somethingWasDone = true;
						}

						if (internalInfo.callAsyncListeners)
						{

							unsigned mask(0);

							if(!llock.owns_lock())
								llock.lock();

							T * audioBuffers[2] = { audioInput[0].data(), audioInput[1].data() };

							auto & listeners = *audioListeners.getObjectWithoutSignaling();


							if (signalChange && !somethingWasDone)
							{
								// 
								//BreakIfDebugged();
								//OutputDebugString("3. Signaled change, while still holding async buffer lock.\n");
							}
							overhead.pause();

							for (auto & listener : listeners)
							{
								auto rawListener = listener.load(std::memory_order_relaxed);
								if (rawListener)
								{
									if (signalChange)
										rawListener->sourcePropertiesChangedAsync(*this, oldInfo);
									// TODO: exception handling?
									mask |= (unsigned) rawListener->onAsyncAudio
									(
										*this,
										audioBuffers,
										channels,
										numSamples[0]
									);
								}
							}

							overhead.resume();
						}


					}

					// Publish into circular buffer here.
					if(internalInfo.storeAudioHistory && internalInfo.audioHistorySize.load(std::memory_order_relaxed) && channels)
					{
						// TODO: figure out if we can conditionally acquire this mutex
						// in the same scope as the previous, without keeping a hold
						// on the listener mutex.
						std::unique_lock<std::mutex> bufferLock(aBufferMutex, std::defer_lock);
						// decide whether to wait on the buffers
						if(!bufferLock.owns_lock())
							internalInfo.blockOnHistoryBuffer ? bufferLock.lock() : bufferLock.try_lock();

						if (bufferLock.owns_lock())
						{

							for (std::size_t i = 0; i < channels; ++i)
							{
								{
									auto && w = audioHistoryBuffers[i].createWriter();
									// first, insert all the old stuff that happened while this buffer was blocked
									w.copyIntoHead(deferredAudioInput[i].data(), deferredAudioInput[i].size());
									// next, insert current samples.
									w.copyIntoHead(audioInput[i].data(), numSamples[i]);
								}
								// clear up temporary deferred stuff
								deferredAudioInput[i].clear();
								// bit redundant, but ensures it will be called.
								numDeferredAsyncSamples.store(0, std::memory_order_release);
							}
						}
						else
						{
							// defer current samples to a later point in time.
							for (std::size_t i = 0; i < channels; ++i)
							{
								deferredAudioInput[i].insert
								(
									deferredAudioInput[i].end(), 
									audioInput[i].begin(), 
									audioInput[i].begin() + numSamples[i]
								);
							}

							numDeferredAsyncSamples.store(deferredAudioInput[0].size(), std::memory_order::memory_order_release);
						}
					}

					// post measurements.
					double timeFraction = (double)std::accumulate(std::begin(numSamples), std::end(numSamples), 0.0) / (std::end(numSamples) - std::begin(numSamples));
					timeFraction /= internalInfo.sampleRate.load(std::memory_order::memory_order_relaxed);
					lpFilterTimeToMeasurement(measures.asyncOverhead, overhead.clocksToCoreUsage(overhead.getTime()), timeFraction);
					lpFilterTimeToMeasurement(measures.asyncUsage, all.clocksToCoreUsage(all.getTime()), timeFraction);
				}

			}
			
			/// <summary>
			/// Only call this if you own aBufferMutex.
			/// May trash all current storage.
			/// </summary>
			/// <param name="audioHistorySize">The size of the circular buffer for the channel, in samples.</param>
			void ensureAudioHistoryStorage(std::size_t channels, std::size_t pSize, std::size_t pCapacity)
			{

				// only add channels...
				const auto nc = std::max(audioHistoryBuffers.size(), channels);

				if (audioHistoryBuffers.size() != nc)
				{
					audioHistoryBuffers.resize(nc);
				}
				for (std::size_t i = 0; i < channels; ++i)
				{
					audioHistoryBuffers[i].setStorageRequirements(pSize, pCapacity, true, T());
				}

			}


			template<typename Vector>
			static inline void ensureVSize(Vector & v, std::size_t size, float factor = 1.5f)
			{
				auto vsize = v.size();
				if (vsize < size)
				{
					v.resize(std::max(size, std::size_t(size * factor)));
				}
			}
			template<class ChannelVector, class OffsetVector>
				static inline void insertFrameIntoBuffer(ChannelVector & inputBuffer, OffsetVector & offsets, AudioFrame & frame)
				{
					auto const totalSize = frame.size;
					switch (frame.utility)
					{
					case AudioFrame::mono:
					{
						ensureVSize(inputBuffer[0], offsets[0] + totalSize);
						// TODO: use std::copy when we can proove _restrict_ to the compiler, so it doesn't default
						// to memmove!
						std::memcpy(inputBuffer[0].data() + offsets[0], frame.begin(), totalSize * AudioFrame::capacity);
						offsets[0] += totalSize;
						break;
					}

					case AudioFrame::separate:
					{
						auto const halfSize = totalSize >> 1;
						auto const startOffset = frame.begin() + halfSize;
						ensureVSize(inputBuffer[0], offsets[0] + halfSize);
						ensureVSize(inputBuffer[1], offsets[1] + halfSize);
						std::memcpy(inputBuffer[0].data() + offsets[0], frame.begin(), halfSize * AudioFrame::capacity);
						std::memcpy(inputBuffer[1].data() + offsets[1], startOffset, halfSize * AudioFrame::capacity);
						offsets[0] += halfSize;
						offsets[1] += halfSize;
						break;
					}

					default:
						break;
					}
				}

			
			std::atomic_bool 
				asyncAudioThreadCreated, 
				asyncAudioThreadInitiated,
				objectIsDead;

			ABoolFlag 
				resizeListeners,
				tidyListeners,
				audioSignalChange,
				asyncSignalChange;

			//std::atomic<std::size_t> audioHistorySize, audioHistoryCapacity;
			std::atomic<std::size_t> numDeferredAsyncSamples;
			std::vector<AudioBuffer> audioHistoryBuffers;
			std::thread::id audioRTThreadID;
			std::thread asyncAudioThread;
			CBlockingLockFreeQueue<AudioFrame> audioFifo;
			ConcurrentObjectSwapper<ListenerQueue> audioListeners;
			AudioStreamInfo internalInfo, oldInfo;
			PerformanceMeasurements measures;
			std::mutex aListenerMutex, aBufferMutex;
		};

	};
#endif