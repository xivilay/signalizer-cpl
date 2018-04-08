/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2016 Janus Lynggaard Thorborg (www.jthorborg.com)

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

#ifndef CPL_CAUDIOSTREAM_H
#define CPL_CAUDIOSTREAM_H

#include <thread>
#include "Utility.h"
#include <vector>
#include <mutex>
#include "AtomicCompability.h"
#include "ConcurrentServices.h"
#include "lib/BlockingLockFreeQueue.h"
#include "lib/CLIFOStream.h"
#include "CProcessorTimer.h"
#include <deque>
#include <algorithm>
#include <numeric>
#include "Protected.h"

#ifdef CPL_MSVC
#pragma warning(push, 3)
#pragma warning(disable:4100) // unused parameters
#endif

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

	template<std::size_t channels, bool biased>
	struct ChannelIterator;

	template<typename T, std::size_t>
	class CAudioStream;

	/// <summary>
	/// Note: You should never delete this class.
	/// </summary>
	class IAudioHistoryPropertyView
	{
	public:
		virtual double getAudioHistorySamplerate() const noexcept = 0;
		virtual std::size_t getAudioHistoryCapacity() const noexcept = 0;
		virtual std::size_t getAudioHistorySize() const noexcept = 0;
	};

	#ifdef CPL_MSVC
	#pragma pack(push, 1)
	#endif

	struct PACKED MessageStreamBase
	{
		/// <summary>
		/// Channel configuration of incoming data
		/// </summary>
		enum class MessageType : std::uint16_t
		{
			None,
			/// <summary>
			/// Signifies a change in the musical arrangement
			/// </summary>
			ArrangementMessage,
			/// <summary>
			/// Signifies a change or discontinuity in the transport
			/// </summary>
			TransportMessage,
			/// <summary>
			/// For N channels, every N + K belongs to the Kth channel
			/// </summary>
			AudioPacketInterleaved,
			/// <summary>
			/// For N channels of M size, every N + K * M belongs to the Kth channel 
			/// </summary>
			AudioPacketSeparate
		} utility;

		MessageStreamBase(MessageType type) : utility(type) {}

	};

