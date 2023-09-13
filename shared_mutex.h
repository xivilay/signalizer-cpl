/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2019 Janus Lynggaard Thorborg (www.jthorborg.com)

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

	file:filesystem.h
	
		Provides a minimal working version of std::shared_mutex
	
*************************************************************************************/

#ifndef CPL_SHARED_MUTEX_H
#define CPL_SHARED_MUTEX_H
#include "MacroConstants.h"


#if defined(CPL_MAC) && MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12

	#include <mutex>

	namespace cpl
	{
		template<typename TMutex>
		using shared_lock = std::lock_guard<TMutex>;

		template<typename TMutex>
		using unique_lock = std::lock_guard<TMutex>;

		using shared_mutex = std::mutex;
	}

#else

	#include <shared_mutex>
	namespace cpl
	{
		template<typename TMutex>
		using shared_lock = std::shared_lock<TMutex>;

		template<typename TMutex>
		using unique_lock = std::unique_lock<TMutex>;

		using shared_mutex = std::shared_mutex;
	}

#endif

#endif
