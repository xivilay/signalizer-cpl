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

	file:FilterBasics.h
		Common basics for designing filters.

*************************************************************************************/

#ifndef CPL_FILTERBASICS_H
	#define CPL_FILTERBASICS_H

	#include <cstdlib>
	#include "../../simd.h"
	#include "../../Misc.h"

	namespace cpl
	{
		namespace dsp
		{
			namespace filters
			{

				/// Filter concept:
				/// template<typename T>
				/// class Filter
				/// {
				///		struct Coefficients
				///		{
				/// 		static Coefficients design(Response, T normalizedFrequency, T Q, T linearGain);
				///			static Coefficients design<Response>(T normalizedFrequency, T Q, T linearGain);
				///			static Coefficients zero();
				///			static Coefficients identity();
				///		};
				///
				///		void reset();
				///		T process(T, const Coeffcients &);
				/// }

				/// <summary>
				/// Common biquadratic responses as implemented by RBJ's cookbook
				/// </summary>
				enum class Response
				{
					Lowpass,
					Bandpass,
					Highpass,
					Notch,
					Peak,
					Bell = Peak,
					Lowshelf,
					Highshelf,
					end
				};

				enum class Type
				{
					SVF,
					Biquad,
					end
				};

				static const char * Responses[] = {
					"Lowpass",
					"Bandpass",
					"Highpass",
					"Notch",
					"Peak",
					/* "Bell", */
					"Low shelf",
					"High shelf"
				};

				static const char * Types[] = {
					"SVF",
					"Biquad"
				};

				inline const char * StringToType(Type r)
				{
					return Types[cpl::enum_cast<std::size_t>(r)];
				}

				inline std::vector<std::string> VectorTypes()
				{
					std::vector<std::string> ret(std::extent<decltype(Types)>::value);
					cpl::foreach_uenum<Type>([&](auto t) {
						ret[t] = Types[t];
					});

					return ret;
				}

				inline std::vector<std::string> VectorResponses()
				{
					std::vector<std::string> ret(std::extent<decltype(Responses)>::value);
					cpl::foreach_uenum<Response>([&](auto r) {
						ret[r] = Responses[r];
					});
					return ret;
				}

				inline Type TypeToString(const char * s)
				{
					for (std::size_t i = 0; i < std::extent<decltype(Types)>::value; ++i)
					{
						if (s == Types[i])
							return cpl::enum_cast<Type>(i);
					}
					return Type::SVF;
				}

				inline const char * StringToResponse(Response r)
				{
					return Responses[cpl::enum_cast<std::size_t>(r)];
				}

				inline Response ResponseToString(const char * s)
				{
					for (std::size_t i = 0; i < std::extent<decltype(Responses)>::value; ++i)
					{
						if (s == Responses[i])
							return cpl::enum_cast<Response>(i);
					}
					return Response::Lowpass;
				}

				template<typename T>
				struct SecondOrderPrototype
				{
					Response response;
					T normalizedFrequency, Q, linearGain;
				};

			};
		};
	};


#endif
