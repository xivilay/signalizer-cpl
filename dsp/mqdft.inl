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

	file:mqdft.inl

		Source code for csignaltransform.h (templated inlines)

*************************************************************************************/
#ifdef _CSIGNALTRANSFORM_H

#include "../simd.h"

namespace cpl
{
	namespace dsp
	{
		template<typename Scalar, class Vector>
		static std::complex<Scalar> goertzel(const Vector & data, std::size_t size, Scalar omega)
		{
			Scalar sine, cosine, coeff, q0(0), q1(0), q2(0), real, imag;

			sine = sin(omega);
			cosine = cos(omega);
			coeff = 2.0 * cosine;

			for (Types::fint_t t = 0; t < size; t++)
			{
				q0 = coeff * q1 - q2 + data[t];
				q2 = q1;
				q1 = q0;
			}

			Scalar real = (q1 - q2 * cosine) / (size * (Scalar)0.5);
			Scalar imag = (q2 * sine) / (size * (Scalar)0.5);

			return sqrt(real*real + imag*imag);

		}
		/*********************************************************************************************/
		template<size_t channels, typename V, class Vector>
		bool CSignalTransform::mqdft_Serial(const Vector & data, std::size_t bufferLength)
		{
			using namespace cpl::simd;
			auto const * dataptr = &data[0];
			// length of the signal per channel
			unsigned length = bufferLength;
			isComputing = true;

			size_t vectorSize = suitable_container<V>::size;

			//#pragma omp parallel for
			for (int idx = 0; idx < numFilters; idx += vectorSize)
			{
				suitable_container<V> unp, unp2, unp3;

				// minLength = the minimum amount of cycles we run the major loop in
				unsigned minLength = length;
				// maxLength = the maximum (chock)
				unsigned maxLength = 0;
				for (int i = 0; i < 8; ++i)
				{
					unp[i] = static_cast<float>(std::sin(cdftData[idx + i].omega));
					unp2[i] = static_cast<float>(std::cos(cdftData[idx + i].omega));
				}

				const V sine = unp;
				const V cosine = unp2;
				const V gcoeff = cosine * set1<V>(2);

				for (int i = 0; i < 8; ++i)
				{

					const unsigned ftLength = cdftData[idx + i].qSamplesNeeded > length ?
						length : cdftData[idx + i].qSamplesNeeded;
					if (ftLength < minLength)
						minLength = ftLength;
					if (ftLength > maxLength)
						maxLength = ftLength;

					float wOmega = std::tan(float(M_PI) / (ftLength - 1));
					float z = 2.0f / (1.0f + wOmega * wOmega);

					unp[i] = z - 1.0f;
					unp2[i] = wOmega * z;
					unp3[i] = ftLength;
				}
				const V lengths = unp3;
				const V wC1 = unp;
				const V wC2 = unp2;
				V t0, ones = set1<V>(1.0f);
				V wSin = zero<V>();
				V wCos = ones;
				V input;

				V q1[channels], q2[channels], q0;
				for (int c = 0; c < channels; ++c)
				{
					q2[c] = q1[c] = zero<V>();
				}



				// process 8 dfts in pair
				unsigned t = 0;

				for (; t < minLength; t++)
				{
					t0 = ones - wCos;

					for (int c = 0; c < channels; ++c)
					{
						// load input and window it for each channel
						input = broadcast<V>(dataptr + c * length + t)/* * t0*/;
						q0 = gcoeff * q1[c] - q2[c] + input;
						q2[c] = q1[c];
						q1[c] = q0;
					}


					t0 = wCos * wC1 - wSin * wC2;
					wSin = wCos * wC2 + wSin * wC1;
					wCos = t0;



				}
				// process remainder here

				for (; t < maxLength; ++t)
				{
					V loopcount = set1<V>(t);
					// here we compare t to the length of the individual transforms.
					// if t exceeds, the mask is set to zero and will propagte through the 
					// algorithm so it wont add anything to the result, thus enabling us to run
					// different-sized transforms parallel.
					auto mask = loopcount <= lengths;

					t0 = ones - wCos;
					t0 = bool_and(t0, mask);

					for (int c = 0; c < channels; ++c)
					{
						// load input and window it for each channel
						input = broadcast<V>(dataptr + c * length + t) * t0;
						q0 = gcoeff * q1[c] - q2[c] + input;
						q2[c] = q1[c];
						q1[c] = q0;
					}



					t0 = wCos * wC1 - wSin * wC2;
					wSin = wCos * wC2 + wSin * wC1;
					wCos = t0;


				}


				// scale transform
				for (int c = 0; c < channels; ++c)
				{
					// real part
					unp = (q1[c] - q2[c] * cosine) / lengths;
					// imaginary
					unp2 = (q2[c] * sine) / lengths;

					for (int i = 0; i < vectorSize; ++i)
					{
						result[numFilters * c * 2 + idx * 2 + i * 2] = unp[i];

						result[numFilters * c * 2 + idx * 2 + i * 2 + 1] = unp2[i];
					}
				}
			}
			return true;
		}

