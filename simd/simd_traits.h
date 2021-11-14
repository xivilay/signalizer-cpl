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

	file:simd_traits.h

		Traits of simd types

*************************************************************************************/

#ifndef CPL_SIMD_TRAITS_H
#define CPL_SIMD_TRAITS_H

#include "../Types.h"
#include <type_traits>

namespace cpl
{
	namespace simd
	{
		using namespace cpl::Types;
#ifdef CPL_MSVC
		template<typename V>
		struct is_simd : public std::false_type {};

		template<>
		struct is_simd<v8sf> : public std::true_type {};

		template<>
		struct is_simd<v4sd> : public std::true_type {};

		template<>
		struct is_simd<v4sf> : public std::true_type {};

		template<>
		struct is_simd<v2sd> : public std::true_type {};

		template<>
		struct is_simd<v128si> : public std::true_type {};

		template<>
		struct is_simd<v256si> : public std::true_type {};

#else
	
	template<class T>
	struct is_simd {
		static std::false_type test(...);

		template<class B>
		static auto test(B) ->
			decltype(__builtin_convertvector(std::declval<B>(), B), std::true_type{});

		static constexpr bool value = decltype(test(std::declval<T>()))::value;
	};
	
	
#endif
	
		template<typename V>
		struct only_simd : public std::false_type {};

		/*template<>
			struct is_simd<float> : public std::true_type {};

		template<>
			struct is_simd<double> : public std::true_type {};*/


		template<std::size_t rank>
		struct integer_of_bytes;

		template<>
		struct integer_of_bytes<1>
		{
			typedef std::int8_t type;
		};

		template<>
		struct integer_of_bytes<2>
		{
			typedef std::int16_t type;
		};

		template<>
		struct integer_of_bytes<4>
		{
			typedef std::int32_t type;
		};

		template<>
		struct integer_of_bytes<8>
		{
			typedef std::int64_t type;
		};



		template<typename V, std::size_t rank = 4>
		struct vector_of;

		template<>
		struct vector_of<float, 4>
		{
			typedef v4sf type;
		};

		template<>
		struct vector_of<float, 8>
		{
			typedef v8sf type;
		};

		template<>
		struct vector_of<float, 1>
		{
			typedef float type;
		};

		template<>
		struct vector_of<double, 2>
		{
			typedef v2sd type;
		};

		template<>
		struct vector_of<double, 4>
		{
			typedef v4sd type;
		};

		template<>
		struct vector_of<double, 1>
		{
			typedef double type;
		};

		template<typename V, std::size_t rank = 4>
		struct scalar_of;



		template<>
		struct scalar_of<v8sf> { typedef float type; };

		template<>
		struct scalar_of<v4sf> { typedef float type; };


		template<>
		struct scalar_of<v128si, 1> { typedef integer_of_bytes<1>::type type; };
		template<>
		struct scalar_of<v128si, 2> { typedef integer_of_bytes<2>::type type; };
		template<>
		struct scalar_of<v128si, 4> { typedef integer_of_bytes<4>::type type; };
		template<>
		struct scalar_of<v128si, 8> { typedef integer_of_bytes<8>::type type; };
		template<>

		struct scalar_of<v256si, 1> { typedef integer_of_bytes<1>::type type; };
		template<>
		struct scalar_of<v256si, 2> { typedef integer_of_bytes<2>::type type; };
		template<>
		struct scalar_of<v256si, 4> { typedef integer_of_bytes<4>::type type; };
		template<>
		struct scalar_of<v256si, 8> { typedef integer_of_bytes<8>::type type; };

		template<>
		struct scalar_of<v4sd> { typedef double type; };

		template<>
		struct scalar_of<v2sd> { typedef double type; };

		template<>
		struct scalar_of<float> { typedef float type; };

		template<>
		struct scalar_of<double> { typedef double type; };

		template<>
		struct scalar_of<std::int32_t> { typedef std::int32_t type; };

		template<>
		struct scalar_of<std::int64_t> { typedef std::int64_t type; };

		template<typename V, std::size_t rank = 4>
		struct elements_of
		{
			static const std::size_t value = sizeof(V) / sizeof(typename scalar_of<V, rank>::type);
		};


		template<typename V>
		struct to_integer;

		template<>
		struct to_integer<v8sf> { typedef v256si type; };

