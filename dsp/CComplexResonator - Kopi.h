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

				static const std::size_t centerFilter = 1;
				static const std::size_t numVectors = Vectors;
				static const std::size_t numChannels = Channels;
				static const std::size_t numBuffersPerVector = (2 * numChannels + 2);
				static const std::size_t numBuffers = numVectors * numBuffersPerVector;
				static const std::size_t numVectorSets = numVectors * 2 * numChannels;



				CComplexResonator()
					: minWindowSize(8), maxWindowSize(8), numFilters(0), qDBs(3), vectorDist(1)
				{
					for (int i = 0; i < numVectors; ++i)
						realCoeff[i] = imagCoeff[i] = realState[i] = imagState[i] = nullptr;
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
				void wresonate(const MultiVector & data, std::size_t numChannels, std::size_t numSamples)
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
							s_m1_r = load<V>(realState[0] + filter), // cos: e^i*omega-q (real)
							s_m1_i = load<V>(imagState[0] + filter), // sin: e^i*omega-q (imag)
							s_m_r = load<V>(realState[1] + filter), // cos: e^i*omega (real)
							s_m_i = load<V>(imagState[1] + filter), // sin: e^i*omega (imag)
							s_p1_r = load<V>(realState[2] + filter), // cos: e^i*omega+q (real)
							s_p1_i = load<V>(imagState[2] + filter); // sin: e^i*omega+q (imag)

						for (Types::fint_t sample = 0; sample < numSamples; ++sample)
						{

							// combing stage
							V input = broadcast<V>(audioInput);


							// -1 stage (m1)

							t0 = s_m1_r * p_m1_r - s_m1_i * p_m1_i;
							s_m1_i = s_m1_r * p_m1_i + s_m1_i * p_m1_r;
							s_m1_r = t0 + input;

							// 0 stage (m)

							t0 = s_m_r * p_m_r - s_m_i * p_m_i;
							s_m_i = s_m_r * p_m_i + s_m_i * p_m_r;
							s_m_r = t0 + input;

							// +1 stage (p1)

							t0 = s_p1_r * p_p1_r - s_p1_i * p_p1_i;
							s_p1_i = s_p1_r * p_p1_i + s_p1_i * p_p1_r;
							s_p1_r = t0 + input;



							audioInput++;

						}

						store(realState[0] + filter, s_m1_r); // state: e^i*omega-q (real)
						store(imagState[0] + filter, s_m1_i); // state: e^i*omega-q (imag)
						store(realState[1] + filter, s_m_r); // state: e^i*omega (real)
						store(imagState[1] + filter, s_m_i); // state: e^i*omega (imag)
						store(realState[2] + filter, s_p1_r); // state: e^i*omega+q (real)
						store(imagState[2] + filter, s_p1_i); // state: e^i*omega+q (imag)

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
					inline std::complex<Scalar> getWindowedResonanceAt(std::size_t resonator)
					{
						Scalar gainCoeff = N.at(resonator) * 1.0 / 20.0; // bounds checking here.
						Scalar real(0), imag(0);

						typedef typename windowFromEnum<Scalar, win>::type window;

						if (lazy)
						{
							gainCoeff = N.at(resonator) * 1.0 / 20.0; // bounds checking here.
							for (int v = 0; v < Vectors; ++v)
							{
								std::size_t offset = Math::circularWrap(resonator + Math::mapAroundZero<signed>(v, Vectors), numFilters);
								real += window::DFTCoeffs[v] * realState[centerFilter][offset];
								imag += window::DFTCoeffs[v] * imagState[centerFilter][offset];
							}
						}
						else
						{
							for (int v = 0; v < Vectors; ++v)
							{
								real += window::DFTCoeffs[v] * realState[v][resonator];
								imag += window::DFTCoeffs[v] * imagState[v][resonator];
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
							imagCoeff[i] = kbuffer.data() + numResonators * i * numBuffersPerVector + numResonators;
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

					reallocBuffers(vSize);

					int i = 0;
					if (vSize == 1)
					{
						auto const bandWidth = minWindowSize;
						auto const Q = sampleRate / bandWidth;
						for (int z = 0; z < numVectors; ++z)
						{
							auto const theta = mappedHz[i] + (z - (numVectors - 1) >> 1) * Q;
							auto const omega = 2 * M_PI * theta / sampleRate;
							auto const coeffs = dsp::filters::design<dsp::filters::FilterType::resonator, 1, double>(omega);

							realCoeff[z][i] = coeffs.c[0].real();
							imagCoeff[z][i] = coeffs.c[0].imag();
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
							auto oldBw = bandWidth;
							if(!qIsFree)
								bandWidth = cpl::Math::confineTo<double>(bandWidth, minWindowSize, maxWindowSize);


							hDiff = (sampleRate) / bandWidth;

							auto const r = exp(Bq * -M_PI * hDiff / sampleRate);

							N[i] = 1.0/(1 - r);
							auto const theta = 2 * M_PI * mappedHz[i] / sampleRate;

							for (int z = 0; z < numVectors; ++z)
							{

								auto const omega = (2 * M_PI * mappedHz[i] + Bq *vectorDist * Math::mapAroundZero<Scalar>(z, numVectors) * hDiff) / sampleRate;

								auto const coeffs = dsp::filters::design<dsp::filters::FilterType::resonator, 1, double>(omega);

								realCoeff[z][i] = r * cos(omega); // coeffs.c[0].real()
								imagCoeff[z][i] = r * sin(omega); // coeffs.c[0].imag()
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

					// 7 == state size of all members
					using namespace cpl;
					CMutex lock(this);
					numFilters = vSize;
					auto numResonators = numFilters + (8 - numFilters & 0x7); // quantize to next multiple of 8, to ensure vectorization
					auto const dataSize = numResonators * 6 * 3 + numResonators;
					bool newData = false;
					/*
						consider creating a copy of buffer (or, the states)
						if the buffer is resized / rads are different, interpolate the old points to the new positions.
					*/
					if (dataSize != buffer.size())
					{
						newData = true;
						buffer.resize(dataSize);
						N.resize(vSize);
						for (std::size_t i = 0; i < 3; ++i)
						{
							realCoeff[i] = buffer.data() + numResonators * i * 4;
							imagCoeff[i] = buffer.data() + numResonators * i * 4 + numResonators;
							realState[i] = buffer.data() + numResonators * i * 4 + numResonators * 2;
							imagState[i] = buffer.data() + numResonators * i * 4 + numResonators * 3;
						}

					}

					int i = 0;
					if (vSize == 1)
					{
						auto const bandWidth = minWindowSize;
						auto const Q = vectorQ / bandWidth;
						for (int z = 0; z < numVectors; ++z)
						{
							auto const theta = mappedRads[i] + (z - (numVectors - 1) >> 1) * Q;

							auto const coeffs = dsp::filters::design<dsp::filters::FilterType::resonator, 1, double>(theta);

							realCoeff[z][i] = coeffs.c[0].real();
							imagCoeff[z][i] = coeffs.c[0].imag();
							if (newData)
							{
								realState[z][i] = 1.0;
								imagState[z][i] = 0;
								real[z][i] = 0;
								imag[z][i] = 0;
							}

						}
						i++;
					}
					else
					{
						for (i = 0; i < vSize; ++i)
						{

							auto const k = i + 1 >= numFilters ? numFilters - 2 : i;
							auto bandWidth = 2 * M_PI / std::abs((double)mappedRads[k + 1] - mappedRads[k]);
							bandWidth = cpl::Math::confineTo<double>(bandWidth, minWindowSize, maxWindowSize);

							auto const Q = vectorQ / bandWidth;


							for (int z = 0; z < numVectors; ++z)
							{
								auto const theta = mappedRads[i] + (z - (numVectors - 1) >> 1) * Q;

								auto const coeffs = dsp::filters::design<dsp::filters::FilterType::resonator, 1, double>(theta);

								realCoeff[z][i] = coeffs.c[0].real();
								imagCoeff[z][i] = coeffs.c[0].imag();
								if (newData)
								{
									realState[z][i] = 1.0;
									imagState[z][i] = 0;
									real[z][i] = 0;
									imag[z][i] = 0;
								}


							}

						}
					}
					// set remainder to zero
					for (; i < numResonators; ++i)
					{
						for (int z = 0; z < numVectors; ++z)
						{
							realCoeff[z][i] = imagCoeff[z][i] = real[z][i] = imag[z][i] = 0;
						}
						lowpass[i] = 0;
					}
				}
					Scalar * realState[Channels][Vectors];
					Scalar * imagState[Channels][Vectors];
				private:
					static const std::size_t numVectors = Vectors;
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