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

	file:ProcessUnix.cpp

        	Implementation of Process.h

*************************************************************************************/

#include "../Process.h"
#include <signal.h>
#include <wait.h>
#include <system_error>
#include <mutex>

extern char ** environ;

namespace cpl
{
    std::int64_t Process::npid = -1;
    std::mutex orphanMutex;
    std::vector<int> orphanList;

    EnvStrings Process::GetEnvironment()
    {
        EnvStrings environment;
        for(int i = 0; environ[i]; ++i)
            environment.string(environ[i]);

        return environment;
    }


	void Process::detach()
    {
        if(!actual())
            CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not actual", std::logic_error);

        pin = nullptr;
        pout = nullptr;
        perr = nullptr;

        std::lock_guard<std::mutex> lock(orphanMutex);
        orphanList.push_back(pid.get());

        pid.set(npid);
    }

	bool Process::doJoin(int timeoutMs)
    {
        if(!actual())
            CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not actual", std::logic_error);

        if(exitCode)
            return true;

        siginfo_t info;

        if(timeoutMs < 1)
        {
            auto flags = WEXITED | WNOWAIT;
            if(timeoutMs >= 0)
                flags |= WNOHANG;

            if(waitid(P_PID, pid.get(), &info, flags))
            {
                if(errno == EINTR)
                    return false;
                else
                    CPL_SYSTEM_EXCEPTION("waitid");
            }
            else
            {
                if(info.si_signo == SIGCHLD && info.si_pid == pid.get())
                {
                    switch(info.si_code)
                    {
                        case CLD_EXITED:
                            exitCode.emplace(info.si_status);
                            return true;
                        case CLD_KILLED:
                        case CLD_DUMPED:
                            exitCode.emplace(-1);
                            return true;
                    }
                }
            }

        }
        else
        {
            auto start = cpl::Misc::QuickTime();

            do
            {
                if(waitid(P_PID, pid.get(), &info, WNOHANG | WNOWAIT | WEXITED))
                {
                    if(errno != EINTR)
                        CPL_SYSTEM_EXCEPTION("waitid");
                }
                else
                {
                    if(info.si_signo == SIGCHLD && info.si_pid == pid.get())
                    {
                        switch(info.si_code)
                        {
                            case CLD_EXITED: case CLD_KILLED: case CLD_DUMPED:
                                exitCode.emplace(info.si_status);
                                return true;
                        }
                    }
                }

                cpl::Misc::Delay(5);

            } while(cpl::Misc::QuickTime() - start < (unsigned)timeoutMs);
        }

        return false;
    }

    std::error_code Process::kill()
    {
        if(!actual())
            CPL_RUNTIME_EXCEPTION_SPECIFIC("Process not actual", std::logic_error);

        if(!alive())
            return {};

        if(::kill(pid.get(), SIGKILL))
            return { errno, std::system_category() };

        return {};
    }

    void Process::releaseSpecific() noexcept
    {
        if(pid.get() != npid)
        {
            int ignored;
            // Reap the child. Any call to this function
            // is through the destructor from where
            // the precondition alive() is false.
            // waitpid may set errno to ECHILD which means
            // our child disconnected itself or something else
            // in this process decided to reap our process.
            // nothing to do in that case.
            // EINVAL cannot be handled, either.
            while(true)
            {
                auto signaledPid = waitpid(pid.get(), &ignored, 0);
                if(signaledPid < 0)
                {
                    if(errno == EINTR)
                        continue;
                }
                else if(signaledPid != pid.get())
                {
                    // kernel bug
                    std::terminate();
                }
                else
                {
                    break;
                }
            }

            pid.set(npid);
        }

    }

    Process::PipePair Process::createPipe(int parentEnd)
    {
        detail::Handle
            in = nullptr,
            out = nullptr;

        int fd[2];

        if(pipe(fd))
            CPL_SYSTEM_EXCEPTION("pipe");

        in = fd[0];
        out = fd[1];

        detail::unique_handle fin(in), fout(out);

        return{ std::move(fin), std::move(fout) };
    }


    static bool CloseAllFiles()
    {
        const char * selfFds = "/proc/self/fd";

        if(auto fds = opendir(selfFds))
        {
            const auto itfd = dirfd(fds);

            while(auto entry = readdir(fds))
            {

                auto str = entry->d_name;
                int filedes = 0;
                bool error = false;

                // simple atoi, standard is not reentrant
                while(str && str[0])
                {
                    auto c = *str;

                    // only accept positive, continuous integers
                    if(c < '0' || c > '9')
                    {
                        error = true;
                        break;
                    }

                    filedes = filedes * 10 + c - '0';
                    str++;
                }

                if(error)
                    continue;

                // max stdio fd
                if(filedes < 3)
                    continue;

                // check if fd is our directory iterator
                if(filedes == itfd)
                    continue;

                // discard result, can't handle error anyway
                close(filedes);

            }

            closedir(fds);

            return true;
        }

        return false;
    }


    struct PThreadCancelDisabler
    {
        int oldState;

        PThreadCancelDisabler()
        {
            if(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldState))
                CPL_POSIX_EXCEPTION("pthread_setcancelstate");
        }

