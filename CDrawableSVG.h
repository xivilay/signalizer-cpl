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

	file:CDrawableSVG.h

		An object parsing static SVGs, and turning them into drawable objects based on
		paths.
 
*************************************************************************************/

#ifndef CPL_CDRAWABLESVG_H
	#define CPL_CDRAWABLESVG_H

	#include "Common.h"
	#include "Misc.h"

	namespace cpl
	{
		class CDrawableSVG
		{

			bool parseSVG(const std::string & svgResourceName)
			{
				auto const path = Misc::DirectoryPath() + "/resources/" + svgResourceName;

				auto const file = juce::File(path);

				if (file.existsAsFile())
				{
					if (juce::ScopedPointer<juce::InputStream> fileStream = file.createInputStream())
					{

					}
				}

			}

		private:

			juce::Path svgPath;
		};

	}; // std
#endif