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

	file:CModule.cpp

		Implementation of CModule.h

*************************************************************************************/

#include "CModule.h"
#include "MacroConstants.h"
#ifdef CPL_WINDOWS
#include <Windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#ifdef CPL_CMOD_USECF
#include <mach-o/dyld.h>
#include <mach/mach_time.h>
#endif
#endif
namespace cpl
{
	CModule::CModule()
		: moduleHandle(nullptr)
	{

	}

	CModule::CModule(std::string moduleName)
		: moduleHandle(nullptr)
	{
		load(std::move(moduleName));
	}

	void * CModule::getFuncAddress(const zstr_view functionName)
	{
		#ifdef CPL_WINDOWS
		return GetProcAddress(static_cast<HMODULE>(moduleHandle), functionName.c_str());
		#elif defined(CPL_MAC) && defined(CPL_CMOD_USECF)
		CFStringRef funcName = CFStringCreateWithCString(kCFAllocatorDefault, functionName.c_str(), kCFStringEncodingUTF8);
		void * ret(nullptr);
		if (funcName)
		{
			CFBundleRef bundle(nullptr);
			bundle = reinterpret_cast<CFBundleRef>(moduleHandle);
			if (bundle)
			{
				ret = reinterpret_cast<void*>(CFBundleGetFunctionPointerForName(bundle, funcName));
			}
			CFRelease(funcName);

		}
		return ret;
		#elif defined(CPL_UNIXC)
		return dlsym(moduleHandle, functionName.c_str());
		#else
		#error no implementation for getfuncaddress on anything but windows
		#endif
	}

	int CModule::load(std::string moduleName)
	{
		if (moduleHandle != nullptr)
			return false;
		#ifdef CPL_WINDOWS
		moduleHandle = static_cast<ModuleHandle>(LoadLibraryA(moduleName.c_str()));
		if (moduleHandle != nullptr)
			return 0;
		else
			return GetLastError();
		#elif defined(CPL_MAC) && defined(CPL_CMOD_USECF)
		/*
			This api is only slightly better than old-style K&R c-programming
		*/
		CFBundleRef bundle(nullptr);
		CFURLRef url(nullptr);
		CFStringRef path = CFStringCreateWithCString(kCFAllocatorDefault, name.c_str(), kCFStringEncodingUTF8);
		if (!path)
			return -1;
		url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path, kCFURLPOSIXPathStyle, true);
		// remember to release all of this stuff.
		CFRelease(path);
		if (!url)
		{
			return -2;
		}
		bundle = CFBundleCreate(kCFAllocatorDefault, url);
		CFRelease(url);
		if (!bundle)
		{
			return -3;
		}
		else
		{
			// standard clearly says reinterpret_cast to void * and back is welldefined
			moduleHandle = reinterpret_cast<void *>(bundle);
			return 0;
		}
		#elif defined(CPL_UNIXC)
		moduleHandle = dlopen(moduleName.c_str(), RTLD_NOW);
		if (moduleHandle != nullptr)
			return 0;
		else
			return errno ? errno : -1;

		#else
		#error no implementation for LoadModule
		#endif

		name = std::move(moduleName);
	};

	bool CModule::release()
	{
		if (moduleHandle == nullptr)
			return false;
		bool result = false;
		#ifdef CPL_WINDOWS
		result = !!FreeLibrary(static_cast<HMODULE>(moduleHandle));
		moduleHandle = nullptr;
		#elif defined(CPL_MAC) && defined(CPL_CMOD_USECF)
		CFBundleRef bundle = nullptr;
		bundle = reinterpret_cast<CFBundleRef>(moduleHandle);
		if (bundle)
		{
			CFRelease(bundle);
			moduleHandle = nullptr;
			bundle = nullptr;
			return true;
		}
		else
		{
			moduleHandle = nullptr;
			return false;
		}
		#elif defined(CPL_UNIXC)
		result = !dlclose(moduleHandle);
		#else
		#error no implementation
		#endif
		return result;
	}

	ModuleHandle CModule::getHandle()
	{
		return moduleHandle;
	}

	void CModule::decreaseReference()
	{
		if (moduleHandle == nullptr)
			return;
		#ifdef CPL_WINDOWS
		FreeLibrary(static_cast<HMODULE>(moduleHandle));
		#elif defined(CPL_MAC) && defined(CPL_CMOD_USECF)
		// so long as we dont use CFBundleLoadExecutable, references should be automatically counted by
		// core foundation, as long as we remember to release() the module and increaseReference() it
		#elif defined(CPL_UNIXC)
		dlclose(moduleHandle);
		#else
		#error no implementation for LoadModule on anything but windows
		#endif
	}

	void CModule::increaseReference()
	{
		if (moduleHandle && name.length())
		{
			#ifdef CPL_WINDOWS
			// only increases reference count
			LoadLibraryA(name.c_str());
			#elif defined(CPL_MAC) && defined(CPL_CMOD_USECF)
			/*
				little bit of double code, but sligthly different semantics
				ie. keeps the old module if new one fails.
			*/
			CFBundleRef bundle(nullptr);
			CFURLRef url(nullptr);
			CFStringRef path = CFStringCreateWithCString(kCFAllocatorDefault, name.c_str(), kCFStringEncodingUTF8);
			if (!path)
				return;
			url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path, kCFURLPOSIXPathStyle, true);
			// remember to release all of this stuff.
			CFRelease(path);
			if (!url)
			{
				return;
			}
			// this should increase reference count for the same bundle.
			// the reference is decreased when we CFRelease the bundle.
			bundle = CFBundleCreate(kCFAllocatorDefault, url);
			CFRelease(url);
			if (!bundle)
			{
				return;
			}
			else
			{
				moduleHandle = reinterpret_cast<void*>(bundle);
			}
			#elif defined(CPL_UNIXC)
			dlopen(name.c_str(), RTLD_NOW);
			#else
			#error no implementation
			#endif
		}
	}

	CModule::~CModule()
	{
		release();
	}

	CModule::CModule(const CModule & other)
	{
		this->moduleHandle = other.moduleHandle;
		this->name = other.name;
		increaseReference();
	}
};