        ~PThreadCancelDisabler()
        {
            pthread_setcancelstate(oldState, nullptr);
        }

    };

    struct SignalBlocker
    {
        sigset_t oldState;

        SignalBlocker()
        {
            sigset_t allSet;
            sigfillset(&allSet);
            if(pthread_sigmask(SIG_BLOCK, &allSet, &oldState))
                CPL_POSIX_EXCEPTION("pthread_sigmask");
        }

        ~SignalBlocker()
        {
            pthread_sigmask(SIG_SETMASK, &oldState, nullptr);
        }

    };

    void Process::initialise(PipePair& in, PipePair& out, PipePair& err, EnvStrings * strings, const std::string * cwd, bool launchDetached)
    {
        // reap detached childs
        {
            std::lock_guard<std::mutex> orphanLock(orphanMutex);
            siginfo_t info;

            for(auto it = orphanList.begin(); it != orphanList.end();)
            {
                if(waitid(P_PID, *it, &info, WNOHANG | WEXITED))
                {
                    if(errno != EINTR)
                    {
                        std::swap(*it, orphanList.back());
                        orphanList.pop_back();
                        continue;
                    }
                }
                else
                {
                    if(info.si_signo == SIGCHLD && info.si_pid == *it)
                    {
                        std::swap(*it, orphanList.back());
                        orphanList.pop_back();
                        continue;
                    }
                }
                it++;
            }

        }

        volatile sig_atomic_t result = 0, childError = 0;

        auto exitChild = [&childError, &result] (auto errResult)
        {
            result = errResult;
            childError = errno;
            _exit(errResult);
        };

        auto initialisePipe = [&exitChild, this] (const PipePair& pair, auto streamMask, auto fdes, auto deffd)
        {
            if(fdes < 0)
                exitChild(-2);

            bool output = !(streamMask & IOStreamFlags::In);

            if(flags & streamMask)
            {
                if(dup2(output ? pair.second.get() : pair.first.get(), fdes) < 0)
                    exitChild(-3);
                if(close(output ? pair.first.get() : pair.second.get()))
                    exitChild(-4);
            }
            else
            {
                if(dup2(deffd, fdes) < 0)
                    exitChild(-5);
            }
        };

        PThreadCancelDisabler disabler;
        SignalBlocker blocker;

        auto argv = pArgs.argv();

        auto childEnv = strings ? strings->environ() : environ;
        auto pidf = vfork();

        if(pidf == -1)
            CPL_SYSTEM_EXCEPTION("vfork");



        if(pidf == 0)
        {
            // child
            auto dev0 = open("/dev/null", O_RDWR);

            initialisePipe(out, IOStreamFlags::Out, fileno(stdout), dev0);
            initialisePipe(err, IOStreamFlags::Err, fileno(stderr), dev0);
            initialisePipe(in, IOStreamFlags::In, fileno(stdin), dev0);

            if(close(dev0))
                exitChild(-6);

            // unset all signals, and clear all to default
            if(!CloseAllFiles())
                exitChild(-7);

            struct sigaction sac;
            sac.sa_handler = SIG_DFL;
            sac.sa_flags = 0;

            if(sigemptyset(&sac.sa_mask))
                exitChild(-8);

            sigset_t allSet;
            sigfillset(&allSet);

            for (int i = 0 ; i < NSIG; i++)
            {
                if(i == SIGKILL || i == SIGSTOP)
                    continue;

                if(sigismember(&allSet, i) == 1)
                {
                    if(sigaction(i, &sac, nullptr))
                        exitChild(-9);
                }
            }

            if(pthread_sigmask(SIG_UNBLOCK, &allSet, nullptr))
                exitChild(-10);

            if(cwd && chdir(cwd->c_str()))
                exitChild(-11);

            execve(pname.c_str(), argv, childEnv);

            exitChild(-12);
        }
        else
        {
            // parent
            if(result != 0)
            {
                auto failedDesc = "process creation failed (" + std::to_string(result) + "): " + Misc::GetLastOSErrorMessage(childError);
                // must reap the child
                int ignored;
                if(pidf != waitpid(pidf, &ignored, 0))
                {
                    CPL_SYSTEM_EXCEPTION("waitpid() errornously waited on the wrong process, also: " + failedDesc);
                }

                CPL_RUNTIME_EXCEPTION(failedDesc);
            }
            else
            {
                pid.set(static_cast<std::int64_t>(pidf));
            }
        }
    }

    Process Process::Builder::shell(Args args, int ioFlags, ScopeExitOperation operation)
    {
        return {
            "/bin/sh",
            std::move(Args().arg("-c").arg((process + args).commandLine())),
            ioFlags,
            ScopeExitOperation::Join,
            hasEnv ? &env : nullptr,
            hasCWD ? &cwd : nullptr
        };
    }

    Process Process::Builder::terminal(Args args, ScopeExitOperation operation)
    {
        return {
            "/bin/sh",
            std::move(Args().arg("-c") + Args().arg("gnome-terminal").arg("--disable-factory").arg("-e").arg((process + args).commandLine()).commandLine()),
            IOStreamFlags::None,
            ScopeExitOperation::Join,
            hasEnv ? &env : nullptr,
            hasCWD ? &cwd : nullptr
        };
    }

    void Process::Builder::launchDetached(Args args)
    {
        launch(args).detach();

        /*Process {
            "/bin/sh",
            std::move(Args().arg("-c").arg(Args().arg("setsid").arg((process + args).commandLine()).commandLine())),
            IOStreamFlags::None,
            ScopeExitOperation::Join,
            hasEnv ? &env : nullptr,
            hasCWD ? &cwd : nullptr,
            true
        }; */
    }

}
