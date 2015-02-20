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

	file:dsp.h
	
		Utility, roundings, scalings, extensions of standard math library.

*************************************************************************************/

#ifndef _RESAMPLING_H
	#define _RESAMPLING_H

	#include "Mathext.h"
	#include <complex>
	#include <cstdint>
	#include "Utility.h"


	namespace cpl
	{

		/*rem - QDSS Windowed Sinc ReSampling subroutine in Basic
		rem
		rem function parameters
		rem : x      = new sample point location (relative to old indexes)
		rem            (e.g. every other integer for 0.5x decimation)
		rem : indat  = original data array
		rem : alim   = size of data array
		rem : fmax   = low pass filter cutoff frequency
		rem : fsr    = sample rate
		rem : wnwdth = width of windowed Sinc used as the low pass filter
		rem
		rem resamp() returns a filtered new sample point*/

/*sub resamp(x, indat, alim, fmax, fsr, wnwdth)
  local i,j, r_g,r_w,r_a,r_snc,r_y	: rem some local variables
  r_g = 2 * fmax / fsr           : rem Calc gain correction factor
  r_y = 0
  for i = -wnwdth/2 to (wnwdth/2)-1 : rem For 1 window width
    j       = int(x + i)          : rem Calc input sample index
        : rem calculate von Hann Window. Scale and calculate Sinc
    r_w     = 0.5 - 0.5 * cos(2*pi*(0.5 + (j - x)/wnwdth))
    r_a     = 2*pi*(j - x)*fmax/fsr
    r_snc   = 1  : if (r_a <> 0) then r_snc = sin(r_a)/r_a
    if (j >= 0) and (j < alim) then
      r_y   = r_y + r_g * r_w * r_snc * indat(j)
    endif
  next i
  resamp = r_y                  : rem Return new filtered sample
end sub*/

		template<typename Scalar>
			// cardinal, unnormalized sine to the angle
			inline Scalar sinc(Scalar angle)
			{
				return angle == 0 ? 1 : sin(angle) / angle;
			}

		template<typename Scalar, class Vector>
			inline Scalar resample(const Vector & data, const int size, int windowSize, const Scalar x, Scalar fmax, Scalar sampleRate)
			{
				Scalar acc = 0;
				Scalar cutoff = fmax / sampleRate;
				Scalar gainCorrection = 2 * cutoff;
				Scalar angle;
				int end = std::max<int>(2, std::min<int>(size, windowSize / 2));
				int min = std::min<int>(-windowSize / 2, 0);
		
				int start = std::min<int>(min, end);
				// even though we cap start further below, the window size remains the same effectively.
				// the effect of chopping the first couple of samples is the same as zero padding
				// the input backwards.
				windowSize = end - start;
				if (x + start < 0)
					start -= x + start;

				if (x + end >= size)
					end += size - (x + end);

				for (Types::fsint_t i = start; i < end; ++i)
				{
					int j = static_cast<int>(floor(x + i));
					Scalar offset = j - x;
					Scalar windowAngle = 2 * M_PI * (0.5 + offset / windowSize);
					Scalar window = 0.5 - 0.5 * cos(windowAngle);
					angle = 2 * PI * offset * cutoff;
					acc += gainCorrection * window * sinc(angle) * data[j];
					

				}
				return acc;
			}
	}; // cpl
#endif