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

	file:DecoupledStateObject.h

		Declares object that persist state through destruction and creation, 
		independently.

*************************************************************************************/

#ifndef CPL_DECOUPLED_STATE_OBJECT_H
#define CPL_DECOUPLED_STATE_OBJECT_H

#include "../Exceptions.h"
#include "Serialization.h"
#include <memory>
#include <functional>
#include "../Utility.h"
#include "../gui/Tools.h"

namespace cpl
{
	template<typename T>
	struct UniqueHandle
	{
	public:

		UniqueHandle(std::unique_ptr<T> && ref)
		{
			handle.reset(ref.release());
			handle.get_deleter().doDelete = true;
		}

		UniqueHandle<T> & operator = (std::unique_ptr<T> && ref)
		{
			handle.reset(ref.release());
			handle.get_deleter().doDelete = true;
			ref.get_deleter().doDelete = false;
			return *this;
		}

		UniqueHandle<T> weakCopy()
		{
			UniqueHandle<T> ret;
			ret.handle.reset(handle.get());
			ret.handle.get_deleter().doDelete = false;
			return std::move(ret);
		}

		T * operator -> () const noexcept { return get(); }
		T * operator -> () noexcept { return get(); }

		T * get() noexcept { return handle.get(); }
		T * get() const noexcept { return handle.get(); }

		/// <summary>
		/// Transforms ownership of the contained object. Throws if it isn't owned.
		/// </summary>
		T * acquire()
		{
			if (!handle.get_deleter().doDelete)
				CPL_RUNTIME_EXCEPTION("UniqueHandle asked to release ownership of something it doesn't own");
			handle.get_deleter().doDelete = false;
			return handle.release();
		}

		/// <summary>
		/// Deletes the object if owned, and clears afterwards.
		/// </summary>
		void forget() noexcept
		{
			handle = nullptr;
		}

		/// <summary>
		/// May leak memory. Removes reference to any object, but doesn't delete it.
		/// </summary>
		void clear() noexcept
		{
			handle.release();
		}

		static UniqueHandle<T> null() { return {}; }

		explicit operator bool() const noexcept { return handle.get() != nullptr; }

	private:

		UniqueHandle()
		{
			handle.get_deleter().doDelete = false;
		}

		std::unique_ptr<T, cpl::Utility::MaybeDelete<T>> handle;
	};

	/// <summary>
	/// Provides a optionally lazily loaded instance of some object, where you can (de)serialize the state independently of the instance
	/// </summary>
	template<typename T>
	class DecoupledStateObject
	{
		static_assert(std::is_base_of<cpl::DestructionNotifier, T>::value, "State object must be able to notify of destruction");
	public:

		typedef std::function<void(T &, cpl::CSerializer::Archiver &, cpl::Version)> FSerializer;
		typedef std::function<void(T &, cpl::CSerializer::Builder &, cpl::Version)> FDeserializer;
		typedef std::function<std::unique_ptr<T>()> FGenerator;

		DecoupledStateObject(FGenerator generatorFunc, FSerializer serializerFunc, FDeserializer deserializerFunc)
			: generator(generatorFunc)
			, serializer(serializerFunc)
			, deserializer(deserializerFunc)
			, objectDeathHook(*this)
			, cachedObject(UniqueHandle<T>::null())
		{

		}

		FGenerator replaceGenerator(FGenerator generatorFunc)
		{
			auto old = generator;
			generator = generatorFunc;
			return old;
		}

		FSerializer replaceSerializer(FSerializer serializerFunc)
		{
			auto old = serializer;
			serializer = serializerFunc;
			return old;
		}

		FDeserializer replaceDeserializer(FDeserializer deserializerFunc)
		{

			auto old = deserializer;
			deserializer = deserializerFunc;
			return old;
		}

		UniqueHandle<T> getUnique()
		{
			UniqueHandle<T> ret = hasCached() ? std::move(cachedObject) : create();
			cachedObject = ret.weakCopy();
			return std::move(ret);
		}

		UniqueHandle<T> getCached() { if (!hasCached()) cachedObject = create(); return cachedObject.weakCopy(); }
		bool hasCached() const noexcept { return cachedObject.get() != nullptr; }

		void setState(cpl::CSerializer::Builder & builder, cpl::Version v)
		{
			if (hasCached())
			{
				builder.setMasterVersion(v);
				deserializeState(*getCached().get(), &builder);
			}
			else
			{
				state = builder;
				state.setMasterVersion(v);
			}
		}

		/// <summary>
		/// If serialized state is refreshed, the version will be updated
		/// </summary>
		/// <returns></returns>
		const cpl::CSerializer::Builder & getState()
		{
			if (hasCached())
			{
				serializeState(*getCached().get());
			}
			return state;
		}

	private:

		void onObjectDestruction(cpl::DestructionNotifier * obj)
		{
			CPL_RUNTIME_ASSERTION(obj);
			CPL_RUNTIME_ASSERTION(obj == cachedObject.get());

			serializeState(*getCached().get());

			cachedObject.clear();
		}

		void serializeState(T & obj)
		{
			state.clear();
			state.setMasterVersion(cpl::programInfo.version);
			serializer(obj, state, cpl::programInfo.version);
		}

		void deserializeState(T & obj, cpl::CSerializer::Builder * optionalExternalState = nullptr)
		{
			auto & usableState = optionalExternalState ? *optionalExternalState : state;
			deserializer(obj, usableState, usableState.getLocalVersion());
		}

		UniqueHandle<T> create()
		{
			auto uref = generator();
			if (!state.isEmpty())
				deserializeState(*uref);
			objectDeathHook.listenToObject(uref.get());
			return std::move(uref);
		}

		class DestructionDelegate
			: public cpl::DestructionNotifier::EventListener
		{
		public:
			DestructionDelegate(DecoupledStateObject<T> & parentToNotify) : parent(parentToNotify) { }

			void listenToObject(cpl::DestructionNotifier * notifier)
			{
				notif = notifier;
				notifier->addEventListener(this);
			}

			virtual void onServerDestruction(cpl::DestructionNotifier * v) override
			{
				parent.onObjectDestruction(v);
				hasDied = true;
			}

			~DestructionDelegate()
			{
				if (!hasDied && notif)
				{
					notif->removeEventListener(this);
				}
			}

		private:
			bool hasDied = false;
			DecoupledStateObject<T> & parent;
			cpl::DestructionNotifier * notif = nullptr;
		};

		FGenerator generator;
		FSerializer serializer;
		FDeserializer deserializer;


		cpl::CSerializer state;
		UniqueHandle<T> cachedObject;
		DestructionDelegate objectDeathHook;
	};

	template<class T>
	class SerializableStateObject : public DecoupledStateObject<T>
	{
	public:
		using typename DecoupledStateObject<T>::FGenerator;

		static_assert(std::is_base_of<cpl::SafeSerializableObject, T>::value, "T must be serializable");

		SerializableStateObject(FGenerator generatorFunc)
			: DecoupledStateObject<T>(
				generatorFunc,
				[](T& object, cpl::CSerializer& sz, cpl::Version v) { object.serializeObject(sz, v); },
				[](T& object, cpl::CSerializer& sz, cpl::Version v) { object.deserializeObject(sz, v); }
			)
		{

		}
	};
}

#endif