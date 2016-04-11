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

	file:CMessageSystem.h
		
			A thread safe and wait+lock free coalesced message system with multiple handlers.

*************************************************************************************/

#ifndef CPL_CMESSAGESYSTEM_H
	#define CPL_CMESSAGESYSTEM_H

	#include "../concurrentservices.h"
	#include "../Common.h"
	#include <thread>
	#include "../Utility.h"

	namespace cpl
	{
		class CMessageSystem 
		: 
			public Utility::CNoncopyable, 
			public DestructionNotifier::EventListener
		{
		public:

			class CoalescedMessage : private juce::MessageManager::MessageBase
			{
				friend class CMessageSystem;

			private:

				void registerParent(CMessageSystem & newParent)
				{
					parent = &newParent;
				}

				virtual void messageCallback() override
				{
					return parent->messageCallback(*this);
				}

				CMessageSystem * parent;
			};

			friend class CoalescedMessage;

			template<typename T>
			class PayloadMessage : CoalescedMessage
			{
			public:
				T& payload() { return internalPayload; }

			private:
				T internalPayload;
			};

			class MessageHandler : virtual public DestructionNotifier
			{
			public:
				virtual bool handleMessage(CoalescedMessage & m) = 0;
				virtual ~MessageHandler() {}
			};

			void onServerDestruction(DestructionNotifier * dn)
			{
				cpl::CMutex lock(listenerMutex);
				
				if(auto list = dynamic_cast<MessageHandler *>(dn))
					handlers.erase(list);
			}

			CMessageSystem()
			{
				asyncThread = std::thread(&CMessageSystem::asyncSubsystem, this);
			}

			CMessageSystem & registerMessage(CoalescedMessage & m)
			{
				messages[&m] = false;
				m.incReferenceCount();
				return *this;
			}

			bool postMessage(CoalescedMessage & m)
			{
				auto it = messages.find(&m);

				if (it != messages.end() && it->second.cas(true))
				{
					semaphore.signal();
					return true;
				}

				return false;
			}

			void registerMessageHandler(MessageHandler * handler)
			{
				cpl::CMutex lock(listenerMutex);

				if (handlers.find(handler) != handlers.end())
					return;

				handlers.insert(handler);
				handler->addEventListener(this);
			}

			void removeMessageHandler(MessageHandler * handler)
			{
				cpl::CMutex lock(listenerMutex);

				if (handlers.find(handler) == handlers.end())
					return;

				handlers.erase(handler);
				handler->removeEventListener(this);
			}

			~CMessageSystem()
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

				// needed?
				cpl::CMutex lock(listenerMutex);

				for (auto && l : handlers)
				{
					l->removeEventListener(this);
				}
			}

		private:

			void messageCallback(CoalescedMessage & m)
			{
				cpl::CMutex lock(listenerMutex);

				for (auto && l : handlers)
				{
					if (l->handleMessage(m))
						break;
				}
			}

			void signalAsyncStop()
			{
				semaphore.signal();
			}

			void asyncSubsystem()
			{
				
				while (true)
				{
					semaphore.wait();

					if (quitFlag.load(std::memory_order_acquire))
						return;

					bool hasMessagesBeenPosted = false;

					for (auto && mit : messages)
					{
						if (mit.second.cas())
						{
							hasMessagesBeenPosted = true;
							mit.first->post();
						}
					}

					if (!hasMessagesBeenPosted)
						return;
				}
			}


			std::atomic_bool quitFlag;
			cpl::CMutex::Lockable listenerMutex;
			std::set<MessageHandler *> handlers;
			std::map<CoalescedMessage *, ABoolFlag> messages;
			std::thread asyncThread;
			moodycamel::spsc_sema::Semaphore semaphore;
		};

	};
#endif