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

#ifndef SIMD_H
	#define SIMD_H
	
	#include "Types.h"
	//#include "intrintypes.h"

	namespace cpl
	{
		namespace simd
		{
			using namespace cpl::Types;

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
				struct is_simd<v128si> : public std::true_type{};

			template<>
				struct is_simd<v256si> : public std::true_type{};

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
				struct scalar_of;



			template<>
				struct scalar_of<v8sf>{ typedef float type; };

			template<>
				struct scalar_of<v4sf>{ typedef float type; };


			template<>
				struct scalar_of<v128si, 1>{ typedef integer_of_bytes<1>::type type; };
			template<>
				struct scalar_of<v128si, 2>{ typedef integer_of_bytes<2>::type type; };
			template<>
				struct scalar_of<v128si, 4>{ typedef integer_of_bytes<4>::type type; };
			template<>
				struct scalar_of<v128si, 8>{ typedef integer_of_bytes<8>::type type; };
			template<>

				struct scalar_of<v256si, 1>{ typedef integer_of_bytes<1>::type type; };
			template<>
				struct scalar_of<v256si, 2>{ typedef integer_of_bytes<2>::type type; };
			template<>
				struct scalar_of<v256si, 4>{ typedef integer_of_bytes<4>::type type; };
			template<>
				struct scalar_of<v256si, 8>{ typedef integer_of_bytes<8>::type type; };

			template<>
				struct scalar_of<v4sd>{ typedef double type; };

			template<>
				struct scalar_of<v2sd>{ typedef double type; };

			template<>
				struct scalar_of<float>{ typedef float type; };

			template<>
				struct scalar_of<double>{ typedef double type; };

			template<>
				struct scalar_of<std::int32_t>{ typedef std::int32_t type; };

			template<>
				struct scalar_of<std::int64_t>{ typedef std::int64_t type; };

			template<typename V, std::size_t rank = 4>
				struct elements_of
				{
					static const std::size_t value = sizeof(V) / sizeof(typename scalar_of<V, rank>::type);
				};
			
			
			template<typename V>
				struct to_integer;

			template<>
				struct to_integer<v8sf>{ typedef v256si type; };

			template<>
				struct to_integer<v4sf>{ typedef v128si type; };

			template<>
				struct to_integer<v4sd>{ typedef v256si type; };

			template<>
				struct to_integer<v2sd>{ typedef v128si type; };

			template<>
				struct to_integer<float>{ typedef std::int32_t type; };

			template<>
				struct to_integer<double>{ typedef std::int64_t type; };
	
			
			/*
				While clang and gcc has builtin operators for simd types, msvc does not.
				However, since the simd types are unions / structs on msvc, we can create
				non-member overloads of the operators.
			*/
			
			#ifdef __MSVC__
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
					AddSimdComparisonps(name, op, __m128, _mm); \
					AddSimdComparisonps(name, op, __m256, _mm256); \
					AddSimdComparisonpd(name, op, __m128d, _mm); \
					AddSimdComparisonpd(name, op, __m256d, _mm256); 

				AddOperatorForArchs(add, +);
				AddOperatorForArchs(mul, *);
				AddOperatorForArchs(div, /);
				AddOperatorForArchs(sub, -);
				AddOperatorForArchs(or, |);
				AddOperatorForArchs(and, &);

				AddComparisonForArchs(_CMP_LE_OQ, <= );

				#undef AddSimdOperatorps
				#undef AddSimdCOperatorps
				#undef AddSimdOperatorpd
				//#undef AddSimdCOperatorpd
				//#undef AddOperatorForArchs
				//#undef AddComparisonForArchs
			#endif
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
			
				float zero()
				{
					return 0;
				}

				float load(const float * V1)
				{
					return *V1;
				}

				double load(const double * V1)
				{
					return *V1;
				}

				float broadcast(const float * V1)
				{
					return *V1;
				}

				double broadcast(const double * V1)
				{
					return *V1;
				}

				float set1(const float V1)
				{
					return V1;
				}

				double set1(const double V1)
				{
					return V1;
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
				typename std::enable_if<std::is_same<V, v4sf>::value, v4sf>::type 
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
				typename std::enable_if<std::is_same<V, typename scalar_of<V>::type>::value, typename scalar_of<V>::type>::type
					gather(VectorPtrs p)
				{

					return *p[0];
				}

			template<typename V, class VectorPtrs>
				typename std::enable_if<std::is_same<V, v8sf>::value, v8sf>::type 
					gather(VectorPtrs p)
				{
					return _mm256_set_ps(*p[0], *p[1], *p[2], *p[3], *p[4], *p[5], *p[6], *p[7]);
				}

			template<typename V, class VectorPtrs>
				typename std::enable_if<std::is_same<V, v4sf>::value, v4sf>::type 
					setv(VectorPtrs p)
				{
					return _mm_set_ps(*p[0], *p[1], *p[2], *p[3]);
				}

			template<typename V, class VectorPtrs>
				typename std::enable_if<std::is_same<V, v8sf>::value, v8sf>::type 
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

			template<typename V, typename M>
				inline typename std::enable_if<!is_simd<V>::value, V>::type
					bool_and(V V1, M mask)
				{
					return V1 * mask;
				}

			template<typename V>
				inline typename std::enable_if<is_simd<V>::value, V>::type
					bool_and(V V1, V mask)
				{
					return V1 & mask;
 				}

			template<typename V>
				struct suitable_container;
			
			// alignment-properties must be number-literals STILL in msvc. Grrr
			#ifdef __MSVC__
				#define simd_alignment_of(V) __declspec(align(32))
			#else
				#define simd_alignment_of(V) __alignas(32) /* hack */
			#endif

			template<typename V>
				struct suitable_container;

#warning fix alignment of this type.
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

					operator emulated_ty () { return load<emulated_ty>(c); }
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
				}
	}; // APE
#endif