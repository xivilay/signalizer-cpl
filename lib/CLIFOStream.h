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

	file:CLIFOStream.h
		
		A class buffer that supports efficient wrap-around and iterators, acting like a closed
		ring.
		This class is NOT thread-safe, it is your job to protect it.
		It is not a FIFO queue, although it supports reading the buffer FIFO-style.
		However, per default, it acts like a LIFO - the produer advances the ring, while the consumer doesn't.
		TODO: refactor dual memory system into a templated parameter allocator class,
		and refactor proxyview and writer into a single class controlled by const overloads.

*************************************************************************************/

#ifndef _CLIFOSTREAM_H
	#define _CLIFOSTREAM_H

	#include "CDataBuffer.h"

	namespace cpl
	{
		template<typename T, std::size_t alignment>
			class CLIFOStream
			{

			public:

				typedef CLIFOStream<T, alignment> CBuf;
				typedef CDataBuffer<T, alignment> DataBuf;

				// TODO: ensure sizeof(T) >= 2
				typedef typename std::make_signed<std::size_t>::type ssize_t;

				class ProxyView;
				friend class ProxyView;

				class IteratorBase
				{
				protected:
					IteratorBase(const CBuf & parent)
						: cursor(parent.cursor), bsize(parent.size), buffer(parent.memory), ncParent(&parent)
					{

					}
				public:

					friend CBuf;

					typedef T * iterator;
					typedef const T * const_iterator;
					static const std::size_t iterator_indices = 2;

					IteratorBase() = delete;
					IteratorBase(const IteratorBase &) = delete;
					IteratorBase & operator = (const IteratorBase &) = delete;
					IteratorBase(IteratorBase && other)
					{
						// TODO: refactor into absorb(), when clang starts not being weird
						// (it wants to bind other to a l-value?)
						cursor = other.cursor;
						bsize = other.bsize;
						buffer = other.buffer;
						other.buffer = nullptr; other.ncParent = nullptr;
					}
					IteratorBase & operator = (IteratorBase && other) noexcept
					{
						absorb(other);
					}

					/// <summary>
					/// Indicates to the processor to bring the second part into the cache as well
					/// </summary>
					inline void prefetchSecondPart() const noexcept { _mm_prefetch(second(), _MM_HINT_T1); }

					inline const T * first() const noexcept { return buffer + cursor; }
					inline const T * firstEnd() const noexcept { return buffer + bsize; }
					inline const T * second() const noexcept { return begin(); }
					inline const T * secondEnd() const noexcept { return first(); }
					inline const T * begin() const noexcept { return buffer; }
					inline const T * end() const noexcept { return buffer + bsize; }

					inline T * first() noexcept { return buffer + cursor; }
					inline T * firstEnd() noexcept { return buffer + bsize; }
					inline T * second() noexcept { return begin(); }
					inline T * secondEnd() noexcept { return first(); }
					inline T * begin() noexcept { return buffer; }
					inline T * end() noexcept { return buffer + bsize; }

					inline std::size_t size() const noexcept { return bsize; }
					inline std::size_t cursorPosition() const noexcept { return cursor; }

					/// <summary>
					/// If index is zero, returns first(), otherwise returns second()
					/// </summary>
					/// <param name="index"></param>
					/// <returns></returns>
					inline const T * getItIndex(std::size_t index) const noexcept
					{
						return buffer + (!index) * cursor;
					}

					inline T * getItIndex(std::size_t index) noexcept
					{
						return buffer + (!index) * cursor;
					}

					/// <summary>
					/// returns the valid range for the iterator index.
					/// </summary>
					/// <param name="index"></param>
					/// <returns></returns>
					inline std::size_t getItRange(std::size_t index) const noexcept
					{
						return index ? cursor : bsize - cursor;
					}

					~IteratorBase()
					{

					}

				protected:
					
					void absorb(IteratorBase && other) noexcept
					{
						cursor = other.cursor;
						bsize = other.bsize;
						buffer = other.buffer;
						other.buffer = nullptr; other.ncParent = nullptr;
					}

					std::size_t
						cursor,
						bsize;

					T * buffer;

					const CBuf * ncParent;
				};

				/// <summary>
				/// Provides a constant view of a CLIFOStream.
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
				///			for(int idx = 0; idx < proxy.iterator_indices; ++idx)
				///			{
				///				auto start = proxy.getItIndex(idx)
				///				auto end = start + proxy.getItRange(idx);
				///				for(auto it = start; it != end; ++it)
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

					ProxyView(const CBuf & buf)
						: IteratorBase(buf)
					{
					}

					ProxyView(ProxyView && other)
						: IteratorBase(static_cast<IteratorBase &&>(other))
					{
					}

					ProxyView & operator = (ProxyView && other) noexcept
					{
						absorb(other);
					}

					/// <summary>
					/// Wraps around size. Biased: index 0 = current head of buffer (not the cursor of it)
					/// </summary>
					inline const T & operator [] (std::size_t index) const noexcept
					{
						// TODO: overflow checks.
						#ifdef _DEBUG
							if (cursor + index > cursor)
								CPL_RUNTIME_EXCEPTION("Overflow error");
						#endif
						return this->buffer[(cursor + index) % this->bsize];
					}

					inline T & nonconst(std::size_t index)
					{
						#ifdef _DEBUG
							if (index >= this->bsize)
								CPL_RUNTIME_EXCEPTION("Index out of bounds");
						#endif
						#ifdef _DEBUG
							if (cursor + index > cursor)
								CPL_RUNTIME_EXCEPTION("Overflow error");
						#endif
						return this->buffer[(cursor + index) % this->bsize];
					}
					/// <summary>
					/// Wraps around size, unbiased: index 0 = buffer cursor.
					/// Does NOT wrap around size.
					/// </summary>
					inline const T & unbiasedAccess(std::size_t index) const noexcept
					{
						return this->buffer[index % this->bsize];
					}

					/// <summary>
					/// Does NOT wrap around size, unbiased: index 0 = buffer cursor
					/// </summary>
					inline const T & unbiasedDirectAccess(std::size_t index) const noexcept
					{
						#ifdef _DEBUG
							if (index >= this->bsize)
								CPL_RUNTIME_EXCEPTION("Index out of bounds");
						#endif
						return this->buffer[index];
					}

					/// <summary>
					/// Copies the data from head into mem.
					/// Safe for any bufSize, but it will wrap around, producing a circular output.
					/// </summary>
					void copyFromHead(const T * mem, std::size_t bufSize) const noexcept
					{
						// TODO: if bufSize > size() only copy remainder
						// TODO: handle mem + bufsize > ptrsize()
						// TODO: handle space > ssize_t::max()
						ssize_t n = bufSize;

						Types::fuint_t it = 0;

						while (n > 0)
						{
							auto const space = this->getItRange(it);
							auto const part = std::min(ssize_t(space), n - std::max(ssize_t(0), n - ssize_t(space)));


							auto head = this->getItIndex(it);

							if (part > 0)
							{
								std::memcpy(mem + bufSize - n, head, part * sizeof(T));
							}

							it ^= 1;

							n -= part;
						}

					}

					~ProxyView()
					{
						if (this->ncParent)
						{
							this->ncParent->releaseReader(*this);
						}
					}
				};

				class Writer : public IteratorBase
				{
				public:
					Writer(CBuf & buf)
						: IteratorBase(buf), parent(&buf)
					{
						
					}

					Writer(Writer && other)
						: IteratorBase((IteratorBase &&)other), parent(other.parent)
					{
						other.parent = nullptr; other.ncParent = nullptr;
					}

					Writer & operator = (Writer && other) noexcept
					{
						absorb(other);
						parent = other.parent;
						other.parent = other.ncParent = nullptr;
					}
					/// <summary>
					/// Copies the data from memory into buffer at the head.
					/// Safe for any bufSize, but it will wrap around.
					/// </summary>
					void copyIntoHead(const T * mem, std::size_t bufSize)
					{
						// TODO: handle mem + bufsize > ptrsize()
						// TODO: handle space > ssize_t::max()
						ssize_t n = (ssize_t)bufSize + std::min((ssize_t)this->size() - (ssize_t)bufSize, (ssize_t)0);
						while (n > 0)
						{
							auto const space = this->getItRange(0);
							auto const part = std::min(ssize_t(space), n - std::max(ssize_t(0), n - ssize_t(space)));

							if (part > 0)
							{
								std::memcpy(this->first(), mem + bufSize - n, part * sizeof(T));
								advance(part);
							}

							n -= part;
						}

					}
					/// <summary>
					/// Sets the current value at the head, and advances one element.
					/// </summary>
					void setHeadAndAdvance(const T & newElement) noexcept
					{
						this->buffer[cursor] = newElement;
						advance(1);
					}

					/// <summary>
					/// Alters the cursor position. Capped at size()
					/// </summary>
					void seek(std::size_t newCursor) noexcept
					{
						cursor = std::min(this->bsize, newCursor);
					}

					/// <summary>
					/// Advances the read head.
					/// </summary>
					inline void advance(std::size_t bufSize) noexcept
					{
						this->cursor += bufSize;
						this->cursor %= this->bsize;
					}

					~Writer()
					{
						if (parent)
						{
							parent->releaseWriter(*this);
						}
					}

				private:
					CBuf * parent;
				};

				CLIFOStream()
					: cursor(), size(), capacity(), memory(), isUsingOwnBuffer(true), hasReader(), hasWriter()
				{

				}

				CLIFOStream(CBuf && other)
					: cursor(other.cursor), size(other.size), capacity(other.capacity), 
					memory(other.memory), isUsingOwnBuffer(other.isUsingOwnBuffer)
				{
					if (other.isUsingOwnBuffer)
					{
						internalBuffer = std::move(other.internalBuffer);
					}

					other.memory = nullptr;
					other.size = other.cursor = other.capacity = 0;
					other.isUsingOwnBuffer = true;

					if (other.hasReader || other.hasWriter)
					{
						CPL_RUNTIME_EXCEPTION("CLIFOStream moved while it has either a reader or a writer");
					}
					hasReader = hasWriter = false;
				}

				CLIFOStream & operator = (const CBuf & other) = delete;
				CLIFOStream & operator = (CBuf && other) = delete;
				CLIFOStream(const CBuf & other) = delete;

				ProxyView createProxyView() const 
				{
					#ifdef _DEBUG
						proxyCount++; 
					#endif
					if(hasWriter)
					{
						CPL_RUNTIME_EXCEPTION("Unsafe reader created, while writer exists!");
					}
					hasReader = true;
					return *this;
				}

				Writer createWriter()
				{
					#ifdef _DEBUG
						proxyCount++; 
					#endif
					if (hasReader)
					{
						CPL_RUNTIME_EXCEPTION("Unsafe writer created, while reader exists!");
					}
					hasWriter = true;
					return *this;
				}
				/// <summary>
				/// Sets the maximum size of the container, possibly reallocating (but preserving)
				/// the memory. size() may be reduced to newCapacity.
				/// </summary>
				void setCapacity(std::size_t newCapacity)
				{
					if (hasReader || hasWriter)
					{
						CPL_RUNTIME_EXCEPTION("CLIFOStream resized while it is accessed!");
					}

					if (newCapacity != capacity)
					{
						if (newCapacity < size)
						{
							setSize(newCapacity);
						}
						resize(newCapacity);
					}

				}


				/// <summary>
				/// Sets the virtual size of this container. Incurs no memory reallocations.
				/// </summary>
				/// <param name="modifyDataToFit">
				/// If set, rotates the data around such chronological integrity is preserved
				/// </param>
				/// <param name="dataFill">
				/// A value that is used to initialize any empty space created by increasing the size.
				/// Only used if modifyDataToFit is set.
				/// </param>
				void setSize(std::size_t newSize, bool modifyDataToFit = true, const T & dataFill = T())
				{
					if (hasReader || hasWriter)
					{
						CPL_RUNTIME_EXCEPTION("CLIFOStream resized while it is accessed!");
					}

					if (newSize > capacity)
						CPL_RUNTIME_EXCEPTION("Index out of bounds");

					configureNewSize(newSize, dataFill, modifyDataToFit);
				}

				/// <summary>
				/// Combined setSize & setCapacity
				/// </summary>
				void setStorageRequirements(std::size_t psize, std::size_t pcapacity, bool modifyDataToFit = true, const T & dataFill = T())
				{
					if (hasReader || hasWriter)
					{
						CPL_RUNTIME_EXCEPTION("CLIFOStream resized while it is accessed!");
					}

					if (psize > pcapacity)
						CPL_RUNTIME_EXCEPTION("Index out of bounds");

					if (pcapacity != capacity)
					{
						// notice the order of the resize/sizing here:
						// if we get less memory, we have to resize firstly, 
						// so the reordering inside configureNewSize()
						// works correctly (it maybe has to copy something that
						// will be out of bounds after a resize()).
						// same for the other case: if the size and capacity grows,
						// we have to resize firstly.
						if (pcapacity < size)
						{
							if(psize != size)
								configureNewSize(psize, modifyDataToFit, dataFill);
							resize(pcapacity);
						}
						else
						{
							resize(pcapacity);
							if (psize != size)
								configureNewSize(psize, modifyDataToFit, dataFill);
						}

					}
					else if (psize != size)
					{
						configureNewSize(psize, modifyDataToFit, dataFill);
					}

				}

				/// <summary>
				/// Memory buffer is used for all subsequent operations.
				/// Alignment of memory buffer must match alignment of template parameter.
				/// Buffer is NOT taken ownership of, and must remain valid until 
				/// either this function is called again, unuseMemoryBuffer() is called, or this object's
				/// lifetime is over.
				/// Alters capacity and size
				/// </summary>
				void setMemoryBuffer(T * memoryToUse, std::size_t bufferSize)
				{
					if ((std::uintptr_t)memory % alignment != 0)
						CPL_RUNTIME_EXCEPTION("Unaligned memory buffer provided!");

					if (isUsingOwnBuffer)
					{
						// TODO: copy over contents
						isUsingOwnBuffer = false;
					}
					else
					{
						setSizeInternal(bufferSize);
						internalBuffer.clear();
					}
					memory = memoryToUse;
					capacity = bufferSize;

				}

				/// <summary>
				/// Causes the stream to copy the contents of the previous memory
				/// buffer given by setMemoryBuffer (should match input parameter) into an internal buffer, which it uses from that
				/// point on.
				/// </summary>
				/// <param name="memoryToUnuse"></param>
				void unuseMemoryBuffer(T * memoryToUnuse)
				{
					if (!isUsingOwnBuffer)
						CPL_RUNTIME_EXCEPTION("No previous buffer was provided to be used.");
					if (memory != memoryToUnuse)
						CPL_RUNTIME_EXCEPTION("Mismatch between memory buffers.");

					// TODO: copy memory?
					isUsingOwnBuffer = true;
					internalBuffer = DataBuf(internalBuffer, capacity);
					memory = internalBuffer.data();
				}

				std::size_t getSize() const noexcept { return size; }
				std::size_t getCapacity() const noexcept { return capacity; }

			private:
				/// <summary>
				/// Handles 'size', and ensures buffer contents. That is, if newSize < size, the last
				/// newSize samples are preserved in correct order. If newSize > size, (newSize - size) 
				/// elements (filler) are inserted at the cursor, without changing it.
				/// doesn't consider capacity
				/// </summary>
				void configureNewSize(std::size_t newSize, bool modifyDataToFit = true, const T & filler = T())
				{
					if (size != 0 && newSize != size && modifyDataToFit)
					{
						auto ptr = internalBuffer.begin();
						if (size > newSize)
						{

							if (cursor > newSize)
							{
								auto newCenter = ((ssize_t)size - ((ssize_t)newSize - (ssize_t)cursor)) % size;
								// TODO: find memmove() formula
								std::rotate(internalBuffer.begin(), internalBuffer.begin() + newCenter, internalBuffer.begin() + size);
								cursor = 0;
							}
							else
							{
								auto shift = size - newSize;
								std::memmove(ptr + cursor, ptr + cursor + shift, sizeof(T) * (size - (cursor + shift)));
							}


						}
						else
						{
							// diff = cursor - size
							auto diff = newSize - size;
							std::memmove(ptr + cursor + diff, ptr + cursor, sizeof(T) * (size - cursor));
							std::fill(ptr + cursor, ptr + cursor + diff, filler);
						}
					}
					if (newSize == 0)
						cursor = 0;
					else
						cursor %= newSize;

					if (size == 0 && newSize != 0)
					{
						std::fill(internalBuffer.begin(), internalBuffer.end(), filler);
					}

					setSizeInternal(newSize);
				}

				void setSizeInternal(std::size_t newSize)
				{
					size = newSize;
				}

				/// <summary>
				/// Only handles capacity
				/// </summary>
				void resize(std::size_t newCapacity)
				{
					if (isUsingOwnBuffer)
					{
						internalBuffer.resize(newCapacity);
						memory = internalBuffer.data();
					}
					else
					{
						if (newCapacity > capacity)
						{
							CPL_RUNTIME_EXCEPTION("CLIFOStream out of memory (unable to grow unowned memory");
						}
					}
					capacity = newCapacity;
				}

				#ifdef _DEBUG
					mutable std::atomic<int> proxyCount;

					void releaseProxy(IteratorBase & p) const 
					{
						proxyCount--;
					}
				#endif

				void releaseReader(ProxyView & p) const
				{
					#ifdef _DEBUG
						proxyCount--;
						if (hasWriter)
						{
							CPL_RUNTIME_EXCEPTION("Reader released while writer exists!");
						}
					#endif
					hasReader = false;
				}

				void releaseWriter(Writer & p)
				{
					#ifdef _DEBUG
						proxyCount--;
						if (hasReader)
						{
							CPL_RUNTIME_EXCEPTION("Writer released while reader exists!");
						}
					#endif
					if (!hasWriter)
					{
						CPL_RUNTIME_EXCEPTION("Writer released while it shouldn't exist (multipled writers created?)!");
					}
					hasWriter = false;
					cursor = p.cursor;
				}

				std::size_t
					cursor,
					size,
					capacity;

				T * memory;
				bool isUsingOwnBuffer;
				mutable bool hasReader, hasWriter;
				DataBuf internalBuffer;

			};
	};
#endif