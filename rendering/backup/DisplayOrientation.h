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
 
	file:SubpixelRendering.h
 
		Utilities and types needed for rendering subpixel graphics.

 *************************************************************************************/

#ifndef _DISPLAYORIENTATION_H
	#define _DISPLAYORIENTATION_H

	#include <cmath>

	namespace cpl
	{
		namespace rendering
		{
			enum Orientation
			{
				Invalid, 
				Landscape,
				Portrait,
				LandscapeFlipped,
				PortraitFlipped
			};

			inline Orientation RadsToOrientation(double radians)
			{
				radians = std::fmod(radians, 2 * M_PI);

				if (radians >= 0.f && radians < M_PI / 2)
				{
					return Orientation::Landscape;
				}
				else if (radians >= M_PI / 2 && radians < M_PI)
				{
					return Orientation::Portrait;
				}
				else if (radians >= M_PI && radians < M_PI * 1.5)
				{
					return Orientation::LandscapeFlipped;
				}
				else if (radians >= M_PI * 1.5 && radians < M_PI * 2)
				{
					return Orientation::PortraitFlipped;
				}

				return Orientation::Invalid;
			}

			inline bool IsQuantized(double rotation)
			{
				rotation = std::fmod(rotation, 2 * M_PI);
				return rotation == 0.0 || rotation == M_PI / 2 || rotation == M_PI || rotation == M_PI * 1.5;
			}

			bool GetScreenOrientation(const std::pair<int, int> & pos, double & o);

			inline bool GetScreenOrientation(const std::pair<int, int> & pos, Orientation & o)
			{
				double rotation;
				if (GetScreenOrientation(pos, rotation))
				{
					o = RadsToOrientation(rotation);
					return true;
				}
				else
				{
					o = Orientation::Invalid;
					return false;
				}
			}

		}; // {} rendering
	}; // {} cpl
#endif