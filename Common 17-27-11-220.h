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
	//#include "MacroConstants.h"
	#define DONT_SET_USING_JUCE_NAMESPACE 1
	#include "../JuceLibraryCode/JuceHeader.h"

	namespace cpl
	{
		typedef juce::Colour CColour;
		typedef juce::Colours CColours;
		typedef juce::Point<int> CPoint;
		typedef juce::Rectangle<int> CRect;
		typedef juce::Component GraphicComponent;
		typedef int CCoord;

		class CBaseControl;

		#define ControlSize 80

		enum TextSize
		{
			smallerText = 10,
			smallText = 12,
			normalText = 14,
			largeText = 17
		};
		// enum emulation
	};
#endif