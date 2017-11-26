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

	file:zstr_view.h

		Something very similar to std::string_view, but is guaranteed to be null-terminated,
		and always contain a valid string.

*************************************************************************************/

#ifndef CPL_ZSTR_VIEW_H
#define CPL_ZSTR_VIEW_H

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
					throw std::logic_error("invalid begin/end pairs to basic_zstr_view");

				if(*end != '\0')
					throw std::logic_error("*end != '\0' in basic_zstr_view");
			}

			precondition_zstr(const T* begin)
			{
				if (!begin)
					throw std::logic_error("invalid begin/end pairs to basic_zstr_view");
			}

			precondition_zstr(const T* begin, std::size_t size)
			{
				if (!begin)
					throw std::logic_error("invalid begin/end pairs to basic_zstr_view");

				if (begin[size] != '\0')
					throw std::logic_error("invalid str[size] != '\0'");
			}
		};
	}

	template<typename T>
	class basic_zstr_view : private detail::precondition_zstr<T>, public std::basic_string_view<T>
	{
	public:
		
		basic_zstr_view(const basic_zstr_view<T>& other)
			: std::basic_string_view<T>(other)
		{

		}

		basic_zstr_view(const T* begin)
			: detail::precondition_zstr<T>(begin)
			, std::basic_string_view<T>(begin)
		{

		}

		basic_zstr_view(const T* begin, std::size_t size)
			: detail::precondition_zstr<T>(begin, size)
			, std::basic_string_view<T>(begin, size)
		{

		}

		basic_zstr_view(const T* begin, const T* end)
			: detail::precondition_zstr<T>(begin, end)
			, std::basic_string_view<T>(begin, end - begin)
		{

		}

		basic_zstr_view(const std::string& str)
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

		constexpr void swap(basic_zstr_view& v) noexcept
		{
			std::basic_string_view<T>::swap(v);
		}

	private:

		constexpr void remove_prefix(size_type n);
		constexpr void remove_suffix(size_type n);
	};

	typedef basic_zstr_view<char> zstr_view;

};
#endif

