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
	#error "Update displayorientations to your current platform!"
#endif


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

			LCDMatrixOrientation matrixType;
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

			for (auto & display : juce::Desktop::getInstance().getDisplays().displays)
			{
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
								break;
							case FE_FONTSMOOTHINGORIENTATIONRGB:
								displayOrientation = LCDMatrixOrientation::RGB;
								break;
						};
					}
				#endif

				auto displayOrigin = display.totalArea.getPosition();
				double rotation;
				if (GetScreenOrientation({ displayOrigin.getX(), displayOrigin.getY() }, rotation))
				{
					currentMonitorInfo.screenOrientation = RadsToOrientation(rotation);
					currentMonitorInfo.screenRotation = rotation;

				}
				currentMonitorInfo.displayMatrixOrder = displayOrientation;
				if (systemUsesSubpixelSmoothing &&
					currentMonitorInfo.screenRotation == 0 ||
					currentMonitorInfo.screenRotation == M_PI)
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
				currentMonitorInfo.isMainMonitor = display.isMain;

				displays.push_back(currentMonitorInfo);

			}

		}


	}; // {} rendering
}; // {} cpl