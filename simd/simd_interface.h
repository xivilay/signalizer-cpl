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

	file:simd.h

		Math and operations on vector types, as well as types encapsulating
		raw sse types.

*************************************************************************************/

#ifndef _SIMD_INTERFACE_H
	#define _SIMD_INTERFACE_H
	
	#include "simd_traits.h"

	namespace cpl
	{
		namespace simd
		{

			template<class T>
			class NotSupported;

			template<typename V>
				inline V set1(const typename scalar_of<V>::type);

			template<typename V>
				inline V set();

			template<typename V>
				inline V load(const typename scalar_of<V>::type * );
			template<typename V>
				inline V loadu(const typename scalar_of<V>::type *);

			template<typename V>
				inline V zero();

			template<typename V>
				inline V broadcast(const typename scalar_of<V>::type * );
			
				inline float zero()
				{
					return 0;
				}

				inline float load(const float * V1)
				{
					return *V1;
				}

				inline double load(const double * V1)
				{
					return *V1;
				}

				inline float broadcast(const float * V1)
				{
					return *V1;
				}

				inline double broadcast(const double * V1)
				{
					return *V1;
				}

				inline float set1(const float V1)
				{
					return V1;
				}

				inline double set1(const double V1)
				{
					return V1;
				}

			template<std::size_t elements, typename V>
				inline typename std::enable_if<4 == elements && std::is_same<V, v128i>::value, V>::type
					viequals(V ia, V ib)
				{
					return _mm_cmpeq_epi32(ia, ib);
				}

			template<std::size_t elements, typename V>
				inline typename std::enable_if<2 == elements && std::is_same<V, v128i>::value, V>::type
					viequals(V ia, V ib)
				{
					return _mm_cmpeq_epi64(ia, ib);
				}

			template<std::size_t elements, typename V>
				inline typename std::enable_if<8 == elements && std::is_same<V, v256i>::value, V>::type
					viequals(V ia, V ib)
				{
					return _mm256_cmpeq_epi32(ia, ib);
				}

			template<std::size_t elements, typename V>
				inline typename std::enable_if<4 == elements && std::is_same<V, v256i>::value, V>::type
					viequals(V ia, V ib)
				{
					return _mm256_cmpeq_epi64(ia, ib);
				}

			template<std::size_t elements, typename V>
				inline typename std::enable_if<4 == elements && std::is_same<V, v128i>::value, V>::type
					set1(std::int32_t val)
				{
					return _mm_set1_epi32(val);
				}

			template<std::size_t elements, typename V>
				inline typename std::enable_if<2 == elements && std::is_same<V, v128i>::value, V>::type
					set1(std::int64_t val)
				{
					return _mm_set1_epi64x(val);
				}

			template<std::size_t elements, typename V>
				inline typename std::enable_if<8 == elements && std::is_same<V, v256i>::value, V>::type
					set1(std::int32_t val)
				{
					return _mm256_set1_epi32(val);
				}

			template<std::size_t elements, typename V>
				inline typename std::enable_if<4 == elements && std::is_same<V, v256i>::value, V>::type
					set1(std::int64_t val)
				{
					return _mm256_set1_epi64x(val);
				}

			template<>
				inline v128i zero()
				{
					return _mm_setzero_si128();
				}

			template<>
				inline v256i zero()
				{
					return _mm256_setzero_si256();
				}

			/*template<typename V>
				inline typename std::enable_if<!is_simd<V>::value, V>::type
					load(const V const * V1)
				{
					return (V)*V1;
				}

			template<typename V>
				inline typename std::enable_if<!is_simd<V>::value, V>::type
					loadu(const V const* V1)
				{
					return (V)*V1;
				}*/


			template<unsigned i>
				inline __m256 broadcast(__m256 v)
				{
					return _mm256_permute_ps(_mm256_permute2f128_ps(v, v, (i >> 2) | ((i >> 2) << 4)), _MM_SHUFFLE(i & 3, i & 3, i & 3, i & 3));
				}
	
			template<unsigned i>
				inline __m128 broadcast(__m128 v)
				{
					return _mm_shuffle_ps(v, v, _MM_SHUFFLE(i, i, i, i));
				}
			
			template<>
				inline v8sf broadcast(const float * _in)
				{
					return _mm256_broadcast_ss(_in);
				}					

			template<>
				inline v4sd broadcast(const double * _in)
				{
					return _mm256_set1_pd(*_in);
				}

			template<typename V, class VectorPtrs>					
				inline typename std::enable_if<std::is_same<V, v4sf>::value, v4sf>::type
					gather(VectorPtrs p)
				{
					return _mm_set_ps(*p[0], *p[1], *p[2], *p[3]);
				}

			template<>
				inline v2sd broadcast(const double * _in)
				{
					return _mm_set1_pd(*_in);
				}

			template<typename V, class VectorPtrs>
				inline typename std::enable_if<std::is_same<V, typename scalar_of<V>::type>::value, typename scalar_of<V>::type>::type
					gather(VectorPtrs p)
				{

					return *p[0];
				}

			template<typename V, class VectorPtrs>
				inline typename std::enable_if<std::is_same<V, v8sf>::value, v8sf>::type
					gather(VectorPtrs p)
				{
					return _mm256_set_ps(*p[0], *p[1], *p[2], *p[3], *p[4], *p[5], *p[6], *p[7]);
				}

			template<typename V, class VectorPtrs>
				inline typename std::enable_if<std::is_same<V, v4sf>::value, v4sf>::type
					setv(VectorPtrs p)
				{
					return _mm_set_ps(*p[0], *p[1], *p[2], *p[3]);
				}

			template<typename V, class VectorPtrs>
				inline typename std::enable_if<std::is_same<V, v8sf>::value, v8sf>::type 
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
				inline v8sf zero()
				{
					return _mm256_setzero_ps();
				}

			template<>
				inline v4sf zero()
				{
					return _mm_setzero_ps();
				}

			template<>
				inline v2sd zero()
				{
					return _mm_setzero_pd();
				}

			template<>
				inline v4sd zero()
				{
					return _mm256_setzero_pd();
				}

			template<>
				inline v8sf set1(const float in)
				{
					return _mm256_set1_ps(in);
				}

			template<>
				inline v4sf set1(const float in)
				{
					return _mm_set1_ps(in);
				}

			template<>
				inline v2sd set1(const double in)
				{
					return _mm_set1_pd(in);
				}

			template<>
				inline v4sd set1(const double in)
				{
					return _mm256_set1_pd(in);
				}

			template<>
				inline v4sf broadcast(const float * _in)
				{
					return set1<v4sf>(*_in);
				}
			
			template<>
				inline v8sf loadu(const float *  in)
				{
					return _mm256_loadu_ps(in);
				}

			template<>
				inline v8sf load(const float * in)
				{
					return _mm256_load_ps(in);
				}

			template<>
				inline v4sd load(const double * in)
				{
					return _mm256_load_pd(in);
				}

			template<>
				inline v2sd load(const double * in)
				{
					return _mm_load_pd(in);
				}
			template<>
				inline v4sf load(const float *in)
				{
					return _mm_load_ps(in);
				}


			template<>
				inline v4sf loadu(const float * in)
				{
					return _mm_loadu_ps(in);
				}

			// ps
			inline void storeu(float * in, const v8sf ymm)
			{
				_mm256_store_ps(in, ymm);
			}

			inline void store(float * in, const v8sf ymm)
			{
				_mm256_store_ps(in, ymm);
			}
			
			inline void storeu(float * in, const v4sf xmm)
			{
				_mm_storeu_ps(in, xmm);
			}

			inline void store(float * in, const v4sf xmm)
			{
				_mm_store_ps(in, xmm);
			}

			// pd
			inline void storeu(double * in, const v4sd ymm)
			{
				_mm256_store_pd(in, ymm);
			}

			inline void store(double * in, const v4sd ymm)
			{
				_mm256_store_pd(in, ymm);
			}
			
			inline void storeu(double * in, const v2sd xmm)
			{
				_mm_storeu_pd(in, xmm);
			}

			inline void store(double * in, const v2sd xmm)
			{
				_mm_store_pd(in, xmm);
			}

			inline void storeu(float * in, float out)
			{
				*in = out;
			}

			inline void storeu(double * in, double out)
			{
				*in = out;
			}

			inline void store(float * in, float out)
			{
				*in = out;
			}

			inline void store(double * in, double out)
			{
				*in = out;
			}
#ifndef __MSVC__
			#define _mm256_set_m128i(hi, lo) (_mm256_inserti128_si256(_mm256_castsi128_si256(hi), lo, 1))
			#define _mm256_set_m128(hi, lo) (_mm256_insertf128_ps(_mm256_castps128_ps256(hi), lo, 1))
#endif
			inline v128i viget_low_part(v256i ia)
			{
				return _mm256_extractf128_si256(ia, 0);
			}

			inline v128i viget_high_part(v256i ia)
			{
				return _mm256_extractf128_si256(ia, 1);
			}

			inline v256i vicompose(v128i ia, v128i ib)
			{
				return _mm256_set_m128i(ib, ia);
			}
			
			
			// alignment-properties must be number-literals STILL in msvc. Grrr
			#ifdef __MSVC__
				#define simd_alignment_of(V) __declspec(align(32))
			#else
				#define simd_alignment_of(V) __alignas(32) /* hack */
			#endif

			template<typename V>
				struct suitable_container;

#pragma cwarn( "fix alignment of this type.")
			template<typename V>
				struct simd_alignment_of(V) suitable_container
				{
					typedef typename scalar_of<V>::type Ty;
					typedef V emulated_ty;
				public:
					
					#ifndef __MSVC__
						Ty & operator [] (unsigned idx) { return c[idx]; }
					#endif
					operator Ty* () { return c; }
					Ty * data() { return c; }

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
		}; // simd
	}; // cpl
#endif