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

	file:variable_array.h

		A "stack"-allocated analog of std::array<>, with runtime size. Cannot be resized once
		created.

*************************************************************************************/

#ifndef CPL_VARIABLE_ARRAY_H
#define CPL_VARIABLE_ARRAY_H

#include "ThreadAllocator.h"
#include <iterator>
#include <initializer_list>

namespace cpl
{
	template<typename T>
	class variable_array
	{
		enum class uninitialized_tag;

	public:

		using value_type = T;
		using reference = T&;
		using const_reference = std::add_lvalue_reference_t<const std::remove_reference_t<reference>>;
		using pointer = value_type*;
		using const_pointer	= const value_type*;
		using iterator = pointer;
		using const_iterator = const pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

		variable_array(std::size_t size, const T& value = T())
			: buffer(static_cast<T*>(ThreadAllocator::get().alloc(alignof(T), sizeof(T) * size)))
			, length(size)
		{
			initialize(value);
		}

		variable_array(const_pointer beginIt, const_pointer endIt)
			: buffer(static_cast<T*>(ThreadAllocator::get().alloc(alignof(T), sizeof(T) * std::distance(beginIt, endIt))))
			, length(std::distance(beginIt, endIt))
		{
			if constexpr(std::is_trivially_copyable<T>::value)
			{
				std::copy(beginIt, endIt, begin());
			}
			else
			{
				for (std::size_t i = 0; i < length; ++i)
					emplace(buffer + i, *beginIt++);
			}
		}

		variable_array(variable_array<T>&& other)
			: buffer(other.buffer)
			, length(other.length)
		{
			other.buffer = nullptr;
		}

		variable_array(const variable_array<T>& other)
			: variable_array<T>(other.length, uninitialized_tag())
		{
			if constexpr(std::is_trivially_copyable<T>::value)
			{
				std::copy(other.begin(), other.end(), begin());
			}
			else
			{
				for (std::size_t i = 0; i < length; ++i)
					emplace(buffer + i, other[i]);
			}

		}

		variable_array(std::initializer_list<T> init)
			: variable_array<T>(init.begin(), init.end())
		{

		}

		template<typename Fn>
		variable_array(std::size_t size, Fn&& generator)
			: variable_array<T>(size, uninitialized_tag())
		{
			for (std::size_t i = 0; i < size; ++i)
			{
				emplace(buffer + i, generator(i));
			}
		}

		~variable_array()
		{
			if (buffer)
			{
				if (length != 0)
				{
					for (auto elem = rbegin(); elem != rend(); ++elem)
						(void)(*elem).~T();
				}

				ThreadAllocator::get().free(buffer);
			}
		}

		const_reference operator[] (std::size_t index) const CPL_NOEXCEPT_IF_RELEASE
		{
#ifdef _DEBUG
			if (index >= length)
				CPL_RUNTIME_EXCEPTION("Index out of bounds in variable_array");
#endif
			return buffer[index];
		}

		reference operator[] (std::size_t index) CPL_NOEXCEPT_IF_RELEASE
		{
#ifdef _DEBUG
			if (index >= length)
				CPL_RUNTIME_EXCEPTION("Index out of bounds in variable_array");
#endif
			return buffer[index];
		}

		const_reference at(std::size_t index) const
		{
			if (index >= length)
				CPL_RUNTIME_EXCEPTION("Index out of bounds in variable_array");

			return buffer[index];
		}

		reference at(std::size_t index)
		{
			if (index >= length)
				CPL_RUNTIME_EXCEPTION("Index out of bounds in variable_array");

			return buffer[index];
		}

		const_pointer data() const noexcept { return buffer; }
		pointer data() noexcept { return buffer; }

		const_pointer begin() const noexcept { return buffer; }
		const_pointer end() const noexcept { return buffer + length; }

		const_pointer cbegin() const noexcept { return buffer; }
		const_pointer cend() const noexcept { return buffer + length; }

		pointer begin() noexcept { return buffer; }
		pointer end() noexcept { return buffer + length; }

		reverse_iterator rbegin() CPL_NOEXCEPT_IF_RELEASE { return std::make_reverse_iterator(end()); }
		reverse_iterator rend() CPL_NOEXCEPT_IF_RELEASE { return std::make_reverse_iterator(begin()); }

		const_reverse_iterator crbegin() const CPL_NOEXCEPT_IF_RELEASE { return std::make_reverse_iterator(end()); }
		const_reverse_iterator crend() const CPL_NOEXCEPT_IF_RELEASE { return std::make_reverse_iterator(begin()); }

		const_reference front() const CPL_NOEXCEPT_IF_RELEASE { return operator[](0); }
		const_reference back() const CPL_NOEXCEPT_IF_RELEASE { return operator[](length - 1); }

		reference front() CPL_NOEXCEPT_IF_RELEASE { return operator[](0); }
		reference back() CPL_NOEXCEPT_IF_RELEASE { return operator[](length - 1); }

		std::size_t size() const noexcept { return length; }
		bool empty() const noexcept { return length == 0; }

		void fill(const T& value)
		{
			for (auto& elem : *this)
				elem = value;
		}

		static variable_array<T> uninitialized(std::size_t length)
		{
			return variable_array<T>(length, uninitialized_tag());
		}

	private:

		void emplace(pointer where, const T& value)
		{
			new (where) T(value);
		}

		void emplace(pointer where, T&& value)
		{
			new (where) T(std::move(value));
		}

		void initialize(const T& value = T())
		{
			for (std::size_t i = 0; i < length; ++i)
			{
				new (buffer + i) T(value);
			}
		}

		variable_array(std::size_t size, uninitialized_tag)
			: buffer(static_cast<T*>(ThreadAllocator::get().alloc(alignof(T), sizeof(T) * size)))
			, length(size)
		{

		}

		variable_array<T>& operator = (const variable_array<T>& other) = delete;
		variable_array<T>& operator = (variable_array<T>&& other) = delete;

		void* operator new(std::size_t) = delete;
		void operator delete(void*) = delete;
		void* operator new[](std::size_t) = delete;
		void operator delete[](void*) = delete;

		T* buffer;
		std::size_t length;
	};
}

namespace std
{
	template<typename T>
	inline cpl::variable_array<T>&&
		move(cpl::variable_array<T>& t) noexcept = delete;
}

#endif