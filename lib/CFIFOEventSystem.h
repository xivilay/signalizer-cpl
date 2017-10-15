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

	file:CFIFOEventSystem.h

			A wait free SPSC message system, delivering messages asynchronously

*************************************************************************************/

#ifndef CPL_CFIFOEVENTSYSTEM_H
#define CPL_CFIFOEVENTSYSTEM_H

#include "readerwriterqueue/readerwriterqueue.h"
#include <thread>
#include "../Utility.h"

namespace cpl
{

	template<class Message>
	class CFIFOEventSystem : Utility::CNoncopyable
	{
	public:

		class AsyncEventListener
		{
		public:

			virtual void onAsyncMessageEvent(Message & msg) = 0;
			virtual ~AsyncEventListener() {};
		};

		CFIFOEventSystem(AsyncEventListener & l, std::size_t queueSize = 1)
			: queue(queueSize), listener(&l)
		{
			if (!l)
			{
				CPL_RUNTIME_EXCEPTION("Null-pointer listener interface passed!");
			}

			asyncThread = std::thread(asyncSubsystem, this);
		}

		bool postMessage(const Message & m)
		{
			if (queue.try_enqueue(m))
			{
				semaphore.signal();
				return true;
			}
			return false;
		}

		~CFIFOEventSystem()
		{
			if (asyncThread.joinable())
			{
				signalAsyncStop();
				asyncThread.join();
			}
			else
			{
				// this probably requires attention: The async thread either wasn't created or it has crashed
				CPL_BREAKIFDEBUGGED();
			}
		}

	private:

		void signalAsyncStop()
		{
			semaphore.signal();
		}

		void asyncSubsystem()
		{
			Message msg;
			while (true)
			{
				semaphore.wait();
				if (!queue.try_dequeue(msg))
				{
					return; // signaled semaphore
				}
				else
				{
					listener->onAsyncMessageEvent(msg);
				}
			}
		}

		AsyncEventListener * listener;
		std::thread asyncThread;
		moodycamel::ReaderWriterQueue<Message> queue;
		moodycamel::spsc_sema::Semaphore semaphore;
	};

};
#endif