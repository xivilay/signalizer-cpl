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

	file:Common.h
	
		Implements common constants, types & macroes used throughout the program.
		Compilier specific tunings.
		Also #includes commonly used headers.

*************************************************************************************/

#ifndef CPL_COMMON_H
	#define CPL_COMMON_H

	#include "ProgramVersion.h"
	#include <string>
	#include <cstdint>

	struct ProgramInfo
	{
		std::string name; // the name of the program
		cpl::Version version;
		std::string author; // your (company's) name
		std::string programAbbr; // abbreviation of the program's name for small spaces (and file extensions)
		// cpl has a generic tree structure, it looks for in the current working directory.
		// you can specify your own here - will affect filechoosers, temporary files, resources etc.
		// set this to true if you specify your own.
		bool hasCustomDirectory; 
		// a function returning the path (or relative offset) to your custom directory.
		std::string (*customDirectory)();

	};

	namespace cpl
	{
		extern const ProgramInfo programInfo;
	}

	#include "LibraryOptions.h"

	#ifdef CPL_JUCE
		#include "../JuceLibraryCode/JuceHeader.h"
	#endif

#endif