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

	file:CAudioStream.h
		
		A bridge system allowing effecient data processing between threads that mustn't
		suffer from priority inversion, but still handle locking.
		TODO: Implement growable listener queue
*************************************************************************************/

#ifndef _CONCURRENTSERVICES_H
	#define _CONCURRENTSERVICES_H
	#include <thread>
	#include "Utility.h"
	#include <vector>
	#include <atomic>
	#include <memory>

	namespace cpl
	{
		template<class Object, class Delete = typename std::default_delete<Object>>
		class ConcurrentObjectSwapper : Utility::CNoncopyable
		{

		public:

			template<class Object>
			struct ConcurrentEntry
			{
				ConcurrentEntry()
					: obj(), flag()
				{

				}

				Object * obj;
				std::atomic_bool flag;

				static_assert(ATOMIC_BOOL_LOCK_FREE, "Atomic bools need to be lock free.");
				static_assert(ATOMIC_POINTER_LOCK_FREE, "Atomic pointers need to be lock free.");

				void signalInUse() { flag.store(true); }
				bool isSignaled() const { return flag.load(); }
				void reset(Object * newObj) noexcept { obj = newObj; }
				bool hasContent() const noexcept { return !!obj; }

				void clearAndDelete()
				{
					if (obj)
						Delete()(obj);
					obj = nullptr;
					flag.store(false);
				}


			};

			ConcurrentObjectSwapper()
				: wrapIdx()
			{
				current.store(wrappers);
				old = wrappers + 1;
			}

			/// <summary>
			/// Tries to remove any old objects, returning true if succesful 
			/// or false if none existed or the consumer thread hasn't called
			/// getObject yet.
			/// 'Producer' only.
			/// </summary>
			/// <returns></returns>
			bool tryRemoveOld()
			{
				auto newest = current.load();
				auto oldest = old;

				if (oldest->hasContent() && newest->hasContent() && newest->isSignaled())
				{
					oldest->clearAndDelete();
					return true;
				}

				return false;
			}

			/// <summary>
			/// If successful, Object is taken ownership of, and will be returned from getObject.
			/// Should never be called in a loop, as it potentially never returns true if nothing
			/// calls getObject.
			/// 'Producer' thread only.
			/// </summary>
			/// <param name="newObject"></param>
			/// <returns></returns>
			bool tryReplace(Object * newObject)
			{
				if (!old->hasContent())
				{
					old->reset(newObject);
					old = current.exchange(old);
					return true;
				}

				return false;
			}
			/// <summary>
			/// Returns a null pointer if no objects have been stored yet.
			/// Otherwise, returns a pointer to the newest stored object through
			/// tryReplace.
			/// NOTICE: any objects returned previously through this call may be asynchronously
			/// invalidated.
			/// 'Consumer' thread only.
			/// </summary>
			/// <returns></returns>
			Object * getObject()
			{
				auto ce = current.load();
				Object * retptr = nullptr;
				if (ce->hasContent())
				{
					ce->signalInUse();
					retptr = ce->obj;
				}
				return retptr;
			}

			/// <summary>
			/// Deletes any contained objects.
			/// </summary>
			~ConcurrentObjectSwapper()
			{
				wrappers[0].clearAndDelete();
				wrappers[1].clearAndDelete();
			}

		private:
			std::atomic<ConcurrentEntry<Object> *> current;
			ConcurrentEntry<Object> * old;
			std::size_t wrapIdx;
			ConcurrentEntry<Object> wrappers[2];
		};

	};
#endif