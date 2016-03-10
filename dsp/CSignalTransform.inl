/*************************************************************************************
 
	cpl - cross-platform library - v. 0.1.0.
 
	Copyright (C) 2016 Janus Lynggaard Thorborg [LightBridge Studios]
 
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
//#define _DOOLD
	#include "../ffts.h"
	#include "../Misc.h"
	#include "../SysStats.h"
	#include "mqdft.inl"

	#ifdef _OPENMP
	   #include <omp.h>
	#else
	   #define omp_set_num_threads(x)
	#endif

//#define _CDFT_QUANTIZE_TO_8


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
					if (!size)
						return;
					resizeResultAndFilters(numChannels, size);
					auto const piMega = PI / sampleRate;

					for (unsigned i = 0; i < (size - 1); ++i)
					{
						// t0 = floor(sampleRate / bandWidth)
						unsigned bandWidth = static_cast<unsigned>(sampleRate / (freq[i + 1] - freq[i]));

						// numSamples = t0 - (t0 % 8)
						// the 8 magic number corrosponds to the minimum amount of samples needed to vectorize the code
#ifdef _CDFT_QUANTIZE_TO_8
						unsigned numSamples = bandWidth - (bandWidth & 0x7);
						// avoid zero
						if (!numSamples)
							numSamples = 8;
#else
						unsigned numSamples = bandWidth;
						// avoid zero
						if (!numSamples)
							numSamples = 8;
#endif
						cdftData[i].qSamplesNeeded = numSamples;
						double omega = freq[i] * piMega * 2;
						cdftData[i].omega = omega;
						for (int z = 0; z < 8; ++z)
						{
							cdftData[i].sinPhases[z] = static_cast<float>(std::sin(omega * z));
							cdftData[i].cosPhases[z] = static_cast<float>(std::cos(omega * z));
						}

						omega = std::tan(omega/ 2.0);

						//omega = std::tan(std::fmod(omega * oversamplingFactor, sampleRate / 2.0) / 2.0);
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
						cdftData[i].omega = omega;
						for (int z = 0; z < 8; ++z)
						{
							cdftData[i].sinPhases[z] = static_cast<float>(std::sin(omega * z));
							cdftData[i].cosPhases[z] = static_cast<float>(std::cos(omega * z));
						}

						omega = std::tan(omega / 2.0);

						//omega = std::tan(std::fmod(omega * oversamplingFactor, sampleRate / 2.0) / 2.0);
						auto const z = 2.0 / (1.0 + omega * omega);

						cdftData[i].c1 = static_cast<float>(z - 1.0);
						cdftData[i].c2 = static_cast<float>(omega * z);
					}

					copyMemoryToAccelerator();

				}

			template<typename Scalar, class Vector>
				static std::complex<Scalar> CSignalTransform::goertzel(const Vector & data, std::size_t size, Scalar frequency, Scalar sampleRate)
				{
					return dsp::goertzel<Scalar, Vector>(data, size, frequency * 2 * M_PI / sampleRate);
				}

			template<typename Scalar, class Vector>
				static std::complex<Scalar> CSignalTransform::goertzel(const Vector & data, std::size_t size, Scalar omega)
				{
					return dsp::goertzel<Scalar, Vector>(data, size, omega);
				}


			void CSignalTransform::copyMemoryToAccelerator()
			{
				// copy memory onto gpu
				if (flags & Flags::accelerated)
				{
					#ifdef _CPL_AMP_SUPPORT
						if (!prlResult || prlResult->extent.size() != totalDataSize)
						{
							prlResult.reset
							(
								new Concurrency::array<ScalarTy, 1>(totalDataSize, defaultAccelerator.get_default_view())
							);
						}
						prlCdftData.reset
						(
							new Concurrency::array<CDFTData, 1>(numFilters, cdftData.begin(), defaultAccelerator.get_default_view())
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
			void CSignalTransform::sfft(double * data, std::size_t fftSize)
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
						for (unsigned i = 0; i < accs.size(); ++i)
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
			void CSignalTransform::resizeResultAndFilters(size_t channels, size_t filters)
			{
				/*
					The resizing is done through this method, because publicly our size is equal to filters,
					but privately, we have an additional storage for filters such that filters is always the 
					next multiple of 8. This ensure we can run vectorized code optimally, without checking for
					bounds.
				*/
				numChannels = channels;
				numFilters = filters;

				auto safeNumFilters = filters + (8 - filters & 0x7);
				auto safeSize = numChannels * safeNumFilters * 2;
				totalDataSize = numChannels * numFilters * 2;

				cdftData.resize(safeNumFilters); 
				result.resize(safeSize);
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
				if (numChannels != amountOfChannels)
				{
					resizeResultAndFilters(amountOfChannels, numFilters);
					if (flags & Flags::accelerated)
					{
						#ifdef _CPL_AMP_SUPPORT
							if (prlResult || prlResult->get_extent().size() != totalDataSize)
							{
								prlResult.reset(new Concurrency::array<ScalarTy, 1>
									(totalDataSize, defaultAccelerator.get_default_view()));
							}
						#endif
					}
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
					else if (flags & Flags::scalar)
					{
						oversamplingFactor = 1;
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

					auto const numThreads = (flags & Flags::threaded) * SysStats::CProcessorInfo::instance().getNumOptimalThreads();

					omp_set_num_threads
					(
						numThreads ? numThreads : 1
					);

				}
			}
			/*********************************************************************************************

				This is the entry-point and public interface to the minimum Q discrete fourier transform.

			/*********************************************************************************************/
			template<size_t channels, class Vector>
				bool CSignalTransform::mqdft(const Vector & data, std::size_t bufferLength)
				{
					// handles switching between different amount of channels
					ensureBufferSizes(channels);

					/*
						Here we take advantage of specific instruction sets, 
						and dispatch correctly at runtime

						First, check if we should use a massively parallel platform	
					*/
					if (flags & accelerated)
						return mqdft_Parallel<channels>(data, bufferLength);
					else if (flags & scalar)
						if (flags & threaded)
							return mqdft_Threaded<channels, float>(data, bufferLength);
						else
							return mqdft_Serial<channels, float>(data, bufferLength);
					using namespace cpl::SysStats;

					auto const & cpuID = cpl::SysStats::CProcessorInfo::instance();

					/*if (cpuID.test(CProcessorInfo::AVX2))
						return mqdft_FMA<channels>(data, bufferLength);
					else */
					if (flags & threaded)
					{
						if (cpuID.test(CProcessorInfo::AVX))
							return mqdft_Threaded<channels, Types::v8sf>(data, bufferLength);
						else if (cpuID.test(CProcessorInfo::SSE2))
							return mqdft_Threaded<channels, Types::v4sf>(data, bufferLength);
						else
							return mqdft_Threaded<channels, float>(data, bufferLength);
					}
					else
					{
						if (cpuID.test(CProcessorInfo::AVX))
							return mqdft_Serial<channels, Types::v8sf>(data, bufferLength);
						else if (cpuID.test(CProcessorInfo::SSE2))
							return mqdft_Serial<channels, Types::v4sf>(data, bufferLength);
						else
							return mqdft_Serial<channels, float>(data, bufferLength);
					}

				}
			/*********************************************************************************************

				This is the entry-point and public interface to the fast fourier transform.

			/*********************************************************************************************/
			template<size_t channels, class Vector>
				typename std::enable_if<channels <= 2, bool>::type
					CSignalTransform::fft(Vector & data, std::size_t size)
				{
					double * signal = reinterpret_cast<double*>(&data[0]);
					cpl::signaldust::DustFFT_fwdDa(signal, size);

					return true;
				}

			template<size_t channels, class Vector>
				typename std::enable_if<channels <= 2, bool>::type
					CSignalTransform::fft(Vector * data, std::size_t size)
				{
					double * signal = reinterpret_cast<double*>(&data[0]);
					cpl::signaldust::DustFFT_fwdDa(signal, size);

					return true;
				}

		};
	};

#endif