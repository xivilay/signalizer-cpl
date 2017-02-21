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
		
		A Linkwitz-Riley network splits a signal into N bands with variable slope that,
		when summed, yields a flat frequency response. It is thus an all-pass filter.

*************************************************************************************/

#ifndef CPL_LINKWITZRILEYNETWORK_H
	#define CPL_LINKWITZRILEYNETWORK_H
	
	#include <array>
	#include "filters/AC_SVF.h"
	#include "filters/Allpass.h"

	namespace cpl
	{
		namespace dsp
		{

			template<typename Scalar, std::size_t NumBands, std::size_t FilterOrder = 1>
				class LinkwitzRileyNetwork
				{
				public:
					typedef Scalar ScalarTy;

					static_assert(NumBands > 2, "Can't have a crossover with less than two bands!");
					static_assert(FilterOrder == 1, "Only 12 dB (1) filter order supported at the moment.");

				private:
					template<std::size_t C>
					struct Sum
					{
						static const std::size_t result = C + Sum<C - 1>::value;
					};

					template<>
					struct Sum<1>
					{
						static const std::size_t value = 1;
					};

					static std::size_t sum(std::size_t z)
					{
						if (z == 1)
							return 1;
						return z + sum(z - 1);
					}

				public:
					static const std::size_t Order = FilterOrder;
					static const std::size_t Bands = NumBands;
					static const std::size_t Filters = Order * (1 + Bands / 2);
					static const std::size_t CrossOvers = Bands - 1;
					static const std::size_t AllpassSections = CrossOvers - 1;
					static const std::size_t AllpassFilters = Sum<AllpassSections>::value;

					typedef std::array<Scalar, Bands> BandArray;

					typedef filters::Allpass<Scalar> Allpass;
					typedef filters::StateVariableFilter<Scalar> Filter;

					void setup(std::array<Scalar, Bands - 1> crossoverFrequenciesNormalized)
					{
						for (std::size_t i = 0; i < crossoverFrequenciesNormalized.size(); ++i)
						{
							coeffs[i] = Filter::Coefficients::design<filters::Response::Lowpass>(crossoverFrequenciesNormalized[i], 0.5, 1);
						}

						for (std::size_t i = 0; i < AllpassSections; ++i)
						{
							apCoeffs[i] = Allpass::Coefficients::design(crossoverFrequenciesNormalized[i + AllpassSections], 0.5, 1);
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

						for (std::size_t a = 0, offset = 0; a < AllpassSections; ++a)
						{
							auto filters = sum(AllpassSections - a);

							for (std::size_t f = 0; f < filters; ++f)
							{
								ret[a] = allpasses[offset + f].filter(ret[a], apCoeffs[a]);
							}

							offset += filters;
						}

						return ret;
					}


				private:

					typename Filter::Coefficients coeffs[Filters];
					Filter filters[Filters];
					typename Allpass::Coefficients apCoeffs[AllpassSections];
					Allpass allpasses[AllpassFilters];
				};
		};
	};

#endif
