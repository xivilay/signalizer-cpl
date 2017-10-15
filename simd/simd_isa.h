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

	file:simd_isa.h

		Templates common ISA extensions and emulates them if not available.
		Unfortunately, data types and operations are not always synchronized.

*************************************************************************************/
#ifndef CPL_SIMD_ISA_H
#define CPL_SIMD_ISA_H

#include "../Types.h"
#include "simd_traits.h"
#include "simd_interface.h"

namespace cpl
{
	namespace simd
	{
		namespace detail
		{
			template<typename T, typename enable = void>
			struct isa_fma_impl;

			template<typename T>
			struct isa_fma_impl<T, typename std::enable_if<!is_simd<T>::value>::type >
			{
				static inline T fma(T a, T b, T c) noexcept { return a * b + c; }
			};

			template<>
			struct isa_fma_impl<Types::v4sf>
			{
				static inline Types::v4sf fma(Types::v4sf a, Types::v4sf b, Types::v4sf c) noexcept { return _mm_fmadd_ps(a, b, c); }
			};

			template<>
			struct isa_fma_impl<Types::v8sf>
			{
				static inline Types::v8sf fma(Types::v8sf a, Types::v8sf b, Types::v8sf c) noexcept { return _mm256_fmadd_ps(a, b, c); }
			};

			template<>
			struct isa_fma_impl<Types::v2sd>
			{
				static inline Types::v2sd fma(Types::v2sd a, Types::v2sd b, Types::v2sd c) noexcept { return _mm_fmadd_pd(a, b, c); }
			};

			template<>
			struct isa_fma_impl<Types::v4sd>
			{
				static inline Types::v4sd fma(Types::v4sd a, Types::v4sd b, Types::v4sd c) noexcept { return _mm256_fmadd_pd(a, b, c); }
			};

		};

		template<typename T>
		struct isa_base
		{
			typedef T V;
		};

		template<bool has_fma>
		struct isa_fma_base
		{
			static constexpr bool is_fma_accelerated = has_fma;
		};

		template<typename V, bool has_fma>
		struct isa_fma;

		template<typename V>
		struct isa_fma<V, false> : public isa_fma_base<false>
		{
			inline static V fma(V a, V b, V c) noexcept
			{
				return a * b + c;
			}
		};

		template<typename V>
		struct isa_fma<V, true> : public isa_fma_base<false>, detail::isa_fma_impl<V>
		{
			using detail::isa_fma_impl<V>::fma;
		};

		template<typename Type, bool has_fma>
		struct isa_traits : public isa_base<Type>, public isa_fma<Type, has_fma>
		{

		};

		namespace detail
		{

			template<typename Scalar, class ClassDispatcher, typename... Args>
			auto dynamic_isa_dispatch_impl(typename std::enable_if<std::is_same<Scalar, float>::value, float>::type *, Args&&... args)
			{
				if (system::CProcessor::test(system::CProcessor::FMA))
				{
					switch (max_vector_capacity<Scalar>())
					{
						case 32:
						case 16:
						case 8:
							#ifdef CPL_COMPILER_SUPPORTS_AVX
							return ClassDispatcher::dispatch<isa_traits<Types::v8sf, true>>(std::forward<Args>(args)...);
							#endif
						case 4:
							return ClassDispatcher::template dispatch<isa_traits<Types::v4sf, true>>(std::forward<Args>(args)...);
							break;
						default:
							return ClassDispatcher::template dispatch<isa_traits<float, true>>(std::forward<Args>(args)...);
							break;
					}

				}
				else
				{
					switch (max_vector_capacity<Scalar>())
					{
						case 32:
						case 16:
						case 8:
							#ifdef CPL_COMPILER_SUPPORTS_AVX
							return ClassDispatcher::dispatch<isa_traits<Types::v8sf, false>>(std::forward<Args>(args)...);
							#endif
						case 4:
							return ClassDispatcher::template dispatch<isa_traits<Types::v4sf, false>>(std::forward<Args>(args)...);
							break;
						default:
							return ClassDispatcher::template dispatch<isa_traits<float, false>>(std::forward<Args>(args)...);
							break;
					}
				}
			}


			template<typename Scalar, class ClassDispatcher, typename... Args>
			auto dynamic_isa_dispatch_impl(typename std::enable_if<std::is_same<Scalar, double>::value, float>::type *, Args&&... args)
			{
				if (system::CProcessor::test(system::CProcessor::FMA))
				{
					switch (max_vector_capacity<Scalar>())
					{
						case 32:
						case 16:
						case 8:
						case 4:
							#ifdef CPL_COMPILER_SUPPORTS_AVX
							return ClassDispatcher::dispatch<isa_traits<Types::v4sd, true>>(std::forward<Args>(args)...);
							break;
							#endif
						case 2:
							return ClassDispatcher::template dispatch<isa_traits<Types::v2sd, true>>(std::forward<Args>(args)...);
							break;
						default:
							return ClassDispatcher::template dispatch<isa_traits<double, true>>(std::forward<Args>(args)...);
							break;
					}

				}
				else
				{
					switch (max_vector_capacity<Scalar>())
					{
						case 32:
						case 16:
						case 8:
							#ifdef CPL_COMPILER_SUPPORTS_AVX
							return ClassDispatcher::dispatch<isa_traits<Types::v4sd, false>>(std::forward<Args>(args)...);
							break;
							#endif
						case 2:
							return ClassDispatcher::template dispatch<isa_traits<Types::v2sd, false>>(std::forward<Args>(args)...);
							break;
						default:
							return ClassDispatcher::template dispatch<isa_traits<double, false>>(std::forward<Args>(args)...);
							break;
					}
				}
			}
		};

		template<typename Scalar, class ClassDispatcher, typename... Args>
		auto dynamic_isa_dispatch(Args&&... args)
		{
			return detail::dynamic_isa_dispatch_impl<Scalar, ClassDispatcher, Args...>(nullptr, std::forward<Args>(args)...);
		}

	}; // simd
}; // cpl
#endif
