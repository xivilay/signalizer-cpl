/*************************************************************************************
 
 cpl - cross-platform library - v. 0.1.0.
 
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
 
 file:BuildingBlocks.h
 
	Some basic GUI blocks that can be reused
 
 *************************************************************************************/

#ifndef BUILDING_BLOCKS_H
	#define BUILDING_BLOCKS_H

	#include "../Common.h"

	namespace cpl
	{
		struct SemanticBorder : public juce::Component
		{
			SemanticBorder()
				: borderColour(juce::Colours::black), isActive(false), borderSize(1.f)
			{
				setOpaque(false);
				setWantsKeyboardFocus(false);
				setInterceptsMouseClicks(false, false);
			}

			void paint(juce::Graphics & g)
			{
				if (!isActive)
					return;
				g.setColour(borderColour);
				g.drawRect(getBounds().toFloat(), borderSize);
			}
			juce::Colour borderColour;
			float borderSize;
			bool isActive;
		};

	};

#endif