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

				void setNextSample(floatType sample)
				{
					buffer[start] = sample;
					start++;
					start %= size;
				}
				
				Type * getCopy()
				{
					Type * buf = new Type[size];
					// this can be optimized into two memcpy's
					for(std::size_t i = 0; i < size; ++i)
					{
						buf[i] = buffer[(start + i) % size];
						
					}
					return buf;
				}
				
				void clone(CChannelBuffer & other)
				{
					std::memcpy(other.buffer, buffer, buf_size * sizeof(floatType));
					other.size = size;
					other.start = start;
					other.isProcessing = false;
					other.isCircular = isCircular;
					other.sampleRate = sampleRate;
				}
				
				bool raiseAudioEvent(float ** buffer, std::size_t numChannels, std::size_t numSamples)
				{
					cpl::CFastMutex lock(this);
					unsigned mask(0);
					for (auto & listener : listeners)
					{
						mask |= (unsigned)listener->audioCallback(buffer, numChannels, numSamples);
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

				void copyTo(float * buf)
				{
					std::size_t firstChunk = size - start;
					std::memcpy(buf, buffer + start, firstChunk);
					std::memcpy(buf + firstChunk, buffer, start);
					
				}

				void setSampleRate(double newRate)
				{
					sampleRate = newRate;

				}

				bool setLength(double miliseconds)
				{

					auto newLength = cpl::Math::round<std::size_t>((sampleRate / 1000) * miliseconds);
					return setSize(newLength);

				}
				std::size_t maxSize()
				{
					return buf_size;

				}
				bool setSize(std::size_t newSize)
				{
					if (newSize >= buf_size || newSize <= 0)
						return false;

					size = newSize;
					return true;
				}

				floatType & operator[] (std::size_t index)
				{
					// no need to check range, the modulo automagically wraps around
					#ifdef _DEBUG
						if (index > size)
							throw std::range_error("Index out of range for CChannelBuffer");
					#endif
					return buffer[(start + index) % size];

				}
				floatType & singleCheckAccess(std::size_t index)
				{
					
					std::size_t offset = start + index;
					bool mask = offset > (size - 1);

					offset -= size * mask;

					return buffer[offset];
				}
				
				floatType & directAccess(std::size_t index)
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

	};
	typedef cpl::aligned_vector<cpl::CAudioBuffer::CChannelBuffer, 32> AudioBuffer;
#endif