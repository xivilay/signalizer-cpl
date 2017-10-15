/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2016 Janus Lynggaard Thorborg [LightBridge Studios]

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

#ifndef CPL_CMUTEX_H
#define CPL_CMUTEX_H

#include "MacroConstants.h"
#ifndef __CPP11__
#error "No mutex support!"
#else
#include <thread>
#include <atomic>
#endif
namespace cpl
{
	int AlertUserAboutMutex();

	class CMutex
	{
	public:
		class Lockable
		{
			friend class CMutex;
			friend class CFastMutex;
			std::atomic_flag flag;
			std::thread::id ownerThread;
			std::uint32_t refCount;
		public:
			Lockable()
			{
				ownerThread = std::thread::id();
				refCount = 0;
				flag.clear();
			}

			int refCountForThisThread()
			{
				return std::this_thread::get_id() == ownerThread ? refCount : 0;
			}
		};

	public:

		/// <summary>
		/// Acquires the resource, with the default timeout
		/// </summary>
		CMutex(Lockable * l)
			: resource(nullptr)
		{
			acquire(l);

		}

		/// <summary>
		/// Acquires the resource, with the default timeout
		/// </summary>
		CMutex(Lockable & l)
			: resource(nullptr)
		{
			acquire(l);
		}
		/// <summary>
		/// Does nothing, until something is acquired
		/// </summary>
		CMutex() noexcept
			: resource(nullptr)
		{

		}

		CMutex(const CMutex &) = delete;
		CMutex(CMutex && other) noexcept
		{
			resource = other.resource;
			other.resource = nullptr;
		}

		CMutex & operator = (CMutex && other) noexcept
		{
			resource = other.resource;
			other.resource = nullptr;
			return *this;
		}

		/// <summary>
		/// Releases any acquired resources
		/// </summary>
		~CMutex()
		{
			if (resource)
				release(resource);

		}

		/// <summary>
		/// Attemps to acquire the resource.
		/// If it is already hold by this lock, nothing is done (recursive guarantee).
		/// If another resource is locked by this lock, the previous lock is released, and then
		/// attempts to acquire the other resource.
		/// </summary>
		/// <param name="l"></param>
		void acquire(Lockable * l);

		/// <summary>
		/// Attemps to acquire the resource.
		/// If it is already hold by this lock, nothing is done (recursive guarantee). If 
		/// this thread holds the lock, nothing is done (beyond some stuff behind the scenes).
		/// If another resource is locked by this lock, the previous lock is released, and then
		/// attempts to acquire the other resource.
		/// </summary>
		/// <param name="l"></param>
		void acquire(Lockable & l)
		{
			acquire(&l);
		}

		/// <summary>
		/// Releases the resource. Called automatically on destruction.
		/// </summary>
		void release()
		{

			if (resource)
				release(resource);
		}
	private:
		void release(Lockable * l);
		static bool spinLock(unsigned ms, Lockable *  bVal);
		Lockable * resource;
	};


	class CFastMutex
	{
	private:
		typedef CMutex::Lockable Lockable;
		Lockable * resource;
	public:
		/// <summary>
		/// Acquires the resource, with the default timeout
		/// </summary>
		CFastMutex(Lockable * l)
			: resource(nullptr)
		{
			acquire(l);

		}

		/// <summary>
		/// Acquires the resource, with the default timeout
		/// </summary>
		CFastMutex(Lockable & l)
			: resource(nullptr)
		{
			acquire(l);
		}
		/// <summary>
		/// Does nothing, until something is acquired
		/// </summary>
		CFastMutex() noexcept
			: resource(nullptr)
		{

		}

		CFastMutex(const CFastMutex &) = delete;
		CFastMutex(CFastMutex && other) noexcept
		{
			resource = other.resource;
			other.resource = nullptr;
		}

		CFastMutex & operator = (CFastMutex && other) noexcept
		{
			resource = other.resource;
			other.resource = nullptr;
			return *this;
		}

		/// <summary>
		/// Releases any acquired resources
		/// </summary>
		~CFastMutex()
		{
			if (resource)
				release(resource);

		}
		void acquire(Lockable * l);
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
		void release(Lockable * l);
		static bool spinLock(Lockable *  bVal);
	};
};
#endif