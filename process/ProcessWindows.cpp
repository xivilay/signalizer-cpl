/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2017 Janus Lynggaard Thorborg (www.jthorborg.com)

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

	file:ProcessWindows.cpp

		Implementation of Process.h

*************************************************************************************/

#ifndef CPL_PROCESSWINDOWS_H
#define CPL_PROCESSWINDOWS_H

#include <ostream>
#include <istream>
#include <memory>
#include <vector>
#include "../MacroConstants.h"
#include "../PlatformSpecific.h"
#include "../Misc.h"
#include "../Process.h"

namespace cpl
{
	std::string GetShellLocation();

	const std::int64_t Process::npid = -1;
	static const UINT TerminateCode = 0xDEAD;
	static const std::string shellLocation = GetShellLocation();

	std::string GetShellLocation()
	{
		std::string ret;
		ret.resize(GetSystemDirectoryA(nullptr, 0));

		if (GetSystemDirectoryA(ret.data(), static_cast<UINT>(ret.size())) != ret.size() - 1)
		{
			CPL_SYSTEM_EXCEPTION("GetWindowsDirectoryA");
		}

		// previous call stores null terminator that we don't care about.
		ret.pop_back();

		ret += "\\cmd.exe";
		return ret;
	}

	struct Options
	{
		enum
		{
			None = 0,
			Terminal,
			Detached
		};
	};

	EnvStrings Process::GetEnvironment()
	{
		EnvStrings environment;
		for (int i = 0; environ[i]; ++i)
			environment.string(environ[i]);

		return environment;
	}

	std::error_code Process::kill()
	{
		if (!actual())
			CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not actual", std::logic_error);

		if (!alive())
			return {};

		if (!TerminateProcess(childProcessHandle.get(), TerminateCode))
			return {static_cast<int>(GetLastError()), std::system_category()};

		return {};
	}

	bool Process::doJoin(int timeoutMs)
	{
		if (!actual())
			CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not actual", std::logic_error);

		if (exitCode)
			return true;

		DWORD timeout = timeoutMs < 0 ? INFINITE : static_cast<DWORD>(timeoutMs);

		switch (WaitForSingleObject(childProcessHandle.get(), timeout))
		{
			default:
			case WAIT_FAILED: CPL_SYSTEM_EXCEPTION("WaitForSingleObject");
			case WAIT_TIMEOUT: return false;
			case WAIT_OBJECT_0:
			{
				DWORD dwExitCode;
				if (!GetExitCodeProcess(childProcessHandle.get(), &dwExitCode))
				{
					CPL_SYSTEM_EXCEPTION("GetExitCodeProcess");
				}
				else
				{
					exitCode.emplace(static_cast<std::int64_t>(dwExitCode));
					return true;
				}
			}
		}

		return false;
	}

	void Process::detach()
	{
		if (!actual())
			CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not actual", std::logic_error);

		pin = nullptr;
		pout = nullptr;
		perr = nullptr;

		releaseSpecific();

		pid.set(npid);
	}

	void Process::releaseSpecific() noexcept
	{
		childProcessHandle = nullptr;
		childThreadHandle = nullptr;
	}

	Process::PipePair Process::createPipe(int parentEnd)
	{
		detail::Handle
			in = nullptr,
			out = nullptr;

		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = true;
		saAttr.lpSecurityDescriptor = nullptr;

		if (!CreatePipe(&in.handle, &out.handle, &saAttr, 0))
			CPL_SYSTEM_EXCEPTION("Error creating pipe pair");

		detail::unique_handle fin(in), fout(out);

		if (parentEnd == 0 && !SetHandleInformation(fin.get(), HANDLE_FLAG_INHERIT, 0))
			CPL_RUNTIME_EXCEPTION("Error setting inheritance permissions on pipes");


		if (parentEnd == 1 && !SetHandleInformation(fout.get(), HANDLE_FLAG_INHERIT, 0))
			CPL_RUNTIME_EXCEPTION("Error setting inheritance permissions on pipes");

		return{std::move(fin), std::move(fout)};
	}

