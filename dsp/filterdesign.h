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

	file:filterdesign.h
	
		template functions that design filters (resonators, oscillators, actual filters)
		with specified order, samplerate etc..

*************************************************************************************/

#ifndef _FILTERDESIGN_H
	#define _FILTERDESIGN_H

	#include "../dsp.h"
	#include "../mathext.h"
	#include <complex>

	namespace cpl
	{
		namespace dsp
		{
			namespace filters
			{

				enum class type
				{
					resonator, 
					oscillator,
					butterworth
				};

				template<type f, size_t order, typename T>
					struct coefficients
					{
						typedef T Scalar;
						Scalar gain;
						std::complex<Scalar> c[order];
					};

				//template<type f, size_t order, typename T> 
				//	coefficients<f, order, T> design(T stuff);


				template<type f, size_t order, typename T>
					typename std::enable_if < f == type::oscillator, coefficients<type::resonator, 1, T>>::type
						design(T rads)
					{
						coefficients<type::oscillator, 1, T> ret;

						auto const g = std::tan(rads / 2.0);
						auto const z = 2.0 / (1.0 + g * g);

						ret.c[0] = std::complex<T>(z - 1.0, z * g);
						ret.gain = (T)1.0;
						return ret;
					}

				template<type f, size_t order, typename T>
					typename std::enable_if < f == type::resonator && order == 1,  coefficients<type::resonator, order, T>>::type
						design(T rads, T bandWidth, T QinDBs)
					{
						coefficients<type::resonator, order, T> ret;

						auto const Bq = (3 / QinDBs) * M_E / 12.0;
						auto const r = std::exp(Bq * -bandWidth / 2.0);
						auto const gain = (1.0 - r);

						ret.c[0] = std::polar(r, rads);
						ret.gain = gain;
						return ret;
					}


				
				template<type f, size_t order, typename T>
					typename std::enable_if < f == type::resonator && order == 3,  coefficients<type::resonator, order, T>>::type
						design(T rads, T bandWidth, T QinDBs)
					{
						coefficients<type::resonator, order, T> ret;

						auto const Bq = (3 / QinDBs) * M_E / 12.0;
						auto const r = std::exp(Bq * -bandWidth / 2.0);
						auto const gain = 1.0 / (1.0 - r);

						for (int z = 0; z < order; ++z)
						{
							ret.c[z] = std::polar(r, rads + Math::mapAroundZero<signed>(z, order) * Bq * bandWidth);
						}

						ret.gain = gain;
						return ret;
					}


			};
		};
	};

#endif