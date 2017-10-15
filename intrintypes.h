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

	file:intrintypes.h

		Emulations of vector-types as classes, with template arguments being
		the instruction set and fundamental type. Thus, with nice compiler-optimizations,
		these encapsulating classes should compile down to only the raw instruction,
		so your app supports all the new fancy instructions without you having to
		rewrite/duplicate your code all the time.

*************************************************************************************/

#ifndef INTRINTYPES_H
#define INTRINTYPES_H

#include "Types.h"

namespace cpl
{
	namespace simd
	{
		enum class iset
		{
			avx,
			avx2,
			sse2

		};

		template<iset instructionSet, typename scalar>
		class var;

		template<iset instructionSet, class scalar>
		struct rank_of
		{
			static const std::size_t value = var<instructionSet, scalar>::elements;
		};


		/*
			The general example of what the class var should implement.
			I'm too lazy to implement all the architechtures right now, adding them as i need them..
			Feel free to send them to me.
		*/
		template<>
		struct var<iset::avx, float>
		{
			typedef Types::v8sf val_type;
			typedef var<iset::avx, float> type;
			typedef float scalar;
			static const std::size_t size = sizeof(val_type);
			static const std::size_t elements = sizeof(val_type) / sizeof(scalar);

			type & operator =(const type & right) { value = right.value; return *this; }
			type & operator =(val_type right) { value = right; return *this; }
			var(val_type val) : value(val) {}
			var() {};

			type operator * (type right) { return _mm256_mul_ps(value, right); }
			type operator + (type right) { return _mm256_mul_ps(value, right); }
			type operator - (type right) { return _mm256_sub_ps(value, right); }
			type operator / (type right) { return _mm256_div_ps(value, right); }
			type operator & (type right) { return _mm256_and_ps(value, right); }
			type operator | (type right) { return _mm256_or_ps(value, right); }

			type & operator *= (type right) { value = _mm256_mul_ps(value, right); return *this; }
			type & operator += (type right) { value = _mm256_mul_ps(value, right); return *this; }
			type & operator -= (type right) { value = _mm256_sub_ps(value, right); return *this; }
			type & operator /= (type right) { value = _mm256_div_ps(value, right); return *this; }
			type & operator &= (type right) { value = _mm256_and_ps(value, right); return *this; }
			type & operator |= (type right) { value = _mm256_or_ps(value, right); return *this; }

			type operator == (type right) { return _mm256_cmp_ps(value, right, _CMP_EQ_UQ); }
			type operator < (type right) { return _mm256_cmp_ps(value, right, _CMP_LT_OQ); }

			operator val_type() { return value; }
			static val_type fma(val_type a, val_type b, val_type c) { return a * b + c; }

			val_type value;
		};

		template<>
		struct var<iset::avx2, float>
		{
			typedef Types::v8sf val_type;
			typedef var<iset::avx2, float> type;
			typedef float scalar;
			static const std::size_t size = sizeof(val_type);
			static const std::size_t elements = sizeof(val_type) / sizeof(scalar);

			type & operator =(const type & right) { value = right.value; return *this; }
			type & operator =(val_type right) { value = right; return *this; }
			var(val_type val) : value(val) {}
			var() {};

			type operator * (type right) { return _mm256_mul_ps(value, right); }
			type operator + (type right) { return _mm256_mul_ps(value, right); }
			type operator - (type right) { return _mm256_sub_ps(value, right); }
			type operator / (type right) { return _mm256_div_ps(value, right); }
			type operator & (type right) { return _mm256_and_ps(value, right); }
			type operator | (type right) { return _mm256_or_ps(value, right); }

			type & operator *= (type right) { value = _mm256_mul_ps(value, right); return *this; }
			type & operator += (type right) { value = _mm256_mul_ps(value, right); return *this; }
			type & operator -= (type right) { value = _mm256_sub_ps(value, right); return *this; }
			type & operator /= (type right) { value = _mm256_div_ps(value, right); return *this; }
			type & operator &= (type right) { value = _mm256_and_ps(value, right); return *this; }
			type & operator |= (type right) { value = _mm256_or_ps(value, right); return *this; }

			type operator == (type right) { return _mm256_cmp_ps(value, right, _CMP_EQ_UQ); }
			type operator < (type right) { return _mm256_cmp_ps(value, right, _CMP_LT_OQ); }

			operator val_type() { return value; }
			static val_type fma(val_type a, val_type b, val_type c) { /*return _mm256_fmadd(a,b,c);*/ return a; }

			val_type value;
		};

		template<>
		struct var<iset::avx, double>
		{
			typedef Types::v4sd val_type;
			typedef var<iset::avx, double> type;
			typedef double scalar;
			static const std::size_t size = sizeof(val_type);
			static const std::size_t elements = sizeof(val_type) / sizeof(scalar);

			type & operator =(const type & right) { value = right.value; return *this; }
			type & operator =(val_type right) { value = right; return *this; }
			var(val_type val) : value(val) {}
			var() {};

			type operator * (type right) { return _mm256_mul_pd(value, right); }
			type operator + (type right) { return _mm256_mul_pd(value, right); }
			type operator - (type right) { return _mm256_sub_pd(value, right); }
			type operator / (type right) { return _mm256_div_pd(value, right); }
			type operator & (type right) { return _mm256_and_pd(value, right); }
			type operator | (type right) { return _mm256_or_pd(value, right); }

			type & operator *= (type right) { value = _mm256_mul_pd(value, right); return *this; }
			type & operator += (type right) { value = _mm256_mul_pd(value, right); return *this; }
			type & operator -= (type right) { value = _mm256_sub_pd(value, right); return *this; }
			type & operator /= (type right) { value = _mm256_div_pd(value, right); return *this; }
			type & operator &= (type right) { value = _mm256_and_pd(value, right); return *this; }
			type & operator |= (type right) { value = _mm256_or_pd(value, right); return *this; }

			type operator == (type right) { return _mm256_cmp_pd(value, right, _CMP_EQ_UQ); }
			type operator < (type right) { return _mm256_cmp_pd(value, right, _CMP_LT_OQ); }

			operator val_type() { return value; }
			static val_type fma(val_type a, val_type b, val_type c) { return a * b + c; }

			val_type value;
		};

	} // simd
}; // cpl
#endif