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

	file:LexicalConversion.h

		Very similar to boost::lexical_cast, except this is based around not throwing
		exceptions.

*************************************************************************************/

#ifndef _LEXICAL_CONVERSION
#define _LEXICAL_CONVERSION

#include "Common.h"
#include <sstream>
#include "lib/string_ref.h"

namespace cpl
{
	/// <summary>
	/// Converts from f to t, lexically, using a stringstream.
	/// Returns success.
	/// </summary>
	/// <param name="f"></param>
	/// <param name="t"></param>
	/// <returns></returns>
	template<typename From, typename To>
	inline bool lexicalConversion(const From & f, To & t)
	{
		std::stringstream ss;
		if ((ss << f) && (ss >> t))
			return true;
		return false;
	}

	/// <summary>
	/// Optimized for std::strings
	/// </summary>
	/// <param name="from"></param>
	/// <param name="to"></param>
	/// <returns></returns>
	inline bool lexicalConversion(const string_ref from, double & to)
	{
		double output;
		char * endPtr = nullptr;
		output = strtod(from.c_str(), &endPtr);
		if (endPtr > from.c_str())
		{
			to = output;
			return true;
		}
		return false;
	}

	/// <summary>
	/// Optimized for std::strings
	/// </summary>
	/// <param name="from"></param>
	/// <param name="to"></param>
	/// <returns></returns>
	inline bool lexicalConversion(const string_ref from, std::int64_t & to)
	{
		std::int64_t output;
		char * endPtr = nullptr;
		output = strtoll(from.c_str(), &endPtr, 0);
		if (endPtr > from.c_str())
		{
			to = output;
			return true;
		}
		return false;
	}

	/// <summary>
	/// Optimized for numbers to strings
	/// </summary>
	/// <param name="from"></param>
	/// <param name="to"></param>
	/// <returns></returns>
	/*inline bool lexicalConversion(const std::int64_t & from, std::string & to)
	{
		to.reserve(std::numeric_limits<std::int64_t>::max_digits10());
		double output;
		char * endPtr = nullptr;
		output = strtoll(from.c_str(), &endPtr, 0);
		if (endPtr > from.c_str())
		{
			to = output;
			return true;
		}
		return false;
	}*/
}; // cpl
#endif
