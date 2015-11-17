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
	#include <deque>
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
					static_assert(sizeof(AudioPacket<T, bufsize>) == bufsize, "Wrong alignment");
				}

				AudioPacket(std::uint16_t elementsUsed, std::uint16_t channelConfiguration)
					: size(elementsUsed), utility((util_t)channelConfiguration)
				{
					static_assert(sizeof(AudioPacket<T, bufsize>) == bufsize, "Wrong alignment");
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

				static int getChannelCount(util_t channelType) noexcept
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

			/// <summary>
			/// Holds info about the audio stream. Wow.
			/// </summary>
			struct AudioStreamInfo
			{
				double sampleRate;

				std::size_t 
					anticipatedSize,
					anticipatedChannels,
					circularBufferSize;

				bool
					isFrozen,
					isSuspended,
					/// <summary>
					/// if false, removes one mutex from the async subsystem and 
					/// improves performance
					/// See AudioStreamListener::onAsyncAudio
					/// </summary>
					callAsyncListeners,
					/// <summary>
					/// If false, removes a lot of locking complexity and improves performance.
					/// See AudioStreamListener::onRTAudio
					/// </summary>
					callRTListeners;
			};

			class AudioStreamListener;
			static const std::size_t packetSize = PacketSize;

			typedef AudioPacket<T, PacketSize> AudioFrame;
			// atomic vector of pointers, atomic because we have unsynchronized read-access between threads (UB without atomic access)
			typedef std::vector<std::atomic<AudioStreamListener *>> ListenerQueue;

			/// <summary>
			/// A class that enables listening callbacks on both real-time and async audio
			/// channels from a CAudioStream
			/// </summary>
			class AudioStreamListener : Utility::CNoncopyable
			{
			public:

				typedef CAudioStream<T, PacketSize> Stream;

				AudioStreamListener()
					: internalSource(nullptr)
				{

				}

				/// <summary>
				/// Called from a real-time thread.
				/// </summary>
				virtual bool onRTAudio(const Stream & source, T ** buffer, std::size_t numChannels, std::size_t numSamples) { return false; };
				/// <summary>
				/// Called from a non real-time thread.
				/// </summary>
				virtual bool onAsyncAudio(const Stream & source, T ** buffer, std::size_t numChannels, std::size_t numSamples) { return false; };
				
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
				virtual ~AudioStreamListener()
				{
					detachFromSource();
				}

				void sourcePropertiesChanged(const Stream & changedSource, const typename Stream::AudioStreamInfo & before)
				{
					auto s1 = internalSource.load(std::memory_order_acquire);
					if (&changedSource != s1)
						crash("Inconsistency between argument CAudioStream and internal source; corrupt listener chain.");
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
			/// Integrity of audio stream is not guaranteed, especially if you block it for longer times.
			/// It subsystem also continuously updates a circular buffer which you can lock.
			/// 
			/// Fifo sizes refer to the buffer size of the lock free fifo. The fifo stores AudioFrames.
			/// </summary>
			/// <param name="enableAsyncSubsystem"></param>
			CAudioStream(std::size_t defaultListenerBankSize = 16, bool enableAsyncSubsystem = false, std::size_t initialFifoSize = 20, std::size_t maxFifoSize = 1000)
				: droppedAudioFrames(), internalInfo(), audioFifo(initialFifoSize, maxFifoSize), objectIsDead(false)
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
			/// This must be called at least once, from the audio thread.
			/// TODO: refactor, specify and consider what thread this makes sense to be initialized on
			/// </summary>
			void initializeInfo(const AudioStreamInfo & info)
			{
				audioRTThreadID = std::this_thread::get_id();
				AudioStreamInfo before = internalInfo;
				internalInfo = info;
				auto & listeners = *audioListeners.getObject();

				for (auto & listener : listeners)
				{
					auto rawListener = listener.load(std::memory_order_acquire);
					if (rawListener)
					{
						rawListener->sourcePropertiesChanged(*this, before);
					}

				}
			}

			/// <summary>
			/// Should only be called from the audio thread.
			/// Deterministic, wait free and lock free, so long as listeners are as well.
			/// </summary>
			/// <returns>True if incoming audio was changed</returns>
			bool processIncomingRTAudio(T ** buffer, std::size_t numChannels, std::size_t numSamples)
			{
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

				auto & listeners = *audioListeners.getObject();
				if (internalInfo.callRTListeners)
				{
					for (auto & listener : listeners)
					{
						auto rawListener = listener.load(std::memory_order_acquire);
						if (rawListener)
						{
							// TODO: figure out if it is UB to read the listener values without synchronization
							mask |= (unsigned)rawListener->onIncomingRTAudio(*this, buffer, numChannels, numSamples);
						}
					}
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
							std::memcpy(af.buffer, (buffer[0] + numSamples - n), aSamples * AudioFrame::element_size);

							if (!audioFifo.pushElement(af))
								droppedAudioFrames++;
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
							std::memcpy(af.buffer, (buffer[0] + numSamples - n), aSamples * AudioFrame::element_size);
							std::memcpy(af.buffer + aSamples, (buffer[1] + numSamples - n), aSamples * AudioFrame::element_size);
							if (!audioFifo.pushElement(af))
								droppedAudioFrames++;
						}

						n -= aSamples;
					}

					break;
				default:
					// TODO: what to do?
					BreakIfDebugged();
					break;
				}

				// return whether any data was changed.
				return mask ? true : false;
			}


			~CAudioStream()
			{
				// the audio thread is created inside this flag.
				// and that flag is set by this thread. 
				if (asyncAudioThreadCreated.load())
				{
					// so the thread has been created; wait for it to enter function space.
					bool success = cpl::Misc::WaitOnCondition(10000,
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

			bool isAudioThread() noexcept
			{
				return std::this_thread::get_id() == audioRTThreadID;
			}

			bool isAsyncThread() noexcept
			{
				return std::this_thread::get_id() == asyncAudioThread.get_id();
			}

			/// <summary>
			/// Returns the number of dropped frames from the audio thread,
			/// due to the FIFO being filled up as the async thread hasn't
			/// catched up (due to being blocked or simple have too much work).
			/// </summary>
			std::uint64_t getDroppedFrames() const noexcept
			{
				return droppedAudioFrames;
			}

			/// <summary>
			/// The number of AudioFrames in the async FIFO
			/// </summary>
			std::size_t getASyncBufferSize() const noexcept
			{
				return audioFifo.size();
			}

		protected:

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
			std::pair<bool, bool> addListener(AudioStreamListener * newListener, bool tryForceSuccess = false)
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
			std::pair<bool, bool> removeListener(AudioStreamListener * listenerToBeRemoved, bool forceSuccess = false)
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
			bool insertIntoListenerQueue(AudioStreamListener * newListener)
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
			bool removeFromListenerQueue(AudioStreamListener * listenerToBeRemoved)
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

			/// <summary>
			/// Asynchronous subsystem.
			/// </summary>
			void asyncAudioSystem()
			{
				
				asyncAudioThreadInitiated.store(true);

				AudioFrame recv;
				int pops(20);
				// TODO: support more channels
				std::vector<T> audioInput[2];

				typedef decltype(cpl::Misc::ClockCounter()) ctime_t;

				// when it returns false, its time to quit this thread.
				while (audioFifo.popElementBlocking(recv))
				{
					ctime_t then = cpl::Misc::ClockCounter();

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

					// Locking scope of listener queue
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

						if (internalInfo.callAsyncListeners)
						{

							unsigned mask(0);

							if(!llock.owns_lock())
								llock.lock();

							T * audioBuffers[2] = { audioInput[0].data(), audioInput[1].data() };

							auto & listeners = *audioListeners.getObjectWithoutSignaling();
							for (auto & listener : listeners)
							{
								auto rawListener = listener.load(std::memory_order_relaxed);
								if (rawListener)
								{
									mask |= (unsigned) rawListener->onAsyncAudio
									(
										*this,
										audioBuffers,
										AudioFrame::getChannelCount(recv.utility),
										numSamples[0]
									);
								}
							}
						}
					}



					// Publish into circular buffer here.


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

			ABoolFlag resizeListeners;
			ABoolFlag tidyListeners;

			std::thread::id audioRTThreadID;
			std::thread asyncAudioThread;
			CBlockingLockFreeQueue<AudioFrame> audioFifo;
			std::uint64_t droppedAudioFrames;
			ConcurrentObjectSwapper<ListenerQueue> audioListeners;
			AudioStreamInfo internalInfo;
			std::mutex aListenerMutex, aBufferMutex;
		};

	};
#endif