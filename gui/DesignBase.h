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
 
 file:DesignBase.h
 
	Contains the methods, classes and data that encompass the standard 'design', 
	look or feel needed to give an uniform look.
 
 *************************************************************************************/

#ifndef _DESIGNBASE_H
	#define _DESIGNBASE_H

	#include "../Common.h"
	#include <map>
	#include <vector>

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

				extern BoundingRect Rectangle;
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




			/*extern juce::Colour colourDeactivated;
			extern juce::Colour colourNormal;
			extern juce::Colour colourActivated;
			extern juce::Colour colourAux;
			extern juce::Colour colourAuxFont;
			extern juce::Colour colourSeparator;
			extern juce::Colour colourSelFont;
			extern juce::Colour colourSuccess;
			extern juce::Colour colourError;
			extern juce::Colour colourCtrlText;*/

			//extern juce::Font systemFont;

			struct SchemeColour
			{
				juce::Colour colour;
				std::string name;
				std::string description;
			};

			enum class ColourEntry
			{
				deactivated,
				normal,
				activated,
				aux,
				auxfont,
				separator,
				selfont,
				success,
				error,
				ctrltext,
				End
			};



			class CLookAndFeel_CPL
			:
				public juce::LookAndFeel_V3,
				public cpl::Utility::CNoncopyable
			{
			public:

				static const std::size_t numColours = (std::size_t)ColourEntry::End;

				ColourEntry mapNameToColour() const;
				SchemeColour & getSchemeColour(ColourEntry entry);
				SchemeColour & getSchemeColour(std::size_t entry);
				virtual ~CLookAndFeel_CPL();

				static CLookAndFeel_CPL & defaultLook();
				// overrides
				juce::Typeface::Ptr getTypefaceForFont(juce::Font const& font) override;
				virtual juce::LowLevelGraphicsContext * createGraphicsContext(
					const Image &imageToRenderOn,
					const Point< int > &origin,
					const RectangleList< int > &initialClip) override;
				juce::Font getPopupMenuFont() override;
				juce::Font getComboBoxFont(ComboBox &) override;
				void drawComboBox(
					Graphics &, 
					int width, 
					int height, 
					bool isButtonDown, 
					int buttonX, 
					int buttonY, 
					int buttonW, 
					int buttonH, 
					ComboBox &) override;

				// extra interface
				juce::Font getStdFont();
				virtual const std::vector<char> & getFaceMemory(const std::string & s);
				virtual void setShouldRenderSubpixels(bool shouldRender);
				virtual void updateColours();
			protected:

				CLookAndFeel_CPL();

			private:
				bool tryToRenderSubpixel;
				std::vector<SchemeColour> colours;
				std::map<std::string, std::vector<char>> loadedFonts;
			};

			// saves some typing if you only want the colour.
			inline juce::Colour GetColour(ColourEntry colourEntry)
			{
				return CLookAndFeel_CPL::defaultLook().getSchemeColour(colourEntry).colour;
			}

		#endif

	};

#endif