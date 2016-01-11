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

	file:CMutex.h
		
		Provides an interface for easily locking objects through RAII so long as
		they derive from CMutex::Lockable.
		Uses an special spinlock that yields the thread instead of busy-waiting.
		time-outs after a specified interval, and asks the user what to do, as well
		as providing debugger breakpoints in case of locks.

*************************************************************************************/

#ifndef _CMUTEX_H
	#define _CMUTEX_H

	#include "MacroConstants.h"
	#include "Misc.h"
	#ifndef __CPP11__
		#error "No mutex support!"
	#else
		#include <thread>
		#include <atomic>
	#endif
	namespace cpl
	{

		class MutexBase
		{
			/// <summary>
			/// Concepts: BasicLockable + Lockable + TimedLockable
			/// </summary>
			class Mutex
			{
				std::atomic_flag flag;
			public:
				Mutex()
				{
					flag.clear();
				}

				virtual void lock()
				{

				}

				virtual void unlock()
				{

				}

				virtual bool try_lock()
				{

				}

				template<typename T>
					virtual bool try_lock_until()
					{

					}
			};

			class ReintrantMutex : protected Mutex
			{
				std::thread::id ownerThread;



			};




		protected:

			void release()
			{
				// default value
				if (resource)
				{
					l->ownerThread = std::thread::id();

					l->flag.clear();
					resource = nullptr;
				}

			}

			Mutex * resource;

		};

		/// <summary>
		/// Reintrant scoped lock guard for mutual exclusion.
		/// Has a default timeout, in which it asks the user what to do.
		/// </summary>
		class CMutex
		{
		public:

		private:
			Lockable * resource;
		public:
			CMutex(Lockable * l, std::size_t waitTime = 2000)
			{
				resource = l;
				acquire(l, waitTime);

			}
			CMutex(Lockable & l, std::size_t waitTime = 2000)
			{
				resource = &l;
				acquire(l, waitTime);
			}
			CMutex()
				: resource(nullptr)
			{

			}
			~CMutex()
			{
				if (resource)
					release(resource);

			}
			void acquire(Lockable * l, std::size_t waitTime = -1)
			{
				auto this_id = std::this_thread::get_id();
				if (l->ownerThread == this_id)
					return;
				if (!resource)
					resource = l;
				if (!spinLock(2000, resource))
					//explode here
					return;
				l->ownerThread = std::this_thread::get_id();
			}
			void acquire(Lockable & l, std::size_t waitTime = -1)
			{
				acquire(&l, waitTime);
			}
			void release()
			{
				
				if (resource)
					release(resource);
			}
		private:

			static bool spinLock(std::size_t ms, Lockable *  bVal)
			{
				using namespace Misc;
				decltype(QuickTime()) start;
				int ret;

			loop:
				
				start = QuickTime();
				while (bVal->flag.test_and_set(std::memory_order_relaxed)) {
					if ((QuickTime() - start) > ms)
						goto time_out;
					Delay(0);
				}
				// normal exitpoint
				return true;
				// deadlock occurs

			time_out:
				// hello - you have reached this point if mutex was frozen.
				BreakIfDebugged();
				ret = Misc::MsgBox("Deadlock detected in spinlock: Protected resource is not released after max interval. "
					"Wait again (try again), release resource (continue) - can create async issues - or exit (cancel)?",
					_PROGRAM_NAME_ABRV " Error!",
					sConTryCancel | iStop,
					NULL,
					true);
				switch (ret)
				{
				case MsgButton::bTryAgain:
					goto loop;
				case MsgButton::bContinue:
					bVal->flag.clear(); // flip val
					// try again 
					goto loop;
				case MsgButton::bCancel:
					exit(-1);
				}
				// not needed (except for warns)
				return false;
			}
		};
		
		
		class CFastMutex
		{
		private:
			typedef CMutex::Lockable Lockable;
			Lockable * resource;
		public:
			CFastMutex(Lockable * l)
			{
				resource = l;
				acquire(l);
				
			}
			CFastMutex(Lockable & l)
			{
				resource = &l;
				acquire(l);
			}
			CFastMutex()
			: resource(nullptr)
			{
				
			}
			~CFastMutex()
			{
				if (resource)
					release(resource);
				
			}
			void acquire(Lockable * l)
			{
				auto this_id = std::this_thread::get_id();
				if(l->ownerThread == this_id)
					return;
				
				if (!resource)
					resource = l;
				if (!spinLock(resource))
					//explode here
					return;
				l->ownerThread = this_id;
			}
			void acquire(Lockable & l)
			{
				acquire(&l);
			}
			void release()
			{
				
				if (resource)
					release(resource);
			}
		private:
			void release(Lockable * l)
			{
				l->ownerThread = std::thread::id();
				if (l)
					l->flag.clear();
				resource = nullptr;
				
			}
			static bool spinLock(Lockable *  bVal, std::size_t ms = 2000)
			{
				using namespace Misc;
				decltype(QuickTime()) start;
				int ret;

			loop:

				start = QuickTime();
				while (bVal->flag.test_and_set(std::memory_order_relaxed)) {
					if ((QuickTime() - start) > ms)
						goto time_out;
				}
				// normal exitpoint
				return true;
				// deadlock occurs

			time_out:
				// hello - you have reached this point if mutex was frozen.
				BreakIfDebugged();
				ret = Misc::MsgBox("Deadlock detected in spinlock: Protected resource is not released after max interval. "
					"Wait again (try again), release resource (continue) - can create async issues - or exit (cancel)?",
					_PROGRAM_NAME_ABRV " Error!",
					sConTryCancel | iStop,
					NULL,
					true);
				switch (ret)
				{
				case MsgButton::bTryAgain:
					goto loop;
				case MsgButton::bContinue:
					bVal->flag.clear(); // flip val
										// try again 
					goto loop;
				case MsgButton::bCancel:
					exit(-1);
				}
				// not needed (except for warns)
				return false;
			}
		};
	};
#endif