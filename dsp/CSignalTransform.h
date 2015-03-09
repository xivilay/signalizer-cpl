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

	file:CSignalTransform.h
	
		A class containing methods to calculating fourier/wavelet transforms of n-d data,
		using different combinations of (a)sync parallel / accelerated DFT's and FFTs

*************************************************************************************/

#ifndef _CSIGNALTRANSFORM_H
	#define _CSIGNALTRANSFORM_H

	#include <complex>
	#include <cstdint>
	#include <vector>
	#include <type_traits>
	#include "../MacroConstants.h"
	#include "../dsp.h"
	#include "../Utility.h"
	#include "../lib/AlignedAllocator.h"

	#ifndef _CPL_NO_ACCELERATION /* define this if you dont want accelerated code */

		#ifdef __MSVC__ /* C++ AMP currently only supported on visual c++ targets */

			#include <amp.h>
			#include <amp_math.h>
			#include <amp_graphics.h>
			#include <amp_short_vectors.h>

			#define _CPL_AMP_SUPPORT // C++ AMP supported
			#define _CPL_ACC_SUPPORT // accelerated computation available

		#else
	
		#endif

	#endif

	namespace cpl
	{
		namespace dsp
		{
			//template<typename Scalar>
				class CSignalTransform 
				: 
					public Utility::CNoncopyable
				{
				public:

					typedef float ScalarTy;
					typedef std::vector<ScalarTy> ResultVector;

					class ResultData
					{
						const ResultVector & result;
						std::size_t size;
					public:
						ResultData(const ResultVector & res, std::size_t sizeOfSingleChannel)
							: result(res), size(sizeOfSingleChannel)
						{
						}

						inline const ScalarTy * data() const { return result.data(); }
						inline const ResultVector & vector() const { return result;  }
						inline const ScalarTy * operator[] (std::size_t channel) const
						{
							return result.data() + channel * size;
						}

						inline std::complex<ScalarTy> complexAt(std::size_t channel, std::size_t idx) const
						{
							return std::complex<ScalarTy>(
								result[channel * size + idx * 2],
								result[channel * size + idx * 2 + 1]
							);
						}
					private:
						ResultData();
					};

					enum Flags
					{
						scalar = 1,
						vectorized = 1 << 2,
						accelerated = 1 << 3,
						threaded = 1 << 4,
					};

					/*	haar digital wavelet transform
							data and out are of size size. the transform is put into out, while 
							data is used for workspace.
					*/
					template<class Vector>
						static void haarDWT(Vector & data, Vector & out, std::size_t size);

					/*	custom discrete fourier transform
							runs the fourier transform for each of the frequencies in freqs.
							the data is a vector of Scalar types of dftSize, while the freqs is assumed to be a complex
							vector with pairs of reals and imags. the frequency component to be evaluated is the real part,
							and the dft result will be both the real and imag components. 
							freqs has a size of freqSize * 2 * sizeof Scalar, therefore you evaluate freqSize / 2 frequencies.
							data _has_ to be 16-byte aligned, and a multiple of 16 (not necessarily a power of two!)
					*/
					template<class Vector, class InOutVector>
						static void customDFT(const Vector & data, std::size_t dftSize, InOutVector & freqs, std::size_t freqSize, ScalarTy sampleRate);

					/*	fast fourier transform (static)
							data _has_ to be 16-byte aligned and a power of 2.
							data is a complex vector of double, that is, pairs of reals/imags
							the process is in-place.
					*/
					static void sfft(double * data, std::size_t fftSize);

					template<typename Scalar, class Vector> 
						static std::complex<Scalar> goertzel(const Vector & data, std::size_t size, Scalar rads);
					template<typename Scalar, class Vector>
						static std::complex<Scalar> goertzel(const Vector & data, std::size_t size, Scalar frequency, Scalar sampleRate);


					/*	custom data for optimized custom dft
							precompute coefficients and copy to accelerated device
					*/
					template<class Vector>
						void setKernelData(const Vector & frequencies, std::size_t size);


					/*	custom dft kernel start
							execute the fourier transforms of the frequencies previously given by setCustomData.
							note that this call might be async, the result is retrieved through getTransformResult()
							which will wait - but you might benefit from doing other work in between.
							
							output is not scaled and input is not windowed
					*/
					template<class Vector>
						bool runCustomDFTKernel(const Vector & data, std::size_t size);

					/*	custom dft kernel start, q is adjusted to minimum bandwith needed.
							execute the fourier transforms of the frequencies previously given by setCustomData.
							note that this call might be async, the result is retrieved through getTransformResult()
							which will wait - but you might benefit from doing other work in between.

							note: output is necessarily scaled and input is also windowed by this function.
						
					*/
					template<size_t channels, class Vector>
						bool mqdft(const Vector & data, std::size_t size);

					template<size_t channels, class Vector>
						typename std::enable_if<channels <= 2, bool>::type
							fft(Vector & data, std::size_t size);

					template<size_t channels, class Vector>
						typename std::enable_if<channels <= 2, bool>::type
							fft(Vector * data, std::size_t size);

					ResultData getTransformResult();

					CSignalTransform(double sampleRate, Flags flags = Flags::vectorized)
						: sampleRate(sampleRate), isComputing(false),
						numChannels(1), numFilters(0), totalDataSize(0), oversamplingFactor(1)
					{
						setFlags(flags);
						selectAppropriateAccelerator();
					}


					void setSampleRate(double sampleRate)
					{
						this->sampleRate = sampleRate;
					}

					/*
						change flags. this will not recalculate coefficients, so incompatible settings
						may require a new call to setKernelData(), however it is safe not to do so.
					*/
					void setFlags(int f);
					int getFlags() { return flags; }
					~CSignalTransform()
					{

					}

				protected:
					void selectAppropriateAccelerator();
				private:

					void resizeResultAndFilters(size_t, size_t);
					void copyMemoryToAccelerator();

					template<size_t channels, typename V, class Vector>
						bool mqdft_Serial(const Vector & data, std::size_t bufferLength);

					template<size_t channels, typename V, class Vector>
						bool mqdft_Threaded(const Vector & data, std::size_t bufferLength);


					template<size_t channels, class Vector>
					typename std::enable_if<channels == 2, bool>::type
						mqdft_Parallel(const Vector & data, std::size_t size);

					template<size_t channels, class Vector>
					typename std::enable_if<channels == 1, bool>::type
						mqdft_Parallel(const Vector & data, std::size_t size);


					void ensureBufferSizes(std::size_t numChannels);
					struct CDFTData
					{
						float wc1, wc2;
						__alignas(32) float sinPhases[8]; // starting sine phases
						__alignas(32) float cosPhases[8]; // starting cosine phases
						float c1, c2; // coefficients for oscillators
						Types::fint_t qSamplesNeeded; // amount of samples needed to satisfy correct resolution
						Types::fint_t decimationFactor; // amount of decimation that can be done, while still satisfying the nyquist theorem
						double omega; // frequency of this oscillator, rads / sample
					};

					#ifdef _CPL_AMP_SUPPORT
						std::unique_ptr<Concurrency::array<CDFTData, 1>> prlCdftData;
						std::unique_ptr<Concurrency::array<ScalarTy, 1>> prlResult;
						Concurrency::accelerator defaultAccelerator;
						/*
							Wrapper to circumvent type problems with amp lambda's
						*/
						struct GPUData
						{
							Concurrency::array<CDFTData, 1> & cdft;
							Concurrency::array<ScalarTy, 1> & result;
						};

					#else
						std::vector<CDFTData> cdftData;
					#endif
					
					std::vector<ScalarTy> result;
					cpl::aligned_vector<CDFTData, 32u> cdftData;
					volatile bool isComputing;
					double sampleRate, oversamplingFactor;
					int numChannels, numFilters, totalDataSize;
					Flags flags;

				}; // CSignalTransform

		}; // dsp
	}; // cpl

	#include "CSignalTransform.inl"

#endif