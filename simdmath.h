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

	file:simdmath.h

		Standard math library routines like trigonometry and stuff.
		Most is derived from the old cephes library.


*************************************************************************************/

#ifndef _SIMDMATH_H
	#define _SIMDMATH_H
	
	#include "Types.h"
	//#include "intrintypes.h"

	namespace cpl
	{
		namespace simd
		{
			/*
			template<typename V>
			struct constants
			{
				typedef typename scalar_of<V>::type Scalar;

				static const V pi = { Scalar(M_PI) };
				static const V e = { Scalar(M_E) };
				static const V tau = { Scalar(M_PI * 2) };
				static const V pi_half = { Scalar(M_PI / 2) };
				static const V pi_quarter = { Scalar(M_PI / 4) };
				static const V one = { Scalar(1) };
				static const V half = { Scalar(0.5) };
				static const V two = { Scalar(2) };
				static const V sqrt_two = { Scalar(1.4142135623730950488016887242097) };
				static const V sqrt_half = { Scalar(0.70710678118654752440084436210485) };
				static const V sign_bit = { Scalar(-0.0) };
			};
			*/
		}
	}; // APE
#endif