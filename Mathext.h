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

	file:Mathext.h
	
		Utility, roundings, scalings, extensions of standard math library.

*************************************************************************************/

#ifndef _MATHEXT_H
	#define _MATHEXT_H

	// check this!
	//http ://fastapprox.googlecode.com/svn/trunk/fastapprox/src/fastonebigheader.h

	#include <math.h>
	#include "MacroConstants.h"
	#include <vector>
	#include "Types.h"
	#include "ssemath.h"

	#include <functional>
	#include "stdext.h"

	#ifndef M_PI
		#define M_PI 3.1415926535897932384626433832795028841971693993751058209
	#endif
	#ifndef M_E
		#define M_E 2.7182818284590452353602874713527
	#endif
	#ifndef PI
		#define PI M_PI
	#endif
	#ifndef TAU
		#define TAU (M_PI * 2)
	#endif
	#ifndef HALFPI
		#define HALFPI (M_PI / 2)
	#endif

	namespace cpl
	{

		template<typename T, unsigned int size>
			class CBoxFilter
			{
			public:

				typedef T scalar;

				CBoxFilter() : buf(), ptr(0) {};

				void setNext(scalar input)
				{
					buf[ptr] = input;
					ptr++;
					ptr %= size;
				}

				scalar getAverage()
				{
					// siiiiiiiiimd
					scalar sum = 0;
					for (scalar n : buf)
						sum += n;
					sum /= size;
					return sum;
				}

			private:
				scalar buf[size];
				std::size_t ptr;
			};



		namespace Math
		{
			using namespace std;

			namespace Complex
			{
			
			};

			/*
				Returns a coefficient which guarantees that:
					1/e = 1 * expDecay(N)^N 

				That is, your state will have fallen by 1/e after N repeated multiplications.
				Equivalent to powerDecay(1/e, N)
			*/
			template<typename Scalar>
				Scalar expDecay(Scalar coeff)
				{
					return static_cast<Scalar>(std::exp(-1.0 / coeff));
				}
			/*
				Returns a coefficient which guarantees that:
					endValue = 1 * powerDecay(N)^N

				That is, your state will have fallen by endValue after N repeated multiplications.
			*/
			template<typename Scalar>
				Scalar powerDecay(Scalar endValue, Scalar N)
				{
					return static_cast<Scalar>(std::pow(endValue, 1.0 / N));
				}

			/*
				Implements the size*2-periodic triangular function, with a DC offset of size/2.
				This can be used to wrap negatives into positive range again, and over-bounds 
				back into range.
			*/
			template<typename Scalar>
				inline typename std::enable_if<std::is_signed<Scalar>::value, Scalar>::type 
					circularWrap(Scalar offset, Scalar size)
				{
					return std::abs(std::modulus<Scalar>()(offset + size, size * Scalar(2)) - size * Scalar(2));
				}

			/*
				Implements the size*2-periodic triangular function, with a DC offset of size/2.
				This can be used to wrap negatives into positive range again, and over-bounds
				back into range.

				Specialized for size_t.
			*/
			inline std::size_t circularWrap(std::size_t offset, std::size_t size)
			{
				std::ptrdiff_t o1 = static_cast<std::ptrdiff_t>(offset);
				std::ptrdiff_t s1 = static_cast<std::ptrdiff_t>(size);
				return static_cast<std::size_t>(std::abs(std::modulus<std::ptrdiff_t>()(o1 + s1, s1 << 1) - (s1 << 1)));
			}

			/*
				
			*/
			template<typename Scalar>
				inline typename std::enable_if<std::is_signed<Scalar>::value, Scalar>::type
					mapAroundZero(Scalar offset, Scalar size)
				{
					return offset - (size - Scalar(1))/Scalar(2);
				}

			// for situations where you calculate a signed fraction added to an offset
				template<int divisor, typename T>
				inline std::div_t indexDivision(T dividend)
				{
					static_assert(divisor != 0, "Zero-division protection");

					return std::div(dividend < 0 ? (dividend - (divisor - 1)) : dividend, divisor);

					//return  / divisor  : dividend / divisor;
				}

			template<typename Scalar, size_t size>
				inline Scalar compileVector(const  Scalar(&vec)[size])
				{
					Scalar acc = Scalar(0);
					for (Types::fint_t z = 0; z < size; ++z)
					{
						acc += vec[z];
					}
					return acc;
				}

			template<typename Scalar>
				inline Scalar compileVector(const std::vector<Scalar> & vec)
				{
					Scalar acc = Scalar(0);
					auto size = vec.size();
					for (Types::fint_t z = 0; z < size; ++z)
					{
						acc += vec[z];
					}
					return acc;
				}

			template<class Vector>
				inline auto compileVector(const Vector & vec, std::size_t size)
					-> unq_typeof(vec[0])
				{
					typedef unq_typeof(vec[0]) Scalar;
					Scalar acc = Scalar(0);
					for (Types::fint_t z = 0; z < size; ++z)
					{
						acc += vec[z];
					}
					return acc;
				}

			inline float compileVector(Types::v4sf xmm)
			{

				__alignas(16) float vec[4];
				_mm_store_ps(vec, xmm);
				return compileVector(vec);
			}

			inline double compileVector(Types::v4sd ymm)
			{
				__alignas(32) double vec[4];
				_mm256_store_pd(vec, ymm);
				return compileVector(vec);
			}

			inline double compileVector(Types::v8sf ymm)
			{
				__alignas(32) float vec[8];
				_mm256_store_ps(vec, ymm);
				return compileVector(vec);
			}

			inline double compileVector(Types::v2sd xmm)
			{
				
				__alignas(16) double vec[2];
				_mm_store_pd(vec, xmm);
				return compileVector(vec);
			}

			template<class V1, class V2>
				auto calculateError(const V1 & v1, const V2 & v2, std::size_t size)
					-> unq_typeof(v1[0])
				{
					typedef unq_typeof(v1[0]) Ty;
					Ty rms = 0, error = 0;
					for(std::size_t i = 0; i < size; ++i)
					{
						error= v1[i] - v2[i];
						rms += error * error;
					}
					return sqrt(rms / (double)size);
	
				}

			inline float fastabs(float x)
			{
				union __alignas(4) binary_float
				{
					float f;
					struct
					{
						unsigned mantissa : 24;
						unsigned exponent : 7;
						unsigned sign : 1;
					} binary;
				};
				((binary_float*)(&x))->binary.sign = 0;

				return x;

				//std::uint32_t ipart = std::uint32_t((*reinterpret_cast<std::uint32_t*>(&x)) & ~0x80000000);
				//return *(float*)&ipart;
			}

			inline int fastabs(int x)
			{
				return (unsigned)x & ~0x80000000u;
			}

			inline float fastsine(float x)
			{
				const float B = static_cast<float>(4 / M_PI);
				const float C = static_cast<float>(-4 / (M_PI*M_PI));

				float y = B * x + C * x * abs(x);

				#ifdef EXTRA_PRECISION
					//  const float Q = 0.775;
					const float P = 0.225;

					y = P * (y * abs(y) - y) + y;   // Q * y + P * y * abs(y)
				#endif
				return y;
			}
			
			inline float fastcosine(float x)
			{
				const float B = static_cast<float>(4 / M_PI);
				const float C = static_cast<float>(-4 / (M_PI*M_PI));
				// if(x > PI) x -= 2 * PI
				x -= static_cast<float>((x > M_PI) * (TAU));
				float y = B * x + C * x * fastabs(x);

				#ifdef EXTRA_PRECISION
					//  const float Q = 0.775;
					const float P = 0.225;

					y = P * (y * abs(y) - y) + y;   // Q * y + P * y * abs(y)
				#endif
				return y;
			}

			
			template<typename Scalar>
				typename std::enable_if<std::is_integral<Scalar>::value, Scalar>::type
					nextPow2Inc(Scalar x)
				{
					if ((x & (x - 1)) == 0)
						return x;
					else
						return nextPow2(x);
				}

			template<typename Scalar>
				typename std::enable_if<std::is_integral<Scalar>::value, Scalar>::type
				nextPow2(Scalar x)
				{
					Scalar power = 2;
					while (x >>= 1) 
						power <<= 1;
					return power;
				}
			template<typename Scalar>
				typename std::enable_if<std::is_integral<Scalar>::value, Scalar>::type
				lastPow2(Scalar x)
				{
					Scalar power = 1;
					while (x >>= 1) 
						power <<= 1;
					return power;
				}

			template<typename Scalar = double>
				Scalar logg(Scalar x, Scalar min, Scalar max)
				{
					return (max - min) / log10(max - min + 1) * log10((max - min) * x + 1) + min;
				}

			template <typename Scalar, class Scale>
				Scalar transform(Scalar input, Scalar _min, Scalar _max, Scale scale /* not inlined??*/)
				{
					return scale(input, _min, _max) / _max + _min;
				}

			template<typename Scalar, typename T1, typename T2>
				inline Scalar confineTo(Scalar val, T1 _min, T2 _max)
				{
					return std::max<Scalar>(std::min<Scalar>(static_cast<Scalar>(val), 
						static_cast<Scalar>(_max)), static_cast<Scalar>(_min));
				}

			template<typename Scalar>
				inline Scalar fractionToDB(Scalar val)
				{
					return (Scalar(20.0) * log10(val));
				}

			template<typename Scalar>
				inline Scalar dbToFraction(Scalar dBValue)
				{
					return std::pow((Scalar)10, dBValue / Scalar(20.0));
				}

			template<typename Scalar, typename Input>
				inline Scalar round(Input number)
				{
					return (number >= Input(0.0)) ? static_cast<Scalar>(number + Input(0.5)) : static_cast<Scalar>(number - Input(0.5));
				}

			// floors to next integer, down to zero infinity
			template<typename T, typename T2 = T>
				inline typename std::enable_if<std::is_floating_point<T2>::value, T2>::type
					floorToNInf(T input)
				{
					return static_cast<T2>(std::floor(input));
				}

			// this is a no-op for integers
			template<typename T, typename T2 = T>
				inline typename std::enable_if<!std::is_floating_point<T2>::value, T2>::type
					floorToNInf(T input)
				{
					return input;
				}


			template<typename T, typename T2 = T>
				inline typename std::enable_if<std::is_floating_point<T2>::value, T2>::type
					frac(T input)
				{
					return static_cast<T2>(input - std::floor(input));
				}
			
			template<typename T>
				static inline T mod(T a, T b) noexcept
				{
					if (b < 0) //you can check for b == 0 separately and do what you want
						return mod(-a, -b);
					T ret = a % b;

					//if (ret < 0)
					//	ret += b;
					ret += (ret < 0) * b;

					return ret;
				}

			template<typename T>
				static inline T div(T a, T b) noexcept
				{
					if (b < 0) //you can check for b == 0 separately and do what you want
						return div(-a, -b);
					T ret = a / b;

					//if (ret < 0)
					//	ret += b;
					ret += (ret < 0) * b;

					return ret;
				}


			inline static std::uint8_t roundedMul(std::uint8_t a, std::uint8_t b) noexcept
			{
				return static_cast<std::uint8_t>((static_cast<std::uint_fast16_t>(a)* b + 0x80) >> 8);
			}
			inline static std::uint8_t roundedMul(std::uint8_t a, std::uint8_t b, std::uint8_t c) noexcept
			{
				return static_cast<std::uint8_t>((static_cast<std::uint_fast32_t>(a)* b * c + 0x8000) >> 16);
			}


			template<int divisor, typename T>
				typename std::enable_if<std::is_same<T, std::uint8_t>::value, T>::type
					reciprocalDivision(T a) noexcept
				{
					const T divisorRecip = std::numeric_limits<T>::max() / divisor;
					return static_cast<T>((static_cast<std::uint_fast16_t>(a)* divisorRecip + 0x80) >> 8);
				}

			template<int divisor, typename T>
				typename std::enable_if<std::is_same<T, std::uint16_t>::value, T>::type
					reciprocalDivision(T a) noexcept
				{
					const T divisorRecip = std::numeric_limits<T>::max() / divisor;
					return static_cast<T>((static_cast<std::uint_fast32_t>(a)* divisorRecip + 0x8000) >> 8);
				}


			template<typename Scalar>
				Scalar distance(Scalar x0, Scalar y0, Scalar x1, Scalar y1)
				{
					auto d1 = x1 - x0;
					auto d2 = y1 - y0;
					return std::sqrt<Scalar>(d1 * d1 + d2 * d2);
				}

			template<typename Scalar, class Abs>
				inline bool fequals(Scalar x, Scalar y, Scalar eps = (Scalar)0.000001, Abs absolute = std::abs)
				{
					return absolute(x - y) < eps;
				}
			
			template <typename ScalarTy>
				class Matrix2DRotater
				{
					typedef ScalarTy Scalar;
					double c, s;
					
				public:
					Matrix2DRotater() : c(0), s(0) {};
					void setRotation(double radians) { c = std::cos(radians); s = std::sin(radians); }
					
					inline void rotate(Scalar & x, Scalar & y)
					{
						auto xn = x * c - y * s;
						auto yn = x * s + y * c;
						x = xn;
						y = yn;
					}
					
					inline void rotate(Scalar xbuf[], Scalar ybuf[], std::size_t size)
					{
						// simd optimize this.
						for(int i = 0; i < size; ++i)
						{
							auto & x = xbuf[i];
							auto & y = ybuf[i];
							auto xn = x * c - y * s;
							auto yn = x * s + y * c;
							x = xn;
							y = yn;
						}
					}
					
					inline static void rotate(Scalar & x, Scalar & y, double radians)
					{
						auto cosrol = std::cos(radians);
						auto sinrol = std::sin(radians);
						auto xn = x * cosrol - y * sinrol;
						auto yn = x * sinrol + y * cosrol;
						x = xn;
						y = yn;
					}
				}; // Matrix2DRotater
			


			class UnityScale
			{
			public:
				template<typename Ty>
					inline static Ty exp(Ty value, Ty _min, Ty _max)
					{
						return _min * pow(_max/_min, value);
					}
				template<typename Ty>
					inline static Ty linear(Ty value, Ty _min, Ty _max)
					{
						return value * (_max - _min) + _min;
					}
				template<typename Ty>
					inline static Ty log(Ty value, Ty _min, Ty _max)
					{
						return linear(1 - value, _min, _max) - exp(1 - value, _min, _max) + linear(value, _min, _max);
					}
				template<typename Ty>
					inline static Ty polyExp(Ty value, Ty _min, Ty _max)
					{
						return value * value * (_max - _min) + _min;
					}
				template<typename Ty>
					inline static Ty polyLog(Ty value, Ty _min, Ty _max)
					{
						return (-(value * value) + 2 * value) * (_max - _min) + _min; 
					}
				class Inv
				{
				public:
					template<typename Ty>
						inline static Ty exp(Ty y, Ty _min, Ty _max)
						{
							return log10(y / _min) / log10(_max / _min);
						}

					template<typename Ty>
						inline static Ty linear(Ty y, Ty _min, Ty _max)
						{
							return (y - _min) / (_max - _min);
						}
				}; // Inv
			}; // UnityScale
		}; // Math


	}; // cpl
#endif