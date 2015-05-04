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
 
	file:LexicalConversion.h

		Very similar to boost::lexical_cast, except this is based around not throwing
		exceptions.

*************************************************************************************/

#ifndef _LEXICAL_CONVERSION
	#define _LEXICAL_CONVERSION

	#include "common.h"
	#include <sstream>

	namespace cpl
	{
		template<typename From, typename To>
			bool lexicalConversion(const From & f, To & t)
			{
				std::stringstream ss;
				if ((ss << f) && (ss >> t))
					return true;
				return false;
			}

		#ifdef CPL_JUCE
			bool lexicalConversion(const juce::String & from, double & to)
			{
				double output;
				char * endPtr = nullptr;
				output = strtod(from.getCharPointer(), &endPtr);
				if (endPtr > from.getCharPointer())
				{
					to = output;
					return true;
				}
				return false;
			}
		#endif

		bool lexicalConversion(const std::string & from, double & to)
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
	}; // cpl
#endif