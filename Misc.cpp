/*************************************************************************************

	Audio Programming Environment VST. 
		
		VST is a trademark of Steinberg Media Technologies GmbH.

    Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:Misc.cpp
		
		Implementation of Misc.h

*************************************************************************************/

#include "MacroConstants.h"
#include "Misc.h"
#include <ctime>
#include "PlatformSpecific.h"
#include "CThread.h"
#include <cstdarg>
#include "Types.h"
#include "stdext.h"
#ifdef __GNUG__
	#include <cstdlib>
	#include <memory>
	#include <cxxabi.h>
#endif

namespace cpl
{
	namespace Misc {

		int addHandler();

		static int instanceCount = 0;
		static std::string GetDirectoryPath();
		static int GetInstanceCounter();
		static int __unusedInitialization = addHandler();

		
		#ifdef __GNUG__

			std::string DemangleRawName(const std::string & name) {
				//http://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
				int status = -4; // some arbitrary value to eliminate the compiler warning
				
				// enable c++11 by passing the flag -std=c++11 to g++
				std::unique_ptr<char, void(*)(void*)> res {
					abi::__cxa_demangle(name.c_str(), NULL, NULL, &status),
					std::free
				};
				
				return (status==0) ? res.get() : name ;
			}
			
		#else
			std::string DemangleRawName(const std::string & name) {
				return name;
			}
			
		#endif
		
		/*********************************************************************************************

			This function allows the user/programmer to attach a debugger on fatal errors.
			Otherwise, crash.

		*********************************************************************************************/
		void CrashIfUserDoesntDebug(const std::string & errorMessage)
		{
			auto ret = MsgBox(errorMessage + std::newl + std::newl + "Press yes to break after attaching a debugger. Press no to crash.", "cpl: Fatal error", 
				MsgStyle::sYesNoCancel | MsgIcon::iStop);
			if (ret == MsgButton::bYes)
			{
				DBG_BREAK();
				cpl::Misc::Delay(1);
			}
		}

		void __cdecl _purescall(void)
		{
			cpl::Misc::CrashIfUserDoesntDebug("Pure virtual function called. This is a programming error, usually happening if a freed object is used again, "
				"or calling virtual functions inside destructors/constructors.");
		}
		int addHandler()
		{
			static bool hasBeenAdded = false;
			if (!hasBeenAdded)
			{
				#ifdef __MSVC__
					hasBeenAdded = true;
					_set_purecall_handler(_purescall);
					_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
				#endif
			}
			return 0;
		}
		/*********************************************************************************************

			Delays the execution for at least msecs. Should have good precision bar context-switches,
			may spin.

		*********************************************************************************************/
		void PreciseDelay(double msecs)
		{
			#ifdef CPL_JUCE
				auto start = juce::Time::getHighResolutionTicks();
				double factor = 1.0 / juce::Time::getHighResolutionTicksPerSecond();
				msecs /= 1000;
				while ((juce::Time::getHighResolutionTicks() - start) * factor < msecs)
					;
			#else
				#error Needs implementation
			#endif
		}

		/*********************************************************************************************
		 
			Returns the path of our directory.
			For macs, this is <pathtobundle>/contents/
			for windows, this is the directory of the DLL.
		 
		 *********************************************************************************************/
		const std::string & DirectoryPath()
		{
			static std::string dirPath;
			if(!dirPath.size())
			{
				dirPath = GetDirectoryPath();
				return dirPath;
			}
			else
			{
				return dirPath;
			}
		}

		/*********************************************************************************************
		 
			Returns the path of our directory.
			For macs, this is <pathtobundle>/contents/
			for windows, this is the directory of the DLL.
		 
		 *********************************************************************************************/

		Types::OSError GetLastOSErrorCode()
		{
			#ifdef __WINDOWS__
				return GetLastError();
			#else
				return errno;
			#endif
		}

		Types::tstring GetLastOSErrorMessage()
		{
			auto lastError = GetLastOSErrorCode();
			#ifdef __WINDOWS__
				Types::char_t * apiPointer = nullptr;

				Types::OSError numChars = FormatMessage
				(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					0, 
					lastError, 
					0,
					reinterpret_cast<LPTSTR>(&apiPointer), 
					0, 
					0
				);
			
				Types::tstring ret(apiPointer, numChars);
				LocalFree(apiPointer);
				return ret;
			#else
#warning fix this
				return "";
			#endif
		}

