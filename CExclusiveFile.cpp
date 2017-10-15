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

	file:CExclusiveFile.cpp

		Implementation of CExclusiveFile class.

*************************************************************************************/

#include "CExclusiveFile.h"
#include "MacroConstants.h"
#include "PlatformSpecific.h"
#include "Misc.h"

namespace cpl
{

	CExclusiveFile::CExclusiveFile()
		: isOpen(false), handle(0)
	{

	}

	CExclusiveFile::~CExclusiveFile()
	{
		if (isOpened())
			close();
	}

	bool CExclusiveFile::open(const std::string & path, std::uint32_t m, bool waitForLock)
	{
		if (isOpened())
			close();
		fileName = path;
		fileMode = (mode)m;
		#if defined(CPL_CEF_USE_CSTDLIB)

		handle = ::fopen(fileName.c_str(), "w");
		if (!handle) {
			isOpen = false;
			handle = 0;
		}
		isOpen = true;
		return isOpened();

		#elif defined(CPL_WINDOWS)
		// if we're opening in writemode, we always create a new, empty file
		// if it's reading mode, we only open existing files.
		auto openMode = OPEN_ALWAYS;
		auto fileMask = fileMode & append ? FILE_APPEND_DATA : fileMode;

		auto start = Misc::QuickTime();
		do
		{
			handle = ::CreateFileA(path.c_str(),
				fileMask,
				0x0, // this is the important part - 0 as dwShareMode will open the file exclusively (default)
				nullptr,
				openMode,
				FILE_ATTRIBUTE_NORMAL,
				nullptr
			);

			if (handle != INVALID_HANDLE_VALUE)
				break;

			// timed out.
			if (Misc::QuickTime() > start + 2000)
				return false;

			Misc::Delay(0); // yield to other threads.
		} while (waitForLock);
		if (handle != INVALID_HANDLE_VALUE)
		{
			isOpen = true;
		}
		else
		{
			isOpen = false;
			handle = 0;
		}

		#elif defined(CPL_UNIXC)
		int openMask = m & mode::writeMode ? O_WRONLY | O_CREAT : O_RDONLY;
		openMask |= m & mode::append ? O_APPEND : m & mode::writeMode ? O_TRUNC : 0;

		auto start = Misc::QuickTime();
		auto lockGranted = 0;

		// TODO: refactor using open_excl on linux and O_EXLOCK on BSD
		do
		{
			if (m & mode::writeMode)
			{

				mode_t permission = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;

				handle = ::open(fileName.c_str(), openMask, permission);
			}
			else
			{
				handle = ::open(fileName.c_str(), openMask);

			}
			if (handle < 0)
			{
				isOpen = false;
				handle = 0;
				return false;
			}
			int mask = waitForLock ? 0 : LOCK_NB;
			mask |= LOCK_EX;
			lockGranted = ::flock(handle, mask);

			if (lockGranted == 0)
				break;
			else
				::close(handle);

			// timed out.
			if (Misc::QuickTime() > start + 2000)
				return false;

			Misc::Delay(10); // yield to other threads.

		} while (waitForLock);

		// could not obtain lock on file, file is opened in another instance
		if (lockGranted == 0)
		{
			isOpen = true;
		}

		#endif

		return isOpened();
	}

	bool CExclusiveFile::newline()
	{
		return write(newl);
	}

	std::int64_t CExclusiveFile::getFileSize()
	{
		#if defined(CPL_CEF_USE_CSTDLIB)
		if (isOpen)
		{
			auto oldPos = ftell(handle);
			fseek(handle, 0L, SEEK_END);
			auto size = ftell(handle);
			fseek(handle, oldPos, SEEK_SET);
			return size;
		}

		#elif defined(CPL_WINDOWS)
		if (isOpen)
		{
			LARGE_INTEGER fileSize = {0};
			if (::GetFileSizeEx(handle, &fileSize) != FALSE)
				return static_cast<std::int64_t>(fileSize.QuadPart);
		}
		#elif defined(CPL_UNIXC)
		if (isOpen)
		{
			struct stat fileStats = {};
			auto ret = fstat(handle, &fileStats);
			return ret == 0 ? fileStats.st_size : 0;
		}
		#endif
		return 0;
	}

	bool CExclusiveFile::read(void * src, std::int64_t bufsiz)
	{
		#if defined(CPL_CEF_USE_CSTDLIB)
		if (isOpen)
			return std::fwrite(src, bufsiz, 1, handle) == bufsiz;
		#elif defined(CPL_WINDOWS)
		DWORD dwRead(0);
		if (bufsiz > std::numeric_limits<DWORD>::max())
			CPL_RUNTIME_EXCEPTION("buffer size too large");
		DWORD dwSize = static_cast<DWORD>(bufsiz);
		auto ret = ::ReadFile(handle, src, dwSize, &dwRead, nullptr);
		return (ret != FALSE) && (dwRead == dwSize);
		#elif defined(CPL_UNIXC)
		if (isOpen)
			return ::read(handle, src, (std::size_t)bufsiz) == bufsiz;
		#endif
		return false;
	}

	bool CExclusiveFile::write(const void * src, std::int64_t bufsiz)
	{
		#if defined(CPL_CEF_USE_CSTDLIB)
		if (isOpen)
			return std::fwrite(src, bufsiz, 1, handle) == bufsiz;
		#elif defined(CPL_WINDOWS)
		DWORD dwWritten(0);
		DWORD dwSize = static_cast<DWORD>(bufsiz);
		if (bufsiz > std::numeric_limits<DWORD>::max())
			CPL_RUNTIME_EXCEPTION("buffer size too large");
		auto ret = ::WriteFile(handle, src, dwSize, &dwWritten, nullptr);
		return (ret != FALSE) && (dwWritten == dwSize);
		#elif defined(CPL_UNIXC)
		if (isOpen)
			return ::write(handle, src, (std::size_t)bufsiz) == bufsiz;
		#endif
		return false;
	}

	bool CExclusiveFile::isFileExclusive(const std::string & path)
	{
		CExclusiveFile f;
		f.open(path, mode::readMode, false);
		return !f.isOpened();
	}

	bool CExclusiveFile::reset()
	{
		close();
		return open(fileName);
	}

	bool CExclusiveFile::remove()
	{

		if (isOpened())
		{
			close();
			std::remove(fileName.c_str());
			return true;
		}
		return false;
	}

	bool CExclusiveFile::write(const char * src)
	{
		return this->write(src, (std::int64_t)std::strlen(src));
	}

	bool CExclusiveFile::isOpened()
	{
		return isOpen;
	}

	const std::string & CExclusiveFile::getName()
	{
		return fileName;
	}

	bool CExclusiveFile::flush()
	{
		if (!isOpen)
			return false;
		#if defined(CPL_CEF_USE_CSTDLIB)
		return !std::fflush(handle);
		#elif defined(CPL_WINDOWS)
		return !!::FlushFileBuffers(handle);
		#elif defined(CPL_UNIXC)
		return ::fsync(handle) < 0 ? false : true;
		#endif
		return false;
	}

	bool CExclusiveFile::close()
	{
		if (!isOpen)
			return false;
		bool ret(false);
		#if defined(CPL_CEF_USE_CSTDLIB)
		ret = !std::fclose(handle);
		#elif defined(CPL_WINDOWS)
		ret = !!::CloseHandle(handle);
		#elif defined(CPL_UNIXC)
		::flock(handle, LOCK_UN);
		ret = ::close(handle) < 0 ? false : true;
		#endif
		isOpen = false;
		handle = (decltype(handle))0;
		return ret;
	}
};
