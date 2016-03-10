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

	file:MacSupport.h
	
		Since Objective-C and C++ doesn't always easily mix, access to the NSApi
		has been stuffed into its own source file.
 
*************************************************************************************/

#ifndef CPL_MACSUPPORT_H
	#define CPL_MACSUPPORT_H

	#include <cstdlib>

	struct OSXExtendedScreenInfo
	{
		// true if display is confirmable digital
		bool displayIsDigital;
		// gamma correction values for each channel, 1.2 .. 2.2
		double redGamma, blueGamma, greenGamma;
		// average colour gamma, 1.2 .. 2.2
		double averageGamma;
		// see AppleFontSmoothing system defaults. 0 is none, 1 .. 4 is variying degrees.
		int fontSmoothingLevel;
		// screen rotation, counterclockwise, in _degrees_
		double screenRotation;
		// can be kDisplaySubPixelLayoutRGB, kDisplaySubPixelLayoutBGR or kDisplaySubPixelLayoutUndefined
		unsigned int subpixelOrientation;
		
	};


	bool GetExtendedScreenInfo(long x, long y, OSXExtendedScreenInfo * info);

	int MacBox(void * hwndParent, const char *text, const char *caption, int type);
	std::size_t GetBundlePath(char * buf, std::size_t bufsize);

#endif