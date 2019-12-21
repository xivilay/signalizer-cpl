/*************************************************************************************

	 cpl - cross-platform library - v. 0.1.0.

	 Copyright (C) 2017 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:CModule.h

		Interface for the wrapper class CModule. CModule wraps loading and dynamic
		binding of external libraries like DLLs, dylibs, SOs etc. with safe
		copy construction (using reference counting abilities of underlying OS).

	options:
		#define CPL_CMOD_USECF
			uses corefoundation instead of the dyld loader on mac (advised)

*************************************************************************************/


#ifndef CPL_CMODULE_H
#define CPL_CMODULE_H

#include <string>
//#include "Common.h"
#include "filesystem.h"
#include "Core.h"
#include <vector>
// if set, uses corefoundation instead of the dyld loader on mac

typedef void * ModuleHandle;

namespace cpl
{
	class CModule
	{
	public:
		CModule();
		CModule(std::string moduleName);

		bool addSearchPath(const fs::path& directory);

		/// <summary>
		/// Returns a pointer to a symbol inside the loaded module of this instance
		/// </summary>
		void* getFuncAddress(const string_ref functionName);
		/// <summary>
		/// If no module is loaded, loads moduleName
		/// </summary>
		int load(std::string moduleName);
		/// <summary>
		/// If no module is loaded, loads moduleName
		/// </summary>
		int load(std::string moduleName, std::string& error);
		/// <summary>
		/// Increases the reference of the loaded module
		/// </summary>
		void increaseReference();
		/// <summary>
		/// Decreases the reference count of the loaded module.
		/// </summary>
		void decreaseReference();
		/// <summary>
		/// Releases the module, decreasing it's reference count and losing the reference.
		/// </summary>
		bool release();
		/// <summary>
		/// Returns the native handle to the module
		/// </summary>
		ModuleHandle getHandle();
		CModule(const CModule &);
		CModule(CModule &&) = default;
		CModule& operator = (CModule&& other) = default;
		/// <summary>
		/// Destructor. Releases the module
		/// </summary>
		~CModule();

	private:

		ModuleHandle moduleHandle;
		std::string name;
		std::vector<ModuleHandle> directoryCookies;
	};
}
#endif
