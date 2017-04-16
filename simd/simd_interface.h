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

	file:simd.h

		Math and operations on a unified interface for scalar and vector types, 
		as well as types encapsulating raw sse types.

*************************************************************************************/

#ifndef SIMD_INTERFACE_H
	#define SIMD_INTERFACE_H
	
	#include "simd_traits.h"
	#include "../system/SysStats.h"

	#define CPL_SIMD_FUNC CPL_VECTOR_TARGET inline

	namespace cpl
	{
		namespace simd
		{

			template<class T>
			class NotSupported;

			template<typename V>
				CPL_SIMD_FUNC V set1(const typename scalar_of<V>::type);

			template<typename V>
				CPL_SIMD_FUNC V set();

			template<typename V>
				CPL_SIMD_FUNC V load(const typename scalar_of<V>::type * );
			template<typename V>
				CPL_SIMD_FUNC V loadu(const typename scalar_of<V>::type *);

			template<typename V>
				CPL_SIMD_FUNC V zero();

			template<typename V>
				CPL_SIMD_FUNC V broadcast(const typename scalar_of<V>::type * );
			
			template<>
				CPL_SIMD_FUNC float zero()
				{
					return 0;
				}

			template<>
				CPL_SIMD_FUNC double zero()
				{
					return 0;
				}


				CPL_SIMD_FUNC float broadcast(const float * V1)
				{
					return *V1;
				}

				CPL_SIMD_FUNC double broadcast(const double * V1)
				{
					return *V1;
				}

				CPL_SIMD_FUNC float set1(const float V1)
				{
					return V1;
				}

				CPL_SIMD_FUNC double set1(const double V1)
				{
					return V1;
				}

			template<std::size_t elements, typename V>
				CPL_SIMD_FUNC typename std::enable_if<4 == elements && std::is_same<V, v128i>::value, V>::type
					viequals(V ia, V ib)
				{
					return _mm_cmpeq_epi32(ia, ib);
				}

			template<std::size_t elements, typename V>
				CPL_SIMD_FUNC typename std::enable_if<2 == elements && std::is_same<V, v128i>::value, V>::type
					viequals(V ia, V ib)
				{
					return _mm_cmpeq_epi64(ia, ib);
				}

			template<std::size_t elements, typename V>
				CPL_SIMD_FUNC typename std::enable_if<8 == elements && std::is_same<V, v256i>::value, V>::type
					viequals(V ia, V ib)
				{
					return _mm256_cmpeq_epi32(ia, ib);
				}

			template<std::size_t elements, typename V>
				CPL_SIMD_FUNC typename std::enable_if<4 == elements && std::is_same<V, v256i>::value, V>::type
					viequals(V ia, V ib)
				{
					return _mm256_cmpeq_epi64(ia, ib);
				}

			template<std::size_t elements, typename V>
				CPL_SIMD_FUNC typename std::enable_if<4 == elements && std::is_same<V, v128i>::value, V>::type
					set1(std::int32_t val)
				{
					return _mm_set1_epi32(val);
				}

			template<std::size_t elements, typename V>
				CPL_SIMD_FUNC typename std::enable_if<2 == elements && std::is_same<V, v128i>::value, V>::type
					set1(std::int64_t val)
				{
					return _mm_set1_epi64x(val);
				}

			template<std::size_t elements, typename V>
				CPL_SIMD_FUNC typename std::enable_if<8 == elements && std::is_same<V, v256i>::value, V>::type
					set1(std::int32_t val)
				{
					return _mm256_set1_epi32(val);
				}

			template<std::size_t elements, typename V>
				CPL_SIMD_FUNC typename std::enable_if<4 == elements && std::is_same<V, v256i>::value, V>::type
					set1(std::int64_t val)
				{
					return _mm256_set1_epi64x(val);
				}

			template<>
				CPL_SIMD_FUNC v128i zero()
				{
					return _mm_setzero_si128();
				}

			template<>
				CPL_SIMD_FUNC v256i zero()
				{
					return _mm256_setzero_si256();
				}

			/*template<typename V>
				CPL_SIMD_FUNC typename std::enable_if<!is_simd<V>::value, V>::type
					load(const V const * V1)
				{
					return (V)*V1;
				}

			template<typename V>
				CPL_SIMD_FUNC typename std::enable_if<!is_simd<V>::value, V>::type
					loadu(const V const* V1)
				{
					return (V)*V1;
				}*/






			template<typename V, class VectorPtrs>					
				CPL_SIMD_FUNC typename std::enable_if<std::is_same<V, v4sf>::value, v4sf>::type
					gather(VectorPtrs p)
				{
					return _mm_set_ps(*p[0], *p[1], *p[2], *p[3]);
				}



			template<typename V, class VectorPtrs>
				CPL_SIMD_FUNC typename std::enable_if<std::is_same<V, typename scalar_of<V>::type>::value, typename scalar_of<V>::type>::type
					gather(VectorPtrs p)
				{

					return *p[0];
				}

			template<typename V, class VectorPtrs>
				CPL_SIMD_FUNC typename std::enable_if<std::is_same<V, v8sf>::value, v8sf>::type
					gather(VectorPtrs p)
				{
					return _mm256_set_ps(*p[0], *p[1], *p[2], *p[3], *p[4], *p[5], *p[6], *p[7]);
				}

			template<typename V, class VectorPtrs>
				CPL_SIMD_FUNC typename std::enable_if<std::is_same<V, v4sf>::value, v4sf>::type
					setv(VectorPtrs p)
				{
					return _mm_set_ps(*p[0], *p[1], *p[2], *p[3]);
				}

			template<typename V, class VectorPtrs>
				CPL_SIMD_FUNC typename std::enable_if<std::is_same<V, v8sf>::value, v8sf>::type 
					setv(VectorPtrs p)
				{
					return _mm256_set_ps(*p[0], *p[1], *p[2], *p[3], *p[4], *p[5], *p[6], *p[7]);
				}

			/*template <std::size_t size, typename Scalar>
				typename std::enable_if<size == 4, v4sf>::type 
					gather(Scalar (&ptrs)[size])
			{
				return _mm_set_ps
				(
					*ptrs[0], 
					*ptrs[1], 
					*ptrs[2], 
					*ptrs[3]
				);
			}

			template <std::size_t size, typename Scalar>
				typename std::enable_if<size == 8, v4sf>::type
					gather(Scalar (&ptrs)[size])
				{
						return _mm256_set_ps
						(
							*ptrs[0], 
							*ptrs[1], 
							*ptrs[2], 
							*ptrs[3],
							*ptrs[4],
							*ptrs[5],
							*ptrs[6],
							*ptrs[7]
						);
				}*/

			template<>
				CPL_SIMD_FUNC v8sf zero()
				{
					return _mm256_setzero_ps();
				}

			template<>
				CPL_SIMD_FUNC v4sf zero()
				{
					return _mm_setzero_ps();
				}

			template<>
				CPL_SIMD_FUNC v2sd zero()
				{
					return _mm_setzero_pd();
				}

			template<>
				CPL_SIMD_FUNC v4sd zero()
				{
					return _mm256_setzero_pd();
				}

			template<>
				CPL_SIMD_FUNC v8sf set1(const float in)
				{
					return _mm256_set1_ps(in);
				}

			template<>
				CPL_SIMD_FUNC v4sf set1(const float in)
				{
					return _mm_set1_ps(in);
				}

			template<>
				CPL_SIMD_FUNC float set1(const float in)
				{
					return in;
				}

			template<>
				CPL_SIMD_FUNC double set1(const double in)
				{
					return in;
				}

			template<>
				CPL_SIMD_FUNC v2sd set1(const double in)
				{
					return _mm_set1_pd(in);
				}

			template<>
				CPL_SIMD_FUNC v4sd set1(const double in)
				{
					return _mm256_set1_pd(in);
				}


			template<>
				CPL_SIMD_FUNC v8sf loadu(const float *  in)
				{
					return _mm256_loadu_ps(in);
				}

			template<>
				CPL_VECTOR_TARGET CPL_SIMD_FUNC v8sf load(const float * in)
				{
					return _mm256_load_ps(in);
				}

			template<>
				CPL_SIMD_FUNC float load(const float * in)
				{
					return *in;
				}

			template<>
				CPL_SIMD_FUNC double load(const double * in)
				{
					return *in;
				}

			template<>
				CPL_SIMD_FUNC v4sd load(const double * in)
				{
					return _mm256_load_pd(in);
				}

			template<>
				CPL_SIMD_FUNC v2sd load(const double * in)
				{
					return _mm_load_pd(in);
				}

			template<>
				CPL_SIMD_FUNC double loadu(const double * in)
				{
					return *in;
				}

			template<>
				CPL_SIMD_FUNC v4sf load(const float *in)
				{
					return _mm_load_ps(in);
				}

			template<>
				CPL_SIMD_FUNC v4sf loadu(const float * in)
				{
					return _mm_loadu_ps(in);
				}

			template<>
				CPL_SIMD_FUNC float loadu(const float * in)
				{
					return *in;
				}

			// broadcasts has to be declared after set1<>'s because the broadcasts may explicitly initialize the templates

			template<>
				CPL_SIMD_FUNC float broadcast(const float * _in)
				{
					return *_in;
				}

			template<>
				CPL_SIMD_FUNC double broadcast(const double * _in)
				{
					return *_in;
				}

			template<unsigned i>
				CPL_SIMD_FUNC __m256 broadcast(__m256 v)
				{
					return _mm256_permute_ps(_mm256_permute2f128_ps(v, v, (i >> 2) | ((i >> 2) << 4)), _MM_SHUFFLE(i & 3, i & 3, i & 3, i & 3));
				}
	
			template<unsigned i>
				CPL_SIMD_FUNC __m128 broadcast(__m128 v)
				{
					return _mm_shuffle_ps(v, v, _MM_SHUFFLE(i, i, i, i));
				}
			
			template<>
				CPL_VECTOR_TARGET CPL_SIMD_FUNC v8sf broadcast(const float * _in)
				{
					return _mm256_broadcast_ss(_in);
				}					

			template<>
				CPL_SIMD_FUNC v4sd broadcast(const double * _in)
				{
					return _mm256_set1_pd(*_in);
				}

			template<>
				CPL_SIMD_FUNC v4sf broadcast(const float * _in)
				{
					return set1<v4sf>(*_in);
				}

			template<>
				CPL_SIMD_FUNC v2sd broadcast(const double * _in)
				{
					return _mm_set1_pd(*_in);
				}

			// ps
			CPL_SIMD_FUNC void storeu(float * in, const v8sf ymm)
			{
				_mm256_storeu_ps(in, ymm);
			}

            CPL_VECTOR_TARGET CPL_SIMD_FUNC void store(float * in, const v8sf ymm)
			{
				_mm256_store_ps(in, ymm);
			}
			
			CPL_SIMD_FUNC void storeu(float * in, const v4sf xmm)
			{
				_mm_storeu_ps(in, xmm);
			}

			CPL_SIMD_FUNC void store(float * in, const v4sf xmm)
			{
				_mm_store_ps(in, xmm);
			}

			// pd
			CPL_SIMD_FUNC void storeu(double * in, const v4sd ymm)
			{
				_mm256_storeu_pd(in, ymm);
			}

			CPL_SIMD_FUNC void store(double * in, const v4sd ymm)
			{
				_mm256_store_pd(in, ymm);
			}
			
			CPL_SIMD_FUNC void storeu(double * in, const v2sd xmm)
			{
				_mm_storeu_pd(in, xmm);
			}

			CPL_SIMD_FUNC void store(double * in, const v2sd xmm)
			{
				_mm_store_pd(in, xmm);
			}

			CPL_SIMD_FUNC void storeu(float * in, float out)
			{
				*in = out;
			}

			CPL_SIMD_FUNC void storeu(double * in, double out)
			{
				*in = out;
			}

			CPL_SIMD_FUNC void store(float * in, float out)
			{
				*in = out;
			}

			CPL_SIMD_FUNC void store(double * in, double out)
			{
				*in = out;
			}
#ifndef CPL_MSVC
			#define _mm256_set_m128i(hi, lo) (_mm256_inserti128_si256(_mm256_castsi128_si256(hi), lo, 1))
			#define _mm256_set_m128(hi, lo) (_mm256_insertf128_ps(_mm256_castps128_ps256(hi), lo, 1))
#endif
			CPL_SIMD_FUNC v128i viget_low_part(v256i ia)
			{
				return _mm256_extractf128_si256(ia, 0);
			}

			CPL_SIMD_FUNC v128i viget_high_part(v256i ia)
			{
				return _mm256_extractf128_si256(ia, 1);
			}

			CPL_SIMD_FUNC v256i vicompose(v128i ia, v128i ib)
			{
				return _mm256_set_m128i(ib, ia);
			}
			
			
			// alignment-properties must be number-literals STILL in msvc. Grrr
			#if defined(CPL_MSVC) && _MSC_VER < 1900
				#pragma message cwarn( "fix alignment of this type.")
				#define simd_alignment_of(V) __declspec(align(32))
			#else
				#define simd_alignment_of(V) CPL_ALIGNAS(sizeof(V)) /* hack */
			#endif

			template<typename V>
				struct suitable_container;


			template<typename V>
				struct simd_alignment_of(V) suitable_container
				{
					typedef typename scalar_of<V>::type Ty;
					typedef V emulated_ty;
				public:
					
					#ifndef CPL_MSVC
						Ty & operator [] (unsigned idx) { return c[idx]; }
					#else
						operator Ty* () { return c; }
					#endif
					Ty * data() { return c; }

					Ty * begin() { return c; }
					Ty * end() { return c + size; }

					static const size_t size = elements_of<V>::value;

					operator emulated_ty const () { return toType(); }
					emulated_ty toType() const { return load<emulated_ty>(c); }
					suitable_container & operator = (emulated_ty right) 
					{ 
						store(c, right); 
						return *this;
					}
					
					suitable_container() {};
					suitable_container(emulated_ty right)
					{
						store(c, right);
					}

				private:
					
					Ty c[size];

				};
			#undef simd_alignment_of
			template<typename V>
			typename std::enable_if<is_simd<V>::value, std::ostream &>::type
				operator << (std::ostream & o, V v1)
				{
					suitable_container<V> vec;
					store(vec, v1);
					o << "(" << vec[0];
					for (int i = 1; i < suitable_container<V>::size; ++i)
					{
						o << ", " << vec[i];
					}
					return o << ")";
				}
			template<typename Scalar>
				CPL_SIMD_FUNC typename std::enable_if<std::is_floating_point<Scalar>::value, std::size_t>::type 
					max_vector_capacity()
				{
					const auto factor = 8 / sizeof(Scalar);
					using namespace cpl::system;
					if (CProcessor::test(CProcessor::AVX))
					{
						return factor * 4;
					}
					else if (CProcessor::test(CProcessor::SSE2))
					{
						return factor * 2;
					}

					return 1;
				}

		}; // simd
	}; // cpl


#endif