		Types::tstring GetLastOSErrorMessage(Types::OSError errorToUse)
		{
			auto lastError = errorToUse;
			#ifdef __WINDOWS__
				Types::char_t * apiPointer = nullptr;

				Types::OSError numChars = FormatMessage
				(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					0, 
					lastError, 
					0,
					reinterpret_cast<LPTSTR>(&apiPointer), 
					0, 
					0
				);
			
				Types::tstring ret(apiPointer, numChars);
				LocalFree(apiPointer);
				return ret;
			#else
#warning fix this
				return "";
			#endif
		}
		/*********************************************************************************************
		 
			This returns an identifier, that is unqiue system- and cross-process-wide.
		 
		 *********************************************************************************************/
		int ObtainUniqueInstanceID()
		{
			int pID;
			#if defined(__WINDOWS__)
				pID = GetProcessId(GetCurrentProcess());
			#elif defined(__MAC__)
				pID = getpid();
			#endif
			if(instanceCount > std::numeric_limits<unsigned char>::max())
				MsgBox("Warning: You currently have had more than 256 instances open, this may cause a wrap around in instance-id's",
					   _PROGRAM_NAME, iInfo | sOk, nullptr, true);
			int iD = (pID << 8) | GetInstanceCounter();
			return iD;
		}
		/*********************************************************************************************
		 
			Releases a previous unique id.
		 
		 *********************************************************************************************/
		void ReleaseUniqueInstanceID(int id)
		{
			
		}
		/*********************************************************************************************
		 
			Returns a global, ever increasing counter each call.
		 
		 *********************************************************************************************/
		int GetInstanceCounter()
		{
			// maybe change this to a threaded solution some day,
			// although it is highly improbably it will ever pose a problem
			return instanceCount++;
		}
		/*********************************************************************************************
		 
			Checks whether we are being debugged.
		 
		 *********************************************************************************************/
		bool IsBeingDebugged()
		{
			#ifdef __WINDOWS__
				return IsDebuggerPresent() ? true : false;
			#elif defined(__MAC__)
				#ifdef _DEBUG
					int                 junk;
					int                 mib[4];
					struct kinfo_proc   info;
					size_t              size;
					
					// Initialize the flags so that, if sysctl fails for some bizarre
					// reason, we get a predictable result.
					
					info.kp_proc.p_flag = 0;
					
					// Initialize mib, which tells sysctl the info we want, in this case
					// we're looking for information about a specific process ID.
					
					mib[0] = CTL_KERN;
					mib[1] = KERN_PROC;
					mib[2] = KERN_PROC_PID;
					mib[3] = getpid();
					
					// Call sysctl.
					
					size = sizeof(info);
					junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
					
					
					// We're being debugged if the P_TRACED flag is set.
					if(junk == 0)
						return ( (info.kp_proc.p_flag & P_TRACED) != 0 );
					else
						return false;
				#endif
			#endif
			return false;
		}
		
		/*********************************************************************************************
		 
			returns the amount of characters needed to print pargs to the format list,
			EXCLUDING the null character
		 
		 *********************************************************************************************/
		int GetSizeRequiredFormat(const char * fmt, va_list pargs)
		{
			#ifdef __WINDOWS__
				return _vscprintf(fmt, pargs);
			#else
				int retval;
				va_list argcopy;
				va_copy(argcopy, pargs);
				retval = vsnprintf(NULL, 0, fmt, argcopy);
				va_end(argcopy);
				return retval;
			#endif
			
		}
		/*********************************************************************************************
		 
			Define a symbol for __rdtsc if it doesn't exist
		 
		 *********************************************************************************************/
		#ifndef __MSVC__
			#ifdef __M_64BIT_
				__inline__ uint64_t __rdtsc() {
					uint64_t a, d;
					__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
					return (d<<32) | a;
				}
			#else
				__inline__ uint64_t __rdtsc() {
					uint64_t x;
					__asm__ volatile ("rdtsc" : "=A" (x));
					return x;
				}
			#endif
		
		#endif
		/*********************************************************************************************
		 
			Returns the CPU clock counter
		 
		 *********************************************************************************************/
		long long ClockCounter()
		{
			return (long long)__rdtsc();
		}
		/*********************************************************************************************

			Returns a precise point in time. Use TimeDifference() to get the difference in miliseconds
			between time points.

		*********************************************************************************************/
		long long TimeCounter()
		{
			long long t = 0;

			#ifdef _WINDOWS_
				::QueryPerformanceCounter((LARGE_INTEGER*)&t);
			#elif defined(__MAC__)
				auto t1 = mach_absolute_time();
				*(decltype(t1)*)&t = t1;
			#elif defined(__CPP11__)
				using namespace std::chrono;
				auto tpoint = high_resolution_clock::now().time_since_epoch().count();
				(*(decltype(tpoint)*)&t) = tpoint;
			#endif

			return t;
		}
		double TimeDifference(long long time)
		{
			return TimeToMilisecs(TimeCounter() - time);
		}

