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

	file:CMutex.cpp

		Implementation of some basic routines to avoid cyclic dependencies.

*************************************************************************************/
#include "CMutex.h"

namespace cpl
{
	int AlertUserAboutMutex()
	{
		return Misc::MsgBox("Deadlock detected in spinlock: Protected resource is not released after max interval. "
			"Wait again (try again), release resource (continue) - can create async issues - or exit (cancel)?",
			programInfo.name,
			Misc::MsgStyle::sConTryCancel | Misc::MsgIcon::iStop,
			NULL,
			true);
	}

	void CMutex::release(CMutex::Lockable * l)
	{
		// default value
		if (l->refCount == 0)
		{
			CPL_RUNTIME_EXCEPTION("Releasing CMutex with a ref-count of zero!");
		}
		l->refCount--;
		if (l->refCount == 0)
		{
			l->ownerThread = std::thread::id();

			if (l)
				l->flag.clear();
			std::atomic_thread_fence(std::memory_order_release);
			resource = nullptr;
		}

	}

	bool CMutex::spinLock(unsigned ms, CMutex::Lockable *  bVal)
	{
		using namespace Misc;
		unsigned int start;
		int ret = MsgButton::bTryAgain;
	loop:
		int count = 0;
		start = QuickTime();
		while (bVal->flag.test_and_set(std::memory_order_relaxed)) {
			count++;
			if (count > 200)
			{
				count = 0;
				if ((QuickTime() - start) > ms)
					goto time_out;
				Delay(0);
			}

		}

		// normal exitpoint
		return true;
		// deadlock occurs

	time_out:
		// hello - you have reached this point if mutex was frozen.
		CPL_BREAKIFDEBUGGED();
		ret = AlertUserAboutMutex();
		switch (ret)
		{
			default:
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

	void CMutex::acquire(CMutex::Lockable * l)
	{
		auto this_id = std::this_thread::get_id();

		// TODO: fix race condition
		bool lockOwnedByThread = l->ownerThread == this_id;

		if (resource)
		{
			if (lockOwnedByThread && l == resource)
			{
				// mutex owned by self
				return;
			}
			else
			{
				release(resource);
			}
		}

		if (!lockOwnedByThread)
		{
			if (!spinLock(2000, l))
			{
				//explode here
				return;
			}
			l->ownerThread = this_id;
			if (l->refCount != 0)
				CPL_RUNTIME_EXCEPTION("Acquired non-recursed mutex had non-zero ref count");
		}
		l->refCount++;
		resource = l;

		std::atomic_thread_fence(std::memory_order_acquire);
	}


	void CFastMutex::acquire(CMutex::Lockable * l)
	{
		auto this_id = std::this_thread::get_id();

		// TODO: fix race condition
		bool lockOwnedByThread = l->ownerThread == this_id;

		if (resource)
		{
			if (lockOwnedByThread && l == resource)
			{
				// mutex owned by self
				return;
			}
			else
			{
				release(resource);
			}
		}

		if (!lockOwnedByThread)
		{
			if (!spinLock(l))
			{
				//explode here
				return;
			}
			l->ownerThread = this_id;
			if (l->refCount != 0)
				CPL_RUNTIME_EXCEPTION("Acquired non-recursed mutex had non-zero ref count");
		}
		l->refCount++;
		resource = l;

		std::atomic_thread_fence(std::memory_order_acquire);
	}

	void CFastMutex::release(CMutex::Lockable * l)
	{
		if (l->refCount == 0)
		{
			CPL_RUNTIME_EXCEPTION("Releasing CMutex with a ref-count of zero!");
		}
		l->refCount--;
		if (l->refCount == 0)
		{
			l->ownerThread = std::thread::id();
			if (l)
				l->flag.clear();
			resource = nullptr;

			std::atomic_thread_fence(std::memory_order_release);
		}

	}

	bool CFastMutex::spinLock(CMutex::Lockable *  bVal)
	{
		using namespace Misc;
		unsigned int start;
		int ret;
		unsigned int ms = 2000;
	loop:

		start = QuickTime();
		int count = 0;
		while (bVal->flag.test_and_set(std::memory_order_relaxed)) {
			count++;
			if (count > 2000)
			{
				count = 0;
				if ((QuickTime() - start) > ms)
					goto time_out;
			}
		}
		// normal exitpoint
		return true;
		// deadlock occurs

	time_out:
		// hello - you have reached this point if mutex was frozen.
		CPL_BREAKIFDEBUGGED();
		ret = AlertUserAboutMutex();
		switch (ret)
		{
			default:
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