/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2022 Janus Lynggaard Thorborg (www.jthorborg.com)

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

	file:weak_atomic.h

		Simple move/constructible declarative atomic primitives

*************************************************************************************/

#ifndef CPL_WEAK_ATOMIC_H
#define CPL_WEAK_ATOMIC_H

#include <atomic>

namespace cpl
{
	/// <summary>
	/// Provides relaxed semantics
	/// </summary>
	template<typename T>
	class relaxed_atomic
	{
	public:

		relaxed_atomic() { }
		template<typename U> relaxed_atomic(U&& x) : value(std::forward<U>(x)) {  }
		relaxed_atomic(relaxed_atomic<T> const& other) : value(other.value.load(std::memory_order_relaxed)) {  }
		relaxed_atomic(relaxed_atomic<T>&& other) : value(other.value.load(std::memory_order_relaxed)) {  }
		operator T() const { return load(); }

		template<typename U>
		relaxed_atomic<T> const& operator=(U && x)
		{
			value.store(std::forward<U>(x), std::memory_order_relaxed);
			return *this;
		}

		relaxed_atomic<T> const& operator=(relaxed_atomic const& other)
		{
			value.store(other.value.load(std::memory_order_relaxed), std::memory_order_relaxed);
			return *this;
		}

		T load() const 
		{ 
			return value.load(std::memory_order_relaxed);
		}

		T fetch_add(T increment)
		{
			return value.fetch_add(increment, std::memory_order_relaxed);
		}

		std::atomic<T>& get() { return value; }

	private:

		std::atomic<T> value;
	};

	/// <summary>
	/// Provides acquire/release semantics
	/// </summary>
	template<typename T>
	class weak_atomic
	{
	public:

		weak_atomic() { }
		template<typename U> weak_atomic(U&& x) : value(std::forward<U>(x)) {  }
		weak_atomic(weak_atomic const& other) : value(other.value.load(std::memory_order_acquire)) {  }
		weak_atomic(weak_atomic&& other) : value(std::move(other.value.load(std::memory_order_acquire))) {  }
		operator T() const { return load(); }

		template<typename U>
		weak_atomic<T> const& operator=(U&& x)
		{
			value.store(std::forward<U>(x), std::memory_order_release);
			return *this;
		}

		weak_atomic<T> const& operator=(weak_atomic const& other)
		{
			value.store(other.value.load(std::memory_order_acquire), std::memory_order_release);
			return *this;
		}

		T load() const
		{
			return value.load(std::memory_order_acquire);
		}

		T fetch_add(T increment)
		{
			return value.fetch_add(increment, std::memory_order_acquire);
		}

		std::atomic<T>& get() { return value; }

	private:
		std::atomic<T> value;
	};
};


#endif
