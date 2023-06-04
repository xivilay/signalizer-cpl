/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2023 Janus Lynggaard Thorborg (www.jthorborg.com)

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

	file:uarray.h

		Temporary replacement for std::span until C++20 is supported.

*************************************************************************************/


#ifndef CPL_UARRAY_H
#define CPL_UARRAY_H

#include <cstdint>
#include <vector>
#include <assert.h>
#include <type_traits>

namespace cpl
{
	/// <summary>
	/// An unowned array wrapper - a mutable "view" of something else, that cannot be resized.
	/// Construction parameters are referenced directly, and no copies of data are ever taken / made.
	/// This also means you should take care to ensure referred-to data outlives any uarray. 
	/// Following that, <see cref="uarray"/>s should probably only ever exist on the stack.
	/// </summary>
	/// <typeparam name="T">
	/// The source content type.
	/// Append const to the type for perfectly enforced read-only access to the contents.
	/// </typeparam>
	template<typename T>
	struct uarray
	{
	public:

		/// <summary>
		/// Alias for <typeparamref name="T"/>
		/// </summary>
		typedef T value_type;

		/// <summary>
		/// Construct from a mutable vector.
		/// </summary>
		template<typename Alloc>
		uarray(std::vector<T, Alloc>& source) : uarray(source.data(), source.size()) {}
		/// <summary>
		/// Construct a read-only uarray from a const-qualified vector.
		/// </summary>
		template<typename Alloc>
		uarray(const std::vector<typename std::remove_const_t<T>, Alloc>& source) : uarray(source.data(), source.size()) {}
		/// <summary>
		/// Construct from a possibly cv-qualified source
		/// </summary>
		uarray(T* buffer, std::size_t length)
			: buffer(buffer), length(length)
		{
			assert(buffer);
		}
		/// <summary>
		/// Construct from a possibly cv-qualified iterator pair
		/// </summary>
		uarray(T* begin, T* end)
			: buffer(begin), length(end - begin)
		{
			assert(begin);
			assert(end);
		}

		/// <summary>
		/// Access a potential read-only element at <paramref name="index"/>
		/// </summary>
		T& operator [] (std::size_t index) noexcept
		{
			assert(index < length);
			return buffer[index];
		}

		/// <summary>
		/// Access a read-only element at <paramref name="index"/>
		/// </summary>
		const T& operator [] (std::size_t index) const noexcept
		{
			assert(index < length);
			return buffer[index];
		}

		/// <summary>
		/// Returns an iterator to the beginning of the array pointed to
		/// </summary>
		T* begin() noexcept { return buffer; }
		/// <summary>
		/// Returns a const iterator to the beginning of the array pointed to
		/// </summary>
		const T* begin() const noexcept { return buffer; }
		/// <summary>
		/// Returns an iterator pointing to 1 element past the end of the array pointed to
		/// </summary>
		T* end() noexcept { return buffer + length; }
		/// <summary>
		/// Returns a const iterator pointing to 1 element past the end of the array pointed to
		/// </summary>
		const T* end() const noexcept { return buffer + length; }
		/// <summary>
		/// Retrieve a raw pointer to the array pointed to
		/// </summary>
		T* data() noexcept { return buffer; }
		/// <summary>
		/// Retrieve a const raw pointer to the array pointed to
		/// </summary>
		const T* data() const noexcept { return buffer; }

		/// <summary>
		/// Returns the size of the array pointed to by this <see cref="uarray"/>
		/// </summary>
		std::size_t size() const noexcept { return length; }
		
		/// <summary>
		/// Returns a new, constant uarray formed from a slice of the original.
		/// </summary>
		/// <param name="offset">
		/// How much to skip from the start
		/// </param>
		/// <param name="newLength">
		/// The length of the slice, starting from <paramref name="offset"/>.
		/// The default value adopts the current length, and substract the offset
		/// (ie. the remaining). 
		/// </param>
		uarray<T> slice(std::size_t offset, std::size_t newLength = -1) noexcept
		{
			assert(offset <= length);
			if (newLength == -1)
				newLength = length - offset;

			assert((offset + newLength) <= length);
			return { buffer + offset, newLength };
		}

		template<typename Other>
		typename std::enable_if<std::is_standard_layout<Other>::value && !std::is_const<T>::value, uarray<Other>>::type
			reinterpret() noexcept
		{
			static_assert((sizeof(T) / sizeof(Other)) * sizeof(Other) == sizeof(T), "Other is not divisble by T");
			static_assert(alignof(T) <= alignof(Other), "Other is less aligned than T");

			constexpr auto ratio = sizeof(T) / sizeof(Other);

			return { reinterpret_cast<Other*>(buffer), length * ratio };
		} 

		template<typename Other>
		typename std::enable_if<std::is_standard_layout<Other>::value && std::is_const<T>::value, uarray<const Other>>::type
			reinterpret() const noexcept
		{
			static_assert((sizeof(T) / sizeof(Other)) * sizeof(Other) == sizeof(T), "Other is not divisble by T");
			static_assert(alignof(T) <= alignof(Other), "Other is less aligned than T");

			constexpr auto ratio = sizeof(T) / sizeof(Other);

			return { reinterpret_cast<const Other*>(buffer), length * ratio };
		}

		/// <summary>
		/// Returns a new, constant uarray formed from a slice of the original.
		/// </summary>
		/// <param name="offset">
		/// How much to skip from the start
		/// </param>
		/// <param name="newLength">
		/// The length of the slice, starting from <paramref name="offset"/>.
		/// The default value adopts the current length, and substract the offset
		/// (ie. the remaining). 
		/// </param>
		uarray<const T> slice(std::size_t offset, std::size_t newLength = -1) const noexcept
		{
			assert(offset <= length);

			if (newLength == -1)
				newLength = length - offset;

			assert((offset + newLength) <= length);
			return { buffer + offset, newLength };
		}


		/// <summary>
		/// Implicit conversion operator to a const / read-only version of this <see cref="uarray"/>
		/// </summary>
		operator uarray<const T>() const noexcept
		{
			return { begin(), end() };
		}

	private:

		T* const buffer;
		const std::size_t length;
	};


	/// <summary>
	/// Clear a <see cref="uarray"/> of non-const qualified <typeparamref name="T"/> elements to a default-initialized value. 
	/// </summary>
	template<typename T, typename Alloc>
	inline void clear(std::vector<T, Alloc>& arr) noexcept
	{
		std::fill(arr.begin(), arr.end(), T());
	}

	/// <summary>
	/// Clear a <see cref="uarray"/> of non-const qualified <typeparamref name="T"/> elements to a default-initialized value. 
	/// </summary>
	template<typename T>
	inline typename std::enable_if<!std::is_const_v<T>>::type 
		clear(uarray<T> arr) noexcept
	{
		std::fill(arr.begin(), arr.end(), T());
	}


	template<typename T, typename Alloc>
	inline uarray<T> as_uarray(std::vector<T, Alloc>& vec)
	{
		return { vec };
	}

	template<typename T, typename Alloc>
	inline uarray<const T> as_uarray(const std::vector<T, Alloc>& vec)
	{
		return { vec };
	}

	template<typename T>
	inline uarray<T> as_uarray(T* data, std::size_t size)
	{
		return { data, size };
	}

	template<typename T>
	inline uarray<const T> as_uarray(const T* data, std::size_t size)
	{
		return { data, size };
	}
}

#endif