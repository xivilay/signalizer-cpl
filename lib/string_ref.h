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

	file:string_ref.h

		Something very similar to std::string_view, but is guaranteed to be null-terminated,
		and always contain a valid string.

*************************************************************************************/

#ifndef CPL_STRING_REF_H
#define CPL_STRING_REF_H

#include <string_view>
#include <string>

namespace cpl
{
	namespace detail
	{
		template<typename T>
		class precondition_zstr
		{
		public:

			precondition_zstr()
			{

			}

			precondition_zstr(const T* begin, const T* end)
			{
				if (!begin || !end)
					throw std::logic_error("invalid begin/end pairs to basic_string_ref");

				if(*end != '\0')
					throw std::logic_error("*end != '\0' in basic_string_ref");
			}

			precondition_zstr(const T* begin)
			{
				if (!begin)
					throw std::logic_error("invalid begin/end pairs to basic_string_ref");
			}

			precondition_zstr(const T* begin, std::size_t size)
			{
				if (!begin)
					throw std::logic_error("invalid begin/end pairs to basic_string_ref");

				if (begin[size] != '\0')
					throw std::logic_error("invalid str[size] != '\0'");
			}
		};
	}

	template<typename T>
	class basic_string_ref : private detail::precondition_zstr<T>, public std::basic_string_view<T>
	{
	public:
		
		using typename std::basic_string_view<T>::size_type;

		basic_string_ref(const basic_string_ref<T>& other)
			: std::basic_string_view<T>(other)
		{

		}

		basic_string_ref(const T* begin)
			: detail::precondition_zstr<T>(begin)
			, std::basic_string_view<T>(begin)
		{

		}

		basic_string_ref(const T* begin, std::size_t size)
			: detail::precondition_zstr<T>(begin, size)
			, std::basic_string_view<T>(begin, size)
		{

		}

		basic_string_ref(const T* begin, const T* end)
			: detail::precondition_zstr<T>(begin, end)
			, std::basic_string_view<T>(begin, end - begin)
		{

		}

		basic_string_ref(const std::string& str)
			: std::basic_string_view<T>(str.c_str(), str.size())
		{

		}

		constexpr const T* c_str() const noexcept
		{
			return data();
		}

		std::basic_string<T> string() const
		{
			return { begin(), end() };
		}

		constexpr void swap(basic_string_ref& v) noexcept
		{
			std::basic_string_view<T>::swap(v);
		}

	private:

		constexpr void remove_prefix(size_type n);
		constexpr void remove_suffix(size_type n);
	};

	typedef basic_string_ref<char> string_ref;

};
#endif

