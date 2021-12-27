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

	file:simd_cast.h

		Provides static_vector_cast to and from float/ints (includes truncation and
		rounding), as well as reinterpret_vector_cast, which is a bitwise copy
		of source to destination (in reality, it is a no-op: it exists for type conversion
		in compilers).

*************************************************************************************/

#ifndef CPL_SIMD_CAST_H
#define CPL_SIMD_CAST_H

#include "../Types.h"

namespace cpl
{
	namespace simd
	{
		using namespace cpl::Types;



		//////////////////////////////////////

		/*
			v4sd vector to v256i integer - note requires avx512 for some insane reason.
		*/
		template<typename Vdest>
		CPL_SIMD_FUNC typename std::enable_if<std::is_same<Vdest, v256i>::value, v256i>::type
			static_vector_cast(v4sd Vin)
		{
			#ifndef _CPL_HAS_AVX_512
			static_assert(delayed_error<Vdest>::value, "Your compiler doesn't support AVX-512 intrinsics ('zmmintrin.h'), "
				"code not compilable.");
			return zero<v128i>();
			#else
			return _mm256_cvttpd_epi64(Vin);
			#endif
		}

		/*
			v2sd vector to v128i integer - note requires avx512 for some insane reason.
		*/
		template<typename Vdest>
		CPL_SIMD_FUNC typename std::enable_if<std::is_same<Vdest, v128i>::value, v128i>::type
			static_vector_cast(v2sd Vin)
		{
			#ifndef _CPL_HAS_AVX_512
			static_assert(delayed_error<Vdest>::value, "Your compiler doesn't support AVX-512 intrinsics ('zmmintrin.h'), "
				"code not compilable.");
			return zero<v128i>();
			#else
			return _mm_cvttpd_epi64(Vin);
			#endif
		}


		// identity static_cast
		template<typename Vdest>
		CPL_SIMD_FUNC Vdest static_vector_cast(Vdest in)
		{
			return in;
		}
		/*
			v8sf vector to v256i integer
		*/
		template<typename Vdest>
		CPL_SIMD_FUNC typename std::enable_if<std::is_same<Vdest, v256i>::value, v256i>::type
			static_vector_cast(v8sf Vin)
		{
			return _mm256_cvttps_epi32(Vin);
		}

		/*
			v4sf vector to v128i integer
		*/
		template<typename Vdest>
		CPL_SIMD_FUNC typename std::enable_if<std::is_same<Vdest, v128i>::value, v128i>::type
			static_vector_cast(v4sf Vin)
		{
			return _mm_cvttps_epi32(Vin);
		}
	
		////////////////////////////////////// reinterpret_vector_cast

		// identity reinterpret_cast
		template<typename Vto, typename Vfrom>
		CPL_SIMD_FUNC typename std::enable_if<std::is_same<Vto, Vfrom>::value, Vto>::type
			reinterpret_vector_cast(Vfrom in)
		{
			return in;
		}

		// generic reinterpret_cast
		template<typename Vto, typename Vfrom>
		CPL_SIMD_FUNC typename std::enable_if<
			sizeof(Vfrom) == sizeof(Vto) &&
			!is_simd<Vfrom>::value &&
			!is_simd<Vto>::value &&
			!std::is_same<Vto, Vfrom>::value,
			Vto>::type
			reinterpret_vector_cast(Vfrom Vin)
		{
			Vto to;
			std::memcpy(&to, &Vin, sizeof(Vto));
			return to;
		}
	
#ifndef CPL_MSVC
	
		// clang and GCC allows reinterpret_cast for compatible vector types
	
		template<typename Vto, typename Vfrom>
		CPL_SIMD_FUNC typename std::enable_if<
			sizeof(Vfrom) == sizeof(Vto) &&
			is_simd<Vfrom>::value &&
			is_simd<Vto>::value &&
			!std::is_same<Vto, Vfrom>::value,
			Vto>::type
			reinterpret_vector_cast(Vfrom Vin)
		{
			return reinterpret_cast<Vto>(Vin);
		}
	
#else
		// primary template
		template<typename Vto, typename Vfrom>
		CPL_SIMD_FUNC typename std::enable_if<!std::is_same<Vto, Vfrom>::value && is_simd<Vfrom>::value && is_simd<Vto>::value, Vto>::type
			reinterpret_vector_cast(Vfrom Vin);
	
