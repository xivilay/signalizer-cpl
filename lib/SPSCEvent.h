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

	file:AlignedAllocator.h
	
		An allocator for stl containers, that allocates N-aligned memory.

*************************************************************************************/

#ifndef _SPSC_EVENT_H
	#define _SPSC_EVENT_H
	
	#include "../common.h"
	#include <atomic>

	#ifdef __C11__
		#include <stdatomic>
	#endif

	#if ATOMIC_LLONG_LOCK_FREE != 2
		#pragma message cwarn("warning: Atomic integer operations are not lock-free for this platform!") 
	#endif

	#if ATOMIC_POINTER_LOCK_FREE != 2
		#pragma message cwarn("warning: Atomic pointer operations are not lock-free for this platform!") 
	#endif

	namespace cpl
	{


	};
#endif