#define AUDIOSTREAM_AUDIOPACKET_DATA_ALIGNMENT 8

	/// <summary>
	/// A simple blob of audio channel data transmitted,
	/// used for transmitting data from real time threads
	/// to worker threads.
	/// </summary>
	template<typename T, std::size_t bufsize>
	struct PACKED AudioPacket : public MessageStreamBase
	{
	public:

		static_assert(bufsize > AUDIOSTREAM_AUDIOPACKET_DATA_ALIGNMENT + sizeof(T), "Audio packet cannot hold a single element");

		AudioPacket(std::uint8_t numChannels, std::uint16_t elementsUsed)
			: MessageStreamBase(MessageType::None), size(elementsUsed), channels(numChannels)
		{
			static_assert(sizeof(AudioPacket<T, bufsize>) == bufsize, "Wrong packing");
		}

		AudioPacket(MessageType channelConfiguratio, std::uint8_t numChannels, std::uint16_t elementsUsed)
			: MessageStreamBase(channelConfiguration), size(elementsUsed), channels(numChannels)
		{
			static_assert(sizeof(AudioPacket<T, bufsize>) == bufsize, "Wrong packing");
		}

		AudioPacket() : size() {}

		static constexpr std::size_t getCapacityForChannels(std::size_t channels) noexcept { return capacity / channels; }
		constexpr std::size_t getChannelCount() noexcept { return channels; }
		constexpr std::size_t getNumFrames() noexcept { return size / channels; }
		constexpr std::size_t getTotalSamples() noexcept { return size; }
		
		T * begin() noexcept { return buffer; }
		T * end() noexcept { return buffer + size; }
		const T * begin() const noexcept { return buffer; }
		const T * end() const noexcept { return buffer + size; }

		static const std::size_t element_size = sizeof(T);

	private:

		static const std::uint32_t capacity = (bufsize - AUDIOSTREAM_AUDIOPACKET_DATA_ALIGNMENT) / element_size;

		/// <summary>
		/// The total number of samples.
		/// </summary>
		std::uint16_t size;
		std::uint8_t channels;

		alignas(AUDIOSTREAM_AUDIOPACKET_DATA_ALIGNMENT) T buffer[capacity];
	};

	struct PACKED ArrangementData : public MessageStreamBase
	{
		#ifdef CPL_JUCE
		ArrangementData(const juce::AudioPlayHead::CurrentPositionInfo & cpi)
			: MessageStreamBase(MessageType::ArrangementMessage)
			, beatsPerMinute(cpi.bpm)
			, signatureDenominator(cpi.timeSigDenominator)
			, signatureNumerator(cpi.timeSigNumerator)
		{}
		#endif
		ArrangementData() : MessageStreamBase(MessageType::ArrangementMessage), beatsPerMinute(), signatureDenominator(), signatureNumerator() {}

		double beatsPerMinute;
		std::uint16_t signatureDenominator;
		std::uint16_t signatureNumerator;
	};

	inline bool operator == (const ArrangementData & left, const ArrangementData & right)
	{
		return left.beatsPerMinute == right.beatsPerMinute && left.signatureDenominator == right.signatureDenominator && left.signatureNumerator == right.signatureNumerator;
	}

	inline bool operator != (const ArrangementData & left, const ArrangementData & right)
	{
		return !(left == right);
	}

	struct PACKED TransportData : public MessageStreamBase
	{
		#ifdef CPL_JUCE
		TransportData(const juce::AudioPlayHead::CurrentPositionInfo & cpi)
			: MessageStreamBase(MessageType::TransportMessage)
			, samplePosition(cpi.timeInSamples)
			, isPlaying(cpi.isPlaying)
			, isLooping(cpi.isLooping)
			, isRecording(cpi.isRecording)
		{
		}
		#endif
		TransportData() : MessageStreamBase(MessageType::TransportMessage), samplePosition(), isPlaying(), isLooping(), isRecording() {}
		std::int64_t samplePosition;
		std::uint16_t isPlaying : 1, isLooping : 1, isRecording : 1;
	};

	inline bool operator == (const TransportData & left, const TransportData & right)
	{
		return left.samplePosition == right.samplePosition && left.isPlaying == right.isPlaying && left.isLooping == right.isLooping && left.isRecording == right.isRecording;
	}

	inline bool operator != (const TransportData & left, const TransportData & right)
	{
		return !(left == right);
	}

	template<typename T, std::size_t bufSize>
	union PACKED StreamMessage
	{
		typedef AudioPacket<T, bufSize> AudioFrameType;
		StreamMessage() { streamBase.utility = MessageStreamBase::MessageType::None; }
		MessageStreamBase streamBase;
		ArrangementData arrangement;
		TransportData transport;
		AudioFrameType audioPacket;
	};

	#ifdef CPL_MSVC
	#pragma pack(pop)
	#endif

	template<typename T, std::size_t PacketSize = 64>
	class CAudioStream
		: private Utility::CNoncopyable
		, public IAudioHistoryPropertyView
	{
	public:

		struct PerformanceMeasurements
		{
			PerformanceMeasurements()
				: asyncOverhead(0), rtOverhead(0), asyncUsage(0), rtUsage(0), droppedAudioFrames(0)
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
				std_memory_fence(std::memory_order_acquire);

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

				std_memory_fence(std::memory_order_release);

				return *this;
			}

			AudioStreamInfo()
			{
				std_memory_fence(std::memory_order_acquire);

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

				std_memory_fence(std::memory_order_release);

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

		typedef typename StreamMessage<T, PacketSize>::AudioFrameType AudioFrame;
		typedef StreamMessage<T, PacketSize> Frame;
		// atomic vector of pointers, atomic because we have unsynchronized read-access between threads (UB without atomic access)
		typedef std::vector<std::atomic<Listener *>> ListenerQueue;
		typedef typename cpl::CLIFOStream<T, storageAlignment> AudioBuffer;
		typedef typename cpl::CLIFOStream<T, storageAlignment>::ProxyView AudioBufferView;
		typedef T DataType;
		typedef typename AudioBuffer::IteratorBase::iterator BufferIterator;
		typedef typename AudioBuffer::IteratorBase::const_iterator CBufferIterator;
		typedef CAudioStream<T, PacketSize> StreamType;

		struct Playhead
		{
			#ifdef CPL_JUCE
			Playhead(const juce::AudioPlayHead::CurrentPositionInfo & info, double sampleRate) : sampleRate(sampleRate), arrangement(info), transport(info) {}
			#endif
			friend class CAudioStream<T, PacketSize>;

			void advance(cpl::ssize_t samples)
			{
				steadyClock += samples;

				if (transport.isPlaying)
					transport.samplePosition += samples;
			}

			std::uint64_t getSteadyClock() const noexcept { return steadyClock; }
			bool isPlaying() const noexcept { return transport.isPlaying; }
			bool isLooping() const noexcept { return transport.isLooping; }
			bool isRecording() const noexcept { return transport.isRecording; }

			double getBPM() const noexcept { return arrangement.beatsPerMinute; }
			std::pair<int, int> getSignature() const noexcept { return std::make_pair(arrangement.signatureNumerator, arrangement.signatureDenominator); }
			std::int64_t getPositionInSamples() const noexcept { return transport.samplePosition; }
			double getPositionInSeconds() const noexcept { return getPositionInSamples() / sampleRate; }

			static Playhead empty() { return{}; }

			Playhead & operator=(const Playhead &) = delete;

			void copyVolatileData(const Playhead & other)
			{
				sampleRate = other.sampleRate;
				arrangement = other.arrangement;
				transport = other.transport;
			}

		private:

			Playhead() {}
			double sampleRate = 0;
			ArrangementData arrangement;
			TransportData transport;
			std::uint64_t steadyClock = 0;
		};

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
			typedef StreamType InheritStreamType;
			typedef typename AudioBuffer::IteratorBase::iterator iterator;
			typedef typename AudioBuffer::IteratorBase::const_iterator const_iterator;

			AudioBufferAccess(std::mutex & m, const std::vector<AudioBuffer> & audioChannels)
				: lock(m), audioChannels(audioChannels)
			{

			}

			AudioBufferAccess(const AudioBufferAccess &) = delete;
			AudioBufferAccess & operator = (const AudioBufferAccess &) = delete;

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
			///		(std::size_t sampleFrame, AudioStream::Data & channel0, AudioStream::Data & channelN ...)
			/// </summary>
			template<std::size_t Channels, bool biased, typename Functor>
			inline void iterate(const Functor & f, std::size_t offset = 0)
			{
				ChannelIterator<Channels, biased>::run(*this, f, offset);
			}

			/// <summary>
			/// Iterate over the samples in the buffers. If biased is set, the samples arrive in chronological order.
			/// The functor shall accept the following parameters:
			///		(std::size_t sampleFrame, AudioStream::Data channel0, AudioStream::Data channelN ...)
			/// </summary>
			template<std::size_t Channels, bool biased, typename Functor>
			inline void iterate(const Functor & f, std::size_t offset = 0) const
			{
				ChannelIterator<Channels, biased>::run(*this, f, offset);
			}

		private:

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
			virtual void onSourceDied(Stream & dyingSource) { (void)dyingSource; };

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
				CPL_BREAKIFDEBUGGED();
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
			numDeferredAsyncSamples(0),
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
				asyncAudioThread = std::thread(&CAudioStream::protectedAsyncSystemEntry, this);
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
			//AudioStreamInfo oldInfo = internalInfo;
			// TODO: implement onRtPropertiesChanged here.
			internalInfo = info;

			audioSignalChange = true;
			asyncSignalChange = true;
		}

		/// <summary>
		/// Returns the playhead for the async subsystem.
		/// Only valid to call and read, while the async buffers are locked
		/// or you're inside a async callback
		/// </summary>
		const Playhead & getASyncPlayhead() const noexcept
		{
			return asyncPlayhead;
		}

		/// <summary>
		/// Returns the playhead for the async subsystem.
		/// Only valid to call and read, while you're inside a
		/// real time callback.
		/// </summary>
		const Playhead & getRealTimePlayhead() const noexcept
		{
			return realTimePlayhead;
		}

		#ifdef CPL_JUCE
		/// <summary>
		/// Should only be called from the audio thread.
		/// Deterministic ( O(N) ), wait free and lock free, so long as listeners are as well.
		/// </summary>
		/// <returns>True if incoming audio was changed</returns>
		bool processIncomingRTAudio(T ** buffer, std::size_t numChannels, std::size_t numSamples, juce::AudioPlayHead & ph)
		{
			juce::AudioPlayHead::CurrentPositionInfo cpi;
			ph.getCurrentPosition(cpi);
			return processIncomingRTAudio(buffer, numChannels, numSamples, {cpi, internalInfo.sampleRate.load(std::memory_order_relaxed)});
		}
		#endif
		/// <summary>
		/// Should only be called from the audio thread.
		/// Deterministic ( O(N) ), wait free and lock free, so long as listeners are as well.
		/// </summary>
		/// <returns>True if incoming audio was changed</returns>
		bool processIncomingRTAudio(T ** buffer, std::size_t numChannels, std::size_t numSamples, const Playhead & ph)
		{
			#ifdef CPL_TRACEGUARD_ENTRYPOINTS
			return CPL_TRACEGUARD_START
				#endif
				cpl::CProcessorTimer overhead, all;
			overhead.start(); all.start();

			auto const timeFraction = double(numSamples) / internalInfo.sampleRate.load(std::memory_order_relaxed);

			auto id = std::this_thread::get_id();
			if (audioRTThreadID != std::thread::id() && id != audioRTThreadID)
			{
				// so, another realtime thread is calling this.
				// this should be interesting to debug.
				// TODO: this is an actual problem. Store all audio thread ids in a set, and compare instead.
				// NOTE: resolved for now (very temporary), all audio threads shouldn't call getListeners()
				//CPL_BREAKIFDEBUGGED();
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

			auto oldPlayhead = realTimePlayhead;
			realTimePlayhead.copyVolatileData(ph);
			// publish all data to listeners
			unsigned mask(0);

			//bool signalChange = audioSignalChange.cas();

			auto & listeners = *audioListeners.getObject();
			if (internalInfo.callRTListeners)
			{
				overhead.pause();
				for (auto & listener : listeners)
				{
					auto rawListener = listener.load(std::memory_order_acquire);
					if (rawListener)
					{
						// see comment in initializeInfo
						//if(signalChange)
						//	rawListener->sourcePropertiesChangedRT(*this, oldInfo);

						// TODO: exception handling?
						mask |= (unsigned)rawListener->onIncomingRTAudio(*this, buffer, numChannels, numSamples);
					}
				}
				overhead.resume();
			}


			// remaining samples
			std::int64_t n = numSamples;

			Frame frame;

			bool anyNewProblemsPushingPlayHeads = false;

			if (framesWereDropped || problemsPushingPlayHead || realTimePlayhead.transport != oldPlayhead.transport)
			{
				frame.transport = realTimePlayhead.transport;
				if (!audioFifo.pushElement(frame))
				{
					measures.droppedAudioFrames++;
					anyNewProblemsPushingPlayHeads = true;
				}
			}

			if (framesWereDropped || problemsPushingPlayHead || realTimePlayhead.arrangement != oldPlayhead.arrangement)
			{
				frame.arrangement = realTimePlayhead.arrangement;
				if (!audioFifo.pushElement(frame))
				{
					measures.droppedAudioFrames++;
					anyNewProblemsPushingPlayHeads = true;
				}
			}

			realTimePlayhead.advance(numSamples);

			problemsPushingPlayHead = anyNewProblemsPushingPlayHeads;

			std::size_t droppedSamples = 0;

			// publish all data to audio consumer thread
			if (numChannels == 1)
			{
				constexpr std::int64_t singleChannelCapacity = static_cast<std::int64_t>(AudioFrame::getCapacityForChannels(1));

				while (n > 0)
				{

					auto const aSamples = std::min(singleChannelCapacity, n - std::max(std::int64_t(0), n - singleChannelCapacity));

					if (aSamples > 0)
					{
						// TODO: ensure aSamples < std::uint16_T::max()
						AudioFrame af(AudioFrame::MessageType::AudioPacketSeparate, 1, static_cast<std::uint16_t>(aSamples));
						frame.audioPacket = af;
						std::memcpy(frame.audioPacket.buffer, (buffer[0] + numSamples - n), static_cast<std::size_t>(aSamples * AudioFrame::element_size));

						if (!audioFifo.pushElement(frame))
						{
							measures.droppedAudioFrames++;
							droppedSamples += static_cast<std::size_t>(aSamples);
						}

					}

					n -= aSamples;
				}
			}
			else
			{
				const std::int64_t capacity = static_cast<std::int64_t>(AudioFrame::getCapacityForChannels(numChannels));

				while (n > 0)
				{

					auto const aSamples = std::min(capacity, n - std::max(std::int64_t(0), n - capacity));

					if (aSamples > 0)
					{
						// TODO: ensure aSamples < std::uint16_T::max()
						frame.audioPacket = AudioFrame(AudioFrame::MessageType::AudioPacketSeparate, numChannels, static_cast<std::uint16_t>(aSamples * numChannels));

						auto const byteSize = static_cast<std::size_t>(aSamples * AudioFrame::element_size);

						for (std::size_t c = 0; c < numChannels; ++c)
						{
							std::memcpy(frame.audioPacket.buffer + aSamples * c, (buffer[c] + numSamples - n), byteSize);
						}

						if (!audioFifo.pushElement(frame))
						{
							measures.droppedAudioFrames++;
							droppedSamples += static_cast<std::size_t>(aSamples);
						}

					}

					n -= aSamples;
				}
			}
			
			framesWereDropped = droppedSamples != 0;

			// post new measures

			lpFilterTimeToMeasurement(measures.rtOverhead, overhead.clocksToCoreUsage(overhead.getTime()), timeFraction);
			lpFilterTimeToMeasurement(measures.rtUsage, all.clocksToCoreUsage(all.getTime()), timeFraction);

			// return whether any data was changed.
			return mask ? true : false;

			#ifdef CPL_TRACEGUARD_ENTRYPOINTS
			CPL_TRACEGUARD_STOP("AudioStream real-time processor");
			#endif
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
			return {aBufferMutex, audioHistoryBuffers};
		}

		/// <summary>
		/// Safe to call from any thread (wait free).
		/// </summary>
		const AudioStreamInfo & getInfo() const noexcept
		{
			return internalInfo;
		}

		virtual ~CAudioStream() noexcept(false)
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
		virtual std::size_t getAudioHistorySize() const noexcept override
		{
			return audioHistoryBuffers.size() ? audioHistoryBuffers[0].getSize() : internalInfo.audioHistorySize.load(std::memory_order_relaxed);
		}

		/// <summary>
		/// Safe to call from any thread (wait free).
		/// The actual current value, may not equal whatever set through setAudioHistoryCapacity()
		/// </summary>
		virtual std::size_t getAudioHistoryCapacity() const noexcept override
		{
			return audioHistoryBuffers.size() ? audioHistoryBuffers[0].getCapacity() : internalInfo.audioHistoryCapacity.load(std::memory_order_relaxed);
		}

		virtual double getAudioHistorySamplerate() const noexcept override
		{
			return internalInfo.sampleRate.load(std::memory_order_acquire);
		}

		/// <summary>
		/// Safe to call from any thread (wait free), however it may not take effect immediately
		/// </summary>
		void setAudioHistorySize(std::size_t newSize) noexcept
		{
			internalInfo.audioHistorySize.store(newSize, std::memory_order_relaxed);
			audioSignalChange = true;
			asyncSignalChange = true;
		}

		/// <summary>
		/// Safe to call from any thread (wait free), however it may not take effect immediately
		/// </summary>
		void setAudioHistoryCapacity(std::size_t newCapacity) noexcept
		{
			internalInfo.audioHistorySize.store(std::min(internalInfo.audioHistorySize.load(), newCapacity), std::memory_order_relaxed);
			internalInfo.audioHistoryCapacity.store(newCapacity, std::memory_order_relaxed);
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
			oldInfo.audioHistorySize.store(internalInfo.audioHistorySize.load(std::memory_order_relaxed));
			oldInfo.audioHistoryCapacity.store(internalInfo.audioHistoryCapacity.load(std::memory_order_relaxed));
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

			tryForceSuccess ? lock.lock() : (void)lock.try_lock();

			if (!lock.owns_lock())
				return{false, false};

			if (insertIntoListenerQueue(newListener))
			{
				return{true, true};
			}

			else if (tryForceSuccess)
			{
				return {true, expandListenerQueue() && insertIntoListenerQueue(newListener)};
			}
			else
			{
				// request more listener slots.
				resizeListeners = true;
				return{true, false};
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

			forceSuccess ? lock.lock() : (void)lock.try_lock();

			if (!lock.owns_lock())
				return {false, false};

			return {true, removeFromListenerQueue(listenerToBeRemoved)};

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
		/// Note: You should never need to call this from an audio thread (stop doing it).
		/// </summary>
		ListenerQueue & getListeners()
		{
			// TODO: reimplement
			/*if (isAudioThread())
			{
				return *audioListeners.getObject();
			}
			else
			{
				return *audioListeners.getObjectWithoutSignaling();
			}*/
			return *audioListeners.getObjectWithoutSignaling();
		}
		/// <summary>
		/// Assumes you have appropriate locks!!
		/// Compresses all listeners such that they are not scattered.
		/// TODO: figure out if this is needed at all
		/// </summary>
		bool tidyListenerQueue()
		{
			auto & listeners = *audioListeners.getObject();
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
			std_memory_fence(std::memory_order::memory_order_release);
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

			std_memory_fence(std::memory_order_release);

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
			#ifdef CPL_TRACEGUARD_ENTRYPOINTS
			CPL_TRACEGUARD_START
				#endif
				asyncAudioSystem();
			#ifdef CPL_TRACEGUARD_ENTRYPOINTS
			CPL_TRACEGUARD_STOP("Async audio thread");
			#endif
		}

		struct ChannelMatrix
		{
			void ensureSize(std::size_t channels, std::size_t samples)
			{
				buffer.resize(channels);
				pointer.resize(channels);

				for (std::size_t c = 0; c < channels; ++c)
				{
					buffer[c].resize(samples);
				}
			}

			void resetOffsets()
			{
				currentSamplesContained = 0;
			}

			void insertFrameIntoBuffer(const AudioFrame & frame)
			{
				const auto numSamples = frame.getNumSamples();
				const auto numChannels = frame.getNumChannels();

				ensureSize(numChannels, numSamples + containedSamples);

				switch (frame.utility)
				{
					case AudioFrame::MessageType::AudioPacketSeparate:
					{
						for (std::size_t c = 0; c < numChannels; ++c)
						{
							std::memcpy(
								buffer[c].data() + containedSamples,
								frame.begin() + c * numSamples + containedSamples,
								numSamples * sizeof(T)
							);
						}

						break;
					}

					case AudioFrame::MessageType::AudioPacketInterleaved:
					{
						for (std::size_t c = 0; c < numChannels; ++c)
						{
							for (std::size_t n = 0; n < numSamples; ++n)
							{
								buffer[c][n + containedSamples] = *(frame.begin() + n * numChannels + c);
							}
						}

						break;
					}
				}

				containedSamples += numSamples;
			}



			/*
			T* begin(std::size_t y) noexcept { return buffer.data() + y * rowSize; }
			T* end(std::size_t y) noexcept { return buffer.data() + y * rowSize + rowSize; }

			const T* begin(std::size_t y) const noexcept { return buffer.data() + y * rowSize; }
			const T* end(std::size_t y) const noexcept { return buffer.data() + y * rowSize + rowSize; } */

			std::vector<T*> pointer;
			std::size_t containedSamples;
			std::vector<std::vector<T>> buffer;
		};

		/// <summary>
		/// Asynchronous subsystem.
		/// </summary>
		void asyncAudioSystem()
		{
			_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
			asyncAudioThreadInitiated.store(true);

			Frame recv;
			int pops(20);

			ChannelMatrix audioInput, deferredAudioInput;

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

				audioInput.resetOffsets();

				// insert the frame we already captured at the top of the loop
				auto handleFrame = [&](const auto & frame) 
				{
					switch (frame.streamBase.utility)
					{
						case MessageStreamBase::MessageType::TransportMessage:
							asyncPlayhead.transport = frame.transport;
							break;
						case MessageStreamBase::MessageType::ArrangementMessage:
							asyncPlayhead.arrangement = frame.arrangement;
							break;
						default:
							audioInput.insertFrameIntoBuffer(frame.audioPacket);
							break;
					}
				};

				handleFrame(recv);

				for (size_t i = 0; i < numExtraEntries; i++)
				{
					if (!audioFifo.popElementBlocking(recv))
						return;
					handleFrame(recv);
				}

				auto channels = audioInput.buffer.size();

				bool signalChange = audioHistoryBuffers.size() != channels;
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

					signalChange = asyncSignalChange.cas() || signalChange;
					auto localAudioHistorySize = internalInfo.audioHistorySize.load(std::memory_order_acquire);
					auto localAudioHistoryCapacity = internalInfo.audioHistoryCapacity.load(std::memory_order_acquire);
					bool somethingWasDone = false;


					// resize audio history buffer here, so it takes effect,
					// before any async callers are notified.
					if (signalChange && (internalInfo.storeAudioHistory &&
						(localAudioHistorySize != getAudioHistorySize()) ||
						(localAudioHistoryCapacity != getAudioHistoryCapacity()) ||
						audioHistoryBuffers.size() != channels))
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

						if (!llock.owns_lock())
							llock.lock();

						auto & listeners = *audioListeners.getObjectWithoutSignaling();


						if (signalChange && !somethingWasDone)
						{
							//
							//CPL_BREAKIFDEBUGGED();
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
								mask |= (unsigned)rawListener->onAsyncAudio
								(
									*this,
									audioInput.pointer,
									channels,
									audioInput.containedSamples
								);
							}
						}

						asyncPlayhead.advance(audioInput.containedSamples);

						if (signalChange)
						{
							oldInfo = internalInfo;
						}

						overhead.resume();
					}


				}

				// Publish into circular buffer here.
				if (internalInfo.storeAudioHistory && internalInfo.audioHistorySize.load(std::memory_order_relaxed) && channels)
				{
					// TODO: figure out if we can conditionally acquire this mutex
					// in the same scope as the previous, without keeping a hold
					// on the listener mutex.
					std::unique_lock<std::mutex> bufferLock(aBufferMutex, std::defer_lock);
					// decide whether to wait on the buffers
					if (!bufferLock.owns_lock())
						internalInfo.blockOnHistoryBuffer ? bufferLock.lock() : (void)bufferLock.try_lock();

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
				if (std::isnormal(timeFraction))
				{
					timeFraction /= internalInfo.sampleRate.load(std::memory_order::memory_order_relaxed);
					lpFilterTimeToMeasurement(measures.asyncOverhead, overhead.clocksToCoreUsage(overhead.getTime()), timeFraction);
					lpFilterTimeToMeasurement(measures.asyncUsage, all.clocksToCoreUsage(all.getTime()), timeFraction);
				}

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
			// TODO: Invalid RTL validation here, possible buffer overflow somewhere?
			auto vsize = v.size();
			if (vsize < size)
			{
				v.resize(std::max(size, std::size_t(size * factor)));
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
		Playhead realTimePlayhead, asyncPlayhead;
		bool framesWereDropped {false}, problemsPushingPlayHead {false};
		std::atomic<std::size_t> numDeferredAsyncSamples;
		std::vector<AudioBuffer> audioHistoryBuffers;
		std::thread::id audioRTThreadID;
		std::thread asyncAudioThread;
		CBlockingLockFreeQueue<Frame> audioFifo;
		ConcurrentObjectSwapper<ListenerQueue> audioListeners;
		AudioStreamInfo internalInfo, oldInfo;
		PerformanceMeasurements measures;
		std::mutex aListenerMutex, aBufferMutex;
	};


	template<>
	struct ChannelIterator<2, true>
	{
		template<typename Functor, class StreamBufferAccess>
		static void run(StreamBufferAccess & access, const Functor & f, std::size_t offset = 0)
		{
			typedef typename StreamBufferAccess::InheritStreamType StreamType;
			typedef typename StreamType::BufferIterator BufferIterator;

			typename StreamType::AudioBufferView views[2] = {
				access.getView(offset + 0),
				access.getView(offset + 1)
			};

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

		template<typename Functor, class StreamBufferAccess>
		static void run(const StreamBufferAccess & access, const Functor & f, std::size_t offset = 0)
		{
			typedef typename StreamBufferAccess::InheritStreamType StreamType;
			typedef typename StreamType::BufferIterator BufferIterator;

			typename StreamType::AudioBufferView views[2] = {
				access.getView(offset + 0),
				access.getView(offset + 1)
			};

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
		template<typename Functor, class StreamBufferAccess>
		static void run(const StreamBufferAccess & access, const Functor & f, std::size_t offset = 0)
		{
			typedef typename StreamBufferAccess::InheritStreamType StreamType;
			typedef typename StreamType::BufferIterator BufferIterator;

			typename StreamType::AudioBufferView view = access.getView(offset);

			for (std::size_t indice = 0, n = 0; indice < StreamType::bufferIndices; ++indice)
			{
				BufferIterator left = view.getItIndex(indice);

				std::size_t range = view.getItRange(indice);

				while (range--)
				{
					f(n++, *left++);
				}
			}
		}
	};

};

#ifdef CPL_MSVC
#pragma warning(pop)
#endif

#endif
