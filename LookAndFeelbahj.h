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

	file:LookAndFeel.h
		
		Speciales the graphic library to achieve a uniform look.

*************************************************************************************/

#ifndef _LOOKANDFEEL_H
	#define _LOOKANDFEEL_H
	#if defined(TYPEFACE_BITMAP_RENDERING) && defined(CPL_HINT_FONT)
		#include "vf_lib/vf_gui/vf_FreeTypeFaces.h"
	#endif
	namespace cpl
	{
		#ifdef CPL_JUCE
			typedef juce::Colour CColour;
			typedef juce::Colours CColours;
			typedef juce::Point<int> CPoint;
			typedef juce::Rectangle<int> CRect;
			typedef juce::Component GraphicComponent;

			typedef int CCoord;

			class CBaseControl;


			namespace ControlSize
			{
				struct BoundingRect
				{
					long width, height;
				};

				extern BoundingRect Square;

				extern BoundingRect Rect;
			};

			/*enum TextSize
			{
				smallerText = 10,
				smallText = 12,
				normalText = 14,
				largeText = 17
			};*/

			namespace TextSize
			{
				extern float smallerText;
				extern float smallText;
				extern float normalText;
				extern float largeText;
			};

			// other font sizes for Verdana
			// smaller = 10.3
			// small = 11.7
			// medium = 12.8 / 13
			// large = 16.1
			// enum emulation


			extern juce::Colour colourDeactivated;
			extern juce::Colour colourActivated;
			extern juce::Colour colourAux;
			extern juce::Colour colourAuxFont;
			extern juce::Colour colourSeparator;
			extern juce::Colour colourSelFont;
			extern juce::Colour colourSuccess;
			extern juce::Colour colourError;

			extern juce::Font systemFont;


			extern char* helveticaneueltcommd_ttf;
			extern int   helveticaneueltcommd_ttfSize;

			#if defined(TYPEFACE_BITMAP_RENDERING) && defined(CPL_HINT_FONT)
				class CPLLookAndFeel : public LookAndFeel
				{
				public:
					CPLLookAndFeel()
					{
						// Add the TrueType font "Helvetica Neue LT Com 65 Medium" and
						// use hinting when the font height is between 7 and 12 inclusive.

						FreeTypeFaces::getInstance()->addFaceFromMemory(
							7.f, 12.f,
							helveticaneueltcommd_ttf,
							helveticaneueltcommd_ttfSize);
					}

					const Typeface::Ptr CustomLookAndFeel::getTypefaceForFont(Font const& font)
					{
						Typeface::Ptr tf;

						String faceName(font.getTypefaceName());

						// Make requests for the default sans serif font use our
						// FreeType hinted font instead.

						if (faceName == Font::getDefaultSansSerifFontName())
						{
							// Create a new Font identical to the old one, then
							// switch the name to our hinted font.

							Font f(font);

							// You'll need to know the exact name embedded in the font. There
							// are a variety of free programs for retrieving this information.

							f.setTypefaceName("Helvetica Neue LT Com 65 Medium");

							// Now get the hinted typeface.

							tf = FreeTypeFaces::createTypefaceForFont(f);
						}

						// If we got here without creating a new typeface
						// then just use the default LookAndFeel behavior.

						if (!tf)
							tf = LookAndFeel::getTypefaceForFont(font);

						return tf;
					}
				};
			#else
				using CPLLookAndFeel = juce::LookAndFeel;
			#endif
		#endif
	};
#endif