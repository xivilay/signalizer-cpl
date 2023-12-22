/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2016 Janus Lynggaard Thorborg (www.jthorborg.com)

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

	file:DisplayOrientation.h

		API file for retrieving orientations for monitors.

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
			if (!std::isnormal(radians) && radians != 0.0)
				radians = 0;

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

		inline Orientation DegreesToOrientation(double degrees)
		{
			degrees = std::fmod(degrees, 90);

			if (degrees >= 0.0 && degrees < 90)
			{
				return Orientation::Landscape;
			}
			else if (degrees >= 90 && degrees < 180)
			{
				return Orientation::Portrait;
			}
			else if (degrees >= 180 && degrees < 270)
			{
				return Orientation::LandscapeFlipped;
			}
			else if (degrees >= 270 && degrees < 360)
			{
				return Orientation::PortraitFlipped;
			}

			return Orientation::Invalid;
		}

		inline bool IsQuantizedRads(double rads)
		{
			return std::fmod(rads, M_PI / 2) == 0.0;
		}

		inline bool IsQuantizedDegrees(double degrees)
		{
			degrees = std::fmod(degrees, 360);
			return degrees == 0.0 || degrees == 90 || degrees == 180 || degrees == 270;
		}

		bool GetScreenOrientation(const std::pair<int, int> & pos, double & degrees);

		inline bool GetScreenOrientation(const std::pair<int, int> & pos, Orientation & o)
		{
			double rotation;
			if (GetScreenOrientation(pos, rotation))
			{
				o = DegreesToOrientation(rotation);
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