		double TimeToMilisecs(long long time)
		{
			double ret = 0.0;

			#ifdef _WINDOWS_
				long long f;

				::QueryPerformanceFrequency((LARGE_INTEGER*)&f);

				//long long t = TimeCounter();
				ret = (time) * (1000.0/f);
			#elif defined(__MAC__)	
				auto t1 = *(decltype(mach_absolute_time())*)&time;

				struct mach_timebase_info tinfo;
				if(mach_timebase_info(&tinfo) == KERN_SUCCESS)
				{
					double hTime2nsFactor = (double)tinfo.numer / tinfo.denom;
					ret = (((t1) * hTime2nsFactor) / 1000.0) / 1000.0;
				}
			
			#elif defined(__CPP11__)
				using namespace std::chrono;
		
				high_resolution_clock::rep t1;
				t1 = *(high_resolution_clock::rep *)&time;
				milliseconds elapsed(time);
				ret = elapsed.count();
			#endif

			return ret;

		}

		/*********************************************************************************************

			Get a formatted time string in the format "hh:mm:ss"

		*********************************************************************************************/
		std::string GetTime () 
		{
			time_t timeObj;
			tm * ctime;
			time(&timeObj);
			#ifdef __MSVC__
				tm pTime;
				gmtime_s(&pTime, &timeObj);
				ctime = &pTime;
			#else
				// consider using a safer alternative here.
				ctime = gmtime(&timeObj);			
			#endif
			char buffer[100];
			// not cross platform.
			#ifdef __MSVC__
			
				sprintf_s(buffer, "%d:%d:%d", ctime->tm_hour, ctime->tm_min, ctime->tm_sec);
			#else
				sprintf(buffer, "%d:%d:%d", ctime->tm_hour, ctime->tm_min, ctime->tm_sec);
			#endif
			return buffer;
		}

		/*********************************************************************************************

			'private' function, initializes the global DirectoryPath

		*********************************************************************************************/

		static std::string GetDirectoryPath()
		{
			if (programInfo.hasCustomDirectory)
				return programInfo.customDirectory();
			#ifdef __WINDOWS__
				// change to MAX_PATH on all supported systems
				char path[MAX_PATH + 2];
				unsigned long nLen = 0;
				unsigned long nSize = sizeof path;
				/*
					GetModuleFileName() returns the path to the dll, inclusive of the dll name.
					We shave this off to get the directory
				*/

				HMODULE hMod;
				GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
					(char*)&GetDirectoryPath, &hMod);

				nLen = GetModuleFileNameA(hMod, path, nSize);
				if (!nLen)
					return "";
				while (nLen--> 0) {
					if (DIRC_COMP(path[nLen])) {
						path[nLen] = '\0';
						break;
					}
				};
				path[MAX_PATH + 1] = NULL;
				return path;
			#elif defined(__MAC__) && defined(__APE_LOCATE_USING_COCOA)
				/*
					GetBundlePath() returns the path to the bundle, inclusive of the bundle name.
					Since everything we have is in the subdir /contents/ we append that.
					NOTE: This is broken
				*/
				// change to MAX_PATH on all supported systems
				char path[MAX_PATH + 2];
				unsigned long nLen = 0;
				unsigned long nSize = sizeof path;
				nLen = GetBundlePath(path, nSize);
				if(!nLen)
					return "";
				path[nLen] = '\0';
				std::string ret = path;
				ret += "/Contents";
				return ret;
			#elif defined(__MAC__) && defined(__APE_LOCATE_USING_CF)
				// change to MAX_PATH on all supported systems
				char path[MAX_PATH + 2];
				unsigned long nLen = 0;
				unsigned long nSize = sizeof path;
				CFBundleRef ref = CFBundleGetBundleWithIdentifier(CFSTR("com.Lightbridge.AudioProgrammingEnvironment"));
				if(ref)
				{
					CFURLRef url = CFBundleCopyBundleURL(ref);
					if(url)
					{
						CFURLGetFileSystemRepresentation(url, true, (unsigned char*)path, nSize);
						CFRelease(url);
						return path;
					}
				}
			#elif defined(__MAC__)
				Dl_info exeInfo;
				dladdr ((void*) GetDirectoryPath, &exeInfo);
				// need to chop off 2 directories here
				std::string fullPath(exeInfo.dli_fname);
				for (int i = fullPath.length(), z = 0; i != 0; --i) {
					// directory slash detected
					if (DIRC_COMP(fullPath[i]))
						z++;
					// return minus 2 directories
					if(z == 2)
						return std::string(fullPath.begin(), fullPath.begin() + (long)i);
				}
			#endif
			return "<Error getting directory of executable>";
		}
		/*********************************************************************************************

			Floating point rounding function.

		*********************************************************************************************/
		long Round(double number)
		{
			return (number >= 0.0) ? (long)(number + 0.5) : (long)(number - 0.5);
		}

