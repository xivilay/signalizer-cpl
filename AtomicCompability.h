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

	file:AtomicCompability.h
	
		While we're all waiting on standard compliant headers, this will have to do.

*************************************************************************************/

#ifndef CPL_ATOMICCOMPABILITY_H
	#define CPL_ATOMICCOMPABILITY_H

	#ifndef _MSC_VER
		#include <stdatomic>
	#endif
	#include <atomic>

	namespace cpl
	{
		inline void std_memory_fence(std::memory_order order)
		{
			#ifdef CPL_CLANG
				return atomic_thread_fence(order);
			#else
				return std::atomic_thread_fence(order);
			#endif
		}
	};

#endif