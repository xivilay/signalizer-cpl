/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2017 Janus Lynggaard Thorborg (www.jthorborg.com)

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

	file:ThreadAllocator.h

		A linear, lock-free stack-based fast thread allocator, for small allocations.
		Larger allocations, or if full, will default to the global operator new.

*************************************************************************************/

#ifndef CPL_THREAD_ALLOCATOR_H
#define CPL_THREAD_ALLOCATOR_H

#define CPL_THREAD_ALLOCATOR_SUPPORTED

#include <vector>
#include <cstdint>
#include <assert.h>
#include <algorithm>
#include "../Misc.h"

namespace cpl
{

	class ThreadAllocator
	{
	public:

		static ThreadAllocator& get()
		{
#ifdef CPL_THREAD_ALLOCATOR_SUPPORTED
			thread_local ThreadAllocator allocator;
#else
			static ThreadAllocator allocator;
#endif
			return allocator;
		}

		void* alloc(std::size_t align, std::size_t bytes)
		{
			assert((align & (align - 1)) == 0 && "Alignment must be a power of two");

#ifdef CPL_THREAD_ALLOCATOR_SUPPORTED

			align = std::min(sizeof(Node), align);

			char* base = memory<char>() + position;
			std::uintptr_t integral = reinterpret_cast<std::uintptr_t>(base);
			const auto pad = align - (integral & (align - 1));

			const auto sizeRequired = Node::overhead(pad) + bytes;

			if (sizeRequired + position < storage.size())
			{
				Node* n = Node::emplace(base, pad, bytes);
				position += n->blobSize();
				return n->memory;
			}
			else
			{
				return Node::heap(align, bytes)->memory;
			}
#else
			return cpl::Misc::alignedBytesMalloc(bytes, align);
#endif
		}

		void free(void* p) noexcept
		{
#ifdef CPL_THREAD_ALLOCATOR_SUPPORTED

			Node* n = Node::fromUnmanaged(p);
			
			if (!n->heap())
			{
				assert(n->tip() == memory<char>() + position && "Freed allocation wasn't at the top of the stack");
				position -= Node::fromUnmanaged(p)->blobSize();
			}
			else
			{
				n->heapFree();
			}
#else
			cpl::Misc::alignedFree(p);
#endif
		}

		ThreadAllocator(const ThreadAllocator&) = delete;
		ThreadAllocator(ThreadAllocator&&) = delete;
		ThreadAllocator& operator = (const ThreadAllocator&) = delete;
		ThreadAllocator& operator = (ThreadAllocator&&) = delete;

	private:


		ThreadAllocator()
#ifdef CPL_THREAD_ALLOCATOR_SUPPORTED
			: storage(1 << 14, 0)
#endif
		{

		}

		struct Node
		{
			std::size_t size;
			void* memory;
			char padding[1];

			constexpr static std::size_t overhead(std::size_t pad) noexcept
			{
				return sizeof(Node) + pad + sizeof(void*);
			}

			std::size_t amountOfPad() const noexcept
			{
				return static_cast<std::size_t>(reinterpret_cast<const char*>(memory) - (reinterpret_cast<const char*>(this) + sizeof(Node)));
			}

			std::size_t blobSize() const noexcept
			{
				return static_cast<std::size_t>(tip() - reinterpret_cast<const char*>(this));
			}

			static Node* fromUnmanaged(void* location) noexcept
			{
				return reinterpret_cast<Node*>(reinterpret_cast<void**>(location)[-1]);
			}

			static Node* emplace(char* where, std::size_t pad, std::size_t size) noexcept
			{
				assert(pad > 0);

				Node* n = reinterpret_cast<Node*>(where);
				n->size = size;
				n->memory = where + overhead(pad);
				n->padding[0] = 0;
				reinterpret_cast<void**>(n->memory)[-1] = n;
				return n;
			}

			static Node* heap(std::size_t align, std::size_t size)
			{
				auto memory = cpl::Misc::alignedBytesMalloc(size + overhead(sizeof(void*)), align);

				Node* ret = emplace(reinterpret_cast<char*>(memory), sizeof(void*), size);
				ret->padding[0] = 1;
				return ret;
			}

			void heapFree()
			{
				assert(heap() && "Node wasn't allocated on the heap");
				Node* _this = this;
				cpl::Misc::alignedFree(_this);
			}

			const char* tip() const noexcept
			{
				return reinterpret_cast<const char*>(memory) + size;
			}

			constexpr bool heap() const noexcept
			{
				return padding[0] == 1;
			}

		};

#ifdef CPL_THREAD_ALLOCATOR_SUPPORTED

		template<typename T>
		T* memory() noexcept
		{
			return reinterpret_cast<T*>(storage.data());
		}

		std::size_t position = 0;
		std::vector<char> storage;

#endif
	};

	class ScopedThreadBlock
	{
	public:
		ScopedThreadBlock(std::size_t align, std::size_t size)
			: mem(ThreadAllocator::get().alloc(align, size))
		{

		}

		void* release() noexcept
		{
			auto local = mem;
			mem = nullptr;
			return local;
		}

		void* get() noexcept
		{
			return mem;
		}

		~ScopedThreadBlock()
		{
			if (mem)
			{
				ThreadAllocator::get().free(mem);
			}
		}

		void* mem;
	};

};
#endif