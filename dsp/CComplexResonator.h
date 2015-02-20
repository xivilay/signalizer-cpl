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
					: minWindowSize(8), maxWindowSize(8), numFilters(0), qDBs(3), vectorDist(1)
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

				void setQ(double dBs)
				{
					qDBs = dBs;
				}
				void setVectorDist(double in)
				{
					vectorDist = in;
			
				}



				template<typename V, class MultiVector>
				void wresonate(const MultiVector & data, std::size_t numDataChannels, std::size_t numSamples)
				{

					using namespace cpl;
					using namespace cpl::simd;
					CFastMutex lock(this);

					auto const vfactor = suitable_container<V>::size;
					V t0;




					for (Types::fint_t filter = 0; filter < numFilters; filter += vfactor)
					{

						// pointer to current sample
						auto audioInput = &data[0][0];

						// load coefficients
						const V
							p_m1_r = load<V>(realCoeff[0] + filter), // coeff: e^i*omega-q (real)
							p_m1_i = load<V>(imagCoeff[0] + filter), // coeff: e^i*omega-q (imag)
							p_m_r = load<V>(realCoeff[1] + filter), // coeff: e^i*omega (real)
							p_m_i = load<V>(imagCoeff[1] + filter), // coeff: e^i*omega (imag)
							p_p1_r = load<V>(realCoeff[2] + filter), // coeff: e^i*omega+q (real)
							p_p1_i = load<V>(imagCoeff[2] + filter); // coeff: e^i*omega+q (imag)
						// load states

						V
							s_m1_r[numChannels],
							s_m1_i[numChannels],
							s_m_r[numChannels],
							s_m_i[numChannels],
							s_p1_r[numChannels],
							s_p1_i[numChannels];

						for (Types::fint_t c = 0; c < numChannels; ++c)
						{
							s_m1_r[c] = load<V>(realState[c][0] + filter); // cos: e^i*omega-q (real)
							s_m1_i[c] = load<V>(imagState[c][0] + filter); // sin: e^i*omega-q (imag)
							s_m_r[c] = load<V>(realState[c][1] + filter); // cos: e^i*omega (real)
							s_m_i[c] = load<V>(imagState[c][1] + filter); // sin: e^i*omega (imag)
							s_p1_r[c] = load<V>(realState[c][2] + filter); // cos: e^i*omega+q (real)
							s_p1_i[c] = load<V>(imagState[c][2] + filter); // sin: e^i*omega+q (imag)
						}



						for (Types::fint_t sample = 0; sample < numSamples; ++sample)
						{





							for (Types::fint_t c = 0; c < numChannels; ++c)
							{
								// combing stage
								V input = broadcast<V>(audioInput);

								// -1 stage (m1)
								t0 = s_m1_r[c] * p_m1_r - s_m1_i[c] * p_m1_i;
								s_m1_i[c] = s_m1_r[c] * p_m1_i + s_m1_i[c] * p_m1_r;
								s_m1_r[c] = t0 + input;

								// 0 stage (m)

								t0 = s_m_r[c] * p_m_r - s_m_i[c] * p_m_i;
								s_m_i[c] = s_m_r[c] * p_m_i + s_m_i[c] * p_m_r;
								s_m_r[c] = t0 + input;

								// +1 stage (p1)

								t0 = s_p1_r[c] * p_p1_r - s_p1_i[c] * p_p1_i;
								s_p1_i[c] = s_p1_r[c] * p_p1_i + s_p1_i[c] * p_p1_r;
								s_p1_r[c] = t0 + input;
							}


							audioInput++;

						}
						for (Types::fint_t c = 0; c < numChannels; ++c)
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

				inline std::complex<Scalar> getResonanceAt(std::size_t resonator)
				{
					auto const gainCoeff = N.at(resonator); // bounds checking here.
					return std::complex<Scalar>
					(
						realState[centerFilter][resonator] / gainCoeff, 
						imagState[centerFilter][resonator] / gainCoeff
					);
				}

				template<WindowTypes win, bool lazy = false>
					inline std::complex<Scalar> getWindowedResonanceAt(std::size_t resonator, std::size_t channel)
					{
						Scalar gainCoeff = N.at(resonator) * 1.0 / 20.0; // bounds checking here.
						Scalar real(0), imag(0);

						typedef typename windowFromEnum<Scalar, win>::type window;

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

				bool reallocBuffers(int minimumSize)
				{
					numFilters = minimumSize;
					// quantize to next multiple of 8, to ensure vectorization
					numResonators = numFilters + (8 - numFilters & 0x7);
					auto const dataSize = numBuffers * numResonators;
					bool newData = false;
					/*
					consider creating a copy of buffer (or, the states)
					if the buffer is resized / rads are different, interpolate the old points to the new positions.
					*/
					if (dataSize != buffer.size())
					{
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

				template<typename Vector>
				void mapSystemHz(const Vector & mappedHz, int vSize, double sampleRate)
				{

					// 7 == state size of all members
					using namespace cpl;
					CMutex lock(this);

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

							auto const k = i + 1 >= numFilters ? numFilters - 2 : i;
							auto hDiff = std::abs((double)mappedHz[k + 1] - mappedHz[k]);
							auto bandWidth = sampleRate / hDiff;
							const bool qIsFree = false;
							//auto oldBw = bandWidth;
							if(!qIsFree)
								bandWidth = cpl::Math::confineTo<double>(bandWidth, minWindowSize, maxWindowSize);


							hDiff = (sampleRate) / bandWidth;

							auto const r = exp(Bq * -M_PI * hDiff / sampleRate);

							N[i] = 1.0/(1 - r);
							//auto const theta = 2 * M_PI * mappedHz[i] / sampleRate;


							//auto const hDiffRads = 2 * M_PI * hDiff / sampleRate;

							for (int z = 0; z < numVectors; ++z)
							{

								auto const omega = (2 * M_PI * (mappedHz[i] + Bq * Math::mapAroundZero<Scalar>(z, numVectors) * hDiff)) / sampleRate;

								//auto const coeff = filters::design<filters::type::resonator, 3, Scalar>(theta, 2 * M_PI * hDiff / sampleRate, qDBs);
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
							realCoeff[z][i] = imagCoeff[z][i];
						}
					}
				}



				template<typename Vector>
				void mapSystemRads(const Vector & mappedRads, int vSize)
				{


				}
					Scalar * realState[Channels][Vectors];
					Scalar * imagState[Channels][Vectors];
				private:

					Scalar * realCoeff[Vectors];
					Scalar * imagCoeff[Vectors];


					std::size_t numFilters, numResonators;
					double maxWindowSize;
					double minWindowSize;
					double qDBs;
					double vectorDist;
					std::vector<Scalar, cpl::AlignmentAllocator<Scalar, 32u>> buffer;
					std::vector<Scalar> N;

			};

		};
	};
#endif