		/*********************************************************************************************

			Delays execution for a certain amount of time. Sleeping for a time less than (or equal to)
			zero yields the thread instead.

		*********************************************************************************************/
		long Delay(int ms)
		{
			#ifdef __CPP11__
				if (ms <= 0)
					std::this_thread::yield();
				else
					std::this_thread::sleep_for(std::chrono::milliseconds(ms));
			#elif defined (__WINDOWS__)
				::Sleep(ms);
			#elif defined(__MAC__)
				usleep(ms * 1000);
			#endif
			return 0;
		}

		/*********************************************************************************************

			Imprecise, incrementing timer.
			At least 1000/60 seconds precision.

		*********************************************************************************************/
		unsigned int QuickTime()
		{
			#ifdef __WINDOWS__
				return GetTickCount();
			#else
				// from sweep/lice
				struct timeval tm={0,};
				gettimeofday(&tm,NULL);
				// this is not really ideal, but i would really like to keep the counter in a 32-bit int.
				// a simple static_cast<unsigned> does NOT work as expected: result is TRUNCATED instead
				// of following the standard which says unsigned integers wrap around INT_MAX.
				// we do this ourselves, therefore.
				return static_cast<unsigned long long>(tm.tv_sec*1000.0 + tm.tv_usec/1000.0) % UINT32_MAX;
			#endif
		}

		/*********************************************************************************************

			'private' function, maps to a MessageBox

		*********************************************************************************************/
		inline static int _mbx(void * systemData, const char * text, const char * title, int nStyle = 0)
		{
			#ifdef __WINDOWS__ 
				if (!systemData)
					nStyle |=  MB_DEFAULT_DESKTOP_ONLY;
				return (int)MessageBoxA(reinterpret_cast<HWND>(systemData), text, title, nStyle);
			#elif defined(__MAC__)
				return MacBox(systemData, text, title, nStyle);
			#else
				#error "Implement a similar messagebox for your target"
			#endif
		}

		/*********************************************************************************************

			MsgBoxData - used to transfer information about a messagebox through threads

		*********************************************************************************************/
		struct MsgBoxData
		{
			std::string title;
			std::string text;
			int nStyle;
			void * systemWindow;
			MsgBoxData(const char * title, const char * text, int style, void * window = NULL) 
				: title(title), text(text), nStyle(style), systemWindow(window) {} ;
			MsgBoxData(const std::string & title, const std::string & text, int style, void * window = NULL) 
				: title(title), text(text), nStyle(style), systemWindow(window) {} ;
		};

		/*********************************************************************************************

			Spawns a messagebox in a threaded context. Max of 10 open boxes.

		*********************************************************************************************/
		static void * ThreadedMessageBox(void * imp_data)
		{
			//Global for all instances of this plugin: But we do not want to spam screen with msgboxes
			static volatile int nOpenMsgBoxes = 0; 
			const int nMaxBoxes = 10;

			if(nOpenMsgBoxes >= nMaxBoxes)
				return reinterpret_cast<void*>(-1);
			MsgBoxData * data = reinterpret_cast<MsgBoxData*>(imp_data);
			nOpenMsgBoxes++;

			int ret = _mbx(data->systemWindow, data->text.c_str(), data->title.c_str(), data->nStyle);
			delete data;

			nOpenMsgBoxes--;

			return reinterpret_cast<void*>(ret);
		}

		/*********************************************************************************************

			MsgBox - Spawns a messagebox, optionally blocking.

		*********************************************************************************************/
		int MsgBox(const std::string & text, const std::string & title, int nStyle, void * parent, const bool bBlocking)
		{
			if(bBlocking)
				return _mbx(parent, text.c_str(), title.c_str(), nStyle);
			else
			{
				// spawn and detach instantly.
				/* NOTE:
					This is a severely deprecated method on both windows and osx
					Implement this as some sort of global stack of messageboxes,
					which gets shown when a global, static RenderMsgBoxes() function
					is called from a gui loop
				*/
				 
				CThread msgThread(ThreadedMessageBox);
				msgThread.run(new MsgBoxData(title, text, nStyle));
				return 0;
			}
		}
	};
};