/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

#ifndef _COMMON_H
	#define _COMMON_H

	#include <string>

	struct ProgramInfo
	{
		std::string name; // the name of the program
		std::string version; // a version string of the program
		long long int versionInteger; // an integer version literal, that follows compare semantics
		std::string author; // your (company's) name
		std::string programAbbr; // abbreviation of the program's name for small spaces (and file extensions)
		// cpl has a generic tree structure, it looks for in the current working directory.
		// you can specify your own here - will affect filechoosers, temporary files, resources etc.
		// set this to true if you specify your own.
		bool hasCustomDirectory; 
		// the path (or relative offset) to your custom directory.
		std::string customDirectory;

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