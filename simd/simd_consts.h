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


#ifndef _SIMD_CONSTS_H
	#define _SIMD_CONSTS_H
	#include "../Mathext.h"
	#include <float.h>
	#include "simd_traits.h"

	namespace cpl
	{
		namespace simd
		{
			
			#define SQRT_TWO 1.4142135623730950488016887242097
			#define SQRT_HALF_TWO 0.70710678118654752440084436210485

			#define FILL_8(val, ty) {static_cast<ty>(val), static_cast<ty>(val), static_cast<ty>(val), static_cast<ty>(val), \
									static_cast<ty>(val), static_cast<ty>(val), static_cast<ty>(val), static_cast<ty>(val)}
			#define FILL_4(val, ty) {static_cast<ty>(val), static_cast<ty>(val), static_cast<ty>(val), static_cast<ty>(val)}
			#define FILL_2(val, ty) {static_cast<ty>(val), static_cast<ty>(val)}


			#define DECLARE_SIMD_CONSTS(name, value) \
				template<> const v4sf consts<v4sf>::name = FILL_4(value, float); \
				template<> const v8sf consts<v8sf>::name = FILL_8(value, float); \
				template<> const v2sd consts<v2sd>::name = FILL_2(value, double); \
				template<> const v4sd consts<v4sd>::name = FILL_4(value, double); \
				template<> const v128si consts<v128si>::name = FILL_4(value, std::int32_t);

			template<typename V>
				struct consts
				{
					typedef typename scalar_of<V>::type VTy;

					// standard constants
					static const V pi;
					static const V e;
					static const V tau;
					static const V pi_half;
					static const V pi_quarter;
					static const V four_over_pi;
					static const V one;
					static const V minus_one;
					static const V minus_two;
					static const V half;
					static const V quarter;
					static const V two;
					static const V four;
					static const V sqrt_two;
					static const V sqrt_half_two;
					static const V sign_bit;
					static const V sign_mask;
					static const V epsilon;
					static const V max;
					static const V min;
					static const V zero;
					static const V all_bits;

					// cephes specific magic constants
					static const V cephes_e__4;
					static const V cephes_small;
					static const V cephes_2414;
					static const V cephes_0414;
					static const V cephes_8053;
					static const V cephes_1387;
					static const V cephes_1997;
					static const V cephes_3333;
					// for extended precision modular passes.
					static const V cephes_mdp1;
					static const V cephes_mdp2;
					static const V cephes_mdp3;
					// sine / cosine calculation coefficients
					static const V cephes_sin_p0;
					static const V cephes_sin_p1;
					static const V cephes_sin_p2;
					static const V cephes_cos_p0;
					static const V cephes_cos_p1;
					static const V cephes_cos_p2;
				};


			struct bit_helper
			{
				static const std::uint64_t bits = 0xFFFFFFFFFFFFFFFFULL;
				// 32-bit IEEE floating point sign mask
				static const std::uint32_t fsm = 0x7FFFFFFFU;
				// 64-bit IEEE floating point sign mask
				static const std::uint64_t dsm = 0x7FFFFFFFFFFFFFFFULL;

			};

			// standards
			DECLARE_SIMD_CONSTS(pi, M_PI);
			DECLARE_SIMD_CONSTS(e, M_E);
			DECLARE_SIMD_CONSTS(tau, M_PI * 2);
			DECLARE_SIMD_CONSTS(pi_half, M_PI / 2);
			DECLARE_SIMD_CONSTS(pi_quarter, M_PI / 4);
			DECLARE_SIMD_CONSTS(four_over_pi, 4 / M_PI);
			DECLARE_SIMD_CONSTS(zero, 0.0);
			DECLARE_SIMD_CONSTS(two, 2.0);
			DECLARE_SIMD_CONSTS(four, 4.0);
			DECLARE_SIMD_CONSTS(one, 1.0);
			DECLARE_SIMD_CONSTS(minus_one, -1.0);
			DECLARE_SIMD_CONSTS(minus_two, -1.0);
			DECLARE_SIMD_CONSTS(half, 0.5);
			DECLARE_SIMD_CONSTS(quarter, 0.125);
			DECLARE_SIMD_CONSTS(sqrt_two, SQRT_TWO);
			DECLARE_SIMD_CONSTS(sqrt_half_two, SQRT_HALF_TWO);
			DECLARE_SIMD_CONSTS(sign_bit, -0.0);

			// cephes magic numbers
			DECLARE_SIMD_CONSTS(cephes_e__4, 1.0e-4);
			DECLARE_SIMD_CONSTS(cephes_small, 1.0e-35);
			DECLARE_SIMD_CONSTS(cephes_2414, 2.414213562373095);
			DECLARE_SIMD_CONSTS(cephes_0414, 0.4142135623730950);
			DECLARE_SIMD_CONSTS(cephes_8053, 8.05374449538e-2);
			DECLARE_SIMD_CONSTS(cephes_1387, 1.38776856032E-1);
			DECLARE_SIMD_CONSTS(cephes_1997, 1.99777106478E-1);
			DECLARE_SIMD_CONSTS(cephes_3333, 3.33329491539E-1);
			// extended precision arithmetic
			DECLARE_SIMD_CONSTS(cephes_mdp1, -0.78515625);
			DECLARE_SIMD_CONSTS(cephes_mdp2, -2.4187564849853515625e-4);
			DECLARE_SIMD_CONSTS(cephes_mdp3, -3.77489497744594108e-8);
			// sine/cosine calculation coefficients
			DECLARE_SIMD_CONSTS(cephes_sin_p0, -1.9515295891E-4);
			DECLARE_SIMD_CONSTS(cephes_sin_p1, 8.3321608736E-3);
			DECLARE_SIMD_CONSTS(cephes_sin_p2, -1.6666654611E-1);
			DECLARE_SIMD_CONSTS(cephes_cos_p0, 2.443315711809948E-005);
			DECLARE_SIMD_CONSTS(cephes_cos_p1, -1.388731625493765E-003);
			DECLARE_SIMD_CONSTS(cephes_cos_p2, 4.166664568298827E-002);

			// epsilon
			template<> const v4sf consts<v4sf>::epsilon = FILL_4(FLT_EPSILON, float); 
			template<> const v8sf consts<v8sf>::epsilon = FILL_8(FLT_EPSILON, float); 
			template<> const v2sd consts<v2sd>::epsilon = FILL_2(DBL_EPSILON, double); 
			template<> const v4sd consts<v4sd>::epsilon = FILL_4(DBL_EPSILON, double);
			// min
			template<> const v4sf consts<v4sf>::min = FILL_4(FLT_MIN, float);
			template<> const v8sf consts<v8sf>::min = FILL_8(FLT_MIN, float);
			template<> const v2sd consts<v2sd>::min = FILL_2(DBL_MIN, double);
			template<> const v4sd consts<v4sd>::min = FILL_4(DBL_MIN, double);
			// min
			template<> const v4sf consts<v4sf>::max = FILL_4(FLT_MAX, float);
			template<> const v8sf consts<v8sf>::max = FILL_8(FLT_MAX, float);
			template<> const v2sd consts<v2sd>::max = FILL_2(DBL_MAX, double);
			template<> const v4sd consts<v4sd>::max = FILL_4(DBL_MAX, double);


			// all_bits - need a better solution here, that doesn't involve intrinsics.
			template<> const v4sf consts<v4sf>::all_bits = { *(float*)&bit_helper::bits, *(float*)&bit_helper::bits, 
															 *(float*)&bit_helper::bits, *(float*)&bit_helper::bits };
			template<> const v8sf consts<v8sf>::all_bits = { *(float*)&bit_helper::bits, *(float*)&bit_helper::bits,
															 *(float*)&bit_helper::bits, *(float*)&bit_helper::bits,
															 *(float*)&bit_helper::bits, *(float*)&bit_helper::bits,
															 *(float*)&bit_helper::bits, *(float*)&bit_helper::bits };
			template<> const v2sd consts<v2sd>::all_bits = { *(double*)&bit_helper::bits, *(double*)&bit_helper::bits};
			template<> const v4sd consts<v4sd>::all_bits = { *(double*)&bit_helper::bits, *(double*)&bit_helper::bits,
															 *(double*)&bit_helper::bits, *(double*)&bit_helper::bits };

			// sign mask - need a better solution here, that doesn't involve intrinsics.
			template<> const v4sf consts<v4sf>::sign_mask = { *(float*)&bit_helper::fsm, *(float*)&bit_helper::fsm,
															 *(float*)&bit_helper::fsm, *(float*)&bit_helper::fsm };
			template<> const v8sf consts<v8sf>::sign_mask = { *(float*)&bit_helper::fsm, *(float*)&bit_helper::fsm,
															 *(float*)&bit_helper::fsm, *(float*)&bit_helper::fsm,
															 *(float*)&bit_helper::fsm, *(float*)&bit_helper::fsm,
															 *(float*)&bit_helper::fsm, *(float*)&bit_helper::fsm };
			template<> const v2sd consts<v2sd>::sign_mask = { *(double*)&bit_helper::dsm, *(double*)&bit_helper::dsm };
			template<> const v4sd consts<v4sd>::sign_mask = { *(double*)&bit_helper::dsm, *(double*)&bit_helper::dsm,
															 *(double*)&bit_helper::dsm, *(double*)&bit_helper::dsm };

			#undef FILL_8
			#undef FILL_4
			#undef FILL_2
			#undef SQRT_TWO
			#undef SQRT_HALF
			#undef DECLARE_SIMD_CONSTS

		}; // simd
	}; // cpl
#endif

