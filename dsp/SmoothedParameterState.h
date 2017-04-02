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

	file:SmoothedParameterState.h

		Provides N-order parameter smoothing (adjusted so they have same decay rate as 
		an often used equivalent one-pole).

		It is essentially a non-resonant N pole lowpass.

*************************************************************************************/

#ifndef CPL_SMOOTHEDPARAMETERSTATE_H
	#define CPL_SMOOTHEDPARAMETERSTATE_H

	#include "../Mathext.h"

	namespace cpl
	{
		namespace dsp
		{

			template<typename T, std::size_t Order>
			class SmoothedParameterState
			{
			public:

				typedef T PoleState;

				static_assert(Order > 0, "Order must be greater than one");

				template<typename Ty>
				static PoleState design(Ty ms, Ty sampleRate)
				{
					const auto a = std::sqrt((Ty)Order);
					const auto b = std::exp(-1 / ((ms / 5000) * sampleRate));

					return static_cast<T>(std::pow(b, a));
				}

				template<typename Y> Y process(PoleState pole, Y input)
				{
					state[0] = input + pole * (state[0] - input);

					for (std::size_t i = 1; i < Order; ++i)
					{
						state[i] = state[i - 1] + pole * (state[i] - state[i - 1]);
					}

					return static_cast<Y>(state[Order - 1]);
				}

				inline T getState() const noexcept { return state[Order - 1]; }

				inline void reset()
				{
					std::memset(state, 0, sizeof state);
				}

			private:

				T state[Order]{};
			};
		};
	};
#endif