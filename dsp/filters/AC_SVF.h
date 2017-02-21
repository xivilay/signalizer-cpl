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

	file:AC_SVF.h
		Implementation of the linear trapezoidial integration state variable filter,
		as analyzed by Andrew from Cytomic:
		http://www.cytomic.com/technical-papers

*************************************************************************************/

#ifndef CPL_AC_SVF_H
#define CPL_AC_SVF_H

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
			struct StateVariableFilter
			{
				struct Coefficients
				{
				public:
					using Consts = cpl::simd::consts<T>;

					template<Response R>
					static Coefficients design(T normalizedFrequency, T Q, T linearGain);

					template<>
					static Coefficients design<Response::Lowpass>(T normalizedFrequency, T Q, T linearGain)
					{
						const T A = linearGain;
						const T g = std::tan(Consts::pi * normalizedFrequency);
						const T k = 1 / Q;
						const T a1 = 1 / (1 + g * (g + k));
						const T a2 = g * a1;
						const T a3 = g * a2;
						const T m0 = 0;
						const T m1 = 0;
						const T m2 = 1;
						return{ A, g, k, a1, a2, a3, m0, m1, m2 };
					}

					template<>
					static Coefficients design<Response::Bandpass>(T normalizedFrequency, T Q, T linearGain)
					{
						auto coeffs = design<Response::Lowpass>(normalizedFrequency, Q, linearGain);
						coeffs.m1 = 1;
						coeffs.m2 = 0;
						return coeffs;
					}

					template<>
					static Coefficients design<Response::Highpass>(T normalizedFrequency, T Q, T linearGain)
					{
						auto coeffs = design<Response::Lowpass>(normalizedFrequency, Q, linearGain);
						coeffs.m0 = 1;
						coeffs.m1 = -coeffs.k;
						coeffs.m2 = -1;
						return coeffs;
					}

					template<>
					static Coefficients design<Response::Notch>(T normalizedFrequency, T Q, T linearGain)
					{
						auto coeffs = design<Response::Lowpass>(normalizedFrequency, Q, linearGain);
						coeffs.m0 = 1;
						coeffs.m1 = -coeffs.k;
						coeffs.m2 = 0;
						return coeffs;
					}

					/* template<>
						static Coefficients design<Response::Peak>(T normalizedFrequency, T Q, T linearGain)
						{
							// missing A somewhere
							auto coeffs = design<Response::Lowpass>(normalizedFrequency, Q, linearGain);
							coeffs.m0 = 1;
							coeffs.m1 = -coeffs.k;
							coeffs.m2 = -2;
							return coeffs;
						} */

					template<>
					static Coefficients design<Response::Bell>(T normalizedFrequency, T Q, T linearGain)
					{
						const T A = linearGain;
						const T g = std::tan(Consts::pi * normalizedFrequency);
						const T k = 1 / (Q * A);
						const T a1 = 1 / (1 + g * (g + k));
						const T a2 = g * a1;
						const T a3 = g * a2;
						const T m0 = 1;
						const T m1 = k * (A * A - 1);
						const T m2 = 0;
						return{ A, g, k, a1, a2, a3, m0, m1, m2 };
					}

					template<>
					static Coefficients design<Response::Lowshelf>(T normalizedFrequency, T Q, T linearGain)
					{
						const T A = linearGain;
						const T g = std::tan(Consts::pi * normalizedFrequency) / std::sqrt(A);
						const T k = 1 / Q;
						const T a1 = 1 / (1 + g * (g + k));
						const T a2 = g * a1;
						const T a3 = g * a2;
						const T m0 = 1;
						const T m1 = k * (A - 1);
						const T m2 = A * A - 1;
						return{ A, g, k, a1, a2, a3, m0, m1, m2 };
					}

					template<>
					static Coefficients design<Response::Highshelf>(T normalizedFrequency, T Q, T linearGain)
					{
						const T A = linearGain;
						const T g = std::tan(Consts::pi * normalizedFrequency) * std::sqrt(A);
						const T k = 1 / Q;
						const T a1 = 1 / (1 + g * (g + k));
						const T a2 = g * a1;
						const T a3 = g * a2;
						const T m0 = A * A;
						const T m1 = k * (1 - A) * A;
						const T m2 = 1 - A * A;
						return{ A, g, k, a1, a2, a3, m0, m1, m2 };
					}


					static Coefficients design(Response response, T normalizedFrequency, T Q, T linearGain)
					{
						using R = Response;

						switch (response)
						{
						case R::Lowpass:	return design<R::Lowpass>(normalizedFrequency, Q, linearGain);
						case R::Bandpass:	return design<R::Bandpass>(normalizedFrequency, Q, linearGain);
						case R::Highpass:	return design<R::Highpass>(normalizedFrequency, Q, linearGain);
						case R::Notch:		return design<R::Notch>(normalizedFrequency, Q, linearGain);
							//case R::Peak:		return design<R::Peak>(normalizedFrequency, Q, linearGain);
						case R::Bell:		return design<R::Bell>(normalizedFrequency, Q, linearGain);
						case R::Lowshelf:	return design<R::Lowshelf>(normalizedFrequency, Q, linearGain);
						case R::Highshelf:	return design<R::Highshelf>(normalizedFrequency, Q, linearGain);
						default:
							return Coefficients::zero();
						}
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
					return c.m0 * input + c.m1 * v1 + c.m2 * v2;
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