		/*
			bitwise-cast v128i integer to v4sf vector
		*/
		template<>
		CPL_SIMD_FUNC v4sf
			reinterpret_vector_cast<v4sf>(v128i Vin)
		{
			return _mm_castsi128_ps(Vin);
		}
	
		/*
			bitwise-cast v256i integer to v8sf vector
		*/
		template<>
		CPL_SIMD_FUNC v8sf
			reinterpret_vector_cast<v8sf>(v256i Vin)
		{
			return _mm256_castsi256_ps(Vin);
		}

		/*
			bitwise-cast v128i integer to v2sd vector
		*/
		template<>
		CPL_SIMD_FUNC v2sd
			reinterpret_vector_cast<v2sd>(v128i Vin)
		{
			return _mm_castsi128_pd(Vin);
		}

		/*
			bitwise-cast v256i integer to v4sd vector
		*/
		template<>
		CPL_SIMD_FUNC v4sd
			reinterpret_vector_cast<v4sd>(v256i Vin)
		{
			return _mm256_castsi256_pd(Vin);
		}

		//////////////////////////////////////

		/*
			bitwise-cast v4sd vector to v256i integer
		*/
		template<>
		CPL_SIMD_FUNC v256i
			reinterpret_vector_cast<v256i>(v4sd Vin)
		{
			return _mm256_castpd_si256(Vin);
		}

		/*
			bitwise-cast v2sd vector to v128i integer
		*/
		template<>
		CPL_SIMD_FUNC v128i
			reinterpret_vector_cast<v128i>(v2sd Vin)
		{
			return _mm_castpd_si128(Vin);
		}


		/*
			bitwise-cast v8sf vector to v256i integer
		*/
		template<>
		CPL_SIMD_FUNC v256i
			reinterpret_vector_cast<v256i>(v8sf Vin)
		{
			return _mm256_castps_si256(Vin);
		}

		/*
			bitwise-cast v4sf vector to v128i integer
		*/
		template<>
		CPL_SIMD_FUNC v128i
			reinterpret_vector_cast<v128i>(v4sf Vin)
		{
			return _mm_castps_si128(Vin);
		}

		/*
			bitwise-cast v4sf vector to v2sd
		*/
		template<>
		CPL_SIMD_FUNC v2sd
			reinterpret_vector_cast<v2sd>(v4sf Vin)
		{
			return _mm_castps_pd(Vin);
		}

		/*
			bitwise-cast v2sd vector to v4sf
		*/
		template<>
		CPL_SIMD_FUNC v4sf
			reinterpret_vector_cast<v4sf>(v2sd Vin)
		{
			return _mm_castpd_ps(Vin);
		}

		/*
			bitwise-cast v8sf vector to v4sd
		*/
		template<>
		CPL_SIMD_FUNC v4sd
			reinterpret_vector_cast<v4sd>(v8sf Vin)
		{
			return _mm256_castps_pd(Vin);
		}
		/*
			bitwise-cast v4sd vector to v8sf
		*/
		template<>
		CPL_SIMD_FUNC v8sf
			reinterpret_vector_cast<v8sf>(v4sd Vin)
		{
			return _mm256_castpd_ps(Vin);
		}

		/*
			bitwise-cast v256i vector to v128i
		*/
		template<>
		CPL_SIMD_FUNC v128i
			reinterpret_vector_cast<v128i>(v256i Vin)
		{
			return _mm256_castsi256_si128(Vin);
		}

		/*
			bitwise-cast v256i vector to v128i
		*/
		template<>
		CPL_SIMD_FUNC v256i
			reinterpret_vector_cast<v256i>(v128i Vin)
		{
			return _mm256_castsi128_si256(Vin);
		}
	
#endif
	
		//////////////////////////////