		template<>
		struct to_integer<v4sf> { typedef v128si type; };

		template<>
		struct to_integer<v4sd> { typedef v256si type; };

		template<>
		struct to_integer<v2sd> { typedef v128si type; };

		template<>
		struct to_integer<float> { typedef std::int32_t type; };

		template<>
		struct to_integer<double> { typedef std::int64_t type; };


		/*
			While clang and gcc has builtin operators for simd types, msvc does not.
			However, since the simd types are unions / structs on msvc, we can create
			non-member overloads of the operators.
		*/

		#ifdef CPL_MSVC
		#define AddSimdOperatorps(name, op, type, prefix) \
					inline type operator op (const type left, const type right) { return prefix ## _ ## name ## _ps(left, right); }

		#define AddSimdOperatorpd(name, op, type, prefix) \
					inline type operator op (const type left, const type right) { return prefix ## _ ## name ## _pd(left, right); }

		#define AddSimdCOperatorps(name, op, type, prefix) \
					inline type & operator op##= (type & left, const type right) { return left = prefix ## _ ## name ## _ps(left, right); }

		#define AddSimdCOperatorpd(name, op, type, prefix) \
					inline type & operator op## = (type & left, const type right) { return left = prefix ## _ ## name ## _pd(left, right); }

		#define AddSimdComparisonps(name, op, type, prefix) \
					inline type operator op (const type left, const type right) { return prefix ## _cmp_ps(left, right, name); }

		#define AddSimdComparisonpd(name, op, type, prefix) \
					inline type operator op (const type left, const type right) { return prefix ## _cmp_pd(left, right, name); }

		// unfortunately, the generic _mm_cmp_ps is an AVX instruction so for SSE we will have to use the more terse model
		#define AddSimdComparison128(name, op) \
					inline __m128 operator op (const __m128 left, const __m128 right) { return _mm_cmp ## name ## _ps(left, right);} \
					inline __m128d operator op (const __m128d left, const __m128d right) { return _mm_cmp ## name ## _pd(left, right);}



		#define AddOperatorForArchs(name, op) \
					AddSimdOperatorps(name, op, __m128, _mm); \
					AddSimdOperatorps(name, op, __m256, _mm256); \
					AddSimdCOperatorps(name, op, __m128, _mm); \
					AddSimdCOperatorps(name, op, __m256, _mm256); \
					AddSimdOperatorpd(name, op, __m128d, _mm); \
					AddSimdOperatorpd(name, op, __m256d, _mm256); \
					AddSimdCOperatorpd(name, op, __m128d, _mm); \
					AddSimdCOperatorpd(name, op, __m256d, _mm256);


		#define AddComparisonForArchs(name, op) \
					AddSimdComparisonps(name, op, __m256, _mm256); \
					AddSimdComparisonpd(name, op, __m256d, _mm256); 

		AddOperatorForArchs(add, +);
		AddOperatorForArchs(mul, *);
		AddOperatorForArchs(div, / );
		AddOperatorForArchs(sub, -);
		AddOperatorForArchs(or , | );
		AddOperatorForArchs(and, &);
		AddOperatorForArchs(xor, ^);

		AddComparisonForArchs(_CMP_LE_OQ, <= ); // less than or equal, ordered, non-signaling
		AddComparisonForArchs(_CMP_LT_OQ, < ); // less than, ordered, non-signaling
		AddComparisonForArchs(_CMP_GT_OQ, > ); // greater than, -//-
		AddComparisonForArchs(_CMP_GE_OQ, >= ); // greater than or equal, -//-
		AddComparisonForArchs(_CMP_EQ_OQ, == ); // equality, -//-

		// force SSE code generation for v4sf, v2sd
		AddSimdComparison128(le, <= );
		AddSimdComparison128(lt, < );
		AddSimdComparison128(ge, >= );
		AddSimdComparison128(gt, > );
		AddSimdComparison128(eq, == );

		#undef AddSimdOperatorps
		#undef AddSimdCOperatorps
		#undef AddSimdOperatorpd
		#undef AddSimdCOperatorpd
		#undef AddOperatorForArchs
		#undef AddComparisonForArchs
		#endif

		template<typename V>
		struct suitable_container;


	}; // simd
}; // cpl
#endif
