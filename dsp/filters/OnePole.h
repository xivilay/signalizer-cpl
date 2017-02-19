/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2017 Janus Lynggaard Thorborg (www.jthorborg.com)

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

	file:OnePole.h
		
		Class for designing one-pole HP/LP filters.

*************************************************************************************/

#ifndef CPL_ONEPOLE_H
	#define CPL_ONEPOLE_H
	
	#include "../../Mathext.h"
	#include "../../Utility.h"
	#include "FilterBasics.h"

	namespace cpl
	{
		namespace dsp
		{
			namespace filters
			{

				template<typename T>
				class OnePole
				{
				public:

					struct Coefficients
					{
						T a0, b1;
						static constexpr Coefficients identity() { return { 1, 0 }; }
					};

					template<Response r>
					static Coefficients design(T normalizedFrequency)
					{
						return design(r, normalizedFrequency);
					}

					static Coefficients design(Response type, T normalizedFrequency)
					{
						T fc = normalizedFrequency;
						T a0, b1;

#ifdef DEBUG
						CPL_RUNTIME_ASSERTION(type == Response::Highpass || type == Response::Lowpass);
#endif

						switch (type)
						{
						case Response::Highpass:
							b1 = static_cast<T>(-std::exp(-2.0 * M_PI * (0.5 - fc)));
							a0 = 1 + b1;
							break;

						default:
							// an okay approximation is:
							// fc -= 1
							// b1 = fc * fc * fc * fc * fc * fc
							b1 = static_cast<T>(std::exp(-2 * M_PI * fc);
							a0 = 1 - b1;
							break;
						}
						return{ a0, b1 };
					}

					T z1;
				};
			};
		};
	};

#endif
