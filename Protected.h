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

	file:Protected.h
	
		Defines a wrapper call that catches system-level exceptions through a platform
		independant interface.

	options:
		#define CPL_UNSAFE_USE_SIGACTION
		uses sigaction() instead of signal() on unix systems. Highly recommended

*************************************************************************************/

#ifndef CPL_PROTECTED_H
	#define CPL_PROTECTED_H

	#include "LibraryOptions.h"
	#include "PlatformSpecific.h"
	#include "Misc.h"
	#include <vector>
	#include <exception>
	#include <signal.h>
	#include <thread>
	#include "CMutex.h"
    #include <map>
	#include <sstream>
	#include "Utility.h"

#define CPL_TRACEGUARD_START \
	cpl::CProtected::instance().topLevelTraceGuardedCode([&]() {

#define CPL_TRACEGUARD_STOP(traceGuardName) \
	}, traceGuardName)


	namespace cpl
	{

		class CProtected : Utility::CNoncopyable
		{

		public:

			struct CSystemException;

			struct PreembeddedFormatter
			{
				PreembeddedFormatter(const char * prefixToEmbed)
					: prefix(prefixToEmbed)
				{

				}

				std::stringstream & get()
				{
					if (!hasBeenConstructed)
					{
						stream.reference() << "Handler: " << prefix << newl;
						hasBeenConstructed = true;
					}
					return stream.reference();
				}

			private:
				const char * prefix;
				bool hasBeenConstructed = false;
				Utility::LazyStackPointer<std::stringstream> stream;
			};
			static void useFPUExceptions(bool b);
			static std::string formatExceptionMessage(const CSystemException &);

			static CProtected & instance();

			/// <summary>
			/// calls lambda inside 'safe wrappers', catches OS errors and filters them.
			/// Throws CSystemException on errors, crashes on unrecoverable errors.
			/// 
			/// Does NOT catch software exceptions.
			/// 
			/// Note that the stack may NOT be unwound (it wouldn't be anyway if we caught any exception),
			/// so consider your program to be in an UNDEFINED state; write some info to a file and crash gracefully
			/// afterwards!
			/// </summary>
			template <class func>
				void runProtectedCode(func && function)
				{
					threadData.isInStack = true;
					/*
						This is not really crossplatform, but I'm working on it!
					*/
					#ifdef CPL_MSVC
						bool exception_caught = false;

						CSystemException::eStorage exceptionData;

						__try {
							
						//	CPL_BREAKIFDEBUGGED();
							function();
						}
						__except (CProtected::structuredExceptionHandler(GetExceptionCode(), exceptionData, GetExceptionInformation()))
						{
							// this is a way of leaving the SEH block before we throw a C++ software exception
							exception_caught = true;
						}
						if(exception_caught)
							throw CSystemException(exceptionData);
					#else
						// trigger int 3
						//CPL_BREAKIFDEBUGGED();
						// set the jump in case a signal gets raised
						if(sigsetjmp(threadData.threadJumpBuffer, 1))
						{
							/*
								return from exception handler.
								current exception is in CState::currentException
							*/
							threadData.isInStack = false;
							throw CSystemException(threadData.currentExceptionData);
						}
						// run the potentially bad code
						function();

					#endif
					/*
					 
						This is the ideal code.
						Unfortunately, to succesfully throw from a signal handler,
						there must not be any non-c++ stackframes in between.
						If, the exception will be uncaught even though we have this handler
						lower down on the stack. Therefore, we have to implement the 
						exception system using longjmps
					
					 
						try {
							// <-- breakpoint disables SIGBUS
							CPL_BREAKIFDEBUGGED();
							function();
						}
						catch(const CSystemException & e) {
							activeStateObject = nullptr;
							// rethrow it
							throw e;
						}
						catch(...)
						{
							Misc::MsgBox("Unknown exception thrown, breakpoint is triggered afterwards");
							CPL_BREAKIFDEBUGGED();
						
						}
					 */
					threadData.isInStack = false;
				}

			template <class func>
				auto topLevelTraceGuardedCode(
					func && function,
					const char * levelDescription = "Top-level exception/signal handler")
					-> decltype(function())
				{

					PreembeddedFormatter debugOutput(levelDescription);

					threadData.isInStack = true;
					threadData.traceIntercept = true;
					threadData.propagate = true;
					threadData.debugTraceBuffer = &debugOutput;

					auto scopeRelease = []()
					{
						threadData.isInStack = false;
						threadData.traceIntercept = false;
						threadData.propagate = false;
						threadData.debugTraceBuffer = nullptr;
					};

					Utility::OnScopeExit<decltype(scopeRelease)> releaser(
						scopeRelease
					);

					// in this frame we catch signals and SEH exceptions. 
					#ifdef CPL_MSVC
						return internalSEHTraceInterceptor(debugOutput, function);
					#else
						// async signals will be captured further up
						return internalCxxTraceInterceptor(debugOutput, function);
					#endif
				}

			template <class func>
				static void runProtectedCodeErrorHandling(func && function)
				{
					try
					{
						CProtected::instance().runProtectedCode
						(
							[&]()
							{
								function();
							}
						);
					}
					catch (CProtected::CSystemException & cs)
					{
						auto error = CProtected::formatExceptionMessage(cs);
						Misc::LogException(error);
						Misc::CrashIfUserDoesntDebug(error);
						cs.reraise();
					}
					catch (std::exception & e)
					{
						auto error = e.what();
						Misc::LogException(error);
						Misc::CrashIfUserDoesntDebug(error);
						throw;
					}

				}

			/*
				Base exception for all critical exceptions thrown in this program.
				Derives from std::exception, but has no meaningful .what()
				- See Protected::formatExceptionMessage
			*/
			struct CSystemException :  public std::exception
			{
			public:
				// this is kinda ugly, but needed to create a simple interface, abstract to seh and signals
				
				// the retarted create() pattern is used because clang STILL doesn't support thread_local,
				// thus we must use __thread, which DOESN'T support non-trivial destruction (implied by use of constructors).
				// TODO: fix the upper messages.
				struct eStorage
				{
					const void * faultAddr; // the address the exception occured
					const void * attemptedAddr; // if exception is a memory violation, this is the attempted address
					XWORD exceptCode; // the exception code
					int extraInfoCode; // addition, exception-specific code
					int actualCode; // what signal it was

					union {
						// hack hack union
						bool safeToContinue; // exception not critical or can be handled
						bool aVInProtectedMemory; // whether an access violation happened in our protected memory
					};

					static eStorage create(XWORD exp, bool resolved = true, const void * faultAddress = nullptr,
								const void * attemptedAddress = nullptr, int extraCode = 0, int actualCode = 0)
					{
						return {faultAddress, attemptedAddress, exp, extraCode, actualCode, resolved};
					}

					static eStorage create()
					{
						return {};
					}
				} data;

				enum status : XWORD {
					nullptr_from_plugin = 1,
					#ifdef CPL_WINDOWS
						// this is not good: these are not crossplatform constants.
						access_violation = EXCEPTION_ACCESS_VIOLATION,
						intdiv_zero = EXCEPTION_INT_DIVIDE_BY_ZERO,
						fdiv_zero = EXCEPTION_FLT_DIVIDE_BY_ZERO,
						finvalid = EXCEPTION_FLT_INVALID_OPERATION,
						fdenormal = EXCEPTION_FLT_DENORMAL_OPERAND,
						finexact = EXCEPTION_FLT_INEXACT_RESULT,
						foverflow = EXCEPTION_FLT_OVERFLOW,
						funderflow = EXCEPTION_FLT_UNDERFLOW
					#elif defined(CPL_MAC)
						access_violation = SIGSEGV,
						intdiv_zero,
						fdiv_zero,
						finvalid,
						fdenormal,
						finexact,
						foverflow,
						funderflow,
						intsubscript,
						intoverflow
					#endif
				};
				CSystemException()
				{
					
				}

				CSystemException(const eStorage & eData)
				{
					data = eData;
				}

				void reraise() const
				{
					#ifdef CPL_WINDOWS
						RaiseException(data.exceptCode, 0, 0, nullptr);
					#else
						raise(data.exceptCode);
					#endif
				}

				CSystemException(XWORD exp, bool resolved = true, const void * faultAddress = nullptr, const void * attemptedAddress = nullptr, int extraCode = 0, int actualCode = 0)
				{
					data = eStorage::create(exp, resolved, faultAddress, attemptedAddress, extraCode, actualCode);
				}
				const char * what() const noexcept override
				{
					return "CProtected::CSystemException (non-software exception)";
				}
			};
			~CProtected();

		protected:
			CProtected();


		private:
			
			template <class func>
				auto internalCxxTraceInterceptor(PreembeddedFormatter & out, func && function)
					-> decltype(function())
				{
					// in this frame, we catch C++ software exceptions
					try
					{
						return function();
					}
					catch (CProtected::CSystemException & cs)
					{
						out.get() << "Hardware -> " << CProtected::formatExceptionMessage(cs) << newl;
						out.get() << "what() -> " << cs.what() << newl;
						Misc::LogException(out.get().str());
						Misc::CrashIfUserDoesntDebug(out.get().str());
						throw;
					}
					catch (std::exception & e)
					{
						out.get() << "Software -> " << e.what() << newl;
						Misc::LogException(out.get().str());
						Misc::CrashIfUserDoesntDebug(out.get().str());
						throw;
					}
					catch (...)
					{
						out.get() << "Unknown software exception";
						Misc::LogException(out.get().str());
						Misc::CrashIfUserDoesntDebug(out.get().str());
						throw;
					}

				}

			template <class func>
				auto internalSEHTraceInterceptor(PreembeddedFormatter & out, func && function)
					-> decltype(function())
				{

					#ifdef CPL_MSVC					
					CSystemException::eStorage exceptionInformation;
						__try
						{
							return internalCxxTraceInterceptor(out, function);
						}
						__except (structuredExceptionHandlerTraceInterceptor(
							out,
							GetExceptionCode(),
							exceptionInformation,
							GetExceptionInformation())
							)
						{
							std::terminate();
						}
					#endif
				}

			 static struct StaticData
			 {
				#ifndef CPL_WINDOWS
					std::map<int, struct sigaction> oldHandlers;
					struct sigaction newHandler;
					volatile int signalReferenceCount;
					CMutex::Lockable signalLock;
				#endif
			 
			 } staticData;
			 
			 
			static CPL_THREAD_LOCAL struct ThreadData
			{
				/// <summary>
				/// Thread local set on entry and exit of protected code stack frames.
				/// </summary>
				bool isInStack;
				/// <summary>
				/// If set, exception will be propagated further in the handler chain
				/// </summary>
				bool propagate;
				/// <summary>
				/// If set, logs debug output, presents a message box with debugging abilities
				/// </summary>
				bool traceIntercept;

				/// <summary>
				/// A pre-allocated buffer that can be used for scratch space
				/// </summary>
				PreembeddedFormatter * debugTraceBuffer;

				#ifndef CPL_MSVC
					sigjmp_buf threadJumpBuffer;
					CSystemException::eStorage currentExceptionData;
				#endif
				unsigned fpuMask;
			} threadData;

			XWORD structuredExceptionHandler(XWORD _code, CSystemException::eStorage & e, void * _systemInformation);
			XWORD structuredExceptionHandlerTraceInterceptor(PreembeddedFormatter & outputStream, XWORD _code, CSystemException::eStorage & e, void * _systemInformation);

			static void signalTraceInterceptor(CSystemException::eStorage & e);
			static void signalHandler(int some_number);
			static void signalActionHandler(int signal, siginfo_t * siginfo, void * extraData);
			static bool registerHandlers();
			static bool unregisterHandlers();
		};
	};
#endif