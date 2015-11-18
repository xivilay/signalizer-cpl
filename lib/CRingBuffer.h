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

	file:CRingBuffer.h
		
		A class buffer that supports efficient wrap-around and iterators, acting like a close
		ring.
		This class is NOT thread-safe, it is your job to protect it.
		TODO: refactor dual memory system into a templated parameter allocator class,
		and refactor proxyview and writer into a single class controlled by const overloads.

*************************************************************************************/

#ifndef _CRINGBUFFER_H
	#define _CRINGBUFFER_H

	#include "CDataBuffer.h"

	namespace cpl
	{
		template<typename T, std::size_t alignment>
			class CRingBuffer
			{

			public:

				typedef CRingBuffer<T, alignment> CRBuf;
				typedef CDataBuffer<T, alignment> DataBuf;

				// TODO: ensure sizeof(T) >= 2
				typedef typename std::make_signed<std::size_t>::type ssize_t;

				class ProxyView;
				friend class ProxyView;

				class IteratorBase
				{
				public:
					IteratorBase(const CRBuf & parent)
						: start(parent.start), bsize(parent.size), buffer(parent.memory)
						#ifdef _DEBUG
							, parent(parent)
						#endif
					{

					}
					/// <summary>
					/// Indicates to the processor to bring the second part into the cache as well
					/// </summary>
					/// <returns></returns>
					inline void prefetchSecondPart() const noexcept { _mm_prefetch(second(), _MM_HINT_T1); }

					inline const T * first() const noexcept { return buffer + start; }
					inline const T * firstEnd() const noexcept { return buffer + bsize; }
					inline const T * second() const noexcept { return begin(); }
					inline const T * secondEnd() const noexcept { return first(); }
					inline const T * begin() const noexcept { return buffer; }
					inline const T * end() const noexcept { return buffer + bsize; }

					inline T * first() noexcept { return buffer + start; }
					inline T * firstEnd() noexcept { return buffer + bsize; }
					inline T * second() noexcept { return begin(); }
					inline T * secondEnd() noexcept { return first(); }
					inline T * begin() noexcept { return buffer; }
					inline T * end() noexcept { return buffer + bsize; }

					std::size_t size() const noexcept { return bsize; }
					std::size_t startPosition() const noexcept { return start; }

					/// <summary>
					/// If index is zero, returns first(), otherwise returns second()
					/// </summary>
					/// <param name="index"></param>
					/// <returns></returns>
					inline const T * getItIndex(std::size_t index) const noexcept
					{
						return buffer + (index ? 0 : start);
					}

					inline T * getItIndex(std::size_t index) noexcept
					{
						return buffer + (index ? 0 : start);
					}

					/// <summary>
					/// returns the valid range for the iterator index.
					/// </summary>
					/// <param name="index"></param>
					/// <returns></returns>
					inline std::size_t getItRange(std::size_t index) const noexcept
					{
						return index ? start : bsize - start;
					}

					~IteratorBase()
					{
						#ifdef _DEBUG
							parent.releaseProxy(*this);
						#endif
					}

				protected:
					std::size_t
						start,
						bsize;

					T * buffer;

				private:
					#ifdef _DEBUG
						const CRBuf & parent;
					#endif
				};

				/// <summary>
				/// Provides a constant view of a CRingBuffer.
				/// There are three idiomatic ways to iterate the buffer biased, with 1 being the slowest;
				///		1. 
				///			for(int i = 0; i < proxy.size(); ++i)
				///				proxy[i];
				///		2. 
				///			for(auto it = proxy.first(); it != proxy.firstEnd(); ++it)
				///				*it;
				///			for(auto it = proxy.second(); it != proxy.secondEnd(); ++it)
				///				*it;
				///		3. 
				///			for(int idx = 0; idx < 2; ++idx)
				///			{
				///				auto end = proxy.getItIndex(idx) + proxy.getItRange(idx);
				///				for(auto it = proxy.getItIndex(idx); it != end; ++it)
				///					*it; 
				///			}
				/// For unbiased access, one can do:
				///		1.
				///			for(auto & el : proxy)
				///				;
				///		2. 
				///			for(int i = 0; i < proxy.size(); ++i)
				///				proxy.unbiasedDirectAccess(i);
				/// </summary>
				class ProxyView : public IteratorBase
				{
				public:

					ProxyView(const CRBuf & buf)
						: IteratorBase(buf)
					{

					}
					/// <summary>
					/// Wraps around size. Biased: index 0 = current head of buffer (not the start of it)
					/// </summary>
					/// <param name="index"></param>
					/// <returns></returns>
					inline const T & operator [] (std::size_t index) const noexcept
					{
						// TODO: overflow checks.
						return buffer[(start + index) % bsize];
					}

					T & nonconst(std::size_t index)
					{
						return buffer[(start + index) % bsize];
					}
					/// <summary>
					/// Wraps around size, unbiased: index 0 = buffer start
					/// </summary>
					/// <param name="index"></param>
					/// <returns></returns>
					inline const T & unbiasedAccess(std::size_t index) const noexcept
					{
						// TODO: overflow checks.
						return buffer[index % bsize];
					}

					/// <summary>
					/// Does NOT wrap around size, unbiased: index 0 = buffer start
					/// </summary>
					/// <param name="index"></param>
					/// <returns></returns>
					inline const T & unbiasedDirectAccess(std::size_t index) const noexcept
					{
						// TODO: overflow checks.
						#ifdef _DEBUG
							if (index >= bsize)
								CPL_RUNTIME_EXCEPTION("Index out of bounds");
						#endif
						return buffer[index];
					}

				};

				class Writer : public IteratorBase
				{
				public:
					Writer(CRBuf & buf)
						: IteratorBase(buf), ncParent(buf)
					{
						
					}

					void copyIntoHead(const T * mem, std::size_t bufSize)
					{
						// TODO: if bufSize > size() only copy remainder
						ssize_t n = bufSize;
						while (n > 0)
						{
							auto const space = getItRange(0);
							auto const part = std::min(ssize_t(space), n - std::max(ssize_t(0), n - ssize_t(space)));

							if (part > 0)
							{
								std::memcpy(first(), mem + bufSize - n, part * sizeof(T));
								advance(part);
							}

							n -= part;
						}

					}

					void setHeadAndAdvance(const T & newElement)
					{
						buffer[start] = newElement;
						advance(1);
					}

					void advance(std::size_t bufSize)
					{
						start += bufSize;
						start %= bsize;
					}

					~Writer()
					{
						ncParent.start = start;
					}

				private:

					CRBuf & ncParent;
				};

				CRingBuffer()
					: start(), size(), capacity(), memory(), isUsingOwnBuffer(true)
				{

				}

				CRingBuffer(const CRBuf & other);
				CRingBuffer(CRBuf && other);
				CRingBuffer & operator = (const CRBuf & other);
				CRingBuffer & operator = (CRBuf && other);

				const ProxyView createProxyView() const noexcept
				{
					#ifdef _DEBUG
						proxyCount++; 
					#endif
					return *this;
				}

				Writer createWriter()
				{
					#ifdef _DEBUG
						proxyCount++; 
					#endif
					return *this;
				}

				void setCapacity(std::size_t numElements)
				{
					if (numElements != capacity)
					{
						setSize(std::min(capacity, size));
						resize(numElements);
					}

				}

				void setSize(std::size_t elements)
				{
					#ifdef _DEBUG
						if (elements > capacity)
							CPL_RUNTIME_EXCEPTION("Index out of bounds");
					#endif
					shuffleBufferAround(elements);
				}

				/// <summary>
				/// Memory buffer is used for all subsequent operations.
				/// Alignment of memory buffer must match alignment of template parameter.
				/// Buffer is NOT taken ownership of, and must remain valid until 
				/// either this function is called again, unuseMemoryBuffer() is called, or this object's
				/// lifetime is over.
				/// </summary>
				/// <param name="memory"></param>
				/// <param name="bufferSize"></param>
				void setMemoryBuffer(T * memoryToUse, std::size_t bufferSize)
				{
					if (memory % alignment != 0)
						CPL_RUNTIME_EXCEPTION("Unaligned memory buffer provided!");

					if (isUsingOwnBuffer)
					{
						// TODO: copy over contents
						isUsingOwnBuffer = false;
					}
					else
					{
						shuffleBufferAround(bufferSize);
					}
					memory = memoryToUse;
					capacity = bufferSize;
					internalBuffer.clear();
				}

				/// <summary>
				/// Causes the ring buffer to copy the contents of the previous memory
				/// buffer given by setMemoryBuffer into an internal buffer, which it uses from that
				/// point on.
				/// </summary>
				/// <param name="memoryToUnuse"></param>
				void unuseMemoryBuffer(T * memoryToUnuse)
				{
					if (!isUsingOwnBuffer)
						CPL_RUNTIME_EXCEPTION("No previous buffer was provided to be used.");
					if (memory != memoryToUse)
						CPL_RUNTIME_EXCEPTION("Mismatch between memory buffers.");

					isUsingOwnBuffer = true;
					internalBuffer = DataBuf(internalBuffer, capacity);
					memory = internalBuffer.data();
				}

			private:
				/// <summary>
				/// Handles 'size', and tries to ensure buffer contents
				/// </summary>
				void shuffleBufferAround(std::size_t newSize)
				{
					if (newSize < size)
					{

					}
					size = newSize;
				}

				/// <summary>
				/// Only handles capacity
				/// </summary>
				void resize(std::size_t newSize)
				{
					if (isUsingOwnBuffer)
					{
						internalBuffer.resize(newSize);
						memory = internalBuffer.data();
						capacity = newSize;
					}
				}

				#ifdef _DEBUG
					mutable std::atomic<int> proxyCount;

					void releaseProxy(IteratorBase & p) const 
					{
						proxyCount--;
					}
				#endif

				std::size_t
					start,
					size,
					capacity;

				T * memory;
				bool isUsingOwnBuffer;
				DataBuf internalBuffer;

			};
	};
#endif