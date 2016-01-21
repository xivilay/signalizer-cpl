/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

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

	file:Protected.cpp

		Implementation of Protected.h

*************************************************************************************/

#include "Protected.h"

namespace cpl
{
	
	CProtected::StaticData CProtected::staticData;
	__thread_local CProtected::ThreadData CProtected::threadData;
	std::atomic<int> checkCounter = 0;
	// TODO: make atomic. It is safe on all x86 systems, though.
	std::unique_ptr<CProtected> internalInstance;
	CMutex::Lockable creationLock;

	CProtected & CProtected::instance()
	{
		// TODO: perfect double-checked pattern by creating by using atomic<unique_ptr> instead
		// problem is, atomic<T>::T must be trivially copyable.
		std::atomic_thread_fence(std::memory_order_acquire);
		if (!internalInstance.get())
		{
			CMutex lock(creationLock);

			if (!internalInstance.get())
			{
				std::atomic_thread_fence(std::memory_order_release);
				internalInstance.reset(new CProtected());
				return *internalInstance.get();
			}
			else
			{
				return *internalInstance.get();
			}
		}
		else
		{
			return *internalInstance.get();
		}
	}

	/*********************************************************************************************

		le constructor

	 *********************************************************************************************/
	CProtected::CProtected()
	{
		checkCounter.fetch_add(1);
		if (checkCounter.load() > 1)
			CPL_RUNTIME_EXCEPTION("More than one Protected instance created at a time.");
		#ifndef __WINDOWS__
			registerHandlers();
		#endif
	}
	/*********************************************************************************************

		Deconstructor

	 *********************************************************************************************/
	CProtected::~CProtected() 
	{
		unregisterHandlers();
		checkCounter.fetch_sub(1);
	}


	/*********************************************************************************************

		Formats a string and returns it. It explains what went wrong.

	 *********************************************************************************************/
	std::string CProtected::formatExceptionMessage(const CSystemException & e) 
	{
		Misc::CStringFormatter base;
		base << "Exception at " << std::hex << e.data.faultAddr << ": ";
		switch(e.data.exceptCode)
		{
		case CSystemException::status::intdiv_zero:
				return base.str() + "An integral division-by-zero was performed";
		case CSystemException::status::funderflow:
				return base.str() + "A floating point operation resulted in underflow";
		case CSystemException::status::foverflow :
				return base.str() + "A floating point operation resulted in overflow";
		case CSystemException::status::finexact:
				return base.str() + "A floating point operation's result cannot be accurately expressed";
		case CSystemException::status::finvalid:
				return base.str() + "One of the operands for a floating point operation was invalid (typically negative numbers for sqrt, exp, log)";
		case CSystemException::status::fdiv_zero:
				return base.str() + "A floating point division-by-zero was performed";
		case CSystemException::status::fdenormal:
				return base.str() + "One of the operands for a floating point operation was denormal (too small to be represented)";
		case CSystemException::status::nullptr_from_plugin:
				return base.str() + "An API function was called with 'this' as an null pointer.";
		case CSystemException::status::access_violation:
			{
				Misc::CStringFormatter fmt;
				#ifndef __MSVC__
					switch(e.data.actualCode)
					{
						case SIGSEGV:
						{
							fmt << "Segmentation fault ";
							switch(e.data.extraInfoCode)
							{
								case SEGV_ACCERR:
									fmt << "(invalid permission for object) ";
									break;
								case SEGV_MAPERR:
									fmt << "(address not mapped for object) ";
									break;
							}
							break;
						}
						case SIGBUS:
						{
							fmt << "Bus error ";
							switch(e.data.extraInfoCode)
							{
								case BUS_ADRALN:
									fmt << "(invalid address alignment) ";
									break;
								case BUS_ADRERR:
									fmt << "(non-existant address) ";
									break;
								case BUS_OBJERR:
									fmt << "(object hardware error) ";
							}
							break;
							
						}
						default:
							fmt << "Access violation ";
							break;
					}
				#else
					fmt << "Access violation ";

					switch (e.data.actualCode)
					{
						case 0:
							fmt << "reading ";
							break;
						case 1:
							fmt << "writing ";
							break;
						case 8:
							fmt << "executing ";
							break;
						default:
							fmt << "(unknown error?) at ";
							break;
					};


				#endif
				fmt << " address " << std::hex << e.data.attemptedAddr << ".";
				return base.str() + fmt.str();
			}
		default:
			return base.str() + " Unknown exception (BAD!).";
		};
	}

