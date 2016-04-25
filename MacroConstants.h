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

	file:MacroConstants.h
	
		Definitions for lots of different macroes, used throughout the program.
		Detects OS and architechture.
		Solves incompabilities.

*************************************************************************************/

#ifndef CPL_MACROCONSTANTS_H
	#define CPL_MACROCONSTANTS_H
	#include <cstdint>

	#define CPL__xstring(x) #x
	#define CPL__tostring(x) CPL__xstring(x)

	#ifdef JUCE_APP_VERSION
		#define CPL_JUCE
	#endif

	#if defined(_WIN64) || defined(__x86_64__)
		typedef std::uint64_t XWORD;
		#define CPL_M_64BIT_ 1
		#define CPL_ARCH_STRING "64-bit"
	#else
		#define __M_32BIT_
		typedef std::uint32_t XWORD;
		#define CPL_ARCH_STRING "32-bit"
	#endif

	#if defined(_WIN32) || defined (_WIN64)

		#define CPL_WINDOWS
		#define CPL_PROG_EXTENSION ".dll"
		#define CPL_DIR_SEP '\\'
	#elif defined (__MACH__) && (__APPLE__)
		#define CPL_DIR_SEP '/'
		#define CPL_MAC
		#define CPL_UNIXC
		#ifdef APE_JUCE
			#define CPL_PROG_EXTENSION ".component"
		#else
			#define CPL_PROG_EXTENSION ".vst"
		#endif
	#else
		#define CPL_UNIXC
	#endif
	#ifdef _MSC_VER

	#else

    #endif
    //#define cwarn(text) message("[" __FILE__ "] (" CPL__tostring(__LINE__) ") -> " __FUNCTION__ ": " text)

	// an 'operator' that retrieves the unqualified type of the expression x
	// useful when you want to create a lvalue-type from any expression 
	#define unq_typeof(x) typename std::remove_cv<typename std::remove_reference<decltype(x)>::type>::type
	#define val_typeof(x) std::remove_cv<typename std::remove_reference<decltype(x)>::type>::type
	#if defined(CPL_WINDOWS) || defined(CPL_MAC)
		#define CPL_INTEL_ASSEMBLY
		// gcc or msvc assembly syntax?
		#ifdef _MSC_VER
			#define DBG_BREAK() DebugBreak();
		#else
			#define DBG_BREAK() __asm__("int $0x3")
		#endif
	#else
		#error Only Intel-compliant targets supported atm.
	#endif

	#define CPL_NOOP while(false){}

	#if defined(_WIN32) || defined (_WIN64)
		#define CPL_ISDEBUGGED() !!IsDebuggerPresent()
		#define CPL_DEBUGOUT(x) OutputDebugStringA(x)
	#else
		#define CPL_DEBUGOUT(x) fputs(x, stderr)
        // forward declare it
		#define CPL_ISDEBUGGED() cpl::Misc::IsBeingDebugged()
		#define debug_out(x) (void*) 0
	#endif

	/* #ifdef _DEBUG */
		#define CPL_BREAKIFDEBUGGED() if(CPL_ISDEBUGGED()) DBG_BREAK()
	/* #else
		#define CPL_BREAKIFDEBUGGED() (void *)0
	#endif */

	#ifdef _DEBUG
		#define CPL_NOEXCEPT_IF_RELEASE
	#else
		#define CPL_NOEXCEPT_IF_RELEASE noexcept
	#endif

	/*
		On x64 windows, tcc emits wrong calling convention for floating point functions.
		We circumvent this using a hack.
	*/
	#if defined(CPL_WINDOWS) && defined(CPL_M_64BIT_)
		#define APE_CHECK_FOR_TCC_HACK
	#endif

	#ifndef CPL_ARRAYSIZE
		#define CPL_ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))
	#endif

	#define CPL_DIRC_COMP(x) ((x) == '\\' || (x) == '/')

	#if defined(_MSC_VER) 

		// cross-platform size_t specifier for printf-families
		#define CPL_FMT_SZT "%Iu"

		// oh god
		#define NOMINMAX

		#if _MSC_VER >= 1700
			#define __CPP11__
		#endif
		#if _MSC_VER >= 2100
			#define CPL_HAS_CONSTEXPR
		#endif

		#define CPL_MSVC

		#ifndef CPL_ALL_WARNINGS
			#pragma warning(disable:4482) // qualified names
			#pragma warning(disable:4005) // redefinition of macros
			#pragma warning(disable:4201) // nameless structs/unions
			#pragma warning(disable:4100) // unused parameters
			#pragma warning(disable:4702) // unreachable code
			#pragma warning(disable:4996) // std::copy with pointers
			#pragma warning(disable:4752) // avx-instructions without /arch:AVX
			#pragma warning(disable:4351) // member array default initialization
			#pragma warning(disable:4706) // assignment within conditional expression
			#pragma warning(disable:4324) // structure was deliberately padded
			#pragma warning(disable:4512) // assignment-operator could not be generated. 
			// declaration hides something else in enclosing scope:
			#pragma warning(disable:4458)
			#pragma warning(disable:4457)
			#pragma warning(disable:4459)
			#pragma warning(disable:4456)
			//#pragma warning(disable:2228) // typedef ignored on left
			#pragma warning(disable:4091) // same


			#ifndef CPL_HAS_CONSTEXPR
				#pragma warning(disable:4127) // conditional expression is constant
			#endif
		#endif
		#define CPL_llvm_DummyNoExcept

		#if _MSC_VER >= 1900
			#define CPL_ALIGNAS(x) alignas(x)		
			#define CPL_THREAD_LOCAL thread_local
		#else
			#define CPL_ALIGNAS(x) __declspec(align(x))
			#define CPL_THREAD_LOCAL __declspec(thread)
		#endif


		#define siginfo_t void
		#define CPL_NOEXCEPT

		#ifndef __func__
			#define __func__ __FUNCTION__
		#endif

		#define CPL_RESTRICT __restrict

		#define FILE_LINE_LINK __FILE__ "(" CPL__tostring(__LINE__) ") : "
		#define cwarn(exp) (FILE_LINE_LINK  " -> " __FUNCTION__ ": warning: " exp)

		#define CPL_COMPILER_SUPPORTS_AVX


	#elif defined(__llvm__) && defined(__clang__)

		// cross-platform size_t specifier for printf-families
		#define CPL_FMT_SZT "%zu"

		#define __C99__

		// change this to check against _LIBCPP_VERSION to detect cpp11 support
		#if 1
			#define __CPP11__
		#endif
		#define __LLVM__	
		#define CPL_CLANG
		#ifndef CPL_ALL_WARNINGS
			#pragma clang diagnostic ignored "-Wreorder"
			#pragma clang diagnostic ignored "-Wswitch"
		#endif
		// a bug in current apple llvm emits an error message on derived
		// classes constructors/destructors if they don't have this specifier
		#define CPL_llvm_DummyNoExcept noexcept

		#define CPL_ALIGNAS(x) alignas(x)
		#define __alignof(x) alignof(x)

		#define CPL_RESTRICT __restrict

		// Enable inclusion of all simd headers.
		#ifndef __SSE__
			#define __SSE__
		#endif
		#ifndef __SSE2__
			#define __SSE2__
		#endif
		#ifndef __SSE3__
			#define __SSE3__
		#endif
		#ifndef __SSSE3__
			#define __SSSE3__
		#endif
		#ifndef __SSE4_2__
			#define __SSE4_2__
		#endif
		#ifndef __SSE4_1__
			#define __SSE4_1__
		#endif
		#ifndef __AVX__
			#define __AVX__
		#endif
		#ifndef __AVX2__
			#define __AVX2__
		#endif
		
		#define cwarn(exp) ("warning: " exp)

        #if __clang_major__ > 7
            #define CPL_COMPILER_SUPPORTS_AVX
            #define CPL_THREAD_LOCAL thread_local
			#define __C11__
        #elif __clang_major__ >= 7
            #define CPL_COMPILER_SUPPORTS_AVX
    		#define CPL_THREAD_LOCAL __thread
			#define __C11__
        #else
			#define CPL_CLANG_BUGGY_RECURSIVE_LAMBDAS
    		#define CPL_THREAD_LOCAL __thread
            #pragma message cwarn("Your compiler is out of date. Support for AVX codepaths is partially disabled.")
        #endif

	#elif defined(__GNUG__)
		#if __GNUG__ >= 4

			#define __CPP11__
		#endif
		#define APE_API __cdecl
		#define APE_STD_API __cdecl
		#define APE_API_VARI __cdecl
		#define __GCC__
		#define CPL_llvm_DummyNoExcept
		#define CPL_GCC
		#define cwarn(exp) ("warning: " exp)


	#else
		#error "Compiler not supported."
	#endif
		
	#if defined(__LLVM__) || defined(__GCC__)
		// sets a standard for packing structs.
		// this is enforced on msvc by using #pragma pack()
		#define PACKED __attribute__((packed))
		// gives compability on gcc and llvm, and adds security on msvc
		#define sprintf_s sprintf
		#define strcpy_s strcpy
	#else
		#define PACKED
	#endif

	#ifndef MAX_PATH
		#define MAX_PATH 260
	#endif
	#define NI "Error: This feature is not implemented for target (" __FILE__ ", " __LINE__ " - " __FUNCTION__ ".)"

#endif