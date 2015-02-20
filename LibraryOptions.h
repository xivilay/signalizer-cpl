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

#ifndef _LIBRARYOPTIONS_H
	#define _LIBRARYOPTIONS_H
	#include "MacroConstants.h"
	#ifdef CPL_JUCE
		//#define DONT_SET_USING_JUCE_NAMESPACE 1
		//#define TYPEFACE_BITMAP_RENDERING
		//#define CPL_HINT_FONT
		#define JUCE_ENABLE_LIVE_CONSTANT_EDITOR 1
	#endif
	#define CPL_THROW_ON_NO_RESOURCE

#endif