/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:CBaseControl.h
		
		CBaseControl - base class for all implementable controls.
		all methods prefixed with 'b' to avoid future nameclashing.
		This class provides a common interface for all 'controls' to support
		normalized value-, string and position get/set.
		Also encapsulates listeners into a single one.
		Provides a cheap RAII mutex lock as well, as well as optional reference counting.
		Does not derive from the system's base class for graphics to avoid the diamond
		problem (classes deriving from this should set the base class in their constructor)

*************************************************************************************/

#ifndef ALIGNMENT_ALLOCATOR_H
	#define ALIGNMENT_ALLOCATOR_H

	#include <cstdlib>
	#include "../Misc.h"

	namespace cpl
	{

		template <typename T, std::size_t N = 16>
		class AlignmentAllocator {
		public:
			typedef T value_type;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

			typedef T * pointer;
			typedef const T * const_pointer;

			typedef T & reference;
			typedef const T & const_reference;

		public:
			inline AlignmentAllocator() throw () { }

			template <typename T2>
			inline AlignmentAllocator(const AlignmentAllocator<T2, N> &) throw () { }

			inline ~AlignmentAllocator() throw () { }

			inline pointer adress(reference r) {
				return &r;
			}

			inline const_pointer adress(const_reference r) const {
				return &r;
			}

			inline pointer allocate(size_type n) {
				return Misc::alignedMalloc<value_type, N>(n);
			}

			inline void deallocate(pointer p, size_type) {
				Misc::alignedFree(p);
			}

			inline void construct(pointer p, const value_type & wert) {
				new (p)value_type(wert);
			}

			inline void destroy(pointer p) {
				p->~value_type();
			}

			inline size_type max_size() const throw () {
				return size_type(-1) / sizeof (value_type);
			}

			template <typename T2>
			struct rebind {
				typedef AlignmentAllocator<T2, N> other;
			};

			bool operator!=(const AlignmentAllocator<T, N>& other) const  {
				return !(*this == other);
			}

			// Returns true if and only if storage allocated from *this
			// can be deallocated from other, and vice versa.
			// Always returns true for stateless allocators.
			bool operator==(const AlignmentAllocator<T, N>& other) const {
				return true;
			}
		};
	};
#endif