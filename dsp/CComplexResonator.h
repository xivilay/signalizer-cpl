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

	file:CComplexResonator.h
		
		A complex resonator system... More to come.

*************************************************************************************/

#ifndef CCOMPLEX_RESONATOR_H
	#define CCOMPLEX_RESONATOR_H

	#include "../CMutex.h"
	#include "../simd.h"
	#include "filterdesign.h"
	#include "../LibraryOptions.h"
	#include <vector>
	#include "../mathext.h"
	#include "DSPWindows.h"
	#include "../Utility.h"


	namespace cpl
	{
		namespace dsp
		{
			template<typename T, std::size_t Channels = 1, std::size_t Vectors = 3>
			class CComplexResonator : public cpl::CMutex::Lockable
			{
			public:

				typedef T Scalar;

				static_assert(Vectors == 3, "CComplexResonator currently only supports 3 concurrent vectors.");
				static_assert(Channels > 0, "CComplexResonator needs at least one channel");
				static const std::size_t centerFilter = 1;
				static const std::size_t numVectors = Vectors;
				static const std::size_t numChannels = Channels;
				static const std::size_t numBuffersPerVector = (2 * numChannels + 2);
				static const std::size_t numBuffers = numVectors * numBuffersPerVector;
				static const std::size_t numVectorSets = numVectors * 2 * numChannels;



				CComplexResonator()
					: minWindowSize(8), maxWindowSize(8), numFilters(0), qDBs(3), vectorDist(1), numResonators()
				{
					for (int v = 0; v < numVectors; ++v)
					{
						realCoeff[v] = imagCoeff[v] = nullptr;
						for (int c = 0; c < numChannels; ++c)
						{
							realState[c][v] = imagState[c][v] = nullptr;
						}
					}
				}

				void setWindowSize(double minSize, double maxSize)
				{
					minWindowSize = std::min(minSize, maxSize);
					maxWindowSize = std::max(minSize, maxSize);
					
				}

				std::size_t getNumFilters() const noexcept
				{
					return numFilters;
				}

				void setQ(double dBs)
				{
					qDBs = dBs;
				}
				void setVectorDist(double in)
				{
					vectorDist = in;
			
				}



				template<typename V, class MultiVector>
				inline void wresonate(const MultiVector & data, std::size_t numDataChannels, std::size_t numSamples)
				{
					CFastMutex lock(this);

					switch (numDataChannels)
					{
					case 1:
						internalWindowResonate<V, MultiVector, 1>(data, numSamples); break;
					case 2:
						internalWindowResonate<V, MultiVector, 2>(data, numSamples); break;
					default:
						CPL_RUNTIME_EXCEPTION("Unsupported number of channels.");
					}

				}
				/// <summary>
				/// Locks. O(n)
				/// </summary>
				/// <returns></returns>
				inline bool isCompletelyZero() const noexcept
				{
					CFastMutex lock(this);

					for (int c = 0; c < numChannels; ++c)
					{
						for (int z = 0; z < numVectors; ++z)
						{
							for (int i = 0; i < numResonators; ++i)
							{
								if(realState[c][z][i] != 0.0 || imagState[c][z][i] != 0.0);
									return false;
							}
						}
					}

					return true;
				}

				inline std::complex<Scalar> getResonanceAt(std::size_t resonator, std::size_t channel)
				{
					auto const gainCoeff = N.at(resonator) * 0.5; // bounds checking here.
					return std::complex<Scalar>
					(
						realState[channel][centerFilter][resonator] / gainCoeff, 
						imagState[channel][centerFilter][resonator] / gainCoeff
					);
				}

				template<WindowTypes win, bool lazy = false>
					inline std::complex<Scalar> getWindowedResonanceAt(std::size_t resonator, std::size_t channel)
					{
						if (resonator >= N.size())
							BreakIfDebugged();
						Scalar gainCoeff = N.at(resonator) * 1.0 / 8.0; // 2^-3 (3 vectors)
						Scalar real(0), imag(0);

						typedef typename Windows::windowFromEnum<Scalar, win>::type window;

						if (lazy)
						{
							for (int v = 0; v < Vectors; ++v)
							{
								std::size_t offset = Math::circularWrap(resonator + Math::mapAroundZero<signed>(v, Vectors), numFilters);
								real += window::DFTCoeffs[v] * realState[channel][centerFilter][offset];
								imag += window::DFTCoeffs[v] * imagState[channel][centerFilter][offset];
							}
						}
						else
						{
							for (int v = 0; v < Vectors; ++v)
							{
								real += window::DFTCoeffs[v] * realState[channel][v][resonator];
								imag += window::DFTCoeffs[v] * imagState[channel][v][resonator];
							}
						}

						return std::complex<Scalar>(real / gainCoeff, imag / gainCoeff);
					}

				inline Scalar getBandwidth(std::size_t resonator)
				{
					return N.at(resonator);
				}

				/// <summary>
				/// Resets the filters state to zero. Coefficients are untouched, reset them (indirectly) through mapSystemHz.
				/// Blocks the processing.
				/// </summary>
				void resetState()
				{
					CMutex lock(this);
					for (int c = 0; c < numChannels; ++c)
					{
						for (int z = 0; z < numVectors; ++z)
						{
							std::memset(realState[c][z], 0, numFilters * sizeof(T));
							std::memset(imagState[c][z], 0, numFilters * sizeof(T));
						}
					}
				}


				/// <summary>
				/// For completely correct behaviour, this needs to be called on the audio thread.
				/// Otherwise, some filters might become unstable for one frame. This call is SAFE, however
				/// on any thead. If the size is different, it may acquire a mutex and reallocate memory. Otherwise,
				/// it won't.
				/// </summary>
				template<typename Vector>
				void mapSystemHz(const Vector & mappedHz, std::size_t vSize, double sampleRate)
				{
					CMutex lock(this);
					// 7 == state size of all members
					using namespace cpl;

					auto const newData = reallocBuffers(vSize);

					int i = 0;
					if (vSize == 1)
					{
						//auto const bandWidth = minWindowSize;
						//auto const Q = sampleRate / bandWidth;
						for (int z = 0; z < numVectors; ++z)
						{
							//auto const theta = mappedHz[i] + (z - (numVectors - 1) >> 1) * Q;
							//auto const omega = 2 * M_PI * theta / sampleRate;
						//	realCoeff[z][i] = coeffs.c[0].real();
						//	imagCoeff[z][i] = coeffs.c[0].imag();
							if (newData)
							{
								for (int c = 0; c < numChannels; ++c)
								{
									realState[c][z][i] = 0;
									imagState[c][z][i] = 0;
								}
							}

						}
						i++;
					}
					else
					{

						auto const Bq = (3 / qDBs) * M_E/12.0;

						for (i = 0; i < vSize; ++i)
						{

							auto const k = std::size_t(i + 1) >= numFilters ? numFilters - 2 : i;
							auto hDiff = std::abs((double)mappedHz[k + 1] - mappedHz[k]);
							auto bandWidth = sampleRate / hDiff;
							const bool qIsFree = false;

							if(!qIsFree)
								bandWidth = cpl::Math::confineTo<double>(bandWidth, minWindowSize, maxWindowSize);


							hDiff = (sampleRate) / bandWidth;

							// 3 dB law bandwidth of complex resonator
							auto const r = exp(-M_PI * hDiff / sampleRate);

							N[i] = 1.0/(1 - r);

							for (int z = 0; z < numVectors; ++z)
							{
								// so basically, for doing frequency-domain windowing using DFT-coefficients of the windows, we need filters that are linearly 
								// spaced around the frequency like the FFT. DFT bins are spaced linearly like 0.5 / N.
								auto const omega = (2 * M_PI * (mappedHz[i] + Math::mapAroundZero<Scalar>(z, numVectors) * hDiff * 0.5)) / sampleRate;

								auto const real = r * cos(omega);
								auto const imag = r * sin(omega);
								realCoeff[z][i] = real; // coeffs.c[0].real()
								imagCoeff[z][i] = imag; // coeffs.c[0].imag()
								if (newData)
								{
									for (int c = 0; c < numChannels; ++c)
									{
										realState[c][z][i] = 0;
										imagState[c][z][i] = 0;
									}

								}

							}

						}
					}
					// set remainder to zero
					for (; i < numResonators; ++i)
					{
						for (int z = 0; z < numVectors; ++z)
						{
							realCoeff[z][i] = imagCoeff[z][i] = (Scalar) 0;
						}
					}

				}

			private:

				bool reallocBuffers(int minimumSize)
				{
					// the locals here are to ensure we dont change fields before locking (if needed)
					auto numFiltersInt = minimumSize;
					// quantize to next multiple of 8, to ensure vectorization
					auto numResonatorsInt = numFiltersInt + (8 - numFiltersInt & 0x7);
					auto const dataSize = numBuffers * numResonatorsInt;
					bool newData = false;
					/*
					consider creating a copy of buffer (or, the states)
					if the buffer is resized / rads are different, interpolate the old points to the new positions.
					*/
					if (dataSize != buffer.size())
					{
						// change fields now.
						numFilters = numFiltersInt;
						numResonators = numResonatorsInt;

						newData = true;
						buffer.resize(dataSize);
						N.resize(numResonators);
						for (std::size_t i = 0; i < numVectors; ++i)
						{
							realCoeff[i] = buffer.data() + numResonators * i * numBuffersPerVector;
							imagCoeff[i] = buffer.data() + numResonators * i * numBuffersPerVector + numResonators;
							for (std::size_t c = 0; c < numChannels; ++c)
							{
								realState[c][i] = buffer.data() + numResonators * i * numBuffersPerVector + numResonators * (2 + c * 2);
								imagState[c][i] = buffer.data() + numResonators * i * numBuffersPerVector + numResonators * (3 + c * 2);
							}
						}

					}
					return newData;
				}

				template<typename V, class MultiVector, std::size_t inputDataChannels>
				void internalWindowResonate(const MultiVector & data, std::size_t numSamples)
				{

					using namespace cpl;
					using namespace cpl::simd;

					auto const vfactor = suitable_container<V>::size;
					V t0;

					//  iterate over each filter for each sample for each channel.
					for (Types::fint_t filter = 0; filter < numFilters; filter += vfactor)
					{

						// pointer to current sample
						typename scalar_of<V>::type * audioInputs[numChannels];

						// load coefficients
						const V
							p_m1_r = load<V>(realCoeff[0] + filter), // coeff: e^i*omega-q (real)
							p_m1_i = load<V>(imagCoeff[0] + filter), // coeff: e^i*omega-q (imag)
							p_m_r = load<V>(realCoeff[1] + filter), // coeff: e^i*omega (real)
							p_m_i = load<V>(imagCoeff[1] + filter), // coeff: e^i*omega (imag)
							p_p1_r = load<V>(realCoeff[2] + filter), // coeff: e^i*omega+q (real)
							p_p1_i = load<V>(imagCoeff[2] + filter); // coeff: e^i*omega+q (imag)

						// create states
						V
							s_m1_r[inputDataChannels],
							s_m1_i[inputDataChannels],
							s_m_r[inputDataChannels],
							s_m_i[inputDataChannels],
							s_p1_r[inputDataChannels],
							s_p1_i[inputDataChannels];

						// and load them
						for (Types::fint_t c = 0; c < inputDataChannels; ++c)
						{
							audioInputs[c] = &data[c][0];
							s_m1_r[c] = load<V>(realState[c][0] + filter); // cos: e^i*omega-q (real)
							s_m1_i[c] = load<V>(imagState[c][0] + filter); // sin: e^i*omega-q (imag)
							s_m_r[c] = load<V>(realState[c][1] + filter); // cos: e^i*omega (real)
							s_m_i[c] = load<V>(imagState[c][1] + filter); // sin: e^i*omega (imag)
							s_p1_r[c] = load<V>(realState[c][2] + filter); // cos: e^i*omega+q (real)
							s_p1_i[c] = load<V>(imagState[c][2] + filter); // sin: e^i*omega+q (imag)
						}

						for (Types::fint_t sample = 0; sample < numSamples; ++sample)
						{
							for (Types::fint_t c = 0; c < inputDataChannels; ++c)
							{
								// combing stage
								V input = broadcast<V>(audioInputs[c]);

								// -1 stage (m1) (fc - bw)
								t0 = s_m1_r[c] * p_m1_r - s_m1_i[c] * p_m1_i;
								s_m1_i[c] = s_m1_r[c] * p_m1_i + s_m1_i[c] * p_m1_r;
								s_m1_r[c] = t0 + input;

								// 0 stage (m) (fc)
								t0 = s_m_r[c] * p_m_r - s_m_i[c] * p_m_i;
								s_m_i[c] = s_m_r[c] * p_m_i + s_m_i[c] * p_m_r;
								s_m_r[c] = t0 + input;

								// +1 stage (p1) (fc + bw)
								t0 = s_p1_r[c] * p_p1_r - s_p1_i[c] * p_p1_i;
								s_p1_i[c] = s_p1_r[c] * p_p1_i + s_p1_i[c] * p_p1_r;
								s_p1_r[c] = t0 + input;

								audioInputs[c]++;
							}


						}
						for (Types::fint_t c = 0; c < inputDataChannels; ++c)
						{
							store(realState[c][0] + filter, s_m1_r[c]); // state: e^i*omega-q (real)
							store(imagState[c][0] + filter, s_m1_i[c]); // state: e^i*omega-q (imag)
							store(realState[c][1] + filter, s_m_r[c]); // state: e^i*omega (real)
							store(imagState[c][1] + filter, s_m_i[c]); // state: e^i*omega (imag)
							store(realState[c][2] + filter, s_p1_r[c]); // state: e^i*omega+q (real)
							store(imagState[c][2] + filter, s_p1_i[c]); // state: e^i*omega+q (imag)
						}
					}
				}


				Scalar * realCoeff[Vectors];
				Scalar * imagCoeff[Vectors];
				Scalar * realState[Channels][Vectors];
				Scalar * imagState[Channels][Vectors];

				std::size_t numFilters, numResonators;
				double maxWindowSize;
				double minWindowSize;
				double qDBs;
				double vectorDist;
				cpl::aligned_vector<Scalar, 32u> buffer;
				std::vector<Scalar> N;

			};

		};
	};
#endif