	void Process::initialise(PipePair& in, PipePair& out, PipePair& err, EnvStrings * env, const std::string * cwd, int customFlags)
	{
		PROCESS_INFORMATION piProcInfo {};
		STARTUPINFOEXA siStartInfo {};
		using ProcThreadAttributeList = std::remove_pointer<LPPROC_THREAD_ATTRIBUTE_LIST>::type;

		siStartInfo.StartupInfo.cb = sizeof(STARTUPINFOEXA);

		std::size_t numHandles =
			((flags & IOStreamFlags::In) ? 1 : 0) +
			((flags & IOStreamFlags::Out) ? 1 : 0) +
			((flags & IOStreamFlags::Err) ? 1 : 0);

		detail::Handle hIn, hOut, hErr;
		detail::unique_handle hNulls[3];
		std::size_t hNullIndex = 0;

		if ((customFlags & Options::Terminal) == 0)
		{
			for (std::size_t i = 0; i < 3 - numHandles; ++i)
			{
				hNulls[i].reset(CreateFileA("nul", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));

				if (hNulls[i].get() == nullptr)
					CPL_SYSTEM_EXCEPTION("CreateFileA nul");

				if (!SetHandleInformation(hNulls[i].get(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
					CPL_RUNTIME_EXCEPTION("Error setting inheritance permissions on pipes");
			}
		}
		// GetStdHandle(STD_*_HANDLE) won't reliably work if we ourselves are redirected
		hIn = (flags & IOStreamFlags::In) ? in.first.get() : hNulls[hNullIndex++].get();
		hOut = (flags & IOStreamFlags::Out) ? out.second.get() : hNulls[hNullIndex++].get();
		hErr = (flags & IOStreamFlags::Err) ? err.second.get() : hNulls[hNullIndex++].get();

		struct ProcThreadDeleter { void operator()(ProcThreadAttributeList * p) { DeleteProcThreadAttributeList(p); } };

		std::unique_ptr<char[]> handleListStorage;
		std::unique_ptr<ProcThreadAttributeList, ProcThreadDeleter> attributeList;

		if (customFlags == Options::None)
		{

			siStartInfo.StartupInfo.hStdOutput = hOut;
			siStartInfo.StartupInfo.hStdError = hErr;
			siStartInfo.StartupInfo.hStdInput = hIn;
			siStartInfo.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

			SIZE_T dwNeededListSize;
			if (!InitializeProcThreadAttributeList(nullptr, 1, 0, &dwNeededListSize) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
				CPL_SYSTEM_EXCEPTION("InitializeProcThreadAttributeList");

			handleListStorage.reset(new char[dwNeededListSize]);
			LPPROC_THREAD_ATTRIBUTE_LIST tempList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(handleListStorage.get());

			if (!InitializeProcThreadAttributeList(tempList, 1, 0, &dwNeededListSize))
				CPL_SYSTEM_EXCEPTION("InitializeProcThreadAttributeList#2");

			attributeList.reset(tempList);

			HANDLE hHandles[3] = {hIn, hOut, hErr};

			if (!UpdateProcThreadAttribute(attributeList.get(), 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, hHandles, sizeof(HANDLE) * 3, nullptr, nullptr))
			{
				CPL_SYSTEM_EXCEPTION("UpdateProcThreadAttribute");
			}


			siStartInfo.lpAttributeList = attributeList.get();
		}


		DWORD dwCreationFlags = EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW;

		if (customFlags & Options::Terminal)
			dwCreationFlags |= CREATE_NEW_CONSOLE;

		if (customFlags & Options::Detached)
			dwCreationFlags |= CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS;

		auto cmdCopy = pArgs.commandLine();

		auto success = ::CreateProcessA(
			pname.c_str(),
			&(cmdCopy)[0],     // command line
			nullptr,          // pname security attributes
			nullptr,          // primary thread security attributes
			!(customFlags & Options::Terminal),          // handles are inherited
			dwCreationFlags,
			static_cast<void *>(env ? env->doubleNullList().data() : nullptr), // use parent's environment
			cwd ? cwd->c_str() : nullptr,          // use parent's current directory
			reinterpret_cast<LPSTARTUPINFOA>(&siStartInfo),  // STARTUPINFO pointer
			&piProcInfo		// receives PROCESS_INFORMATION
		);

		if (!success)
			CPL_SYSTEM_EXCEPTION("CreateProcess");


		childProcessHandle.reset(piProcInfo.hProcess);
		childThreadHandle.reset(piProcInfo.hThread);
		pid = static_cast<int>(piProcInfo.dwProcessId);

	}

	Process Process::Builder::shell(Args args, int ioFlags, ScopeExitOperation operation)
	{
		return {
			shellLocation,
			std::move(Args().arg("/A").arg("/C").arg((process + args).commandLine(), Args::Escaped)),
			ioFlags,
			ScopeExitOperation::Join,
			hasEnv ? &env : nullptr,
			hasCWD ? &cwd : nullptr
		};
	}

	Process Process::Builder::terminal(Args args, ScopeExitOperation operation)
	{
		return {
			shellLocation,
			std::move(Args().arg("/A").arg("/C").arg((process + args).commandLine(), Args::Escaped)),
			IOStreamFlags::None,
			ScopeExitOperation::Join,
			hasEnv ? &env : nullptr,
			hasCWD ? &cwd : nullptr,
			Options::Terminal
		};
	}

	void Process::Builder::launchDetached(Args args)
	{
		Process {
			process,
			std::move(args),
			IOStreamFlags::None,
			ScopeExitOperation::Join,
			hasEnv ? &env : nullptr,
			hasCWD ? &cwd : nullptr,
			Options::Detached
		}.detach();
	}

}

#endif