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

namespace cpl
{
	namespace detail
	{
		template<typename T, std::ptrdiff_t invalid>
		struct NullableHandle
		{
			static constexpr T null = (T)invalid;
			NullableHandle() : handle(null) {}
			NullableHandle(T h) : handle(h) {}
			NullableHandle(std::nullptr_t) : handle(null) {}

			operator T() { return handle; }

			bool operator ==(const NullableHandle &other) const { return handle == other.handle; }
			bool operator !=(const NullableHandle &other) const { return handle != other.handle; }
			bool operator ==(std::nullptr_t) const { return handle == null; }
			bool operator !=(std::nullptr_t) const { return handle != null; }

			T handle;
		};

#ifdef CPL_WINDOWS
		typedef NullableHandle<HANDLE, -1> Handle;
#else
		typedef NullableHandle<int, -1> Handle;
#endif

		class SysDeleter
		{
		public:
			typedef Handle pointer;

			void operator() (pointer p)
			{
#ifdef CPL_WINDOWS
				::CloseHandle(p);
#else
				::close(p);
#endif
			}
		};

		using unique_handle = std::unique_ptr<Handle, SysDeleter>;

	}

	class Process
	{
		class OutputBuf : public std::streambuf
		{
		public:

			OutputBuf(detail::unique_handle pipe) : pipe(std::move(pipe)) { }

			~OutputBuf()
			{
				close();
			}

			void close()
			{
				if (pipe.get() == nullptr)
					return;

				sync();
				pipe.reset(nullptr);
			}

		protected:

			virtual int sync() override
			{
				if (pipe.get() == nullptr)
					return -1;

				auto nc = pptr() && pbase() ? pptr() - pbase() : 0;

				setp(std::begin(buffer), std::end(buffer));

				if (nc <= 0)
					return 0;

#ifdef CPL_WINDOWS
				DWORD dwWritten;
				if (::WriteFile(pipe.get(), buffer, nc, &dwWritten, nullptr) && dwWritten == nc)
				{
					return 0;
				}
#else
				if (::write(pipe.get(), buffer, nc) == nc)
					return 0;
#endif
				return -1;
			}

			virtual int overflow(int c) override
			{

				if (pipe.get() == nullptr)
					return traits_type::eof();

				sync();

				*pptr() = traits_type::to_char_type(c);
				this->pbump(1);

				return traits_type::not_eof(c);
			}

			detail::unique_handle pipe;
			char buffer[1024];
		};


		struct postream : public std::ostream
		{
			virtual void close() = 0;
			using std::ostream::ostream;
		};

		struct OutputStreamImplementation : public postream
		{
			virtual void close() override
			{
				buf.close();
			}

			OutputStreamImplementation(OutputBuf* buffer) : postream(buffer), buf(*buffer) {}

			OutputBuf& buf;
		};

		template<typename Buf, typename Stream>
		class PipeEdge
		{
		public:

			PipeEdge(detail::unique_handle pipe)
				: buf(std::move(pipe)), stream(&buf)
			{

			}

			Buf buf;
			Stream stream;
		};

		typedef std::pair<detail::unique_handle, detail::unique_handle> PipePair;

    public:

		class Args;

    private:

        Process(const std::string& process, const Args& args, int ioFlags, bool launchSuspended);

	public:


		enum io_streams
		{
			none = 0,
			out = 1 << 0,
			err = 1 << 1,
			in = 1 << 2
		};

		using ios = io_streams;
		class Builder;

		class Builder
		{
		public:

			Builder(Builder&& other) = default;
			Builder& operator = (Builder&& other) = default;

			Builder(const std::string& processLocation)
				: process(processLocation)
			{

			}

			Process launch(const Args& args = Args(), int ioFlags = ios::none)
			{
				return{ process, args.compiledArgs, ioFlags, false };
			}

			Process launchSuspended(const Args& args = Args(), int ioFlags = ios::none)
			{
				return{ process, args.compiledArgs, ioFlags, true };
			}

		private:
			std::string process;
		};


		Process(Process&& other) = default;
		Process(const Process& other) = delete;
		Process& operator = (Process&& other) = default;
		Process& operator = (const Process& other) = delete;

		std::istream & cout()
		{
			if (!pout.get())
				CPL_RUNTIME_EXCEPTION("No stdout pipe allocated for process");

			return pout->stream;
		}

		std::istream & cerr()
		{
			if (!perr.get())
				CPL_RUNTIME_EXCEPTION("No stderr pipe allocated for process");

			return perr->stream;
		}

		postream & cin()
		{
			if (!pin.get())
				CPL_RUNTIME_EXCEPTION("No stdin pipe allocated for process");

			return pin->stream;
		}

		const std::string & name() const noexcept
		{
			return pname;
		}

		void resume() noexcept
		{

		}

		void suspend() noexcept
		{

		}

		void detach() noexcept
		{

		}

		bool isAlive() const noexcept
		{

		}

		void join(int timeoutMs = -1) const noexcept
		{

		}

	private:

