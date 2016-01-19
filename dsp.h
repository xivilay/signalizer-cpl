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

	file:dsp.h
	
		Some definitions, algorithms for dsp. Includes other stuff as well.

*************************************************************************************/

#ifndef _DSP_H
	#define _DSP_H

	#include "Mathext.h"
	#include <vector>
	#include <complex>
	#include <cstdint>
	#include "Utility.h"
	//#include <amp.h>
	//#include <amp_math.h>
	//#include <amp_graphics.h>
	
	//#include <amp_short_vectors.h>

	namespace cpl
	{
		namespace dsp
		{

			template<typename T, class alloc>
			std::vector<T, alloc> real(const std::vector<std::complex<T>, alloc> & cmplx)
			{
				std::vector<T, alloc> ret(cmplx.size());
				for (std::size_t i = 0; i < cmplx.size(); ++i)
					ret[i] = cmplx[i].real();

				return ret;
			}

			template<typename T, class alloc>
			std::vector<T, alloc> imag(const std::vector<std::complex<T>, alloc> & cmplx)
			{
				std::vector<T, alloc> ret(cmplx.size());
				for (std::size_t i = 0; i < cmplx.size(); ++i)
					ret[i] = cmplx[i].imag();

				return ret;
			}

			template<typename T>
			void fftshift(std::complex<T> * fft, std::size_t N)
			{
				std::rotate(fft, fft + N / 2, fft + N);
			}

			template<typename T, class A>
			void fftshift(std::vector<T, A> & fft)
			{
				std::rotate(fft.begin(), fft.begin() + fft.size() / 2, fft.end());
			}

			template<typename T>
			void normalize(T * out, std::size_t N)
			{
				T scale = T(1) / *std::max_element(out, out + N);
				std::for_each(out, out + N, [](T & z) { z *= scale; });
			}

			template<typename T, typename A>
			void normalize(std::vector<T, A> & out)
			{
				T scale = T(1) / *std::max_element(out.begin(), out.end());
				std::for_each(out.begin(), out.end(), [=](T & z) { z *= scale; });
			}


			template<typename T>
			void normalize(std::complex<T> * out, std::size_t N)
			{
				T max = std::abs(out[0]);

				for (std::size_t i = 1; i < N; ++i)
				{
					const T current = std::abs(out[i]);
					if (current > max)
						max = current;
				}

				const T scale = T(1) / max;

				std::for_each(out, out + N, [](T & z) { z *= scale; });
			}

			template<typename T, typename A>
			void normalize(std::vector<std::complex<T>, A> & out)
			{
				T max = std::abs(out[0]);

				for (std::size_t i = 1; i < out.size(); ++i)
				{
					const T current = std::abs(out[i]);
					if (current > max)
						max = current;
				}

				const T scale = T(1) / max;

				std::for_each(out.begin(), out.end(), [](T & z) { z *= scale; });
			}


			template<typename T>
			class Resonator
			{
				typedef T Scalar;
			public:
				Resonator(Scalar omega, std::size_t size)
					: buffer(size), ptr(0), N(size)
				{
					q = 2 * M_PI / N;
					r = Scalar(1.0) - std::numeric_limits<Scalar>::epsilon();
					rn = static_cast<Scalar>(std::pow(1.0 - std::numeric_limits<Scalar>::epsilon(), N));
					pole[0] = std::polar(r, omega - q);
					pole[1] = std::polar(r, omega);
					pole[2] = std::polar(r, omega + q);
				}

				inline void resonate(Scalar x)
				{
					//Scalar input = x;
					Scalar input = buffer[ptr] * rn + x;
					state[0] = state[0] * pole[0] - input;
					state[1] = state[1] * pole[1] - input;
					state[2] = state[2] * pole[2] - input;


					buffer[ptr] = x;
					ptr++;
					ptr %= N;
				}

				std::complex<Scalar> getResonance() {
					return std::conj(Scalar(-0.25) * state[0] + Scalar(0.5) * state[1] + Scalar(-0.25) * state[2]) / Scalar(N);
				}

			private:
				std::complex<Scalar> state[3], pole[3];
				std::vector<Scalar> buffer;
				Scalar r, rn, q;
				std::size_t ptr, N;
			};


			template<typename T>
				inline T radsToDegrees(T input)
				{
					return 360 * input / TAU;
				}


			template<typename Scalar>
				class CFastOscillator
				{
				public:
					typedef Scalar ScalarTy;
					CFastOscillator(ScalarTy period) : z1(0), z2(0) { reset(period); };
					CFastOscillator() {}
					void reset(ScalarTy period, ScalarTy freq = ScalarTy(1), ScalarTy phase = ScalarTy(0))
					{
						double w = (freq * TAU) / period;
						omega = 2 * std::cos(w);
						z1 = std::cos(-w + phase - HALFPI);
						z2 = std::cos(-2 * w + phase - HALFPI);
					}

					inline ScalarTy tick()
					{
						ScalarTy sample = omega * z1 - z2;
						z2 = z1;
						z1 = sample;
						return sample;
					}

				private:
					ScalarTy z1, z2, omega;
				};

			template<bool precise>
			typename std::enable_if<precise, double>::type
				lzresponse(double x, int size)
			{
				return x ? (size * sin(M_PI * x) * sin(M_PI * x / size)) / (M_PI * M_PI * x * x) : 1;
			}

			template<bool precise>
			typename std::enable_if<precise, double>::type
				scresponse(double x)
			{
				return x ? (sin(M_PI * x)) / (M_PI * x ) : 1;
			}

			template<bool precise>
			typename std::enable_if<!precise, double>::type
				lzresponse(double x, int size)
			{
					return x ? (size * cpl::Math::fastsine(M_PI * x) * cpl::Math::fastsine(M_PI * x / size)) / (TAU * x * x) : 1;
			}

			template<typename R, bool precise = true, typename T>
			auto lfilter(T & vec, std::size_t asize, double x, signed int wsize) -> typename std::remove_reference<decltype(vec[0])>::type
			{
				R resonance = 0;
				signed start = static_cast<signed int>(floor(x));
				for (signed int i = start - wsize + 1; i < start + wsize; ++i)
				{
					if (i >= 0 && i < asize)
					{
						auto impulse = vec[i];
						auto response = fresponse<precise>(x - i, wsize);
						resonance += impulse * response;
					}
				}
				return resonance;
			}


			template<typename Scalar, class Vector>
			static std::complex<Scalar> goertzel(const Vector & data, std::size_t size, Scalar omega)
			{
				Scalar sine, cosine, coeff, q0(0), q1(0), q2(0);

				sine = sin(omega);
				cosine = cos(omega);
				coeff = 2.0 * cosine;

				for (Types::fint_t t = 0; t < size; t++)
				{
					q0 = coeff * q1 - q2 + data[t];
					q2 = q1;
					q1 = q0;
				}

				Scalar real = (q1 - q2 * cosine);
				Scalar imag = (q2 * sine);

				return std::complex<Scalar>(real, imag);

			}

			template<typename R, bool precise = true, typename T>
			auto lfilter(T * vec, std::size_t asize, double x, signed int wsize) -> typename std::remove_reference<decltype(vec[0])>::type
			{
				R resonance = 0;
				signed start = static_cast<signed int>(floor(x));
				for (signed int i = start - wsize + 1; i < (start + wsize + 1); ++i)
				{
					if (i >= 0 && i < asize)
					{
						auto impulse = vec[i];
						auto response = lzresponse<precise>(x - i, wsize);
						resonance += impulse * response;
					}
				}
				return resonance;
			}

			template<typename R, bool precise = true, typename T>
			inline auto lanczosFilter(T * vec, Types::fsint_t asize, double x, Types::fsint_t wsize) -> typename std::remove_reference<decltype(vec[0])>::type
			{
				R resonance = 0;
				Types::fsint_t start = cpl::Math::floorToNInf<Types::fsint_t>(x);
				for (Types::fsint_t i = start - wsize + 1; i < (start + wsize + 1); ++i)
				{
					if (i >= 0 && i < asize)
					{
						auto impulse = vec[i];
						auto response = lzresponse<precise>(x - i, wsize);
						resonance += impulse * response;
					}
				}
				return resonance;
			}

			template<typename R, bool precise = true, typename T>
			inline auto sincFilter(T * vec, std::size_t asize, double x, Types::fsint_t wsize) -> typename std::remove_reference<decltype(vec[0])>::type
			{
				R resonance = 0;
				Types::fsint_t start = cpl::Math::floorToNInf<Types::fsint_t>(x);
				for (Types::fsint_t i = start - wsize + 1; i < (start + wsize + 1); ++i)
				{
					if (i >= 0 && i < asize)
					{
						auto impulse = vec[i];
						auto response = scresponse<precise>(x - i);
						resonance += impulse * response;
					}
				}
				return resonance;
			}

			template<typename R, typename T>
			inline auto linearFilter(T * vec, Types::fsint_t asize, double x) -> typename std::remove_reference<decltype(vec[0])>::type
			{
				Types::fsint_t x1 = cpl::Math::floorToNInf<Types::fsint_t>(x);
				Types::fsint_t x2 = std::min(asize - 1, x1 + 1);

				//if (x2 == asize - 1)
				//	return vec[x1];

				auto frac = x - x1;

				return vec[x1] * (1 - frac) + vec[x2] * frac;

			}

			template<typename T>
				void fillWithFreq(T & vec, size_t size, double freq, double samplingRate, double initialPhase, double amplitude = 1)
				{
					auto omega = 2 * M_PI * freq / samplingRate;
					double phase = initialPhase;
					for (unsigned i = 0; i < size; ++i)
					{
						vec[i] = amplitude * std::sin(phase);
						phase += omega;
						if (phase > 2 * M_PI)
							phase -= 2 * M_PI;
					}

				}

			template<typename T>
				void addFillWithFreq(T & vec, size_t size, double freq, double samplingRate, double initialPhase, double amplitude = 1)
				{
					auto omega = 2 * M_PI * freq / samplingRate;
					double phase = initialPhase;
					for (unsigned i = 0; i < size; ++i)
					{
						vec[i] += amplitude * std::sin(phase);
						phase += omega;
						if (phase > 2 * M_PI)
							phase -= 2 * M_PI;
					}

				}

			template<typename T>
				void fillWithRand(T & vec, size_t size)
				{
					typedef unq_typeof(vec[0]) T2;
					for (std::size_t i = 0; i < size; ++i)
					{
						vec[i] = ((T2(RAND_MAX) / (T2)2) - (T2)rand()) / (T2(RAND_MAX) / (T2)2);
					}

				}

			template<typename T, typename T2>
				void linspace(T & vec, std::size_t size, T2 min, T2 max)
				{
					if (size == 1)
						vec[0] = min;
					else
						for (std::size_t i = 0; i < size; ++i)
						{
							vec[i] = min + (max - min) * i / (size - 1);
						}
				}

			typedef std::uint_fast32_t inttype;

			inline void separateTransforms(const double * tsf, double * real, double * imag, std::size_t N)
			{
				N <<= 1;
				double x1, x2, y1, y2;
				for (std::size_t k = 2u; k < N; k += 2)
				{
					x1 = tsf[k];
					x2 = tsf[N - k];
					y1 = tsf[k + 1];
					y2 = tsf[N - k + 1];

					real[k] = (x1 + x2) * 0.5;
					real[k + 1] = (y1 - y2) * 0.5;
					imag[k] = (y1 + y2) * 0.5;
					imag[k + 1] = -(x1 - x2) * 0.5;
				}
				real[0] = tsf[0];
				imag[0] = tsf[1];
			}

			/// <summary>
			/// Assuming a fourier transform, where the input real and imaginary signal have pure real transforms,
			/// separates the transforms such that they mirror around the nyquist bins. Example output:
			/// 
			///		bin[0] = {real DC, imag DC}
			///		bin[X ... N/2 -2] = real bin X
			///		bin[N/2 -1] = real nyquist
			///		bin[N/2] = imag nyquist
			///		bin[N/2 + X... N - 1] = imag bin N/2 - X
			/// 
			/// Based on the concept, "two for the price of one".
			/// http://www.engineeringproductivitytools.com/stuff/T0001/PT10.HTM
			/// </summary>
			/// <param name="tsf">
			/// Array of complex T (real, imag pairs)
			/// </param>
			/// <param name="N">
			/// The amount of complex number pairs in the array. Behaviour is undefined if N isn't a power of 2.
			/// </param>
			template<typename T>
				inline typename std::enable_if<std::is_arithmetic<T>::value, void>::type 
					separateTransformsIPL(T * tsf, std::size_t N)
				{
					auto N2 = N; // N = total size, N2 = half size
					N <<= 1;
					T x1, x2, y1, y2;

					for (auto k = 2u; k < N2; k += 2)
					{
						x1 = tsf[k];
						x2 = tsf[N - k];
						y1 = tsf[k + 1];
						y2 = tsf[N - k + 1];

						// real[k] = (x1 + x2) * 0.5;
						tsf[k] = (x1 + x2) * T(0.5); // real bin N channel 1
						// real[k + 1] = (y1 - y2) * 0.5;
						tsf[N - k] = (y1 + y2) * T(0.5); // real bin N channel 2
						// real[k + 1] = (y1 - y2) * 0.5;
						tsf[k + 1] = (y1 - y2) * T(0.5); // imag bin N channel 1
						// imag[k + 1] = -(x1 - x2) * 0.5;
						tsf[N - k + 1] = -(x1 - x2) * T(0.5); // imag bin N channel 2
					}
					// TODO: figure out why real nyquist is half value.
					tsf[N2 - 2] *= T(2);
					tsf[N2 - 1] *= T(2);
				}

			/// <summary>
			/// Assuming a fourier transform, where the input real and imaginary signal have pure real transforms,
			/// separates the transforms such that they mirror around the nyquist bins. Example output:
			/// 
			///		bin[0] = {real DC, imag DC}
			///		bin[X ... N/2 -2] = real bin X
			///		bin[N/2 -1] = real nyquist
			///		bin[N/2] = imag nyquist
			///		bin[N/2 + X... N - 1] = imag bin N/2 - X
			/// 
			/// Based on the concept, "two for the price of one".
			/// http://www.engineeringproductivitytools.com/stuff/T0001/PT10.HTM
			/// </summary>
			/// <param name="tsf">
			/// Array of complex T (real, imag pairs)
			/// </param>
			/// <param name="N">
			/// The amount of complex number pairs in the array. Behaviour is undefined if N isn't a power of 2.
			/// </param>
			template<typename T>
				inline void separateTransformsIPL(std::complex<T> * tsf, std::size_t N)
				{
					return separateTransformsIPL(reinterpret_cast<double*>(tsf), N);
				}

			/*template<typename Scalar, class Vector, bool scale = false>
				std::complex<Scalar> fourierTransform(const Vector & data, Scalar frequency, Scalar sampleRate, size_t a, size_t b)
				{
					std::complex<Scalar> acc = 0;
					Scalar real = 0, imag = 0;
					//std::complex<Scalar> z = 0; 

					Scalar w = frequency * 2 * PI / sampleRate;
					double i = 1.0;
					for (int t = a; t < b; ++t)
					{
						// e^(-i*w*t) == cos(w*t) + -i * sin(w * t) ?
						//real = std::cos(w * t);
						//imag = -i * std::sin(w * t);
						//z = data[t] * std::complex<Scalar>(std::cos(w * t), -std::sin(w * t));
						// integrate(a, b, { f(t) * e^(-i*w*t) }) <- fourier transform
						acc += Scalar(data[t]) * std::complex<Scalar>(std::cos(w * t), -std::sin(w * t));
					}
					if (scale)
					{
						return acc / std::complex<Scalar>((b - a) / 2.0);
					}
					else
						return acc;

				}*/

			/*template<typename Scalar, bool scale = false, class Vector, class InOutVector>
			void ARFourierTransform(const Vector & data, InOutVector & inout, Scalar sampleRate, size_t siz, size_t a, size_t b)
			{
				for (int i = 0; i < siz; ++i)
				{
					inout[i] = std::abs(fourierTransform<Scalar, Vector, scale>(data, inout[i], sampleRate, a, b));
				}
			}


			template<typename Scalar, bool scale = false, class Vector, class InOutVector>
			void AccFourierTransform(const Vector & data, InOutVector & inout, Scalar sampleRate, size_t siz, size_t len)
			{
				Scalar pimega = Scalar(PI) / sampleRate;
				/*for (int f = 0; f < siz; ++f)
				{
					inout[f] = inout[f] * pimega;

				}

				Concurrency::array_view<Scalar, 1> omegas(siz, inout);
				Concurrency::array<Scalar, 1> signal(len, (Scalar*)data);


				Concurrency::parallel_for_each
				(
					omegas.extent,
					[=, &signal](Concurrency::index<1> idx) restrict(amp)
					{
				
						typedef Concurrency::graphics::float_4 Vector4;
						using namespace Concurrency;
		
						Scalar omega = omegas[idx] * pimega * 2;

						Vector4 _sin(0, fast_math::sin(omega), fast_math::sin(omega * 2), fast_math::sin(omega * 3));
						Vector4 _cos(1, fast_math::cos(omega), fast_math::cos(omega * 2), fast_math::cos(omega * 3)); // sin(0) == 0, cos(0) == 1



						omega = fast_math::tanf(omega * 2);
						const Scalar z = 2 / (1 + omega * omega);
						const Vector4 c1 = z - 1;
						const Vector4 c2 = omega * z;


						Vector4 imag(0), real(0);
						Vector4 t0(0);
						for (int t = 0; t < len; t += 4)
						{
							const Vector4 input(signal[t], signal[t + 1], signal[t + 2], signal[t + 3]);
							real += input * _cos;
							imag += input * _sin;


							t0 = c1 * _cos - c2 * _sin;
							_sin = c2 * _cos + c1 * _sin;
							_cos = t0;


						}

						const Scalar scaleFactor = len / Scalar(2.0);
						Scalar reals = real.r + real.g + real.b + real.a;
						reals /= scaleFactor;
						Scalar imags = imag.r + imag.g + imag.b + imag.a;
						imags /= scaleFactor;
						omegas[idx] = fast_math::sqrtf(reals * reals + imags * imags);

						//inout[f] = std::abs(std::complex<Scalar>(real, -imag) / std::complex<Scalar>(siz / 2.0));
					
						//omegas[idx] = signal[idx % len];
					}
				);
				omegas.synchronize();

			}
			template<typename Scalar, bool scale = false, class Vector, class InOutVector>
			void AccFourierTransform2(const Vector & data, InOutVector & inout, Scalar sampleRate, size_t siz, size_t len)
			{
				Scalar pimega = Scalar(PI) / sampleRate;
				for (int f = 0; f < siz; ++f)
				{
					inout[f] = tanf(inout[f] * pimega);

				}

				Concurrency::array_view<Scalar, 1> omegas(siz, inout);
				Concurrency::array<Scalar, 1> signal(len, (Scalar*)data);


				Concurrency::parallel_for_each
					(
					omegas.extent,
					[=, &signal](Concurrency::index<1> idx) restrict(amp)
				{

					const Scalar omega = omegas[idx];
					const Scalar z = 2 / (1 + omega * omega);
					const Scalar c1 = z - 1;
					const Scalar c2 = omega * z;

					Scalar _sin(0), _cos(1); // sin(0) == 0, cos(0) == 1

					Scalar imag(0), real(0);

					for (int t = 0; t < len; t++)
					{

						real += signal[t] * _cos;
						imag += signal[t] * _sin;


						const Scalar t0 = c1 * _cos - c2 * _sin;
						const Scalar t1 = c2 * _cos + c1 * _sin;
						_cos = t0;
						_sin = t1;


					}

					const Scalar scaleFactor = len / Scalar(2.0);
					real /= scaleFactor;
					imag /= scaleFactor;
					omegas[idx] = Concurrency::fast_math::sqrtf(real * real + imag * imag);

					//inout[f] = std::abs(std::complex<Scalar>(real, -imag) / std::complex<Scalar>(siz / 2.0));

					//omegas[idx] = signal[idx % len];
				}
				);
				omegas.synchronize();

			}*/
			template<typename Scalar, bool scale = false, class Vector, class InOutVector>
			void AccFourierTransformClassic(const Vector & data, InOutVector & inout, Scalar sampleRate, size_t siz, size_t len)
			{
				Scalar pimega = Scalar(PI) / sampleRate;
				for (int f = 0; f < siz; ++f)
				{
					inout[f] = tanf(inout[f] * pimega);

				}
				return;
				for (int f = 0; f < siz; ++f)
				{

					auto const omega = inout[f];
					auto const z = 2 / (1 + omega*omega);
					auto const c1 = z - 1;
					auto const c2 = omega * z;
					Scalar _sin(0), _cos(1); // sin(0) == 0, cos(0) == 1

					Scalar imag(0), real(0);

					for (int t = 0; t < len; t++)
					{

						real += data[t] * _cos;
						imag += data[t] * _sin;


						auto const t0 = c1 * _cos - c2 * _sin;
						auto const t1 = c2 * _cos + c1 * _sin;
						_cos = t0;
						_sin = t1;


					}
					inout[f] = std::abs(std::complex<Scalar>(real, -imag) / std::complex<Scalar>(siz / 2.0));
				}

			}



			template<typename Scalar, class Vector, bool scale = false>
			std::complex<Scalar> fourierTransform(const Vector & data, Scalar frequency, Scalar sampleRate, inttype a, inttype b)
			{

				/*
				theory for increasing classic correlation precision:
				the ft of x(t) is given by correlation of sines and the signal.
				if the signal contains a sine of amplitude 1, then |e^-jwt * signal| yields one.

				The problem is the bandwith of this socalled filter. for low amount of correalation points, close frequencies may correlate to nearly the same values.
				If one could increase the precision of the correlation (so errors would give even lower correlation)...

				the problem is, that if you scale the error coefficient (say, e = e^2), sines that are 1/2 in amplitude
				would give same readings as sines that are f + bw/4 spaced apart for a certain frequency.

				however, the rms of the perfect-frequency-fitting signal would be different (lower than) the untuned sine. this may be used to give increased frequency
				seperation.

				*/



				// unpacked xmm registers
				float unpreal[4];
				float  unpimag[4];

				cpl::sse::v4sf
					xmmsin, // the next 4 sines
					xmmcos, // the next 4 cosines
					xmmsignal, // the next 4 signals
					xmmc1, xmmc2, // coefficients for oscillator
					xmmtemp1, xmmtemp2, xmmtemp3,  // temporary accumulator
					xmmreals = _mm_setzero_ps(), // accumulated reals
					xmmimags = _mm_setzero_ps(); // accumulated imaginaries

				/*
				Now we init the phases for the oscillators.
				Since we run 4 sine/cosine pairs, we shift the phase offset by omega * n
				for the n'th pair
				*/
				Scalar omega = 2 * frequency * Scalar(PI) / sampleRate;
				xmmtemp1 = _mm_setr_ps(0, omega, omega * 2, omega * 3);
				cpl::sse::sincos_ps(xmmtemp1, &xmmsin, &xmmcos);




				/*

					http://www.kvraudio.com/forum/viewtopic.php?p=5775364

				coefficient calculations:
				const omega = tan(pi*cutoff / samplerate);
				const z = 2 / (1 + omega*omega)
				c1 = z - 1
				c2 = omega * z
				xmmcos = cos(0)
				xmmsin = sin(0)
				*/
				// omega
				// note the omega * 2 here - in reality, it should be / 2 given the formulae.
				// however, since we run n oscillators phase shifted by omega * n, they actually have to run at 4x frequency
				// this is to satisfy that xmmsin(t)[n] == sin((n + t) * omega))
				Scalar real = tan(omega * 2);
				// z
				Scalar imag = Scalar(2.0) / (Scalar(1.0) + real * real);

				// c2 = omega * z
				Scalar c0 = real * imag;
				xmmc2 = _mm_load_ps1(&c0);
				//c1 = z - 1
				c0 = imag - Scalar(1.0);
				xmmc1 = _mm_load_ps1(&c0);


				/*

				run the main loop

				*/

				//_mm_prefetch((const char *)data, _MM_HINT_T0); // doesn't do much

				for (inttype t = 0; t < b; t += 4)
				{
					/* run the fourier transform here. */

					// get the next batch of floats from the input

					xmmsignal = _mm_load_ps(&data[t]);
					xmmtemp1 = _mm_mul_ps(xmmsignal, xmmcos);
					xmmtemp2 = _mm_mul_ps(xmmsignal, xmmsin);
					// real += signal[t] * cos(w * t)
					xmmreals = _mm_add_ps(xmmreals, xmmtemp1);
					// imag += signal[t] * sin(w * t)
					xmmimags = _mm_add_ps(xmmimags, xmmtemp2);

					/*
					generate the next sines and cosines:
					const t0 = c1 * cos - c2 * sin
					const t1 = c2 * cos + c1 * sin
					..
					cos = t0
					sin = t1

					*/

					xmmtemp1 = _mm_mul_ps(xmmcos, xmmc1); // c1 * cos
					xmmtemp2 = _mm_mul_ps(xmmsin, xmmc2); // c2 * sin
					xmmtemp3 = _mm_sub_ps(xmmtemp1, xmmtemp2); // final cosine, t0

					// these two lines have no dependency on the upper three, so should be pipelined
					xmmcos = _mm_mul_ps(xmmcos, xmmc2); // c2 * cos
					xmmsin = _mm_mul_ps(xmmsin, xmmc1); // c1 * sin

					xmmsin = _mm_add_ps(xmmcos, xmmsin); // final sine, t1
					xmmcos = xmmtemp3;
				}
				/*
				store the accumulated real/imag pairs into a complex
				*/

				_mm_store_ps(unpreal, xmmreals);
				_mm_store_ps(unpimag, xmmimags);
				real = 0; real += unpreal[0]; real += unpreal[1]; real += unpreal[2]; real += unpreal[3];
				imag = 0; imag += unpimag[0]; imag += unpimag[1]; imag += unpimag[2]; imag += unpimag[3];


				if (scale)
				{
					return std::complex<Scalar>(real, -imag) / std::complex<Scalar>((b - a) / 2.0);
				}
				else
					return std::complex<Scalar>(real, -imag);

			}



			template<typename Scalar, class Vector>
			void haarDWT(Vector & input, Vector & output, std::size_t size)
			{
				for (Types::fint_t length = size >> 1;; length >>= 1)
				{
					for (int i = 0; i < length; ++i) {
						Scalar sum = input[i * 2] + input[i * 2 + 1];
						Scalar difference = input[i * 2] - input[i * 2 + 1];
						output[i] = sum;
						output[length + i] = difference;
					}


					if (length == 1)
						return;

					std::memcpy(input, output, sizeof(Scalar)* length << 1);
				}

			}


			template<typename Scalar, class Vector>
				std::complex<Scalar> constantQTransform(Vector input, 
					size_t size,
					Types::fint_t kbin, 
					Scalar lowestFreq, 
					Scalar numFiltersPerOctave, 
					Scalar sampleRate)
				{
					// "spectral width" per filter
					Scalar filterWidth = std::pow(2, Scalar(1.0) / numFiltersPerOctave); // r = 2 ^ (1 / n)
					filterWidth = std::pow(filterWidth, kbin) * lowestFreq; // r ^ k * fmin == fk

					// centre frequency
					Scalar centreFrequency = std::pow(2, kbin * 1 / numFiltersPerOctave) * lowestFreq;

					// window length for bin
					Scalar windowLength = sampleRate / filterWidth; // fs / fk == N[k]
					Types::fint_t end = static_cast<Types::fint_t>(std::floor(windowLength));
					const Scalar Q = centreFrequency / filterWidth;
					// bounds check
					if (end > size)
						return 0;
					auto hammingWindow = [&](Types::fint_t n) // W[k, n]
					{
						auto const a = 25.0 / 46.0;
						return a - (1 - a) * std::cos((TAU * n) / windowLength);
					};

					std::complex<Scalar> acc;
					Scalar real, imag, sample;

					for (Types::fint_t n = 0; n < end; ++n)
					{
						sample = input[n] * hammingWindow(n);
						real = std::cos((TAU * Q * n) / windowLength) * sample;
						imag = std::sin((TAU * Q * n) / windowLength) * sample;
						acc += std::complex<Scalar>(real, -imag);

					}
					return acc / windowLength; // normalize
				}


			template<typename Scalar>
				class CFourierKernel
				{
					typedef Scalar Ty;
				
					template<class Vector>
					void compileKernels(const Vector & frequencies, std::size_t kernelSize, std::size_t signalSize, bool  applyWindow = true)
					{
						if (kernelSize && signalSize)
						{
							if (kernels)
								delete[] kernels;

							kernels = new Ty[kernelSize * signalSize * 2];

							for (Types::fint_t freq = 0; freq < kernelSize; ++freq)
							{

								Ty phaseIncrement = TAU * frequencies[freq] / sampleRate;

								for (Types::fint_t sample = 0; sample < signalSize; ++sample)
								{

									kernels[freq * kernelSize + sample] = (1 - std::cos(TAU * sample / signalSize)) * std::sin(phaseIncrement * sample);
									kernels[freq * kernelSize + sample] = (1 - std::cos(TAU * sample / signalSize)) * std::cos(phaseIncrement * sample);
								}

							}

						}

					}

					~CFourierKernel()
					{
						if (kernels)
							delete[] kernels;
					}
					Ty sampleRate;
					Ty * kernels;
				};

			typedef std::uint_fast32_t inttype;
			template<typename Scalar, class Vector, bool scale = false>
			std::complex<Scalar> fourierTransformworking2(const Vector & data, Scalar frequency, Scalar sampleRate, inttype a, inttype b)
			{


				// unpacked xmm registers
				float  unpreal[4];
				float  unpimag[4];

				cpl::sse::v4sf 
					xmmsin, // the next 4 sines
					xmmcos, // the next 4 cosines
					xmmsignal, // the next 4 signals
					xmmc1, xmmc2, // coefficients for oscillator
					xmmtemp1, xmmtemp2, xmmtemp3,  // temporary accumulator
					xmmreals = _mm_setzero_ps(), // accumulated reals
					xmmimags = _mm_setzero_ps(); // accumulated imaginaries

				/*
					Now we init the phases for the oscillators.
					Since we run 4 sine/cosine pairs, we shift the phase offset by omega * n
					for the n'th pair
				*/
				Scalar omega = 2 * frequency * Scalar(PI) / sampleRate;
				xmmtemp1 = _mm_setr_ps(0, omega, omega * 2, omega * 3);
				cpl::sse::sincos_ps(xmmtemp1, &xmmsin, &xmmcos);

				/* 
					coefficient calculations:
						const omega = tan(pi*cutoff / samplerate);
						c1 = 2/(1 + omega * omega);
						c2 = omega*c1;
						xmmcos = cos(0...n * omega)
						xmmsin = sin(0...n * omega)
				*/
				// omega
				Scalar real = tan(4 * frequency * Scalar(PI) / sampleRate);
				// z
				Scalar imag = Scalar(2.0) / (Scalar(1.0) + real * real);

				// c2 = omega * z

				xmmc1 = _mm_load_ps1(&imag);
				//c1 = z - 1
				imag = real * imag;
				xmmc2 = _mm_load_ps1(&imag);


				/*
			
					run the main loop
			
				*/
				for (inttype t = a; t < b; t += 4)
				{
					/* run the fourier transform here. */

					// get the next batch of floats from the input
					xmmsignal = *(cpl::sse::v4sf*)(data + t); // could also use _mm_setr_ps or _mm_load_ps


					xmmtemp1 = _mm_mul_ps(xmmsignal, xmmcos);
					xmmtemp2 = _mm_mul_ps(xmmsignal, xmmsin);
					// real += signal[t] * cos(w * t)
					xmmreals = _mm_add_ps(xmmreals, xmmtemp1);
					// imag += signal[t] * sin(w * t)
					xmmimags = _mm_add_ps(xmmimags, xmmtemp2);

					/* 
						generate the next sines and cosines:
							const t0 = c1 * cos - c2 * sin
							const t1 = c2 * cos + c1 * sin
							cos = t0 - cos
							sin = t1 - sin
					*/

					xmmtemp1 = _mm_mul_ps(xmmcos, xmmc1); // c1 * cos
					xmmtemp2 = _mm_mul_ps(xmmsin, xmmc2); // c2 * sin
					xmmtemp3 = _mm_sub_ps(xmmtemp1, xmmtemp2); // t0

					// these two lines have no dependency on the upper three, so should be pipelined
					xmmtemp1 = _mm_mul_ps(xmmcos, xmmc2); // c2 * cos
					xmmtemp2 = _mm_mul_ps(xmmsin, xmmc1); // c1 * sin
				
					xmmtemp1 = _mm_add_ps(xmmtemp1, xmmtemp2); // t1
					xmmcos = _mm_sub_ps(xmmtemp3, xmmcos);
					xmmsin = _mm_sub_ps(xmmtemp1, xmmsin);
				}
				/*
					store the accumulated real/imag pairs into a complex
				*/
				_mm_store_ps(unpreal, xmmreals);
				_mm_store_ps(unpimag, xmmimags);
				real = 0;
				real += unpreal[0];
				real += unpreal[1];
				real += unpreal[2];
				real += unpreal[3];
				imag = 0;
				imag += unpimag[0];
				imag += unpimag[1];
				imag += unpimag[2];
				imag += unpimag[3];


				if (scale)
				{
					return std::complex<Scalar>(real, -imag) / std::complex<Scalar>((b - a) / 2.0);
				}
				else
					return std::complex<Scalar>(real, -imag);

			}


				typedef std::uint_fast32_t inttype;
				template<typename Scalar, class Vector, bool scale = false>
				std::complex<Scalar> fourierTransform4(const Vector & data, Scalar frequency, Scalar sampleRate, inttype a, inttype b)
				{
					//std::complex<Scalar> acc = 0;
					Scalar real = 0, imag = 0;
					//std::complex<Scalar> z = 0; 
					Scalar w = frequency * 2 * PI / sampleRate;

					float unpreal[4], unpimag[4];
					//float __alignas(16) angles[4]; float __alignas(16) cosr[4]; float __alignas(16) sinr[4];
					cpl::sse::v4sf ssecosr, ssesinr, ssedata, accdata, ssereal = _mm_setzero_ps(), sseimag = _mm_setzero_ps();


					float e = 2 * w / 2.0f;
					//float e = 2 * sin(w / 2.0f);
					cpl::sse::v4sf _e = _mm_set_ps(e, e, e, e);

					cpl::sse::v4sf startphases = _mm_setr_ps(0, w, 2 * w, 3 * w);
					cpl::sse::v4sf _x1, _x2;
					cpl::sse::v4sf _y1, _y2;
					cpl::sse::sincos_ps(startphases, &_x2, &_x1);
					// x2 starts off as the cosine, while _x1 is the sine
					// maybe use x as the sine and y as the cosine? check CFastOscillator
					ssesinr = _x2;
					ssecosr = _x1;
					// x1[0 .. n] = sin(phase + n * omega)
					// x2[0 .. n] = sin(phase + PI / 2 + n * omega)
					// _cos = x2
					// _sin = x1

				



					for (inttype t = a; t < b; t += 4)
					{



						// e^(-i*w*t) == cos(w*t) + -i * sin(w * t) ?
						//real = std::cos(w * t);
						//imag = -i * std::sin(w * t);
						//z = data[t] * std::complex<Scalar>(std::cos(w * t), -std::sin(w * t));
						// integrate(a, b, { f(t) * e^(-i*w*t) }) <- fourier transform

						/*angles[0] = w * t;
						angles[1] = w * (t + 1);
						angles[2] = w * (t + 2);
						angles[3] = w * (t + 3);*/


						/*sseargs.m128_f32[0] = w * t;
						sseargs.m128_f32[1] = w * (t + 1);
						sseargs.m128_f32[2] = w * (t + 2);
						sseargs.m128_f32[3] = w * (t + 3);*/

					
						//_mm_store_ps(cosr, ssecosr);
						//_mm_store_ps(sinr, ssesinr);

					
					/*	acc += Scalar(data[t]) * std::complex<Scalar>(ssecosr.m128_f32[0], -ssesinr.m128_f32[0]);
						acc += Scalar(data[t + 1]) * std::complex<Scalar>(ssecosr.m128_f32[1], -ssesinr.m128_f32[1]);
						acc += Scalar(data[t + 2]) * std::complex<Scalar>(ssecosr.m128_f32[2], -ssesinr.m128_f32[2]);
						acc += Scalar(data[t + 3]) * std::complex<Scalar>(ssecosr.m128_f32[3], -ssesinr.m128_f32[3]);*/
					


						/*_Ty _Rightreal = (_Ty)_Right.real();
						_Ty _Rightimag = (_Ty)_Right.imag();

						_Ty _Tmp = this->_Val[_RE] * _Rightreal
							- this->_Val[_IM] * _Rightimag;
						this->_Val[_IM] = this->_Val[_RE] * _Rightimag
							+ this->_Val[_IM] * _Rightreal;
						this->_Val[_RE] = _Tmp;*/
						/*
						double real, imag;

						sample = otherreal;


						real = cos[n] * sample;
						imag = -sin[n] * sample;

						accreal += real;
						accimag += imag;
						*/

						// run the fourier transform
						ssedata = *(cpl::sse::v4sf*)(data + t);

						accdata = _mm_mul_ps(ssedata, ssecosr);

						ssereal = _mm_add_ps(ssereal, accdata);
						accdata = _mm_mul_ps(ssedata, ssesinr);


						sseimag = _mm_add_ps(sseimag, accdata);


						// generate the next sine and cosine waves
						_y1 = _mm_mul_ps(_e, ssesinr); // e * y, ssesinr = y
						_y2 = _mm_mul_ps(_e, _x1); 

						_x1 = _mm_sub_ps(_x1, _y1);
						ssesinr = _mm_add_ps(_y2, _x1);

						_y1 = _mm_mul_ps(_e, ssecosr);
						_y2 = _mm_mul_ps(_e, _x2);

						_x2 = _mm_sub_ps(_x2, _y1);
						ssecosr = _mm_add_ps(_y2, _x2);

						/*
								x = x - e*y;
								y = e*x + y;

						*/


					



					}

					_mm_store_ps(unpreal, ssereal);
					_mm_store_ps(unpimag, sseimag);
					real = 0;
					real += unpreal[0];
					real += unpreal[1];
					real += unpreal[2];
					real += unpreal[3];
					imag = 0;
					imag += unpimag[0];
					imag += unpimag[1];
					imag += unpimag[2];
					imag += unpimag[3];


					if (scale)
					{
						return std::complex<Scalar>(real, -imag) / std::complex<Scalar>((b - a) / 2.0);
					}
					else
						return std::complex<Scalar>(real, -imag);

				}


				typedef std::uint_fast32_t inttype;
				template<typename Scalar, class Vector, bool scale = false>
				std::complex<Scalar> fourierTransformworking(const Vector & data, Scalar frequency, Scalar sampleRate, inttype a, inttype b)
				{
					//std::complex<Scalar> acc = 0;
					Scalar real = 0, imag = 0;
					//std::complex<Scalar> z = 0; 
					Scalar w = frequency * 2 * PI / sampleRate;
					Scalar i = Scalar(1.0);

						float unpreal[4], unpimag[4];
					//float __alignas(16) angles[4]; float __alignas(16) cosr[4]; float __alignas(16) sinr[4];
					cpl::sse::v4sf sseargs, ssecosr, ssesinr, ssedata, accdata, ssereal = _mm_setzero_ps(), sseimag = _mm_setzero_ps();
					for (inttype t = a; t < b; t += 4)
					{

						// e^(-i*w*t) == cos(w*t) + -i * sin(w * t) ?
						//real = std::cos(w * t);
						//imag = -i * std::sin(w * t);
						//z = data[t] * std::complex<Scalar>(std::cos(w * t), -std::sin(w * t));
						// integrate(a, b, { f(t) * e^(-i*w*t) }) <- fourier transform

						/*angles[0] = w * t;
						angles[1] = w * (t + 1);
						angles[2] = w * (t + 2);
						angles[3] = w * (t + 3);*/


						/*sseargs.m128_f32[0] = w * t;
						sseargs.m128_f32[1] = w * (t + 1);
						sseargs.m128_f32[2] = w * (t + 2);
						sseargs.m128_f32[3] = w * (t + 3);*/

						sseargs = _mm_setr_ps(w * t, w * (t + 1), w * (t + 2), w * (t + 3));

						//sseargs = _mm_load_ps(angles);

						cpl::sse::sincos_ps(sseargs, &ssesinr, &ssecosr);

						//_mm_store_ps(cosr, ssecosr);
						//_mm_store_ps(sinr, ssesinr);


						/*	acc += Scalar(data[t]) * std::complex<Scalar>(ssecosr.m128_f32[0], -ssesinr.m128_f32[0]);
						acc += Scalar(data[t + 1]) * std::complex<Scalar>(ssecosr.m128_f32[1], -ssesinr.m128_f32[1]);
						acc += Scalar(data[t + 2]) * std::complex<Scalar>(ssecosr.m128_f32[2], -ssesinr.m128_f32[2]);
						acc += Scalar(data[t + 3]) * std::complex<Scalar>(ssecosr.m128_f32[3], -ssesinr.m128_f32[3]);*/



						/*_Ty _Rightreal = (_Ty)_Right.real();
						_Ty _Rightimag = (_Ty)_Right.imag();

						_Ty _Tmp = this->_Val[_RE] * _Rightreal
						- this->_Val[_IM] * _Rightimag;
						this->_Val[_IM] = this->_Val[_RE] * _Rightimag
						+ this->_Val[_IM] * _Rightreal;
						this->_Val[_RE] = _Tmp;*/
						/*
						double real, imag;

						sample = otherreal;


						real = cos[n] * sample;
						imag = -sin[n] * sample;

						accreal += real;
						accimag += imag;
						*/

						ssedata = *(cpl::sse::v4sf*)(data + t);

						accdata = _mm_mul_ps(ssedata, ssecosr);

						ssereal = _mm_add_ps(ssereal, accdata);
						accdata = _mm_mul_ps(ssedata, ssesinr);


						sseimag = _mm_add_ps(sseimag, accdata);
					}

					_mm_store_ps(unpreal, ssereal);
					_mm_store_ps(unpimag, sseimag);
					real = 0;
					real += unpreal[0];
					real += unpreal[1];
					real += unpreal[2];
					real += unpreal[3];
					imag = 0;
					imag += unpimag[0];
					imag += unpimag[1];
					imag += unpimag[2];
					imag += unpimag[3];


					if (scale)
					{
						return std::complex<Scalar>(real, -imag) / std::complex<Scalar>((b - a) / 2.0);
					}
					else
						return std::complex<Scalar>(real, -imag);

				}


		}; // dsp

	}; // cpl
#endif