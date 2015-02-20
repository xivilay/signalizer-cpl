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
 
	file:CDisplaySetup.cpp
		
		Implementation of CDisplaySetup.h

 *************************************************************************************/

#include "../MacroConstants.h"
#include "CDisplaySetup.h"

#ifdef __WINDOWS__
	#include "DisplayOrientationWindows.cpp"
#else
	// add linux, osx, android, ios here.
	//#error "Update displayorientations to your current platform!"
#endif
#include "../PlatformSpecific.h"
#include "../MacSupport.h"

namespace cpl
{
	namespace rendering
	{
		CDisplaySetup * CDisplaySetup::internalInstance = nullptr;

		void CDisplaySetup::Deleter()
		{
			if (internalInstance)
			{
				delete internalInstance;
			}
		}

		const CDisplaySetup::DisplayData & CDisplaySetup::displayFromPoint(std::pair<int, int> pos) const
		{
			juce::Point<int> point(pos.first, pos.second);
			for (auto & display : displays)
			{
				if (display.bounds.contains(point))
				{
					return display;
				}
			}
			return getMainDisplay();
		}
		const CDisplaySetup::DisplayData & CDisplaySetup::displayFromIndex(std::size_t index) const
		{
			return displays.at(index);
		}
		
		
		std::vector<const CDisplaySetup::DisplayData *> CDisplaySetup::getDuplicateDisplaysFor(const CDisplaySetup::DisplayData & initialDisplay) const
		{
			
			std::vector<const DisplayData *> displayList;
			
			for(auto & display : displays)
			{
				// first line checks if they are at same position as well - ie. duplicated.
				if(&display != &initialDisplay && display.bounds == initialDisplay.bounds)
				{
					displayList.push_back(&display);
				}
			}
			return displayList;
		}
		
		const CDisplaySetup::DisplayData & CDisplaySetup::getMainDisplay() const
		{
			for (auto & display : displays)
			{
				if (display.isMainMonitor)
					return display;
			}
			return defaultDevice;
		}

		CDisplaySetup & CDisplaySetup::instance()
		{
			if (internalInstance == nullptr)
			{
				internalInstance = new CDisplaySetup();
				atexit(&CDisplaySetup::Deleter);
			}
			return *internalInstance;
		}

		CDisplaySetup::CDisplaySetup()
			: defaultFontGamma(1.2)
		{
			update();
		}

		void CDisplaySetup::update()
		{
			displays.clear();

			bool systemUsesSubpixelSmoothing = false;
			double finalGamma = defaultFontGamma;

			#ifdef __WINDOWS__
				BOOL systemSmoothing = FALSE;
				// antialiased text set?
				if (SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &systemSmoothing, 0)
					&& systemSmoothing)
				{
					UINT smoothingType = 0;
					// clear type enabled?
					if (SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &smoothingType, 0)
						&& smoothingType == FE_FONTSMOOTHINGCLEARTYPE)
					{
						UINT systemGamma = 0; // 1000 - 2200

						systemUsesSubpixelSmoothing = true;
						// collect gamma correction
						if (SystemParametersInfo(SPI_GETFONTSMOOTHINGCONTRAST, 0, &systemGamma, 0))
						{
							systemGamma = cpl::Math::confineTo<std::uint16_t>(systemGamma, 1000, 2200);
							finalGamma = systemGamma / 1000.0;
						}
					}
				}
			#endif

			DisplayData currentMonitorInfo;
			int count = 0;
			for (auto & display : juce::Desktop::getInstance().getDisplays().displays)
			{
				
				bool thisIndexUsesSubpixels = false;
				auto displayOrigin = display.userArea.getPosition();
				// default to RGB...
				LCDMatrixOrientation displayOrientation = LCDMatrixOrientation::RGB;
				#ifdef __WINDOWS__
					UINT systemMatrixOrder = 0;
					if (SystemParametersInfo(SPI_GETFONTSMOOTHINGORIENTATION, 0, &systemMatrixOrder, 0))
					{
						switch (systemMatrixOrder)
						{
							case FE_FONTSMOOTHINGORIENTATIONBGR:
								displayOrientation = LCDMatrixOrientation::BGR;
								thisIndexUsesSubpixels = true;
								break;
							case FE_FONTSMOOTHINGORIENTATIONRGB:
								displayOrientation = LCDMatrixOrientation::RGB;
								thisIndexUsesSubpixels = true;
								break;
							default:
								thisIndexUsesSubpixels = false;
						};
					}
				#endif
				#ifndef __MAC__
					if (GetScreenOrientation({ displayOrigin.getX(), displayOrigin.getY() }, rotation))
					{
						currentMonitorInfo.screenOrientation = RadsToOrientation(rotation);
						currentMonitorInfo.screenRotation = rotation;
						
					}

				#else
					// enter the realms of objective-c
					OSXExtendedScreenInfo info {0};
					if(GetExtendedScreenInfo(displayOrigin.getX(), displayOrigin.getY(), &info))
					{
						switch (info.subpixelOrientation)
						{
							case kDisplaySubPixelLayoutUndefined:
								// fall through - we default to RGB.
							case kDisplaySubPixelLayoutRGB:
								systemUsesSubpixelSmoothing = thisIndexUsesSubpixels = true;
								displayOrientation = LCDMatrixOrientation::RGB;
								break;
							case kDisplaySubPixelLayoutBGR:
								systemUsesSubpixelSmoothing = thisIndexUsesSubpixels = true;
								displayOrientation = LCDMatrixOrientation::BGR;
								break;
							default:
								thisIndexUsesSubpixels = false;
								break;
						};
						if(info.averageGamma <= 1.2)
							info.averageGamma = 1.4;
						finalGamma = info.averageGamma;
				
						//if(!info.displayIsDigital)
						//	thisIndexUsesSubpixels = false;
						
						currentMonitorInfo.screenOrientation = RadsToOrientation(info.screenRotation);
						currentMonitorInfo.screenRotation = info.screenRotation;
						
					}
				#endif
				
				currentMonitorInfo.displayMatrixOrder = displayOrientation;
				
				if (systemUsesSubpixelSmoothing &&
					(currentMonitorInfo.screenRotation == 0 || currentMonitorInfo.screenRotation == 180) &&
					thisIndexUsesSubpixels)
				{
					currentMonitorInfo.isApplicableForSubpixels = true;
				}
				else
				{
					// well, if you want to add support for vertical
					// subpixel rendering (useless) do it here.
					currentMonitorInfo.isApplicableForSubpixels = false;
				}
				currentMonitorInfo.bounds = display.totalArea;
				currentMonitorInfo.dpi = display.dpi;
				currentMonitorInfo.scale = display.scale;
				currentMonitorInfo.fontGamma = finalGamma;
				currentMonitorInfo.gammaScale.setGamma(finalGamma);
				currentMonitorInfo.isMainMonitor = display.isMain;
				currentMonitorInfo.index = count++;
				displays.push_back(currentMonitorInfo);
				
			}
			
			// set what displays has duplicates and check compability.
			for(auto & display : displays)
			{
				display.isDuplicatesCompatible = true;
				auto duplicates = getDuplicateDisplaysFor(display);
				if(duplicates.size())
				{
					display.isDisplayDuplicated = true;
					display.duplicates = duplicates;
					for(auto & duplicate : duplicates)
					{
						if(!display.isRenderingCompatibleTo(*duplicate))
						{
							display.isDuplicatesCompatible = false;
							break;
						}
					}
				}
			}
		}


	}; // {} rendering
}; // {} cpl