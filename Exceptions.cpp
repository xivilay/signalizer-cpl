/*************************************************************************************

	 cpl - cross platform library - v. 0.1.0.

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

	file:Exceptions.cpp

		Implementation of Exceptions.h

*************************************************************************************/

#include "Exceptions.h"
#include "Misc.h"
#include "CExclusiveFile.h"
#include "PlatformSpecific.h"

namespace cpl
{
	int GetLastOSError()
	{
		#ifdef CPL_WINDOWS
			return GetLastError();
		#else
			return errno;
		#endif
	}

	std::string GetLastOSErrorMessage(int errorToUse)
	{
		auto lastError = errorToUse;
		#ifdef CPL_WINDOWS
			char * apiPointer = nullptr;

			auto numChars = FormatMessageA
			(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				0,
				lastError,
				0,
				apiPointer,
				0,
				0
			);

			std::string ret(apiPointer, numChars);
			LocalFree(apiPointer);
			return ret;
		#else
			return "Error (" + std::to_string(lastError) + "): " + strerror(lastError);
		#endif
	}

	std::string GetLastOSErrorMessage()
	{
		return GetLastOSErrorMessage(GetLastOSError());
	}


	void LogException(const zstr_view errorMessage)
	{
		using namespace cpl::Misc;
		CExclusiveFile exceptionLog;

		exceptionLog.open(GetDirectoryPath() + "/" + programInfo.name + " exceptions.log",
			exceptionLog.writeMode | exceptionLog.append, true);
		exceptionLog.newline();
		exceptionLog.write(("----------------" + GetDate() + ", " + GetTime() + "----------------").c_str());
		exceptionLog.newline();
		exceptionLog.write(("- Exception in \"" + programInfo.name + "\" v.\"" + std::to_string(programInfo.version) + "\"").c_str());
		exceptionLog.newline();
		exceptionLog.write(errorMessage.data(), errorMessage.size());
		exceptionLog.newline();
	}

	void CrashIfUserDoesntDebug(const zstr_view errorMessage)
	{
		using namespace cpl::Misc;

		auto ret = MsgBox(errorMessage.string() + newl + newl + "Press yes to break after attaching a debugger. Press no to crash.", programInfo.name + ": Fatal error",
			MsgStyle::sYesNo | MsgIcon::iStop);

		if (ret == MsgButton::bYes)
		{
			CPL_BREAKIFDEBUGGED();
		}
	}


}