	/*********************************************************************************************

		Structured exception handler for windows

	 *********************************************************************************************/
	XWORD CProtected::structuredExceptionHandler(XWORD _code, CSystemException::eStorage & e, void * systemInformation)
	{
		BreakIfDebugged();

		#ifdef __WINDOWS__
			auto exceptCode = _code;
			bool safeToContinue(false);
			void * exceptionAddress = nullptr;
			int additionalCode = 0;
			EXCEPTION_POINTERS * exp = reinterpret_cast<EXCEPTION_POINTERS *>(systemInformation);
			if (exp)
				exceptionAddress = exp->ExceptionRecord->ExceptionAddress;
			switch(_code)
			{
			case EXCEPTION_ACCESS_VIOLATION:
				{
					EXCEPTION_POINTERS * exp = reinterpret_cast<EXCEPTION_POINTERS *>(systemInformation);
					std::ptrdiff_t addr = 0; // nullptr invalid here?

					if (exp)
					{
						// the address that was attempted
						addr = exp->ExceptionRecord->ExceptionInformation[1];
						// 0 = read violation, 1 = write violation, 8 = dep violation
						additionalCode = static_cast<int>(exp->ExceptionRecord->ExceptionInformation[0]);
					}

					e = CSystemException::eStorage(exceptCode, safeToContinue, exceptionAddress, (const void *) addr, additionalCode);

					return EXCEPTION_EXECUTE_HANDLER;
				}
				/*
					trap all math errors:
				*/
			case EXCEPTION_INT_DIVIDE_BY_ZERO:
			case EXCEPTION_FLT_UNDERFLOW:
			case EXCEPTION_FLT_OVERFLOW: 
			case EXCEPTION_FLT_INEXACT_RESULT:
			case EXCEPTION_FLT_INVALID_OPERATION:
			case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			case EXCEPTION_FLT_DENORMAL_OPERAND:
				_clearfp();
				safeToContinue = true;

				e = CSystemException::eStorage(exceptCode, safeToContinue, exceptionAddress);

				return EXCEPTION_EXECUTE_HANDLER;

			default:
				return EXCEPTION_CONTINUE_SEARCH;
			};
			return EXCEPTION_CONTINUE_SEARCH;

		#endif
		return 0;
	}

	/*********************************************************************************************

		Implementation specefic exception handlers for unix systems.

	 *********************************************************************************************/
	void CProtected::signalHandler(int some_number)
	{
		throw (XWORD) some_number;
	}