		/*
			v128i integer to v4sf vector
		*/
		template<typename Vdest>
		CPL_SIMD_FUNC typename std::enable_if<std::is_same<Vdest, v4sf>::value, v4sf>::type
			static_vector_cast(v128i Vin)
		{
			return _mm_cvtepi32_ps(Vin);
		}

		/*
			v256i integer to v8sf vector
		*/
		template<typename Vdest>
		CPL_SIMD_FUNC typename std::enable_if<std::is_same<Vdest, v8sf>::value, v8sf>::type
			static_vector_cast(v256i Vin)
		{
			return _mm256_cvtepi32_ps(Vin);
		}

		/*
			v128i integer to v2sd vector
		*/
		template<typename Vdest>
		CPL_SIMD_FUNC typename std::enable_if<std::is_same<Vdest, v2sd>::value, v2sd>::type
			static_vector_cast(v128i Vin)
		{
			return _mm_cvtepi32_pd(Vin);
		}

		/*
			v256i integer to v4sd vector -- not available outside of avx 512.
		*/
		/*template<typename Vdest>
		CPL_SIMD_FUNC typename std::enable_if<std::is_same<Vdest, v4sd>::value, v4sd>::type
			static_vector_cast(v256i Vin)
		{
			return _mm256_cvtepi32_pd(reinterpret_vector_cast<v128i>(Vin));
		} */
		/*/////////////////////////////////////////////

			Below functions are primarily used when writing
			sse code that uses doubles in non-AVX512 mode.
			The problem is that non-AVX512 has no integer
			operations for __m256i types, so they
			are emulated through __m128i types.

		//////////////////////////////////////////////*/
		CPL_SIMD_FUNC v128i vdouble_cvt_int32(v4sd in)
		{
			return _mm256_cvttpd_epi32(in);
		}
		CPL_SIMD_FUNC v128i vdouble_cvt_int32(v2sd in)
		{
			return _mm_cvttpd_epi32(in);
		}

		template<typename V>
		V vint32_reinterpret_double(v128i);
		template<typename V>
		V vfloat_reinterpret_double(v4sf);

		// input = [0, 1, 2, 3], output = [0, 1]
		template<>
		CPL_SIMD_FUNC v2sd vint32_reinterpret_double(v128i in)
		{
			return reinterpret_vector_cast<v2sd>(_mm_shuffle_ps(
				reinterpret_vector_cast<v4sf>(in), reinterpret_vector_cast<v4sf>(in),
				_MM_SHUFFLE(1, 1, 0, 0))
				);
		}


		// input_v128i = [0, 1, 2, 3], output_m256d = [0, 1, 2, 3]
		template<>
		CPL_SIMD_FUNC v4sd vint32_reinterpret_double(v128i in)
		{
			auto const lowerWord = _mm_shuffle_ps(
				reinterpret_vector_cast<v4sf>(in), reinterpret_vector_cast<v4sf>(in),
				_MM_SHUFFLE(1, 1, 0, 0));

			auto const higherWord = _mm_shuffle_ps(
				reinterpret_vector_cast<v4sf>(in), reinterpret_vector_cast<v4sf>(in),
				_MM_SHUFFLE(3, 3, 2, 2));

			return reinterpret_vector_cast<v4sd>(_mm256_set_m128(higherWord, lowerWord));
		}
		template<>
		CPL_SIMD_FUNC v4sd vfloat_reinterpret_double(v4sf in)
		{
			return  vint32_reinterpret_double<v4sd>(reinterpret_vector_cast<v128i>(in));
		}



		template<>
		CPL_SIMD_FUNC v2sd vfloat_reinterpret_double(v4sf in)
		{
			return vint32_reinterpret_double<v2sd>(reinterpret_vector_cast<v128i>(in));
		}

		template<typename V>
		V vint32_cvt_double(v128i);

		template<>
		CPL_SIMD_FUNC v4sd vint32_cvt_double(v128i in)
		{
			return _mm256_cvtepi32_pd(in);
		}
		template<>
		CPL_SIMD_FUNC v2sd vint32_cvt_double(v128i in)
		{
			return _mm_cvtepi32_pd(in);
		}
	}; // simd
}; // cpl
#endif
