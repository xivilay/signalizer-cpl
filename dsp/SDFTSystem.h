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

	file:SDFTSystem.h
	
		A sliding discrete fourier transform system.

*************************************************************************************/

#ifndef _SDFTSYSTEM_H
	#define _SDFTSYSTEM_H

	#include <cpl/CMutex.h>
	#include <cpl/CAudioBuffer.h>
	#include <cpl/dsp.h>
	#include <cpl/simd.h>


	namespace cpl
	{
		template<typename T, std::size_t Vectors>
		struct SDFTSystem : public cpl::CMutex::Lockable
		{
			static const std::size_t numVectors = Vectors;
			T * realPoles[Vectors];
			T * imagPoles[Vectors];
			T * realState[Vectors];
			T * imagState[Vectors];
			unsigned * N;
			unsigned numFilters;
			unsigned maxWindowSize;
			unsigned minWindowSize;
			std::vector<T, cpl::AlignmentAllocator<T, 32u >> relay;
			cpl::CAudioBuffer::CChannelBuffer & audioData;
			std::vector<T, cpl::AlignmentAllocator<T, 32u>> buffer;

			typedef T Scalar;

			SDFTSystem(cpl::CAudioBuffer::CChannelBuffer & buffer)
				: audioData(buffer)
			{

			}
			void setVectorQ(double)
			{}
			void setWindowSize(unsigned _min, unsigned _max)
			{
				minWindowSize = std::min(_min, _max);
				maxWindowSize = std::max(_min, _max);
			}

			template<typename V, class MultiVector>
			void wresonate(const MultiVector & data, std::size_t numChannels, std::size_t numSamples)
			{

				using namespace cpl;
				using namespace cpl::simd;
				CFastMutex lock(this);
				relay.resize(numSamples);
				// starting offset in circular buffer
				Types::fsint_t ptr = audioData.start;

				auto const vfactor = suitable_container<V>::size;
				auto size = audioData.size;
				V t0;
				std::ptrdiff_t s2 = -signed(size);
				typename scalar_of<V>::type * ptrs[elements_of<V>::value];
				const auto upperBounds = audioData.buffer + size;

				auto const maxOffset = ptr + numSamples;

				// save old circulay buffer values
				// if the number of samples added to the current position
				// in the circular buffer, a memcpy call is invalid.
				// we have to copy with range reduction then.
				// this is a rare case though: we will use memcpy almost always,
				// and branch prediction should eradicate the conditional.

				if (size > maxOffset)
				{
					std::memcpy(relay.data(), audioData.buffer + ptr, numSamples * sizeof(typename scalar_of<V>::type));
				}
				else
				{
					auto bufferPtr = audioData.buffer + ptr;
					auto relayPtr = relay.data();
					for (Types::fint_t i = 0; i < numSamples; ++i)
					{
						*relayPtr = *bufferPtr;
						bufferPtr++;
						relayPtr++;
						bufferPtr += (bufferPtr >= upperBounds) ? s2 : (std::ptrdiff_t)0;
					}
				}


				for (Types::fint_t filter = 0; filter < numFilters; filter += vfactor)
				{

					// pointer to current sample
					auto audioInput = data[0];
					// pointer to current sample in circular buffer
					auto audioBuffer = audioData.buffer + ptr;


					// load coefficients
					V
						p_m1_r = load<V>(realPoles[0] + filter), // pole: e^i*omega-q (real)
						p_m1_i = load<V>(imagPoles[0] + filter), // pole: e^i*omega-q (imag)
						p_m_r = load<V>(realPoles[1] + filter), // pole: e^i*omega (real)
						p_m_i = load<V>(imagPoles[1] + filter), // pole: e^i*omega (imag)
						p_p1_r = load<V>(realPoles[2] + filter), // pole: e^i*omega+q (real)
						p_p1_i = load<V>(imagPoles[2] + filter); // pole: e^i*omega+q (imag)
					// load states
					V
						s_m1_r = load<V>(realState[0] + filter), // state: e^i*omega-q (real)
						s_m1_i = load<V>(imagState[0] + filter), // state: e^i*omega-q (imag)
						s_m_r = load<V>(realState[1] + filter), // state: e^i*omega (real)
						s_m_i = load<V>(imagState[1] + filter), // state: e^i*omega (imag)
						s_p1_r = load<V>(realState[2] + filter), // state: e^i*omega+q (real)
						s_p1_i = load<V>(imagState[2] + filter); // state: e^i*omega+q (imag)

					for (Types::fint_t z = 0; z < vfactor; ++z)
					{
						// load comb filter
						auto offset = ptr - (signed int)N[filter + z];
						// wrap into buffer range
						offset += (offset < 0) * size;
						offset -= (offset >= size) * size;
						ptrs[z] = audioData.buffer + offset;

					}


					for (Types::fint_t sample = 0; sample < numSamples; ++sample)
					{

						// combing stage
						V input = broadcast<V>(audioInput) - gather<V>(ptrs);

						// resonate filters
						/*
							UPDATED algo:

								Sk(n) = e^j2*pi*l/N * [Sk(n - 1) + x(n) - x(n - N)].

								http://www.cmlab.csie.ntu.edu.tw/DSPCourse/reference/Sliding%20DFT%20update.pdf

								.. which means

								state.real += x(n) - x(n - N);
								state = c * state;

								.. old is:

								state = c * state;
								state.real += x(n) - x(n - N);

						*/
						s_m1_r += input;
						t0 = s_m1_r * p_m1_r - s_m1_i * p_m1_i;
						s_m1_i = s_m1_r * p_m1_i + s_m1_i * p_m1_r;
						s_m1_r = t0;

						s_m_r += input;
						t0 = s_m_r * p_m_r - s_m_i * p_m_i;
						s_m_i = s_m_r * p_m_i + s_m_i * p_m_r;
						s_m_r = t0;

						s_p1_r += input;
						t0 = s_p1_r * p_p1_r - s_p1_i * p_p1_i;
						s_p1_i = s_p1_r * p_p1_i + s_p1_i * p_p1_r;
						s_p1_r = t0;

						// move combfilter forward
						for (Types::fint_t z = 0; z < vfactor; ++z)
						{
							ptrs[z]++;
							ptrs[z] += (ptrs[z] >= upperBounds) ? s2 : (std::ptrdiff_t)0; // lengths[z] %= size; (wrap around)
						}

						// store current sample in buffer
						*audioBuffer = *audioInput;
						audioInput++;
						audioBuffer++;
						audioBuffer += (audioBuffer >= upperBounds) ? s2 : (std::ptrdiff_t)0; // lengths[z] %= size; (wrap around)
					}



					store(realState[0] + filter, s_m1_r); // state: e^i*omega-q (real)
					store(imagState[0] + filter, s_m1_i); // state: e^i*omega-q (imag)
					store(realState[1] + filter, s_m_r); // state: e^i*omega (real)
					store(imagState[1] + filter, s_m_i); // state: e^i*omega (imag)
					store(realState[2] + filter, s_p1_r); // state: e^i*omega+q (real)
					store(imagState[2] + filter, s_p1_i); // state: e^i*omega+q (imag)

					// here we revert all changes done to the circular buffer
					// so the next filter starts with the same state.
					// kind of bad, but required since the combfilter alters the state
					// per sample, and we loop all samples for each filter.
					// otherwise, we would have to load each filter for each sample instead.
					if (size > maxOffset)
					{
						std::memcpy(audioData.buffer + ptr, relay.data(), numSamples * sizeof(typename scalar_of<V>::type));
					}
					else
					{
						auto bufferPtr = audioData.buffer + ptr;
						auto relayPtr = relay.data();
						for (Types::fint_t i = 0; i < numSamples; ++i)
						{
							*bufferPtr = *relayPtr;
							bufferPtr++;
							relayPtr++;
							bufferPtr += (bufferPtr >= upperBounds) ? s2 : (std::ptrdiff_t)0;
						}
					}

				}
			}

					template<typename Vector>
			void mapResonatingSystem(const Vector & mappedFrequencies, int vSize)
			{

				// 7 == state size of all members
				using namespace cpl;
				CMutex lock(this);
				numFilters = vSize;
				auto numResonators = numFilters + (8 - numFilters & 0x7); // quantize to next multiple of 8, to ensure vectorization
				auto const dataSize = numResonators * 4 * 3 + numResonators;
				auto const sampleRate = audioData.sampleRate;

				buffer.resize(dataSize);

				for (std::size_t i = 0; i < 3; ++i)
				{
					realPoles[i] = buffer.data() + numResonators * i * 4;
					imagPoles[i] = buffer.data() + numResonators * i * 4 + numResonators;
					realState[i] = buffer.data() + numResonators * i * 4 + numResonators * 2;
					imagState[i] = buffer.data() + numResonators * i * 4 + numResonators * 3;
				}



				N = reinterpret_cast<unsigned*>(buffer.data() + numResonators * 2 * 4 + numResonators * 4); // ewww

				std::size_t windowSize = maxWindowSize;
				std::size_t minWindow = minWindowSize;
				int i = 0;
				if (vSize == 1)
				{
					auto const omega = 2 * mappedFrequencies[0] * M_PI / sampleRate;

					auto const bandWidth = maxWindowSize;

					auto const Q = 2 * M_PI / bandWidth;

					realPoles[0][i] = std::cos(omega - Q);
					realPoles[1][i] = std::cos(omega);
					realPoles[2][i] = std::cos(omega + Q);
					imagPoles[0][i] = std::sin(omega - Q);
					imagPoles[1][i] = std::sin(omega);
					imagPoles[2][i] = std::sin(omega + Q);
					N[i] = cpl::Math::round<unsigned>(bandWidth);

					i++;
				}
				else
				{
					for (i = 0; i < vSize; ++i)
					{

						auto const omega = 2 * M_PI * i / double(vSize);


						auto const Q = 2 * M_PI / double(vSize);

						realPoles[0][i] = std::cos(omega - Q);
						realPoles[1][i] = std::cos(omega);
						realPoles[2][i] = std::cos(omega + Q);
						imagPoles[0][i] = std::sin(omega - Q);
						imagPoles[1][i] = std::sin(omega);
						imagPoles[2][i] = std::sin(omega + Q);

						realState[0][i] = realState[1][i] = realState[2][i] = 0;
						imagState[0][i] = imagState[1][i] = imagState[2][i] = 0;

						N[i] = vSize;
					}
				}
				for (; i < numResonators; ++i)
				{
					N[i] = minWindowSize;
					realPoles[0][i] = realPoles[1][i] = realPoles[2][i] = imagPoles[0][i] = imagPoles[1][i] = imagPoles[2][i] = 0;
				}
			}


			template<typename Vector>
			void mapResonatingSystemFree(const Vector & mappedFrequencies, int vSize)
			{

				// 7 == state size of all members
				using namespace cpl;
				CMutex lock(this);
				numFilters = vSize;
				auto numResonators = numFilters + (8 - numFilters & 0x7); // quantize to next multiple of 8, to ensure vectorization
				auto const dataSize = numResonators * 4 * 3 + numResonators;
				auto const sampleRate = audioData.sampleRate;

				buffer.resize(dataSize);

				for (std::size_t i = 0; i < 3; ++i)
				{
					realPoles[i] = buffer.data() + numResonators * i * 4;
					imagPoles[i] = buffer.data() + numResonators * i * 4 + numResonators;
					realState[i] = buffer.data() + numResonators * i * 4 + numResonators * 2;
					imagState[i] = buffer.data() + numResonators * i * 4 + numResonators * 3;
				}



				N = reinterpret_cast<unsigned*>(buffer.data() + numResonators * 2 * 4 + numResonators * 4); // ewww

				std::size_t windowSize = maxWindowSize;
				std::size_t minWindow = minWindowSize;
				int i = 0;
				if (vSize == 1)
				{
					auto const omega = 2 * mappedFrequencies[0] * M_PI / sampleRate;

					auto const bandWidth = maxWindowSize;

					auto const Q = 2 * M_PI / bandWidth;

					realPoles[0][i] = std::cos(omega - Q);
					realPoles[1][i] = std::cos(omega);
					realPoles[2][i] = std::cos(omega + Q);
					imagPoles[0][i] = std::sin(omega - Q);
					imagPoles[1][i] = std::sin(omega);
					imagPoles[2][i] = std::sin(omega + Q);
					N[i] = cpl::Math::round<unsigned>(bandWidth);

					i++;
				}
				else
				{
					for (i = 0; i < vSize; ++i)
					{

						auto const omega = 2 * mappedFrequencies[i] * M_PI / sampleRate;


						auto const k = i + 1 >= numFilters ? numFilters - 2 : i;
						auto bandWidth = sampleRate / ((double)mappedFrequencies[k + 1] - mappedFrequencies[k]);
						bandWidth = cpl::Math::confineTo<double>(bandWidth, minWindow, windowSize);

						auto const Q = 2 * M_PI / bandWidth;

						realPoles[0][i] = std::cos(omega - Q);
						realPoles[1][i] = std::cos(omega);
						realPoles[2][i] = std::cos(omega + Q);
						imagPoles[0][i] = -std::sin(omega - Q);
						imagPoles[1][i] = -std::sin(omega);
						imagPoles[2][i] = -std::sin(omega + Q);
						N[i] = cpl::Math::round<unsigned>(bandWidth);
					}
				}
				for (; i < numResonators; ++i)
				{
					N[i] = minWindowSize;
					realPoles[0][i] = realPoles[1][i] = realPoles[2][i] = imagPoles[0][i] = imagPoles[1][i] = imagPoles[2][i] = 0;
				}
			}
		};

	};
#endif