#ifndef CAUDIOBUFFER_H
	#define CAUDIOBUFFER_H
	#include <vector>
	#include <cpl/MacroConstants.h>
	#include <stdexcept>
	#include <cpl/CMutex.h>
	#include <cpl/Mathext.h>
	#include <cpl/lib/AlignedAllocator.h>

	

	namespace cpl
	{
		template<class Container, typename Value>
		bool exists(const Container & c, const Value & v)
		{
			return std::end(c) != std::find(std::begin(c), std::end(c), v);
		}

		typedef float Type;
		const int buf_size = 44100;

		class CAudioListener;

		class CAudioSource
		{

		public:
			/* true if listener has been added */
			virtual bool addListener(CAudioListener *) { return false; }
			/* true if listener has been removed */
			virtual bool removeListener(CAudioListener *) { return true; }

			virtual ~CAudioSource() {}
		};

		class CAudioListener
		{
		public:
			CAudioListener()
				: source(nullptr)
			{

			}

			virtual bool audioCallback(float ** buffer, std::size_t numChannels, std::size_t numSamples) = 0;

			void listenToSource(CAudioSource & audioSource)
			{
				audioSource.addListener(this);
				source = &audioSource;
			}

			void sourceIsDying()
			{
				source = nullptr;
			}

			virtual ~CAudioListener()
			{
				if (source)
					source->removeListener(this);
			}
		private:
			CAudioSource * source;
		};
		class CAudioBuffer
		{
		public:

			struct __alignas(16) CChannelBuffer : public CAudioSource, public cpl::CMutex::Lockable
			{

				
				typedef Type floatType;
				bool isCircular;
				bool isProcessing;
				std::size_t size;
				std::size_t start;
				std::vector<CAudioListener *> listeners;
				double sampleRate;
				__alignas(16) floatType buffer[buf_size];

				template<std::size_t alignment>
					struct BufferIterator
					{

						BufferIterator(const Type * bufferPointer, std::size_t positionInBuffer, std::size_t lengthOfBuffer)
							: basePointer(bufferPointer), start(positionInBuffer), length(lengthOfBuffer)
						{
							firstSize = length - start;
							secondSize = start;
						}
						
						union
						{
							struct
							{
								std::size_t firstSize, secondSize;
							};
							std::size_t sizes[2];
						};

						inline const Type * getIndex(std::size_t index) const noexcept
						{
							return basePointer + (index ? 0 : secondSize);
						}

						inline const Type * getFirst() const noexcept
						{
							return basePointer + secondSize;
						}

						inline const Type * getSecond() const noexcept
						{
							return basePointer;
						}

					private:
						std::size_t start;
						std::size_t length;
						const Type * basePointer;
					};

				template<std::size_t alignment>
					inline BufferIterator<alignment> getIterator() const noexcept
					{
						// the modulo is a REALLY bad fix for multithreaded crashes
						// (cases where the UI thread updates the size parameter
						// and something else reads the buffer BEFORE the audio
						// thread wraps the start around again)... 
						return BufferIterator<alignment>(buffer, start % size, size);

					}

				CChannelBuffer()
					: isCircular(true), size(buf_size), start(0), isProcessing(false)
				{
					std::memset(buffer, 0, size * sizeof(floatType));

				}

				~CChannelBuffer()
				{
					cpl::CMutex lock(this);
					for (auto & listener : listeners)
						listener->sourceIsDying();
				}

				void setNextSample(floatType sample) noexcept
				{
					buffer[start] = sample;
					start++;
					start %= size;
				}


				
				inline Type * getCopy() const noexcept
				{
					Type * buf = new Type[size];
					// this can be optimized into two memcpy's
					for(std::size_t i = 0; i < size; ++i)
					{
						buf[i] = buffer[(start + i) % size];
						
					}
					return buf;
				}
				
				void clone(CChannelBuffer & other) const noexcept
				{
					std::memcpy(other.buffer, buffer, buf_size * sizeof(floatType));
					other.size = size;
					other.start = start;
					other.isProcessing = false;
					other.isCircular = isCircular;
					other.sampleRate = sampleRate;
				}
				
				bool raiseAudioEvent(float ** audioBuf, std::size_t numChannels, std::size_t numSamples)
				{
					cpl::CFastMutex lock(this);
					unsigned mask(0);
					for (auto & listener : listeners)
					{
						mask |= (unsigned)listener->audioCallback(audioBuf, numChannels, numSamples);
					}
					return mask ? true : false;
				}

				bool addListener(CAudioListener * l)
				{
					cpl::CMutex lock(this);
					if (!exists(listeners, l))
						listeners.push_back(l);
					else
						return false;
					return true;
				}

				bool removeListener(CAudioListener * l)
				{
					cpl::CMutex lock(this);
					auto it = std::find(listeners.begin(), listeners.end(), l);
					if (it != listeners.end())
						listeners.erase(it);
					else
						return false;
					return true;
				}

				void copyTo(float * buf) const noexcept
				{
					std::size_t firstChunk = size - start;
					std::memcpy(buf, buffer + start, firstChunk);
					std::memcpy(buf + firstChunk, buffer, start);
					
				}

				void setSampleRate(double newRate) noexcept
				{
					sampleRate = newRate;

				}

				inline bool setLength(double miliseconds) noexcept
				{

					auto newLength = cpl::Math::round<std::size_t>((sampleRate / 1000) * miliseconds);
					return setSize(newLength);

				}
				inline std::size_t maxSize() const noexcept
				{
					return buf_size;

				}
				inline bool setSize(std::size_t newSize) noexcept
				{
					if (newSize >= buf_size || newSize <= 0)
						return false;

					size = newSize;
					return true;
				}

				inline floatType & operator[] (std::size_t index)
				{
					// no need to check range, the modulo automagically wraps around
					#ifdef _DEBUG
						if (index > size)
							throw std::range_error("Index out of range for CChannelBuffer");
					#endif
					return buffer[(start + index) % size];

				}
				inline floatType & singleCheckAccess(std::size_t index) noexcept
				{
					
					std::size_t offset = start + index;
					bool mask = offset > (size - 1);

					offset -= size * mask;

					return buffer[offset];
				}
				
				inline floatType & directAccess(std::size_t index) noexcept
				{
					return buffer[index];
				}
				
				/*
				floatType & operator[] (std::size_t index)
				{
					if (isCircular)
					{
						std::size_t offset = (start + index) % size;
						// no need to check range, the modulo automagically wraps around
						return buffer[offset];
					}
					else
					{
	#ifdef _DEBUG
						if (index > size)
							throw std::range_error("Index out of range for CChannelBuffer");
	#endif
						return buffer[index];

					}
				}*/
			};

			std::vector<CChannelBuffer> channels;

		public:
			CChannelBuffer & operator [] (std::size_t index)
			{
				//if (index >= channels.size())
				//	channels.emplace_back();
				return channels[index];
			}


			std::size_t getNumBuffers()
			{
				return channels.size();
			}


		};
		typedef cpl::aligned_vector<cpl::CAudioBuffer::CChannelBuffer, 32> AudioBuffer;
	};

#endif