		/*********************************************************************************************/
		template<size_t channels, typename V, class Vector>
		bool CSignalTransform::mqdft_Threaded(const Vector & data, std::size_t bufferLength)
		{
			using namespace cpl::simd;
			auto const * dataptr = &data[0];
			// length of the signal per channel
			unsigned length = bufferLength;
			isComputing = true;

			size_t vectorSize = suitable_container<V>::size;

			#pragma omp parallel for // this has a penalty when not threading
			for (int idx = 0; idx < numFilters; idx += vectorSize)
			{
				suitable_container<V> unp, unp2, unp3;

				// minLength = the minimum amount of cycles we run the major loop in
				unsigned minLength = length;
				// maxLength = the maximum (chock)
				unsigned maxLength = 0;
				for (int i = 0; i < 8; ++i)
				{
					unp[i] = cdftData[idx + i].c1;
					unp2[i] = cdftData[idx + i].c2;
				}

				const V ftC1 = unp;
				const V ftC2 = unp2;
				for (int i = 0; i < 8; ++i)
				{

					const unsigned ftLength = cdftData[idx + i].qSamplesNeeded > length ?
						length : cdftData[idx + i].qSamplesNeeded;
					if (ftLength < minLength)
						minLength = ftLength;
					if (ftLength > maxLength)
						maxLength = ftLength;

					float wOmega = tan(float(M_PI) / (ftLength - 1));
					float z = 2.0f / (1.0f + wOmega * wOmega);

					unp[i] = z - 1.0f;
					unp2[i] = wOmega * z;
					unp3[i] = static_cast<float>(ftLength);
				}
				const V lengths = unp3;
				const V wC1 = unp;
				const V wC2 = unp2;

				V ftSin = zero<V>();
				V ftCos = set1<V>(1.0f);
				V wSin = zero<V>();
				V wCos = ftCos;

				V imag[channels], real[channels], input[channels];
				for (int c = 0; c < channels; ++c)
				{
					imag[c] = real[c] = zero<V>();
				}

				V t0, t1, ones = set1<V>(1.0f);

				// process 8 dfts in pair
				unsigned t = 0;

				for (; t < minLength; t++)
				{
					t0 = ones - wCos;

					for (int c = 0; c < channels; ++c)
					{
						// load input and window it for each channel
						input[c] = broadcast<V>(dataptr + c * length + t) * t0;
						real[c] += input[c] * ftCos;
						imag[c] += input[c] * ftSin;
					}


					t0 = wCos * wC1 - wSin * wC2;
					wSin = wCos * wC2 + wSin * wC1;
					wCos = t0;



					t0 = ftCos * ftC1 - ftSin * ftC2;
					ftSin = ftCos * ftC2 + ftSin * ftC1;
					ftCos = t0;

				}
				// process remainder here
				V mask = zero<V>();
				for (; t < maxLength; ++t)
				{
					V loopcount = set1<V>(static_cast<typename scalar_of<V>::type>(t));
					// here we compare t to the length of the individual transforms.
					// if t exceeds, the mask is set to zero and will propagte through the 
					// algorithm so it wont add anything to the result, thus enabling us to run
					// different-sized transforms parallel.
					auto mask = loopcount <= lengths;

					t0 = ones - wCos;
					// overloaded function that works with bitmasks (simd-types) and booleans (scalar types)
					t0 = bool_and(t0, mask);

					for (int c = 0; c < channels; ++c)
					{
						// load input and window it for each channel
						input[c] = broadcast<V>(dataptr + c * length + t) * t0;
						real[c] += input[c] * ftCos;
						imag[c] += input[c] * ftSin;
					}



					t0 = wCos * wC1 - wSin * wC2;
					wSin = wCos * wC2 + wSin * wC1;
					wCos = t0;

					t0 = ftCos * ftC1 - ftSin * ftC2;
					ftSin = ftCos * ftC2 + ftSin * ftC1;
					ftCos = t0;

				}


				// scale transform
				for (int c = 0; c < channels; ++c)
				{
					real[c] /= lengths;
					imag[c] /= lengths;
					imag[c] *= set1<V>(-1.0f);

					unp = real[c];
					unp2 = imag[c];

					for (int i = 0; i < vectorSize; ++i)
					{
						result[numFilters * c * 2 + idx * 2 + i * 2] = unp[i];

						result[numFilters * c * 2 + idx * 2 + i * 2 + 1] = unp2[i];
					}
				}
			}
			return true;
		}


