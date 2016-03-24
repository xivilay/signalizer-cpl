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

	file:StackBuffer.h
	
		A helper type for dealing with bogous C structs

*************************************************************************************/

#ifndef CPL_STACKBUFFER_H
	#define CPL_STACKBUFFER_H

	#include <cstdlib>
	#include <vector>
	#include "../Misc.h"

	namespace cpl
	{
		template<typename T, std::size_t extraSizeInBytes>
		struct StackBuffer
		{
			typedef StackBuffer<T, extraSizeInBytes> this_t;
			static const std::size_t size = sizeof(T) + extraSizeInBytes;

			static_assert(std::is_pod<T>::value, "cpl::StackBuffer's type must be POD!");

			const T * operator ->() const noexcept { return &get(); }
			T * operator ->() noexcept{ return &get(); }
			T & get() noexcept { return const_cast<T&>(const_cast<const this_t *>(this)->get()); }

			const T & get() const noexcept { return *reinterpret_cast<const T*>(std::addressof(storage)); }
			void zero() { std::memset(std::addressof(storage), 0, size); }

			typename std::aligned_storage<size, alignof(T)>::type storage;
		};

	};
#endif