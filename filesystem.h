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
	
		Provides a minimal working version of TS std::filesystem
	
*************************************************************************************/

#ifndef CPL_FILESYSTEM_H
#define CPL_FILESYSTEM_H
#include "MacroConstants.h"

// TODO: Find version macro to allow fs on deployment targer => 10.15
#if defined(CPL_MAC) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_15

	#include <filesystem>
	namespace cpl { namespace fs = std::filesystem; }

#elif !defined(CPL_MAC) && __has_include(<filesystem>)

	#include <filesystem>
	namespace cpl { namespace fs = std::filesystem; }

#elif !defined(CPL_MAC) && __has_include(<experimental/filesystem>)

	#include <experimental/filesystem>
	namespace cpl { namespace fs = std::experimental::filesystem; }

#elif __has_include(<boost/filesystem.hpp>)

	#include <boost/filesystem.hpp>
	namespace cpl { namespace fs = boost::filesystem; }

#else

	#include "external/filesystem/include/ghc/filesystem.hpp"
	namespace cpl { namespace fs = ghc::filesystem; }
#endif

#endif
