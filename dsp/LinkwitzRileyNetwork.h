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

	file:LinkwitzRileyNetwork.h
		
		A first-order lowpass filter, which resets its value to the input, if abs(it) is
		higher, otherwise slowly decays. This is the type of filter used for dB-meters
		and stuff, that has to reach max instantaneously and precisely, without phase error,
		but then slowly decay.
		It is not suited for audio.

*************************************************************************************/

#ifndef CPL_LINKWITZRILEYNETWORK_H
	#define CPL_LINKWITZRILEYNETWORK_H
	
	#include <array>
	#include "filters/AC_SVF.h"

	namespace cpl
	{
		namespace dsp
		{

			template<typename Scalar, std::size_t NumBands, std::size_t FilterOrder = 1>
				class LinkwitzRileyNetwork
				{
				public:
					typedef Scalar ScalarTy;

					static_assert(NumBands > 1, "Can't have a crossover with less than two bands!");
					static_assert(FilterOrder == 1, "Only 12 dB (1) filter order supported at the moment.");

					static const std::size_t Order = FilterOrder;
					static const std::size_t Bands = NumBands;
					static const std::size_t Filters = Order * (1 + Bands / 2);

					typedef std::array<Scalar, Bands> BandArray;

					typedef filters::StateVariableFilter<Scalar> Filter;

					void setup(std::array<Scalar, Bands - 1> crossoverFrequenciesNormalized)
					{
						for (std::size_t i = 0; i < crossoverFrequenciesNormalized.size(); ++i)
						{
							coeffs[i] = Filter::Coefficients::design<filters::Response::Lowpass>(crossoverFrequenciesNormalized[i], 0.5, 1);
						}
					}

					void reset()
					{
						for (std::size_t i = 0; i < Filters; ++i)
							filters[i].reset();
					}

					BandArray process(Scalar input) noexcept
					{
						BandArray ret;

						typedef Scalar T;

						std::size_t pos = 0;

						for (std::size_t i = 0; i < Filters; ++i)
						{
							T ic1eq = filters[i].ic1eq;
							T ic2eq = filters[i].ic2eq;

							const T v3 = input - ic2eq;
							const T v1 = coeffs[i].a1 * ic1eq + coeffs[i].a2 * v3;
							const T v2 = ic2eq + coeffs[i].a2 * ic1eq + coeffs[i].a3 * v3;
							ic1eq = 2 * v1 - ic1eq;
							ic2eq = 2 * v2 - ic2eq;


							const T lp = v2;
							const T hp = -(input + -coeffs[i].k * v1 - lp);
							//const T bp = v1;

							ret[pos++] = lp;

							if(i == Filters - 1)
							{
								ret[pos++] = hp;
							}

							filters[i].ic1eq = ic1eq;
							filters[i].ic2eq = ic2eq;

							input = hp;
						}

						return ret;
					}


				private:

					typename Filter::Coefficients coeffs[Filters];
					Filter filters[Filters];
				};
		};
	};

#endif