		/******************************** Parallel minimum Q DFT **************************************


		/*********************************************************************************************/
		template<size_t channels, class Vector>
		typename std::enable_if<channels == 2, bool>::type
			CSignalTransform::mqdft_Parallel(const Vector & data, std::size_t bufferLength)
		{

			const Concurrency::array<ScalarTy, 1> signal(bufferLength,
				(ScalarTy*)&data[0],
				defaultAccelerator.get_default_view()
			);

			// length of the signal per channel
			unsigned length = bufferLength;
			const auto offset = numFilters;
			GPUData gdata {*prlCdftData, *prlResult};
			isComputing = true;
			Concurrency::parallel_for_each
			(
				prlCdftData->extent, // == numFilters
				[=, &signal](Concurrency::index<1> idx) restrict(amp)
			{

				typedef Concurrency::graphics::float_4 Vector4;
				using namespace Concurrency;

				const CDFTData & ftData = gdata.cdft[idx];

				// the length of this current fourier transform,
				// where the length is calculated to give a satisfying Q
				const unsigned ftLength = ftData.qSamplesNeeded > length ? length : ftData.qSamplesNeeded;
				// the 4 magic number: since code is vectorized, we oversample the .. things
				auto const wOmega = float(TAU) / ftLength;

				const Vector4 c1 = ftData.c1;
				const Vector4 c2 = ftData.c2;
				Vector4 _sin(0.0f, ftData.sinPhases[1], ftData.sinPhases[2], ftData.sinPhases[3]);
				Vector4 _cos(1.0f, ftData.cosPhases[1], ftData.cosPhases[2], ftData.cosPhases[3]); // sin(0) == 0, cos(0) == 1



				const float omega = fast_math::tanf(wOmega * 2);

				const float z = 2.0f / (1.0f + omega * omega);

				Vector4 wC1 = z - 1.0f;
				Vector4 wC2 = omega * z;


				// create the modified coupled form which constitutes the hanning window
				// since we actually need a cosine, we simply phase-shift the sine by pi / 2
				Vector4 wCos(1.0f); // sin(pi / 2)
				Vector4 wSin(0.0f); // cos(pi / 2)
				//const Vector4 wE = 2 * fast_math::sinf(wOmega / 2.0f);
				// initiate phases for modified coupled form pair
				fast_math::sincosf(wOmega, &wSin.ref_g(), &wCos.ref_g());
				fast_math::sincosf(wOmega * 2, &wSin.ref_b(), &wCos.ref_b());
				fast_math::sincosf(wOmega * 3, &wSin.ref_a(), &wCos.ref_a());

				Vector4 leftImag(0.0f), leftReal(0.0f), rightImag(0.0f), rightReal(0.0f);
				Vector4 t0(0.0f);

				unsigned start = (length - ftLength);

				for (unsigned t = start; t < length; t += 4)
				{

					Vector4 left(signal[t], signal[t + 1], signal[t + 2], signal[t + 3]);
					Vector4 right(signal[t + length], signal[t + length + 1], signal[t + length + 2], signal[t + length + 3]);

					// window function
					left *= 1 - wCos;
					right *= 1 - wCos;
					// fourier transform
					leftReal += left * _cos;
					leftImag += left * _sin;
					rightReal += right * _cos;
					rightImag += right * _sin;
					// this is the standard rotation filter which gives a satisfying sin/cos pair
					t0 = c1 * _cos - c2 * _sin;
					_sin = c2 * _cos + c1 * _sin;
					_cos = t0;
					// same for the window
					t0 = wC1 * wCos - wC2 * wSin;
					wSin = wC2 * wCos + wC1 * wSin;
					wCos = t0;
				}
				// store accumulated filter values
				gdata.result[idx * 2] = (leftReal.r + leftReal.g + leftReal.b + leftReal.a) / (ftLength / 2);
				gdata.result[idx * 2 + 1] = -(leftImag.r + leftImag.g + leftImag.b + leftImag.a) / (ftLength / 2);

				gdata.result[idx * 2 + offset * 2] = (rightReal.r + rightReal.g + rightReal.b + rightReal.a) / (ftLength / 2);
				gdata.result[idx * 2 + 1 + offset * 2] = -(rightImag.r + rightImag.g + rightImag.b + rightImag.a) / (ftLength / 2);
			}
			);
			return true;
		}
		/******************************** Parallel minimum Q DFT **************************************


		/*********************************************************************************************/
		template<size_t channels, class Vector>
		typename std::enable_if<channels == 1, bool>::type
			CSignalTransform::mqdft_Parallel(const Vector & data, std::size_t bufferLength)
		{

			const Concurrency::array<ScalarTy, 1> signal(bufferLength,
				(ScalarTy*)&data[0],
				defaultAccelerator.get_default_view()
			);

			// length of the signal per channel
			unsigned length = bufferLength / channels;
			const auto offset = numFilters;
			GPUData gdata {*prlCdftData, *prlResult};
			isComputing = true;
			Concurrency::parallel_for_each
			(
				prlCdftData->extent, // == numFilters
				[=, &signal](Concurrency::index<1> idx) restrict(amp)
			{

				typedef Concurrency::graphics::float_4 Vector4;
				using namespace Concurrency;

				const CDFTData & ftData = gdata.cdft[idx];

				// the length of this current fourier transform,
				// where the length is calculated to give a satisfying Q
				const unsigned ftLength = ftData.qSamplesNeeded > length ? length : ftData.qSamplesNeeded;
				// the 4 magic number: since code is vectorized, we oversample the .. things
				auto const wOmega = float(TAU) / ftLength;

				const Vector4 c1 = ftData.c1;
				const Vector4 c2 = ftData.c2;
				Vector4 _sin(0.0f, ftData.sinPhases[1], ftData.sinPhases[2], ftData.sinPhases[3]);
				Vector4 _cos(1.0f, ftData.cosPhases[1], ftData.cosPhases[2], ftData.cosPhases[3]); // sin(0) == 0, cos(0) == 1



				const float omega = fast_math::tanf(wOmega * 2);

				const float z = 2.0f / (1.0f + omega * omega);

				Vector4 wC1 = z - 1.0f;
				Vector4 wC2 = omega * z;


				// create the modified coupled form which constitutes the hanning window
				// since we actually need a cosine, we simply phase-shift the sine by pi / 2
				Vector4 wCos(1.0f); // sin(pi / 2)
				Vector4 wSin(0.0f); // cos(pi / 2)
				//const Vector4 wE = 2 * fast_math::sinf(wOmega / 2.0f);
				// initiate phases for modified coupled form pair
				fast_math::sincosf(wOmega, &wSin.ref_g(), &wCos.ref_g());
				fast_math::sincosf(wOmega * 2, &wSin.ref_b(), &wCos.ref_b());
				fast_math::sincosf(wOmega * 3, &wSin.ref_a(), &wCos.ref_a());

				Vector4 imag(0.0f), real(0.0f);
				Vector4 t0(0.0f);

				unsigned start = (length - ftLength);

				for (unsigned t = start; t < length; t += 4)
				{

					Vector4 input(signal[t], signal[t + 1], signal[t + 2], signal[t + 3]);
					// window function
					input *= 1 - wCos;
					// fourier transform
					real += input * _cos;
					imag += input * _sin;
					// this is the standard rotation filter which gives a satisfying sin/cos pair
					t0 = c1 * _cos - c2 * _sin;
					_sin = c2 * _cos + c1 * _sin;
					_cos = t0;
					// same for the window
					t0 = wC1 * wCos - wC2 * wSin;
					wSin = wC2 * wCos + wC1 * wSin;
					wCos = t0;
				}
				// store accumulated filter values
				gdata.result[idx * 2] = (real.r + real.g + real.b + real.a) / (ftLength / 2);
				gdata.result[idx * 2 + 1] = -(imag.r + imag.g + imag.b + imag.a) / (ftLength / 2);

			}
			);
			return true;
		}
	}
};

#endif