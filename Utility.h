/*************************************************************************************
 
	 cpl - cross platform library - v. 0.1.0.
	 
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

	file:Utility.h
	
		Utility classes

*************************************************************************************/

#ifndef _UTILITY_H
	#define _UTILITY_H

	#include "MacroConstants.h"
	#include "Mathext.h"
	#include <functional>
	#include <set>
	#include "stdext.h"

	namespace cpl
	{

		/*
			Maps the input floating-point value evenly to the range of the enum, that must have the element
			'end'. The enumerated values in Enum must be linearly distributed, and 'end' must be the number of
			elements.
		*/
		template<typename Enum, typename VType>
			typename std::enable_if<std::is_enum<Enum>::value, Enum>::type distribute(VType val)
			{
				return static_cast<Enum>(cpl::Math::round<signed>(val * (int(Enum::end) - 1)));
			}

		template<typename EnumStruct, typename VType>
			inline typename std::enable_if<!std::is_enum<EnumStruct>::value, typename EnumStruct::Enum>::type distribute(VType val)
			{
				return static_cast<typename EnumStruct::Enum>(cpl::Math::round<signed>(val * (EnumStruct::end - 1)));
			}


		namespace Utility 
		{
			/*
				Use this code inside frequently run code, where you dont want to pollute the code with conditional
				check swapping.
				usage:

				void runOften()
				{
					// some code will automatically run if the condition swaps.
					// virtually free otherwise.
					condition.setCondition(getSomething());
				}
			*/
			class ConditionalSwap
			{
			public:

				ConditionalSwap(std::function<void()> falseCode, std::function<void()> trueCode, bool initialValue = false, bool runConditionNow = false)
					: falseFunctor(falseCode), trueFunctor(trueCode), oldCondition(initialValue)
				{
					if (runConditionNow)
						runCondition(initialValue);
				}

				inline void setCondition(bool newCondition)
				{
					if (newCondition != oldCondition)
					{
						runCondition(newCondition);
						oldCondition = newCondition;
					}
				}

				void runCondition(bool condition)
				{
					condition ? trueFunctor() : falseFunctor();
				}

			private:
				bool oldCondition;
				std::function<void()> falseFunctor;
				std::function<void()> trueFunctor;
			};

			/*
				Lazy pointers hold unique default constructed data objects,
				constructing/allocating them on the first use.
				They incur a overhead on dereferencing, however
				they are usefull for data objects you don't want
				to load immediately - only on use.
				Follows semantics of std::unique_ptr (RAII as well).
				Not thread-safe.
			*/
			template<class T>
				class LazyPointer
				{
				public:
					// big five.

					LazyPointer() : object(nullptr) {}

					LazyPointer(LazyPointer && other)
					{
						object = other.object;
						other.object = nullptr;
					}

					LazyPointer & operator == (LazyPointer && other)
					{
						object = other.object;
						other.object = nullptr;
					}

					// this class is not copy constructible!
					LazyPointer & operator == (const LazyPointer & other) = delete;
					LazyPointer(const LazyPointer & other) = delete;

					// operators.

					T * operator -> ()
					{
						return get();
					}

					T * release()
					{
						T * o = get();
						object = nullptr;
						return o;
					}

					void reset(T * another)
					{
						if (object)
							delete object;
						object = another;
					}

					T * get()
					{
						if (!object)
							object = new T();
						return object;
					}
					~LazyPointer()
					{
						if (object)
							delete object;
					}
				protected:
					T * object;
				};

		/*	template<typename T, typename Enable = void>
			struct elements_of;

			template<typename Ty>
				struct elements_of<Ty, typename std::enable_if<std::is_array<Ty>::value, Ty>::type>
				{
					static const std::size_t value = sizeof(Ty) / sizeof((Ty())[0]);
				};*/

			/*
				Represents a set of bounding coordinates
			*/
			template<typename Scalar>
				struct Bounds
				{
					union
					{
						Scalar left;
						Scalar top;
					};
					union
					{
						Scalar right;
						Scalar bottom;
					};

					Scalar dist() const { return std::abs(left - right); }

					bool operator == (const Bounds<Scalar> & right) const noexcept
					{
						return left == right.left && this->right == right.right;
					}

					bool operator != (const Bounds<Scalar> & right) const noexcept
					{
						return !(this->operator ==(right));
					}
				};

			template <class T> struct maybe_delete
			{
				void operator()(T* p) const noexcept{ if (!shared) delete p; }
				bool shared = false;
			};
			class COnlyHeapAllocated
			{

			};

			class CNoncopyable
			{

			protected:
				CNoncopyable() {}
				~CNoncopyable() {}
				/*
					move constructor - c++11 delete ?
				*/
			private:
				CNoncopyable(const CNoncopyable & other);
				CNoncopyable & operator=(const CNoncopyable & other);
				#ifdef __CPP11__
					CNoncopyable & operator=(const CNoncopyable && other);
					CNoncopyable(CNoncopyable && other);
				#endif
			};


			/*********************************************************************************************

				Provides a callback when a class that is listened to may die.
				The purpose of making it abstract is to force any listeners to handle cases, where the
				control (or whatever) is deleted (for some reason).

			*********************************************************************************************/

			template<typename Derived>
				class DestructionServer
				{
				public:
					typedef Derived type;

					struct ObjectProxy
					{
						ObjectProxy(const Derived * serverToPresent) : server(serverToPresent) {}
						bool operator == (const Derived * other) const { return server == other; }
						bool operator != (const Derived * other) const { return server != other; }
					private:
						const Derived * server;
					};

					class Client
					{
						friend class DestructionServer<Derived>;
					public:
						typedef DestructionServer Server;

						virtual ~Client()
						{
							for (auto server : servers)
								server->removeClientDestructor(this);
						}

						virtual void onObjectDestruction(const ObjectProxy & destroyedObject) = 0;

					private:
						void onDestruction(Derived * derivedServer)
						{
							if (!contains(servers, derivedServer))
								throw std::runtime_error("Fatal error: DestructionServer::Client is not connected to server!");
							// forget reference to server
							servers.erase(derivedServer);
							// return an unmodifiable reference to the server
							onObjectDestruction(derivedServer);
						}
						std::set<Server*> servers;
					};

					void removeClientDestructor(Client * client) { clients.erase(client); }

					void addClientDestructor(Client * client)
					{
						clients.insert(client);
						client->servers.insert(this);
					}

					virtual ~DestructionServer()
					{
						// ((Derived*)this) is actually deconstructed at this point...
						// UB happens if the pointer is dereferenced
						// this cast can only fail at compile-time
						if (Derived * derivedServer = static_cast<Derived *>(this))
						{
							for (Client * client : clients)
							{
								client->onDestruction(derivedServer);
							}
						}
						/*else --- null pointer handling?
						{
							// in fact, this shouldn't work, as well?
							throw std::runtime_error(
								std::string("Fatal error: ") + typeid(this).name() +
								" doesn't derive from " + typeid(DestructionServer<Derived> *).name()
							);
						}*/
					}
				protected:
					// make it impossible to construct this without deriving from this class.
					DestructionServer() {};

				private:
					std::set<Client *> clients;
				};

		}; // Utility
	}; // cpl
#endif