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

	file:OnePoleNetwork.h
		
		A one-pole network splits a signal into N bands with 6 dB/oct falloff that, when summed,
		yields a flat frequency response. It is thus an all-pass filter.

*************************************************************************************/

#ifndef CPL_ONEPOLENETWORK_H
	#define CPL_ONEPOLENETWORK_H
	
	#include <array>
	#include "filters/OnePole.h"

	namespace cpl
	{
		namespace dsp
		{

			template<typename Scalar, std::size_t NumBands, std::size_t FilterOrder = 1>
				class OnePoleNetwork
				{
				public:
					typedef Scalar ScalarTy;

					static_assert(NumBands > 1, "Can't have a crossover with less than two bands!");
					static_assert(FilterOrder == 1, "Only 12 dB (1) filter order supported at the moment.");

					static const std::size_t Order = FilterOrder;
					static const std::size_t Bands = NumBands;
					static const std::size_t Filters = Order * (1 + Bands / 2);

					typedef std::array<Scalar, Bands> BandArray;

					typedef filters::OnePole<Scalar> Filter;

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
							T z1 = filters[i].z1;

							z1 = coeffs[i].a0 * input + coeffs[i].b1 * z1;

							const T lp = z1;
							const T hp = input - z1;
							//const T bp = v1;

							ret[pos++] = lp;

							if(i == Filters - 1)
							{
								ret[pos++] = hp;
							}

							filters[i].z1 = z1;

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
