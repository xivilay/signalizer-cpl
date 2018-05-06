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

	file:polystack_ptr.h

		A smart-pointer with identical semantics to that of a unique_ptr.
		It is simply just created through make_polystack.
		Allocation is done through a stack-based thread allocator. 

*************************************************************************************/

#ifndef CPL_POLYSTACK_PTR_H
#define CPL_POLYSTACK_PTR_H

#include "ThreadAllocator.h"
#include <memory>

namespace cpl
{
	template<typename T>
	class StackDeleter
	{
	public:
		void operator()(T* el)
		{
			el->~T();
			ThreadAllocator::get().free(el);
		}
	};

	template<typename T>
	using polystack_ptr = std::unique_ptr<T, StackDeleter<T>>;

	template<typename T, typename... Args>
	inline polystack_ptr<T> make_polystack(Args&&... args)
	{
		ScopedThreadBlock block(alignof(T), sizeof(T));
		polystack_ptr<T> ptr;

		new (block.get()) T(std::forward<Args>(args)...);

		ptr.reset(static_cast<T*>(block.release()));
		return ptr;
	}
}


#endif