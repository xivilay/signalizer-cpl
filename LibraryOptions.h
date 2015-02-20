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

	file:LibraryOptions.h
	
		Compile-time switches for the library.
 
		#define CPL_HINT_FONT:
			if application is GUI, includes FreeType and uses
			it for getting outlines for glyphs.
		#define CPL_THROW_ON_NO_RESOURCE:
			if set, will throw an exception if cpl cannot find
			a needed resource.
 
 
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