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
 
 file:CPLSource.cpp
 
	All source code needed to compile the cpl lib. Add this file to your project.
 
 *************************************************************************************/

#include "MacroConstants.h"
#include "LibraryOptions.h"
#include "Common.h"

namespace cpl
{

	namespace TextSize
	{
		float smallerText = 10.3f;
		float smallText = 11.7f;
		float normalText = 12.8f;
		float largeText = 16.1;
	};

	#ifdef CPL_JUCE

		namespace ControlSize
		{
			BoundingRect Square{ 80, 80 };
			BoundingRect Rectangle{ 60, 120 };
		};


		// grey-dark color set
		/*
		namespace cpl
		{
		Colour colourDeactivated	(187, 187, 187);
		Colour colourActivated		(223, 223, 223);
		Colour colourAux			(203, 203, 203);
		Colour colourAuxFont		(115, 115, 115);
		Colour colourSeparator		(115, 115, 115);
		Colour colourSelFont		(0,		0,	0);
		};
		*/

		// dark-white scheme

		Colour colourDeactivated(26, 26, 26);
		Colour colourActivated(50, 50, 50);
		Colour colourAux(203, 203, 203);
		Colour colourAuxFont(128, 128, 128);
		Colour colourSeparator(75, 75, 75);
		Colour colourSelFont(153, 153, 102);
		Colour colourSuccess(0, 0x7F, 0);
		Colour colourError(0x7F, 0, 0);
		juce::Font systemFont("Verdana", TextSize::normalText, juce::Font::plain);

		//auto cplLook = juce::LookAndFeel_V3();

	#endif
};