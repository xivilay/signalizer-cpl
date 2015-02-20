/*************************************************************************************
 
 cpl - cross-platform library - v. 0.1.0.
 
 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
 
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
 
 file:CSignalTransform.inl
 
	Source code for csignaltransform.h (templated inlines)
 
 *************************************************************************************/
#ifdef _CSIGNALTRANSFORM_H

	#include "../ffts.h"
	#include "../Misc.h"
	#include "../SysStats.h"
	#include "../mathext.h"

	namespace cpl
	{
		namespace dsp
		{
			/*********************************************************************************************

				class CSignalTransform

			*********************************************************************************************/
			template<class Vector>
				void CSignalTransform::setKernelData(const Vector & freq, std::size_t size)
				{

					freqs = freq;

					numFilters = size;
					totalDataSize = numFilters * numChannels * 2;
					cdftData.resize(size);

					auto const piMega = PI / sampleRate;

					for (unsigned i = 0; i < (size - 1); ++i)
					{
						// t0 = floor(sampleRate / bandWidth)
						unsigned bandWidth = static_cast<unsigned>(sampleRate / (freq[i + 1] - freq[i]));
					
						// numSamples = t0 - (t0 % 4)
						// the 4 magic number corrosponds to the minimum amount of samples needed to vectorize the code
						unsigned numSamples = bandWidth - (bandWidth & 0x7);
						// avoid zero
						if (!numSamples)
							numSamples = 8;

						cdftData[i].qSamplesNeeded = numSamples;
						double omega = freq[i] * piMega * 2;

						for (int z = 0; z < 8; ++z)
						{
							cdftData[i].sinPhases[z] = static_cast<float>(std::sin(omega * z));
							cdftData[i].cosPhases[z] = static_cast<float>(std::cos(omega * z));
						}

						omega = std::tan(omega * oversamplingFactor / 2.0);
						auto const z = 2.0 / (1.0 + omega * omega);

						cdftData[i].c1 = static_cast<float>(z - 1.0);
						cdftData[i].c2 = static_cast<float>(omega * z);

					}
					// last iteration, special case: we need to interpolate the bandWidth
					{
						unsigned i = size - 1;

						// t0 = floor(sampleRate / bandWidth)
						unsigned bandWidth = static_cast<unsigned>(sampleRate / (freq[i] - freq[i - 1]));

						// numSamples = t0 - (t0 % 4)
						// the 4 magic number corrosponds to the minimum amount of samples needed to vectorize the code
						unsigned numSamples = bandWidth - (bandWidth & 0x7);
						// avoid zero
						if (!numSamples)
							numSamples = 8;

						cdftData[i].qSamplesNeeded = numSamples;
						double omega = freq[i] * piMega * 2;

						for (int z = 0; z < 8; ++z)
						{
							cdftData[i].sinPhases[z] = static_cast<float>(std::sin(omega * z));
							cdftData[i].cosPhases[z] = static_cast<float>(std::cos(omega * z));
						}

						omega = std::tan(omega * oversamplingFactor / 2.0);
						auto const z = 2.0 / (1.0 + omega * omega);

						cdftData[i].c1 = static_cast<float>(z - 1.0);
						cdftData[i].c2 = static_cast<float>(omega * z);
					}

					copyMemoryToAccelerator();

				}

			void CSignalTransform::copyMemoryToAccelerator()
			{
				// copy memory onto gpu
				if (flags & Flags::accelerated)
				{
					#ifdef CPL_AMP_SUPPORT
						if (!prlResult || prlResult->extent.size() != totalDataSize)
						{
							prlResult.reset
							(
								new Concurrency::array<ScalarTy, 1>(totalDataSize, defaultAccelerator.get_default_view())
							);
						}
						prlCdftData.reset
						(
							new Concurrency::array<CDFTData, 1>(numFilters, cdftData, defaultAccelerator.get_default_view())
						);
					#endif
				}
				else
				{
					// remove references
					prlResult.reset();
					prlCdftData.reset();
				}
			}

			/*********************************************************************************************/
			void CSignalTransform::fft(double * data, std::size_t fftSize)
			{
				cpl::signaldust::DustFFT_fwdDa(data, fftSize);
			}
			/*********************************************************************************************/
			void CSignalTransform::selectAppropriateAccelerator()
			{
				if (flags & Flags::accelerated)
				{
					#ifdef _CPL_AMP_SUPPORT
						std::size_t maxMem = 0;
						int z = -1;
						auto const & accs = Concurrency::accelerator::get_all();
						for (int i = 0; i < accs.size(); ++i)
						{
							if (accs[i].get_dedicated_memory() > maxMem)
							{
								maxMem = accs[i].get_dedicated_memory();
								z = i;
							}
						}
						if (z == -1)
						{
							/* apparantly no devices with dedicated memory, select default */
							defaultAccelerator = Concurrency::accelerator();
						}
						else
						{
							defaultAccelerator = accs.at(z);
						}
					#endif
				}
			}

			/*********************************************************************************************/
			CSignalTransform::ResultData CSignalTransform::getTransformResult()
			{
				ResultData ret(result, numFilters * 2); // 2 = complex size

				if (isComputing)
				{
					if (flags & Flags::accelerated)
					{
						#ifdef _CPL_AMP_SUPPORT
							prlResult->get_accelerator_view().wait();
							Concurrency::copy(*prlResult, result.begin());
						#endif
					}
					isComputing = false;
				}
				return ret;
			}
			/*********************************************************************************************/
			void CSignalTransform::ensureBufferSizes(std::size_t amountOfChannels)
			{
				numChannels = amountOfChannels;
				totalDataSize = numChannels * numFilters * 2;
				result.resize(totalDataSize);
				if (prlResult->get_extent().size() != totalDataSize)
				{
					prlResult.reset(new Concurrency::array<ScalarTy, 1>
						(totalDataSize, defaultAccelerator.get_default_view()));
				}
			}
			/*********************************************************************************************/
			void CSignalTransform::setFlags(int f)
			{
				if (flags != f)
				{
					flags = static_cast<Flags>(f);


					if (flags & Flags::accelerated)
					{
						oversamplingFactor = 4;
						copyMemoryToAccelerator();
					}
					else
					{
						using namespace cpl::SysStats;
						auto const & cpuID = cpl::SysStats::CProcessorInfo::instance();

						if (cpuID.test(CProcessorInfo::AVX2) || cpuID.test(CProcessorInfo::AVX))
							oversamplingFactor = 8;
						else if (cpuID.test(CProcessorInfo::SSE2))
							oversamplingFactor = 4;
						else
							oversamplingFactor = 1;
					}
				}
			}
			/*********************************************************************************************

				This is the entry-point and public interface to the minimum Q discrete fourier transform.

			/*********************************************************************************************/
			template<size_t channels, class Vector>
				bool CSignalTransform::mqdft(const Vector & data, std::size_t bufferLength)
				{
					/*
						Here we take advantage of specific instruction sets, 
						and dispatch correctly at runtime

						First, check if we should use a massively parallel platform	
					*/
					if (flags & Flags::accelerated)
						return mqdft_Parallel<channels>(data, bufferLength);

					using namespace cpl::SysStats;

					auto const & cpuID = cpl::SysStats::CProcessorInfo::instance();

					if (cpuID.test(CProcessorInfo::AVX2))
						return mqdft_FMA<channels>(data, bufferLength);
					else if (cpuID.test(CProcessorInfo::AVX))
						return mqdft_8Vector<channels>(data, bufferLength);
					else if (cpuID.test(CProcessorInfo::SSE2))
						return mqdft_4Vector<channels>(data, bufferLength);
					else 
						return mqdft_Scalar<channels>(data, bufferLength);

				}
			/******************************** Scalar minimum Q DFT **************************************

				This is the reference algorithm, completly scalar and mostly un-optimized

			/*********************************************************************************************/
			template<size_t channels, class Vector>
				typename std::enable_if<channels == 2, bool>::type
					CSignalTransform::mqdft_Scalar(const Vector & data, std::size_t bufferLength)
				{
					ensureBufferSizes(2);

					Types::fint_t length = bufferLength / 2;

					for (Types::fint_t filter = 0; filter < numFilters; ++filter)
					{
						const unsigned ftLength = cdftData[filter].qSamplesNeeded > length ?
						length : cdftData[filter].qSamplesNeeded;

						Types::fint_t offset = (length - ftLength);

						float ftSin = 0.0f;
						float ftCos = 1.0f;
						float wSin = 0.0f;
						float wCos = 1.0f;

						float omega = tanf(float(PI) / ftLength);
						float z = 2.0f / (1.0f + omega * omega);
						const float wC1 = z - 1.0f;
						const float wC2 = omega * z;

						omega = tanf(freqs[filter] * float(PI) / (float)sampleRate);
						z = 2.0f / (1.0f + omega * omega);
						const float ftC1 = z - 1.0f;
						const float ftC2 = omega * z;

						float t0, lReal = 0.0f, lImag = 0.0f, rReal = 0.0f, rImag = 0.0f;

						for (Types::fint_t t = offset; t < length; ++t)
						{
							t0 = data[t] * (1 - wCos);
							lReal += ftCos * t0;
							lImag += ftSin * t0;
							t0 = data[t + length] * (1 - wCos);
							rReal += ftCos * t0;
							rImag += ftSin * t0;

							t0 = ftC1 * ftCos - ftC2 * ftSin;
							ftSin = ftC2 * ftCos + ftC1 * ftSin;
							ftCos = t0;

							t0 = wC1 * wCos - wC2 * wSin;
							wSin = wC2 * wCos + wC1 * wSin;
							wCos = t0;
						}


						result[filter * 2] = lReal / (ftLength / 2.0f);
						result[filter * 2 + 1] = -lImag / (ftLength / 2.0f);
						result[filter * 2 + numFilters * 2] = rReal / (ftLength / 2.0f);
						result[filter * 2 + numFilters * 2 + 1] = -rImag / (ftLength / 2.0f);
					}

					return true;
				}
			/*********************************************************************************************/
			template<size_t channels, class Vector>
				typename std::enable_if<channels == 1, bool>::type
					CSignalTransform::mqdft_Scalar(const Vector & data, std::size_t bufferLength)
				{
					ensureBufferSizes(2);

					Types::fint_t length = bufferLength / 2;

					for (Types::fint_t filter = 0; filter < numFilters; ++filter)
					{
						const unsigned ftLength = cdftData[filter].qSamplesNeeded > length ?
						length : oscdata[filter].qSamplesNeeded;

						Types::fint_t offset = (length - ftLength);

						float ftSin = 0.0f;
						float ftCos = 1.0f;
						float wSin = 0.0f;
						float wCos = 1.0f;

						float omega = tanf(float(PI) / ftLength);
						float z = 2.0f / (1.0f + omega * omega);
						const float wC1 = z - 1.0f;
						const float wC2 = omega * z;

						omega = tanf(freqs[filter] * float(PI) / (float)sampleRate);
						z = 2.0f / (1.0f + omega * omega);
						const float ftC1 = z - 1.0f;
						const float ftC2 = omega * z;

						float t0, real = 0.0f, imag = 0.0f;

						for (Types::fint_t t = offset; t < length; ++t)
						{
							t0 = data[t] * (1 - wCos);
							real += ftCos * t0;
							imag += ftSin * t0;

							t0 = ftC1 * ftCos - ftC2 * ftSin;
							ftSin = ftC2 * ftCos + ftC1 * ftSin;
							ftCos = t0;

							t0 = wC1 * wCos - wC2 * wSin;
							wSin = wC2 * wCos + wC1 * wSin;
							wCos = t0;
						}

						result[filter * 2] = lReal / (ftLength / 2.0f);
						result[filter * 2 + 1] = -lImag / (ftLength / 2.0f);
					}

					return true;
				}
			/******************************** Vector minimum Q DFT **************************************

				Unused algorithm, provided to show how 4-float (software) vector implementation would
				look like.

			/*********************************************************************************************/
			template<size_t channels, class Vector>
				typename std::enable_if<channels == 2, bool>::type
					CSignalTransform::mqdft_Vector(const Vector & data, std::size_t bufferLength)
				{

					// handles switching between different amount of channels
					ensureBufferSizes(channels);

					// length of the signal per channel
					unsigned length = bufferLength / channels;
					const auto offset = numFilters;
	
					isComputing = true;
					for (int idx = 0; idx < numFilters; ++idx)
					{
						typedef Concurrency::graphics::float_4 Vector4;
						using namespace Concurrency;

						const CDFTData & ftData = cdftData[idx];

						// the length of this current fourier transform,
						// where the length is calculated to give a satisfying Q
						const unsigned ftLength = ftData.qSamplesNeeded > length ? length : ftData.qSamplesNeeded;
						// the 4 magic number: since code is vectorized, we oversample the .. things
						auto const wOmega =  float(TAU) / ftLength;

						const Vector4 c1 = ftData.c1;
						const Vector4 c2 = ftData.c2;
						Vector4 _sin(0, ftData.sinPhases[1], ftData.sinPhases[2], ftData.sinPhases[3]);
						Vector4 _cos(1, ftData.cosPhases[1], ftData.cosPhases[2], ftData.cosPhases[3]); // sin(0) == 0, cos(0) == 1

						const float omega = tan(wOmega * 2);

						const float z = 2.0f / (1.0f + omega * omega);

						Vector4 wC1 = z - 1.0f;
						Vector4 wC2 = omega * z;


						// create the modified coupled form which constitutes the hanning window
						// since we actually need a cosine, we simply phase-shift the sine by pi / 2
						Vector4 wCos(1.0f); // sin(pi / 2)
						Vector4 wSin(0.0f); // cos(pi / 2)
						//const Vector4 wE = 2 * fast_math::sinf(wOmega / 2.0f);
						// initiate phases for modified coupled form pair

						cpl::sse::v4sf phases = _mm_setr_ps(0, wOmega, wOmega * 2, wOmega * 3);
						cpl::sse::sincos_ps(phases, (cpl::sse::v4sf*)&wSin, (cpl::sse::v4sf*)&wCos);

						/*fast_math::sincosf(wOmega, &wSin.ref_g(), &wCos.ref_g());
						fast_math::sincosf(wOmega * 2, &wSin.ref_b(), &wCos.ref_b());
						fast_math::sincosf(wOmega * 3, &wSin.ref_a(), &wCos.ref_a());*/

						Vector4 leftImag(0.0f), leftReal(0.0f), rightImag(0.0f), rightReal(0.0f);
						Vector4 t0(0.0f);
	
						unsigned start = (length - ftLength);

						for (int t = start; t < length; t += 4)
						{

							Vector4 left(data[t], data[t + 1], data[t + 2], data[t + 3]);
							Vector4 right(data[t + length], data[t + length + 1], data[t + length + 2], data[t + length + 3]);

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
						result[idx * 2] = (leftReal.r + leftReal.g + leftReal.b + leftReal.a) / (ftLength / 2);
						result[idx * 2 + 1] = -(leftImag.r + leftImag.g + leftImag.b + leftImag.a) / (ftLength / 2);

						result[idx * 2 + offset * 2] = (rightReal.r + rightReal.g + rightReal.b + rightReal.a) / (ftLength / 2);
						result[idx * 2 + 1 + offset * 2] = -(rightImag.r + rightImag.g + rightImag.b + rightImag.a) / (ftLength / 2);
					}
					return true;
				}

			/******************************** 8 Vector AVX1 minimum Q DFT **************************************


			/*********************************************************************************************/
			template<size_t channels, class Vector>
				typename std::enable_if<channels == 2, bool>::type
					CSignalTransform::mqdft_8Vector(const Vector & data, std::size_t bufferLength)
				{

					// handles switching between different amount of channels
					ensureBufferSizes(channels);

					float __alignas(32) unp[8];

					auto const * dataptr = &data[0];
					// length of the signal per channel
					unsigned length = bufferLength / channels;
					const auto offset = numFilters;
	
					isComputing = true;

					for (int idx = 0; idx < numFilters; ++idx)
					{
						typedef __m128 Vector4;
						typedef __m256 Vector8;
						using namespace Concurrency;

						const CDFTData & ftData = cdftData[idx];

						// the length of this current fourier transform,
						// where the length is calculated to give a satisfying Q
						const unsigned ftLength = ftData.qSamplesNeeded > length ? length : ftData.qSamplesNeeded;
						// the 4 magic number: since code is vectorized, we oversample the .. things
						auto const wOmega =  float(TAU) / ftLength;

						const Vector8 ftC1 = _mm256_set1_ps(ftData.c1);
						const Vector8 ftC2 = _mm256_set1_ps(ftData.c2);
						Vector8 ftSin = _mm256_load_ps(ftData.sinPhases);
						Vector8 ftCos = _mm256_load_ps(ftData.cosPhases);


						const float omega = tan(wOmega * 4); // 4 = 8x oversampling

						const float z = 2.0f / (1.0f + omega * omega);

						Vector8 wC1 = _mm256_set1_ps(z - 1.0f);
						Vector8 wC2 = _mm256_set1_ps(omega * z);


						// create the modified coupled form which constitutes the hanning window
						// since we actually need a cosine, we simply phase-shift the sine by pi / 2
						Vector8 wCos; // sin(pi / 2)
						Vector8 wSin; // cos(pi / 2)
						//const Vector4 wE = 2 * fast_math::sinf(wOmega / 2.0f);
						// initiate phases for modified coupled form pair

						Vector4 t1, t2;

						Vector4 phases = _mm_setr_ps(0, wOmega, wOmega * 2, wOmega * 3);
						cpl::sse::sincos_ps(phases, &t1, &t2);

						wCos = _mm256_castps128_ps256(t2);
						wSin = _mm256_castps128_ps256(t1);

						phases = _mm_setr_ps(wOmega * 4, wOmega * 5, wOmega * 6, wOmega * 7);
						cpl::sse::sincos_ps(phases, &t1, &t2);

						wCos = _mm256_insertf128_ps(wCos, t2, 1);
						wSin = _mm256_insertf128_ps(wSin, t1, 1);

						

						/*fast_math::sincosf(wOmega, &wSin.ref_g(), &wCos.ref_g());
						fast_math::sincosf(wOmega * 2, &wSin.ref_b(), &wCos.ref_b());
						fast_math::sincosf(wOmega * 3, &wSin.ref_a(), &wCos.ref_a());*/

						Vector8 leftImag = _mm256_setzero_ps(), leftReal = _mm256_setzero_ps(),
							rightImag = _mm256_setzero_ps(), rightReal = _mm256_setzero_ps();
						Vector8 t0, ones = _mm256_set1_ps(1.0f);
						
#ifdef _CDFT_PACKED
						unsigned stop = length * 2;
						unsigned start = (length - ftLength) * 2;

						
						for (int t = start; t < stop; t += 8)
						{

							Vector4 left = _mm_load_ps(data + t);
							Vector4 right = _mm_load_ps(data + t + 4);
#else
						unsigned start = (length - ftLength);

						for (int t = start; t < length; t += 8)
						{

							Vector8 left = _mm256_load_ps(dataptr + t);
							Vector8 right = _mm256_load_ps(dataptr + t + length);

#endif
							// window function
							t0 = _mm256_sub_ps(ones, wCos);
							left = _mm256_mul_ps(t0, left);
							right = _mm256_mul_ps(t0, right);

							// fourier transform

							

							leftReal = _mm256_add_ps(leftReal, _mm256_mul_ps(left, ftCos));
							//leftReal = _mm256_fmadd_ps(left, ftCos, leftReal);
							leftImag = _mm256_add_ps(leftImag, _mm256_mul_ps(left, ftSin));
							//leftImag = _mm256_fmadd_ps(leftImag, ftSin, leftImag);
							rightReal = _mm256_add_ps(rightReal, _mm256_mul_ps(right, ftCos));
							//rightReal = _mm256_fmadd_ps(right, ftCos, rightReal);
							rightImag = _mm256_add_ps(rightImag, _mm256_mul_ps(right, ftSin));
							//rightImag = _mm256_fmadd_ps(right, ftSin, rightImag);

							// this is the standard rotation filter which gives a satisfying sin/cos pair

							t0 = _mm256_sub_ps(_mm256_mul_ps(ftCos, ftC1), _mm256_mul_ps(ftSin, ftC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							ftCos = _mm256_mul_ps(ftCos, ftC2); // c2 * cos
							ftSin = _mm256_mul_ps(ftSin, ftC1); // c1 * sin

							ftSin = _mm256_add_ps(ftCos, ftSin); // final sine, t1
							ftCos = t0;

							// same for the window
							t0 = _mm256_sub_ps(_mm256_mul_ps(wCos, wC1), _mm256_mul_ps(wSin, wC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							wCos = _mm256_mul_ps(wCos, wC2); // c2 * cos
							wSin = _mm256_mul_ps(wSin, wC1); // c1 * sin

							wSin = _mm256_add_ps(wCos, wSin); // final sine, t1
							wCos = t0;

						}
						// store accumulated filter values

						_mm256_store_ps(unp, leftReal);
						result[idx * 2] = Math::compileVector(unp) / (ftLength / 2);

						_mm256_store_ps(unp, leftImag);
						result[idx * 2 + 1] = -Math::compileVector(unp) / (ftLength / 2);

						_mm256_store_ps(unp, rightReal);
						result[idx * 2 + offset * 2] = Math::compileVector(unp) / (ftLength / 2);

						_mm256_store_ps(unp, rightImag);
						result[idx * 2 + 1 + offset * 2] = -Math::compileVector(unp) / (ftLength / 2);
					}
					return true;
				}
			/*********************************************************************************************/
			template<size_t channels, class Vector>
				typename std::enable_if<channels == 1, bool>::type
					CSignalTransform::mqdft_8Vector(const Vector & data, std::size_t bufferLength)
				{

					// handles switching between different amount of channels
					ensureBufferSizes(channels);

					float __alignas(32) unp[8];

					auto const * dataptr = &data[0];
					// length of the signal per channel
					unsigned length = bufferLength / channels;
					const auto offset = numFilters;
	
					isComputing = true;

					for (int idx = 0; idx < numFilters; ++idx)
					{
						typedef __m128 Vector4;
						typedef __m256 Vector8;
						using namespace Concurrency;

						const CDFTData & ftData = cdftData[idx];

						// the length of this current fourier transform,
						// where the length is calculated to give a satisfying Q
						const unsigned ftLength = ftData.qSamplesNeeded > length ? length : ftData.qSamplesNeeded;
						// the 4 magic number: since code is vectorized, we oversample the .. things
						auto const wOmega =  float(TAU) / ftLength;

						const Vector8 ftC1 = _mm256_set1_ps(ftData.c1);
						const Vector8 ftC2 = _mm256_set1_ps(ftData.c2);
						Vector8 ftSin = _mm256_load_ps(ftData.sinPhases);
						Vector8 ftCos = _mm256_load_ps(ftData.cosPhases);


						const float omega = tan(wOmega * 4); // 4 = 8x oversampling

						const float z = 2.0f / (1.0f + omega * omega);

						Vector8 wC1 = _mm256_set1_ps(z - 1.0f);
						Vector8 wC2 = _mm256_set1_ps(omega * z);


						// create the modified coupled form which constitutes the hanning window
						// since we actually need a cosine, we simply phase-shift the sine by pi / 2
						Vector8 wCos; // sin(pi / 2)
						Vector8 wSin; // cos(pi / 2)
						//const Vector4 wE = 2 * fast_math::sinf(wOmega / 2.0f);
						// initiate phases for modified coupled form pair

						Vector4 t1, t2;

						Vector4 phases = _mm_setr_ps(0, wOmega, wOmega * 2, wOmega * 3);
						cpl::sse::sincos_ps(phases, &t1, &t2);

						wCos = _mm256_castps128_ps256(t2);
						wSin = _mm256_castps128_ps256(t1);

						phases = _mm_setr_ps(wOmega * 4, wOmega * 5, wOmega * 6, wOmega * 7);
						cpl::sse::sincos_ps(phases, &t1, &t2);

						wCos = _mm256_insertf128_ps(wCos, t2, 1);
						wSin = _mm256_insertf128_ps(wSin, t1, 1);

						

						/*fast_math::sincosf(wOmega, &wSin.ref_g(), &wCos.ref_g());
						fast_math::sincosf(wOmega * 2, &wSin.ref_b(), &wCos.ref_b());
						fast_math::sincosf(wOmega * 3, &wSin.ref_a(), &wCos.ref_a());*/

						Vector8 imag = _mm256_setzero_ps(), real = _mm256_setzero_ps();
						Vector8 t0, ones = _mm256_set1_ps(1.0f);
						
#ifdef _CDFT_PACKED
						unsigned stop = length * 2;
						unsigned start = (length - ftLength) * 2;

						
						for (int t = start; t < stop; t += 8)
						{

							Vector4 left = _mm_load_ps(data + t);
							Vector4 right = _mm_load_ps(data + t + 4);
#else
						unsigned start = (length - ftLength);

						for (int t = start; t < length; t += 8)
						{

							Vector8 input = _mm256_load_ps(dataptr + t);

#endif
							// window function
							t0 = _mm256_sub_ps(ones, wCos);
							input = _mm256_mul_ps(t0, input);

							// fourier transform

							

							real = _mm256_add_ps(real, _mm256_mul_ps(input, ftCos));
							//leftReal = _mm256_fmadd_ps(left, ftCos, leftReal);
							imag = _mm256_add_ps(imag, _mm256_mul_ps(input, ftSin));
							//leftImag = _mm256_fmadd_ps(leftImag, ftSin, leftImag);

							// this is the standard rotation filter which gives a satisfying sin/cos pair

							t0 = _mm256_sub_ps(_mm256_mul_ps(ftCos, ftC1), _mm256_mul_ps(ftSin, ftC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							ftCos = _mm256_mul_ps(ftCos, ftC2); // c2 * cos
							ftSin = _mm256_mul_ps(ftSin, ftC1); // c1 * sin

							ftSin = _mm256_add_ps(ftCos, ftSin); // final sine, t1
							ftCos = t0;

							// same for the window
							t0 = _mm256_sub_ps(_mm256_mul_ps(wCos, wC1), _mm256_mul_ps(wSin, wC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							wCos = _mm256_mul_ps(wCos, wC2); // c2 * cos
							wSin = _mm256_mul_ps(wSin, wC1); // c1 * sin

							wSin = _mm256_add_ps(wCos, wSin); // final sine, t1
							wCos = t0;

						}
						// store accumulated filter values

						_mm256_store_ps(unp, real);
						result[idx * 2] = compileVector(unp) / (ftLength / 2);
						// remember the sign on the imaginary part!!
						_mm256_store_ps(unp, imag);
						result[idx * 2 + 1] = -compileVector(unp) / (ftLength / 2);

					}
					return true;
				}
			/******************************** 4 Vector SSE minimum Q DFT **************************************


			/*********************************************************************************************/
			template<size_t channels, class Vector>
				typename std::enable_if<channels == 2, bool>::type
					CSignalTransform::mqdft_4Vector(const Vector & data, std::size_t bufferLength)
				{

					// handles switching between different amount of channels
					ensureBufferSizes(channels);

					float __alignas(16) unp[4];

					// length of the signal per channel
					unsigned length = bufferLength / channels;
					const auto offset = numFilters;
	
					auto const * dataptr = &data[0];

					isComputing = true;
					for (int idx = 0; idx < numFilters; ++idx)
					{
						typedef __m128 Vector4;
						using namespace Concurrency;

						const CDFTData & ftData = cdftData[idx];

						// the length of this current fourier transform,
						// where the length is calculated to give a satisfying Q
						const unsigned ftLength = ftData.qSamplesNeeded > length ? length : ftData.qSamplesNeeded;
						// the 4 magic number: since code is vectorized, we oversample the .. things
						auto const wOmega =  float(TAU) / ftLength;

						const Vector4 ftC1 = _mm_set1_ps(ftData.c1);
						const Vector4 ftC2 = _mm_set1_ps(ftData.c2);
						Vector4 ftSin = _mm_loadr_ps(ftData.sinPhases);
						Vector4 ftCos = _mm_loadr_ps(ftData.cosPhases);


						const float omega = tan(wOmega * 2);

						const float z = 2.0f / (1.0f + omega * omega);

						Vector4 wC1 = _mm_set1_ps(z - 1.0f);
						Vector4 wC2 = _mm_set1_ps(omega * z);


						// create the modified coupled form which constitutes the hanning window
						// since we actually need a cosine, we simply phase-shift the sine by pi / 2
						Vector4 wCos; // sin(pi / 2)
						Vector4 wSin; // cos(pi / 2)
						//const Vector4 wE = 2 * fast_math::sinf(wOmega / 2.0f);
						// initiate phases for modified coupled form pair

						cpl::sse::v4sf phases = _mm_setr_ps(0, wOmega, wOmega * 2, wOmega * 3);
						cpl::sse::sincos_ps(phases, &wSin, &wCos);

						/*fast_math::sincosf(wOmega, &wSin.ref_g(), &wCos.ref_g());
						fast_math::sincosf(wOmega * 2, &wSin.ref_b(), &wCos.ref_b());
						fast_math::sincosf(wOmega * 3, &wSin.ref_a(), &wCos.ref_a());*/

						Vector4 leftImag = _mm_setzero_ps(), leftReal = _mm_setzero_ps(),
							rightImag = _mm_setzero_ps(), rightReal = _mm_setzero_ps();
						Vector4 t0, ones = _mm_set1_ps(1.0f);

#ifdef _CDFT_PACKED
						unsigned stop = length * 2;
						unsigned start = (length - ftLength) * 2;

						
						for (int t = start; t < stop; t += 8)
						{

							Vector4 left = _mm_load_ps(data + t);
							Vector4 right = _mm_load_ps(data + t + 4);
#else
						unsigned start = (length - ftLength);

						for (int t = start; t < length; t += 4)
						{

							Vector4 left = _mm_load_ps(dataptr +t);
							Vector4 right = _mm_load_ps(dataptr + t + length);

#endif
							// window function
							t0 = _mm_sub_ps(ones, wCos);
							left = _mm_mul_ps(t0, left);
							right = _mm_mul_ps(t0, right);

							// fourier transform


							leftReal = _mm_add_ps(leftReal, _mm_mul_ps(left, ftCos));
							leftImag = _mm_add_ps(leftImag, _mm_mul_ps(left, ftSin));

							rightReal = _mm_add_ps(rightReal, _mm_mul_ps(right, ftCos));

							rightImag = _mm_add_ps(rightImag, _mm_mul_ps(right, ftSin));


							// this is the standard rotation filter which gives a satisfying sin/cos pair

							t0 = _mm_sub_ps(_mm_mul_ps(ftCos, ftC1), _mm_mul_ps(ftSin, ftC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							ftCos = _mm_mul_ps(ftCos, ftC2); // c2 * cos
							ftSin = _mm_mul_ps(ftSin, ftC1); // c1 * sin

							ftSin = _mm_add_ps(ftCos, ftSin); // final sine, t1
							ftCos = t0;

							// same for the window
							t0 = _mm_sub_ps(_mm_mul_ps(wCos, wC1), _mm_mul_ps(wSin, wC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							wCos = _mm_mul_ps(wCos, wC2); // c2 * cos
							wSin = _mm_mul_ps(wSin, wC1); // c1 * sin

							wSin = _mm_add_ps(wCos, wSin); // final sine, t1
							wCos = t0;


				
						}
						// store accumulated filter values

						_mm_store_ps(unp, leftReal);
						result[idx * 2] = Math::compileVector(unp) / (ftLength / 2);

						_mm_store_ps(unp, leftImag);
						result[idx * 2 + 1] = -Math::compileVector(unp) / (ftLength / 2);

						_mm_store_ps(unp, rightReal);
						result[idx * 2 + offset * 2] = Math::compileVector(unp) / (ftLength / 2);

						_mm_store_ps(unp, rightImag);
						result[idx * 2 + 1 + offset * 2] = -Math:: compileVector(unp) / (ftLength / 2);
					}
					return true;
				}

			/*********************************************************************************************/
			template<size_t channels, class Vector>
				typename std::enable_if<channels == 1, bool>::type
					CSignalTransform::mqdft_4Vector(const Vector & data, std::size_t bufferLength)
				{

					// handles switching between different amount of channels
					ensureBufferSizes(channels);

					float __alignas(16) unp[4];

					// length of the signal per channel
					unsigned length = bufferLength / channels;
					const auto offset = numFilters;
	
					auto const * dataptr = &data[0];

					isComputing = true;
					for (int idx = 0; idx < numFilters; ++idx)
					{
						typedef __m128 Vector4;
						using namespace Concurrency;

						const CDFTData & ftData = cdftData[idx];

						// the length of this current fourier transform,
						// where the length is calculated to give a satisfying Q
						const unsigned ftLength = ftData.qSamplesNeeded > length ? length : ftData.qSamplesNeeded;
						// the 4 magic number: since code is vectorized, we oversample the .. things
						auto const wOmega =  float(TAU) / ftLength;

						const Vector4 ftC1 = _mm_set1_ps(ftData.c1);
						const Vector4 ftC2 = _mm_set1_ps(ftData.c2);
						Vector4 ftSin = _mm_loadr_ps(ftData.sinPhases);
						Vector4 ftCos = _mm_loadr_ps(ftData.cosPhases);


						const float omega = tan(wOmega * 2);

						const float z = 2.0f / (1.0f + omega * omega);

						Vector4 wC1 = _mm_set1_ps(z - 1.0f);
						Vector4 wC2 = _mm_set1_ps(omega * z);


						// create the modified coupled form which constitutes the hanning window
						// since we actually need a cosine, we simply phase-shift the sine by pi / 2
						Vector4 wCos; // sin(pi / 2)
						Vector4 wSin; // cos(pi / 2)
						//const Vector4 wE = 2 * fast_math::sinf(wOmega / 2.0f);
						// initiate phases for modified coupled form pair

						cpl::sse::v4sf phases = _mm_setr_ps(0, wOmega, wOmega * 2, wOmega * 3);
						cpl::sse::sincos_ps(phases, &wSin, &wCos);

						/*fast_math::sincosf(wOmega, &wSin.ref_g(), &wCos.ref_g());
						fast_math::sincosf(wOmega * 2, &wSin.ref_b(), &wCos.ref_b());
						fast_math::sincosf(wOmega * 3, &wSin.ref_a(), &wCos.ref_a());*/

						Vector4 imag = _mm_setzero_ps(), real = _mm_setzero_ps();
						Vector4 t0, ones = _mm_set1_ps(1.0f);

#ifdef _CDFT_PACKED
						unsigned stop = length * 2;
						unsigned start = (length - ftLength) * 2;

						
						for (int t = start; t < stop; t += 8)
						{

							Vector4 left = _mm_load_ps(data + t);
							Vector4 right = _mm_load_ps(data + t + 4);
#else
						unsigned start = (length - ftLength);

						for (int t = start; t < length; t += 4)
						{

							Vector4 input = _mm_load_ps(dataptr + t);

#endif
							// window function
							t0 = _mm_sub_ps(ones, wCos);
							input = _mm_mul_ps(t0, input);

							// fourier transform


							real = _mm_add_ps(leftReal, _mm_mul_ps(input, ftCos));
							imag = _mm_add_ps(leftImag, _mm_mul_ps(input, ftSin));

							// this is the standard rotation filter which gives a satisfying sin/cos pair

							t0 = _mm_sub_ps(_mm_mul_ps(ftCos, ftC1), _mm_mul_ps(ftSin, ftC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							ftCos = _mm_mul_ps(ftCos, ftC2); // c2 * cos
							ftSin = _mm_mul_ps(ftSin, ftC1); // c1 * sin

							ftSin = _mm_add_ps(ftCos, ftSin); // final sine, t1
							ftCos = t0;

							// same for the window
							t0 = _mm_sub_ps(_mm_mul_ps(wCos, wC1), _mm_mul_ps(wSin, wC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							wCos = _mm_mul_ps(wCos, wC2); // c2 * cos
							wSin = _mm_mul_ps(wSin, wC1); // c1 * sin

							wSin = _mm_add_ps(wCos, wSin); // final sine, t1
							wCos = t0;


				
						}
						// store accumulated filter values

						_mm_store_ps(unp, real);
						result[idx * 2] = compileVector(unp) / (ftLength / 2);

						_mm_store_ps(unp, imag);
						result[idx * 2 + 1] = -compileVector(unp) / (ftLength / 2);

					}
					return true;
				}

			/******************************** Parallel minimum Q DFT **************************************


			/*********************************************************************************************/
			template<size_t channels, class Vector>
				typename std::enable_if<channels == 2, bool>::type
					CSignalTransform::mqdft_Parallel(const Vector & data, std::size_t bufferLength)
				{

					// handles switching between different amount of channels
					ensureBufferSizes(channels);

					const Concurrency::array<ScalarTy, 1> signal(bufferLength, 
						(ScalarTy*)&data[0], 
						defaultAccelerator.get_default_view()
					);

					// length of the signal per channel
					unsigned length = bufferLength / channels;
					const auto offset = numFilters;
					GPUData gdata{ *prlCdftData, *prlResult };
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
							auto const wOmega =  float(TAU) / ftLength;

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

							for (int t = start; t < length; t += 4)
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

					// handles switching between different amount of channels
					ensureBufferSizes(channels);

					const Concurrency::array<ScalarTy, 1> signal(bufferLength, 
						(ScalarTy*)&data[0], 
						defaultAccelerator.get_default_view()
					);

					// length of the signal per channel
					unsigned length = bufferLength / channels;
					const auto offset = numFilters;
					GPUData gdata{ *prlCdftData, *prlResult };
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
							auto const wOmega =  float(TAU) / ftLength;

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

							for (int t = start; t < length; t += 4)
							{

								Vector4 input(signal[t], signal[t + 1], signal[t + 2], signal[t + 3]);
								// window function
								input *= 1 - wCos;
								// fourier transform
								leftReal += input * _cos;
								leftImag += input * _sin;
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
			/**************************** 8 Vector AVX2 + FMA minimum Q DFT *******************************


			/*********************************************************************************************/
			template<size_t channels, class Vector>
				typename std::enable_if<channels == 2, bool>::type
					CSignalTransform::mqdft_FMA(const Vector & data, std::size_t bufferLength)
				{

					// handles switching between different amount of channels
					ensureBufferSizes(channels);

					float __alignas(32) unp[8];

					auto const * dataptr = &data[0];
					// length of the signal per channel
					unsigned length = bufferLength / channels;
					const auto offset = numFilters;
	
					isComputing = true;

					for (int idx = 0; idx < numFilters; ++idx)
					{
						typedef __m128 Vector4;
						typedef __m256 Vector8;
						using namespace Concurrency;

						const CDFTData & ftData = cdftData[idx];

						// the length of this current fourier transform,
						// where the length is calculated to give a satisfying Q
						const unsigned ftLength = ftData.qSamplesNeeded > length ? length : ftData.qSamplesNeeded;
						// the 4 magic number: since code is vectorized, we oversample the .. things
						auto const wOmega =  float(TAU) / ftLength;

						const Vector8 ftC1 = _mm256_set1_ps(ftData.c1);
						const Vector8 ftC2 = _mm256_set1_ps(ftData.c2);
						Vector8 ftSin = _mm256_load_ps(ftData.sinPhases);
						Vector8 ftCos = _mm256_load_ps(ftData.cosPhases);


						const float omega = tan(wOmega * 4); // 4 = 8x oversampling

						const float z = 2.0f / (1.0f + omega * omega);

						Vector8 wC1 = _mm256_set1_ps(z - 1.0f);
						Vector8 wC2 = _mm256_set1_ps(omega * z);


						// create the modified coupled form which constitutes the hanning window
						// since we actually need a cosine, we simply phase-shift the sine by pi / 2
						Vector8 wCos; // sin(pi / 2)
						Vector8 wSin; // cos(pi / 2)
						//const Vector4 wE = 2 * fast_math::sinf(wOmega / 2.0f);
						// initiate phases for modified coupled form pair

						Vector4 t1, t2;

						Vector4 phases = _mm_setr_ps(0, wOmega, wOmega * 2, wOmega * 3);
						cpl::sse::sincos_ps(phases, &t1, &t2);

						wCos = _mm256_castps128_ps256(t2);
						wSin = _mm256_castps128_ps256(t1);

						phases = _mm_setr_ps(wOmega * 4, wOmega * 5, wOmega * 6, wOmega * 7);
						cpl::sse::sincos_ps(phases, &t1, &t2);

						wCos = _mm256_insertf128_ps(wCos, t2, 1);
						wSin = _mm256_insertf128_ps(wSin, t1, 1);

						

						/*fast_math::sincosf(wOmega, &wSin.ref_g(), &wCos.ref_g());
						fast_math::sincosf(wOmega * 2, &wSin.ref_b(), &wCos.ref_b());
						fast_math::sincosf(wOmega * 3, &wSin.ref_a(), &wCos.ref_a());*/

						Vector8 leftImag = _mm256_setzero_ps(), leftReal = _mm256_setzero_ps(),
							rightImag = _mm256_setzero_ps(), rightReal = _mm256_setzero_ps();
						Vector8 t0, ones = _mm256_set1_ps(1.0f);
						
#ifdef _CDFT_PACKED
						unsigned stop = length * 2;
						unsigned start = (length - ftLength) * 2;

						
						for (int t = start; t < stop; t += 8)
						{

							Vector4 left = _mm_load_ps(data + t);
							Vector4 right = _mm_load_ps(data + t + 4);
#else
						unsigned start = (length - ftLength);

						for (int t = start; t < length; t += 8)
						{

							Vector8 left = _mm256_load_ps(dataptr + t);
							Vector8 right = _mm256_load_ps(dataptr + t + length);

#endif
							// window function
							t0 = _mm256_sub_ps(ones, wCos);
							left = _mm256_mul_ps(t0, left);
							right = _mm256_mul_ps(t0, right);

							// fourier transform

							
							leftReal = _mm256_fmadd_ps(left, ftCos, leftReal);
							leftImag = _mm256_fmadd_ps(leftImag, ftSin, leftImag);
							rightReal = _mm256_fmadd_ps(right, ftCos, rightReal);
							rightImag = _mm256_fmadd_ps(right, ftSin, rightImag);

							// this is the standard rotation filter which gives a satisfying sin/cos pair

							t0 = _mm256_sub_ps(_mm256_mul_ps(ftCos, ftC1), _mm256_mul_ps(ftSin, ftC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							ftCos = _mm256_mul_ps(ftCos, ftC2); // c2 * cos
							ftSin = _mm256_mul_ps(ftSin, ftC1); // c1 * sin

							ftSin = _mm256_add_ps(ftCos, ftSin); // final sine, t1
							ftCos = t0;

							// same for the window
							t0 = _mm256_sub_ps(_mm256_mul_ps(wCos, wC1), _mm256_mul_ps(wSin, wC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							wCos = _mm256_mul_ps(wCos, wC2); // c2 * cos
							wSin = _mm256_mul_ps(wSin, wC1); // c1 * sin

							wSin = _mm256_add_ps(wCos, wSin); // final sine, t1
							wCos = t0;

						}
						// store accumulated filter values

						_mm256_store_ps(unp, leftReal);
						result[idx * 2] = Math::compileVector(unp) / (ftLength / 2);

						_mm256_store_ps(unp, leftImag);
						result[idx * 2 + 1] = -Math::compileVector(unp) / (ftLength / 2);

						_mm256_store_ps(unp, rightReal);
						result[idx * 2 + offset * 2] = Math::compileVector(unp) / (ftLength / 2);

						_mm256_store_ps(unp, rightImag);
						result[idx * 2 + 1 + offset * 2] = -Math::compileVector(unp) / (ftLength / 2);
					}
					return true;
				}
			/*********************************************************************************************/
			/*********************************************************************************************/
			template<size_t channels, class Vector>
				typename std::enable_if<channels == 1, bool>::type
					CSignalTransform::mqdft_FMA(const Vector & data, std::size_t bufferLength)
				{

					// handles switching between different amount of channels
					ensureBufferSizes(channels);

					float __alignas(32) unp[8];

					auto const * dataptr = &data[0];
					// length of the signal per channel
					unsigned length = bufferLength / channels;
					const auto offset = numFilters;
	
					isComputing = true;

					for (int idx = 0; idx < numFilters; ++idx)
					{
						typedef __m128 Vector4;
						typedef __m256 Vector8;
						using namespace Concurrency;

						const CDFTData & ftData = cdftData[idx];

						// the length of this current fourier transform,
						// where the length is calculated to give a satisfying Q
						const unsigned ftLength = ftData.qSamplesNeeded > length ? length : ftData.qSamplesNeeded;
						// the 4 magic number: since code is vectorized, we oversample the .. things
						auto const wOmega =  float(TAU) / ftLength;

						const Vector8 ftC1 = _mm256_set1_ps(ftData.c1);
						const Vector8 ftC2 = _mm256_set1_ps(ftData.c2);
						Vector8 ftSin = _mm256_load_ps(ftData.sinPhases);
						Vector8 ftCos = _mm256_load_ps(ftData.cosPhases);


						const float omega = tan(wOmega * 4); // 4 = 8x oversampling

						const float z = 2.0f / (1.0f + omega * omega);

						Vector8 wC1 = _mm256_set1_ps(z - 1.0f);
						Vector8 wC2 = _mm256_set1_ps(omega * z);


						// create the modified coupled form which constitutes the hanning window
						// since we actually need a cosine, we simply phase-shift the sine by pi / 2
						Vector8 wCos; // sin(pi / 2)
						Vector8 wSin; // cos(pi / 2)
						//const Vector4 wE = 2 * fast_math::sinf(wOmega / 2.0f);
						// initiate phases for modified coupled form pair

						Vector4 t1, t2;

						Vector4 phases = _mm_setr_ps(0, wOmega, wOmega * 2, wOmega * 3);
						cpl::sse::sincos_ps(phases, &t1, &t2);

						wCos = _mm256_castps128_ps256(t2);
						wSin = _mm256_castps128_ps256(t1);

						phases = _mm_setr_ps(wOmega * 4, wOmega * 5, wOmega * 6, wOmega * 7);
						cpl::sse::sincos_ps(phases, &t1, &t2);

						wCos = _mm256_insertf128_ps(wCos, t2, 1);
						wSin = _mm256_insertf128_ps(wSin, t1, 1);

						

						/*fast_math::sincosf(wOmega, &wSin.ref_g(), &wCos.ref_g());
						fast_math::sincosf(wOmega * 2, &wSin.ref_b(), &wCos.ref_b());
						fast_math::sincosf(wOmega * 3, &wSin.ref_a(), &wCos.ref_a());*/

						Vector8 imag = _mm256_setzero_ps(), real = _mm256_setzero_ps();
						Vector8 t0, ones = _mm256_set1_ps(1.0f);
						
#ifdef _CDFT_PACKED
						unsigned stop = length * 2;
						unsigned start = (length - ftLength) * 2;

						
						for (int t = start; t < stop; t += 8)
						{

							Vector4 left = _mm_load_ps(data + t);
							Vector4 right = _mm_load_ps(data + t + 4);
#else
						unsigned start = (length - ftLength);

						for (int t = start; t < length; t += 8)
						{

							Vector8 input = _mm256_load_ps(dataptr + t);

#endif
							// window function
							t0 = _mm256_sub_ps(ones, wCos);
							input = _mm256_mul_ps(t0, input);

							// fourier transform

							

							//real = _mm256_add_ps(real, _mm256_mul_ps(input, ftCos));
							real = _mm256_fmadd_ps(input, ftCos, real);
							imag = _mm256_fmadd_ps(input, ftSin, imag);

							// this is the standard rotation filter which gives a satisfying sin/cos pair

							t0 = _mm256_sub_ps(_mm256_mul_ps(ftCos, ftC1), _mm256_mul_ps(ftSin, ftC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							ftCos = _mm256_mul_ps(ftCos, ftC2); // c2 * cos
							ftSin = _mm256_mul_ps(ftSin, ftC1); // c1 * sin

							ftSin = _mm256_add_ps(ftCos, ftSin); // final sine, t1
							ftCos = t0;

							// same for the window
							t0 = _mm256_sub_ps(_mm256_mul_ps(wCos, wC1), _mm256_mul_ps(wSin, wC2)); // final cosine, t0

							// these two lines have no dependency on the upper three, so should be pipelined
							wCos = _mm256_mul_ps(wCos, wC2); // c2 * cos
							wSin = _mm256_mul_ps(wSin, wC1); // c1 * sin

							wSin = _mm256_add_ps(wCos, wSin); // final sine, t1
							wCos = t0;

						}
						// store accumulated filter values

						_mm256_store_ps(unp, real);
						result[idx * 2] = compileVector(unp) / (ftLength / 2);
						// remember the sign on the imaginary part!!
						_mm256_store_ps(unp, imag);
						result[idx * 2 + 1] = -compileVector(unp) / (ftLength / 2);

					}
					return true;
				}
		};
	};

#endif