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

	file:Process.cpp

        Implementation of Process.h

*************************************************************************************/
#include "../Process.h"
#include "../MacroConstants.h"
#include "../Misc.h"
#ifdef CPL_WINDOWS
#include "ProcessWindows.cpp"
#elif defined(CPL_UNIXC)
#include "ProcessUnix.cpp"
#endif

namespace cpl
{
    const EnvStrings Process::initialEnvironment = Process::GetEnvironment();

    const EnvStrings& Process::getParentEnvironment() { return initialEnvironment; }
    const EnvStrings& Process::getCreationEnvironment() { return hasCustomEnvironment ? pEnv : getParentEnvironment(); }
    const Args& Process::getCreationArgs() { return pArgs; }

    Process::InputStream & Process::cout()
    {
        if(!actual())
            CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not actual", std::logic_error);

        if (!pout.get())
            CPL_RUNTIME_EXCEPTION_SPECIFIC("No stdout pipe allocated for process", std::logic_error);

        return pout->stream;
    }

    Process::InputStream & Process::cerr()
    {
        if(!actual())
            CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not actual", std::logic_error);

        if (!perr.get())
            CPL_RUNTIME_EXCEPTION_SPECIFIC("No stderr pipe allocated for process", std::logic_error);

        return perr->stream;
    }

    Process::CloseableOutputStream & Process::cin()
    {
        if(!actual())
            CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not actual", std::logic_error);

        if (!pin.get())
            CPL_RUNTIME_EXCEPTION_SPECIFIC("No stdin pipe allocated for process", std::logic_error);

        return pin->stream;
    }

    const std::string & Process::name() const
    {
        if(!actual())
            CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not actual", std::logic_error);

        return pname;
    }

    std::int64_t Process::getExitCode()
    {
        if(!explicitlyJoined)
            CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not succesfully joined", std::logic_error);

        return exitCode.value();
    }

    std::int64_t Process::getPid() const noexcept
    {
        return pid.get();
    }

    bool Process::actual() const noexcept
    {
        return pid.get() != npid;
    }

    bool Process::alive()
    {
        if(!actual())
            CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not actual", std::logic_error);

        if(!exitCode)
        {
            join(0);
        }

        return !exitCode;
    }

    bool Process::join(int timeout)
    {
        return explicitlyJoined = doJoin(timeout);
    }

    Process::Process() noexcept
        : flags(IOStreamFlags::None), pid(npid), scopeExitOp(ScopeExitOperation::Join), explicitlyJoined(false)
    {

    }

	Process::Process(std::string process, Args&& args, int ioFlags, ScopeExitOperation operation, const EnvStrings * strings, const std::string * cwd, bool launchDetached)
        : pname(std::move(process)), pArgs(pname + args), scopeExitOp(operation), pid(npid), explicitlyJoined(false)
	{
		flags = static_cast<IOStreamFlags>(ioFlags);

		PipePair in, out, err;

		if (ioFlags & IOStreamFlags::In)
			in = createPipe(1);
		if (ioFlags & IOStreamFlags::Out)
			out = createPipe(0);
		if (ioFlags & IOStreamFlags::Err)
			err = createPipe(0);

        if(strings)
            pEnv = *strings;

        initialise(in, out, err, strings ? &pEnv : nullptr, cwd, launchDetached);

		if (ioFlags & IOStreamFlags::In)
			pin = std::make_unique<OutPipe>(std::move(in.second));
		if (ioFlags & IOStreamFlags::Out)
			pout = std::make_unique<InPipe>(std::move(out.first));
		if (ioFlags & IOStreamFlags::Err)
			perr = std::make_unique<InPipe>(std::move(err.first));
	}

	Process::ScopeExitOperation Process::callHandler(std::exception * e) const noexcept
	{
        if(!handler)
            return ScopeExitOperation::Terminate;

        try
        {
            return handler(*this, scopeExitOp, e);
        }
        catch(...)
        {
            return ScopeExitOperation::Terminate;
        }
	}


    Process::~Process()
    {
        pin = nullptr;
        pout = nullptr;
        perr = nullptr;

        if(actual())
        {
            bool done = false;

            while(!done)
            {
                try
                {
                    switch(scopeExitOp)
                    {
                        case ScopeExitOperation::Join:
                        {
                            done = join(handler ? 100 : -1);
                            break;
                        }
                        case ScopeExitOperation::Detach:
                        {
                            detach();
                            done = true;
                            break;
                        }
                        case ScopeExitOperation::Terminate:
                        {
                            std::terminate();
                            break;
                        }
                        case ScopeExitOperation::KillJoin:
                        {
                            auto err = kill();
                            if(err)
                                CPL_RUNTIME_EXCEPTION_SPECIFIC_ARGS("kill", std::system_error, (err, "kill"));

                            done = join(handler ? 100 : -1);
                            break;
                        }
                        case ScopeExitOperation::KillDetach:
                        {
                            auto err = kill();
                            if(err)
                                CPL_RUNTIME_EXCEPTION_SPECIFIC_ARGS("kill", std::system_error, (err, "kill"));

                            detach();
                            done = true;
                            break;
                        }
                    }
                }
                catch(std::exception& e)
                {
                    scopeExitOp = callHandler(&e);
                    continue;
                }
                catch(...)
                {

                }

                scopeExitOp = ScopeExitOperation::Terminate;

            }

            releaseSpecific();
        }

    }
}