	void CProtected::signalActionHandler(int sig, siginfo_t * siginfo, void * extra)
	{
		#ifndef __WINDOWS__
			// consider locking signalLock here -- not sure if its well-defined, though
		
			/*
				Firstly, check if the exception occured at our stack, after runProtectedCode 
				which sets activeStateObject to a valid object
			*/
			if(threadData.isInStack)
			{
				/*
					handle the exception here.
				*/
			
				const void * fault_address = siginfo ? siginfo->si_addr : nullptr;
				auto ecode = siginfo->si_code;
				bool safeToContinue = false;

				// -- this should be handled by siglongjmp
				//sigemptyset (&newAction.sa_mask);
				//sigaddset(&newAction.sa_mask, sig);
				//sigprocmask(SIG_UNBLOCK, &newAction.sa_mask, NULL);
			
				switch(sig)
				{
					case SIGBUS:
					case SIGSEGV:
					{

					
						threadData.currentException = CSystemException(CSystemException::access_violation,
												 safeToContinue,
												 in_address,
												 fault_address,
												 ecode,
												 sig);
						// jump back to CState::runProtectedCode. Note, we know that function was called
						// earlier in the stackframe, because threadData.activeStateObject is non-null
						// : that field is __only__ set in runProtectedCode. Therefore, the threadJumpBuffer
						// IS valid.
						siglongjmp(threadData.threadJumpBuffer, 1);
						break;
					}
					case SIGFPE:
					{
						// exceptions that happened are still set in the status flags - always clear these,
						// or the exception might throw again
						std::feclearexcept(FE_ALL_EXCEPT);
						CSystemException::status code_status;
						switch(ecode)
						{
							case FPE_FLTDIV:
								code_status = CSystemException::status::fdiv_zero;
								break;
							case FPE_FLTOVF:
								code_status = CSystemException::status::foverflow;
								break;
							case FPE_FLTUND:
								code_status = CSystemException::status::funderflow;
								break;
							case FPE_FLTRES:
								code_status = CSystemException::status::finexact;
								break;
							case FPE_FLTINV:
								code_status = CSystemException::status::finvalid;
								break;
							case FPE_FLTSUB:
								code_status = CSystemException::status::intsubscript;
								break;
							case FPE_INTDIV:
								code_status = CSystemException::status::intdiv_zero;
								break;
							case FPE_INTOVF:
								code_status = CSystemException::status::intoverflow;
								break;
						}
						safeToContinue = true;
						threadData.currentException = CSystemException(code_status, safeToContinue, fault_address);
					
						// jump back to CState::runProtectedCode. Note, we know that function was called
						// earlier in the stackframe, because threadData.activeStateObject is non-null
						// : that field is __only__ set in runProtectedCode. Therefore, the threadJumpBuffer
						// IS valid. 
						siglongjmp(threadData.threadJumpBuffer, 1);
						break;
					}
					default:
						goto default_handler;
				} // switch signal
			} // if threadData.activeStateObject
		
			/*
				Exception happened in some arbitrary place we have no knowledge off.
				First we try to call the old signal handlers
			*/
		
		
		default_handler:
			/*
				consider checking here that sa_handler/sa_sigaction is actually valid and not something like
				SIG_DFLT, in which case re have to reset the handlers and manually raise the signal again
			*/
		
			if(staticData.oldHandlers[sig].sa_flags & SA_SIGINFO)
			{
				if(staticData.oldHandlers[sig].sa_sigaction)
					return staticData.oldHandlers[sig].sa_sigaction(sig, siginfo, extra);
			}
			else
			{
				if(staticData.oldHandlers[sig].sa_handler)
					return staticData.oldHandlers[sig].sa_handler(sig);
			}
			/*
				WE SHOULD NEVER REACH THIS POINT. NEVER. Except for nuclear war and/or nearby black hole
			*/
		
			// no handler found, throw exception (that will call terminate)
		
			throw std::runtime_error(_PROGRAM_NAME " - CProtected:signalActionHandler called for unregistrered signal; no appropriate signal handler to call.");
		#endif
	}

	/*********************************************************************************************

		Implementation specefic exception handlers for unix systems.

	 *********************************************************************************************/
	bool CProtected::registerHandlers()
	{
		#ifndef __MSVC__
			CMutex lock(staticData.signalLock);
			if(!staticData.signalReferenceCount)
			{
				#ifdef __CSTATE_USE_SIGNALS
					signal(SIGFPE, &CState::signalHandler);
					signal(SIGSEGV, &CState::signalHandler);
				#elif defined (__CSTATE_USE_SIGACTION)
					staticData.newHandler.sa_sigaction = &CState::signalActionHandler;
					staticData.newHandler.sa_flags = SA_SIGINFO;
					staticData.newHandler.sa_mask = 0;
					sigemptyset(&staticData.newHandler.sa_mask);
					sigaction(SIGSEGV, &staticData.newHandler, &staticData.oldHandlers[SIGSEGV]);
					sigaction(SIGFPE, &staticData.newHandler, &staticData.oldHandlers[SIGFPE]);
					sigaction(SIGBUS, &staticData.newHandler, &staticData.oldHandlers[SIGBUS]);
				#endif
			}
			staticData.signalReferenceCount++;
			return true;
		#endif
		return false;
	}
	
	bool CProtected::unregisterHandlers()
	{
		#ifndef __MSVC__
			CMutex lock(staticData.signalLock);
			staticData.signalReferenceCount--;
			if(staticData.signalReferenceCount == 0)
			{
				for(auto & signalData : staticData.oldHandlers)
				{
					// restore all registrered old signal handlers
					sigaction(signalData.first, &signalData.second, nullptr);
				}
				staticData.oldHandlers.clear();
				return true;
			}
			return false;
		#endif
		return false;
	}
}