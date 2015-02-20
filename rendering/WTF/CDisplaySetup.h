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
 
	file: CDisplaySetup.h
 
		Manages a list of all connected screens and their respective properties.
		Update() should only be called from the main thread, and should be called
		on system resizing / device mounting.

 *************************************************************************************/

#ifndef _CDISPLAYSETUP_H
	#define _CDISPLAYSETUP_H
	#include "../Common.h"
	#include "SubpixelRendering.h"
	#include <vector>
	#include <memory>

	namespace cpl
	{
		namespace rendering
		{
			class CDisplaySetup
			{
			public:
				struct DisplayData
				{

					DisplayData()
					:
						isApplicableForSubpixels(false),
						fontGamma(1.2),
						displayMatrixOrder(LCDMatrixOrientation::RGB),
						screenRotation(0.0),
						screenOrientation(Orientation::Landscape),
						gammaScale(fontGamma),
						isMainMonitor(true),
						scale(1.0),
						dpi(72.0)
					{

					}

					bool isApplicableForSubpixels;
					double fontGamma;
					LCDMatrixOrientation displayMatrixOrder;
					double screenRotation;
					Orientation screenOrientation;
					LutGammaScale gammaScale;
					juce::Rectangle<int> bounds;
					bool isMainMonitor;
					double scale, dpi;

				};

				static CDisplaySetup & instance();

				const DisplayData & displayFromPoint(std::pair<int, int> pos) const;
				const DisplayData & displayFromPoint(juce::Point<int> pos) const {
					return displayFromPoint(std::make_pair(pos.getX(), pos.getY()));
				}
				const DisplayData & displayFromIndex(std::size_t index) const;
				const DisplayData & getMainDisplay() const;
				const DisplayData * begin() const { return &displays[0]; }
				const DisplayData * end() const { return &displays[0] + displays.size(); };

				std::size_t numDisplays() const { return displays.size(); }

				void update();

			private:

				DisplayData defaultDevice;
				static void Deleter();
				CDisplaySetup();
				std::vector<DisplayData> displays;
				static CDisplaySetup * internalInstance;
				double defaultFontGamma;
			};

		}; // {} rendering
	}; // {} cpl
#endif