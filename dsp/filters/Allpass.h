/*************************************************************************************

	cpl - cross platform library - v. 0.1.0.

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

	file:Allpass.h
		A first-order allpass filter.

*************************************************************************************/

#ifndef CPL_ALLPASS_H
#define CPL_ALLPASS_H

#include <cstdlib>
#include "FilterBasics.h"
#include "../../simd.h"

namespace cpl 
{
	namespace dsp 
	{
		namespace filters 
		{

			template<typename T>
			struct Allpass
			{
				struct Coefficients
				{
				public:
					using Consts = cpl::simd::consts<T>;

					static Coefficients design(T normalizedFrequency, T Q, T linearGain)
					{
						const T A = linearGain;
						const T g = std::tan(Consts::pi * normalizedFrequency);
						const T k = 1 / Q;
						const T a1 = 1 / (1 + g * (g + k));
						const T a2 = g * a1;
						const T a3 = g * a2;
						const T m0 = 1;
						const T m1 = -k;
						const T m2 = -1;
						return{ A, g, k, a1, a2, a3, m0, m1, m2 };
					}

					static Coefficients zero()
					{
						Coefficients ret;
						std::memset(&ret, 0, sizeof(Coefficients));
						return ret;
					}

					static Coefficients identity()
					{
						Coefficients ret;
						std::memset(&ret, 0, sizeof(Coefficients));
						ret.m0 = 1;
						return ret;
					}

					T A, g, k, a1, a2, a3, m0, m1, m2;
				};


				T filter(T input, const Coefficients & c)
				{
					const T v3 = input - ic2eq;
					const T v1 = c.a1 * ic1eq + c.a2 * v3;
					const T v2 = ic2eq + c.a2 * ic1eq + c.a3 * v3;
					ic1eq = 2 * v1 - ic1eq;
					ic2eq = 2 * v2 - ic2eq;
					return v2 - (c.m0 * input + c.m1 * v1 + c.m2 * v2);
				}

				void reset()
				{
					ic1eq = ic2eq = 0;
				}

				T ic1eq = 0, ic2eq = 0;
			};
		}
	}
};

#endif