		static PipePair createPipe(int parentEnd = -1)
		{
			detail::Handle
				in = nullptr,
				out = nullptr;

#ifdef CPL_WINDOWS
			SECURITY_ATTRIBUTES saAttr;
			saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
			saAttr.bInheritHandle = true;
			saAttr.lpSecurityDescriptor = nullptr;

			if (!CreatePipe(&in.handle, &out.handle, &saAttr, 0))
				CPL_RUNTIME_EXCEPTION("Error creating pipe pair");
#else
            int fd[2];

            if(pipe(fd))
                CPL_RUNTIME_EXCEPTION("Error creating pipe pair");

            in = fd[0];
            out = fd[1];
#endif

			detail::unique_handle fin(in), fout(out);

#ifdef CPL_WINDOWS

			if (parentEnd == 0 && !SetHandleInformation(fin.get(), HANDLE_FLAG_INHERIT, 0))
				CPL_RUNTIME_EXCEPTION("Error setting inheritance permissions on pipes");


			if (parentEnd == 1 && !SetHandleInformation(fout.get(), HANDLE_FLAG_INHERIT, 0))
				CPL_RUNTIME_EXCEPTION("Error setting inheritance permissions on pipes");
#else

#endif
			return{ std::move(fin), std::move(fout) };
		}

		class InputBuffer : public std::streambuf
		{
		public:
			InputBuffer(detail::unique_handle pipe) : pipe(std::move(pipe)) {}

		protected:

			virtual int underflow() override
			{
				if (pipe.get() == nullptr)
					return traits_type::eof();

#ifdef CPL_WINDOWS
				DWORD lpRead;
				if (ReadFile(pipe.get(), buf, sizeof(buf), &lpRead, nullptr))
				{
					setg(buf, buf, buf + lpRead);
					return buf[0];
				}
#else
				auto read = ::read(pipe.get(), buf, sizeof(buf));
				if (read > 0)
				{
					setg(buf, buf, buf + read);
					return buf[0];
				}
#endif
				return traits_type::eof();
			}

			detail::unique_handle pipe;
			char buf[1024];
		};

		int pid;
		ios flags;
		std::string pname;

		typedef PipeEdge<InputBuffer, std::istream> InPipe;
		typedef PipeEdge<OutputBuf, OutputStreamImplementation> OutPipe;

#ifdef CPL_WINDOWS
		detail::unique_handle
			childProcessHandle,
			childThreadHandle;
#endif

		std::unique_ptr<OutPipe> pin;
		std::unique_ptr<InPipe> pout, perr;
	};

	Process::Process(const std::string& process, const Args& args, int ioFlags, bool launchSuspended)
	{
		flags = static_cast<ios>(ioFlags);

		PipePair in, out, err;

		if (ioFlags & ios::in)
			in = createPipe(1);
		if (ioFlags & ios::out)
			out = createPipe(0);
		if (ioFlags & ios::err)
			err = createPipe(0);

#ifdef CPL_WINDOWS
		PROCESS_INFORMATION piProcInfo{};
		STARTUPINFO siStartInfo{};

		siStartInfo.cb = sizeof(STARTUPINFO);

		if (ioFlags)
		{
            siStartInfo.hStdOutput = out.second.get();
			siStartInfo.hStdError = err.second.get();
			siStartInfo.hStdInput = in.first.get();
			siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
		}


		auto copy = process;

		auto success = ::CreateProcess(
			nullptr,
			&(process + " " + commandLine)[0],     // command line
			nullptr,          // pname security attributes
			nullptr,          // primary thread security attributes
			true,          // handles are inherited
			0,             // creation flags
			nullptr,          // use parent's environment
			nullptr,          // use parent's current directory
			&siStartInfo,  // STARTUPINFO pointer
			&piProcInfo		// receives PROCESS_INFORMATION
		);

		if (!success)
			CPL_RUNTIME_EXCEPTION("Error starting process: " + cpl::Misc::GetLastOSErrorMessage());
#else

        /* auto execErrPipe = pipePair();

        if (::fcntl(execErrPipe.second.get(), F_SETFD, ::fcntl(execErrPipe.second.get(), F_GETFD) | FD_CLOEXEC))
        {
            CPL_RUNTIME_EXCEPTION("Error setting fcntl on child error exec pipe");
        } */

        auto proc = process.c_str();
        std::vector<char *> cargs(args.rawArgs().size() + 2);
        cargs[0] = const_cast<char *>(proc);

        for(std::size_t i = 0; i < args.rawArgs().size(); ++i)
            cargs[i + 1] = const_cast<char *>(args.rawArgs()[i].c_str());

        cargs[cargs.size() - 1] = nullptr;

        sig_atomic_t result = 0, childError = 0;

        auto pidf = vfork();

        if(pidf == -1)
        {
            CPL_RUNTIME_EXCEPTION("Error fork'ing()");
        }
        else if(pidf == 0)
        {
            if (ioFlags & ios::out)
                dup2(out.first.get(), fileno(stdout));
            if (ioFlags & ios::err)
                dup2(err.first.get(), fileno(stderr));
            if (ioFlags & ios::in)
                dup2(in.second.get(), fileno(stdin));

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
            }

            // manually reset non-child ends because execv is a hack
            /*err.second = nullptr;
            out.second = nullptr;
            in.first = nullptr; */
            //execErrPipe.first = nullptr;

            execvp(proc, cargs.data());

            result = -1;
            childError = errno;

            // if we made it here, exec failed. write errno to the exec err pipe.
            //::write(execErrPipe.second, &errno, sizeof(int));
            _exit(childError);
        }
        else
        {
            pid = static_cast<int>(pidf);
        }
#endif
		// assign handles
#ifdef CPL_WINDOWS
		childProcessHandle.reset(piProcInfo.hProcess);
		childThreadHandle.reset(piProcInfo.hThread);
		pid = static_cast<int>(piProcInfo.dwProcessId);
#else


#endif


		if (ioFlags & ios::in)
			pin = std::make_unique<OutPipe>(std::move(in.second));
		if (ioFlags & ios::out)
			pout = std::make_unique<InPipe>(std::move(out.first));
		if (ioFlags & ios::err)
			perr = std::make_unique<InPipe>(std::move(err.first));

	}

}

#endif
