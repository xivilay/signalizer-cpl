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

	file:IIRResonator.h

		A infinite impulse response resonator system.

*************************************************************************************/
#ifndef _IIRRESONATOR_H
#define _IIRRESONATOR_H
#include "../CMutex.h"
#include "../CAudioBuffer.h"
#include "../simd.h"
#include "filterdesign.h"
#include "../LibraryOptions.h"
#include <vector>
#include "../mathext.h"

namespace cpl
{
	template<typename T, std::size_t Vectors>
	class IIRResonator : public cpl::CMutex::Lockable
	{
	public:

		typedef T Scalar;

		IIRResonator()
			: minWindowSize(8), maxWindowSize(8), numFilters(0), lowpass(nullptr), vectorQ(2 * M_PI)
		{
			for (int i = 0; i < numVectors; ++i)
				realCoeff[i] = imagCoeff[i] = realState[i] = imagState[i] = nullptr;
		}

		void setWindowSize(double minSize, double maxSize)
		{
			minWindowSize = std::min(minSize, maxSize);
			maxWindowSize = std::max(minSize, maxSize);

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
				auto audioInput = data[0];

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

				// load states
				V
					m1_r = load<V>(real[0] + filter), // real tsf: e^i*omega-q
					m1_i = load<V>(imag[0] + filter), // imag tsf: e^i*omega-q
					m_r = load<V>(real[1] + filter), // real tsf: e^i*omega
					m_i = load<V>(imag[1] + filter), // imag tsf: e^i*omega
					p1_r = load<V>(real[2] + filter), // real tsf: e^i*omega+q
					p1_i = load<V>(imag[2] + filter); // imag tsf: e^i*omega+q


				const V lpCoeff = load<V>(lowpass + filter);

				for (Types::fint_t sample = 0; sample < numSamples; ++sample)
				{

					// combing stage
					V input = broadcast<V>(audioInput);


					// -1 stage (m1)
					t0 = s_m1_r * input;
					m1_r = t0 + lpCoeff * (m1_r - t0);

					t0 = s_m1_i * input;
					m1_i = t0 + lpCoeff * (m1_i - t0);

					t0 = s_m1_r * p_m1_r - s_m1_i * p_m1_i;
					s_m1_i = s_m1_r * p_m1_i + s_m1_i * p_m1_r;
					s_m1_r = t0;

					// 0 stage (m)
					t0 = s_m_r * input;
					m_r = t0 + lpCoeff * (m_r - t0);

					t0 = s_m_i * input;
					m_i = t0 + lpCoeff * (m_i - t0);

					t0 = s_m_r * p_m_r - s_m_i * p_m_i;
					s_m_i = s_m_r * p_m_i + s_m_i * p_m_r;
					s_m_r = t0;

					// +1 stage (p1)
					t0 = s_p1_r * input;
					p1_r = t0 + lpCoeff * (p1_r - t0);

					t0 = s_p1_i * input;
					p1_i = t0 + lpCoeff * (p1_i - t0);

					t0 = s_p1_r * p_p1_r - s_p1_i * p_p1_i;
					s_p1_i = s_p1_r * p_p1_i + s_p1_i * p_p1_r;
					s_p1_r = t0;



					audioInput++;

				}

				store(realState[0] + filter, s_m1_r); // state: e^i*omega-q (real)
				store(imagState[0] + filter, s_m1_i); // state: e^i*omega-q (imag)
				store(realState[1] + filter, s_m_r); // state: e^i*omega (real)
				store(imagState[1] + filter, s_m_i); // state: e^i*omega (imag)
				store(realState[2] + filter, s_p1_r); // state: e^i*omega+q (real)
				store(imagState[2] + filter, s_p1_i); // state: e^i*omega+q (imag)


				store(real[0] + filter, m1_r); // state: e^i*omega-q (real)
				store(imag[0] + filter, m1_i); // state: e^i*omega-q (imag)
				store(real[1] + filter, m_r); // state: e^i*omega (real)
				store(imag[1] + filter, m_i); // state: e^i*omega (imag)
				store(real[2] + filter, p1_r); // state: e^i*omega+q (real)
				store(imag[2] + filter, p1_i); // state: e^i*omega+q (imag)

			}
		}

		void setVectorQ(double Q)
		{
			vectorQ = Q;
		}


		template<typename Vector>
		void mapSystemHz(const Vector & mappedHz, int vSize, double sampleRate)
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

				for (std::size_t i = 0; i < 3; ++i)
				{
					realCoeff[i] = buffer.data() + numResonators * i * 6;
					imagCoeff[i] = buffer.data() + numResonators * i * 6 + numResonators;
					realState[i] = buffer.data() + numResonators * i * 6 + numResonators * 2;
					imagState[i] = buffer.data() + numResonators * i * 6 + numResonators * 3;
					real[i] = buffer.data() + numResonators * i * 6 + numResonators * 4;
					imag[i] = buffer.data() + numResonators * i * 6 + numResonators * 5;
				}


				lowpass = buffer.data() + numResonators * 2 * 6 + numResonators * 6;

			}

			int i = 0;
			if (vSize == 1)
			{
				auto const bandWidth = minWindowSize;
				auto const Q = sampleRate / bandWidth;
				for (int z = 0; z < numVectors; ++z)
				{
					auto const theta = mappedHz[i] + (z - (numVectors - 1) >> 1) * Q;
					auto const omega = 2 * M_PI * theta / sampleRate;
					auto const coeffs = dsp::filters::design<dsp::filters::type::resonator, 1, double>(omega);

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
				lowpass[i] = Math::expDecay(bandWidth);
				i++;
			}
			else
			{
				for (i = 0; i < vSize; ++i)
				{

					auto const k = i + 1 >= numFilters ? numFilters - 2 : i;
					auto const hDiff = std::abs((double)mappedHz[k + 1] - mappedHz[k]);
					auto bandWidth = sampleRate / hDiff;
					bandWidth = cpl::Math::confineTo<double>(bandWidth, minWindowSize, maxWindowSize);

					auto const Q = vectorQ / bandWidth;

					auto const theta = 2 * M_PI * mappedHz[i] / sampleRate;
					for (int z = 0; z < numVectors; ++z)
					{


						int imdone[3] = {-1, 0, 1};
						auto const omega = (2 * M_PI * mappedHz[i] + imdone[z] * hDiff) / sampleRate;

						auto const coeffs = dsp::filters::design<dsp::filters::type::resonator, 1, double>(omega);

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

					lowpass[i] = cpl::Math::expDecay(bandWidth);

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

				for (std::size_t i = 0; i < 3; ++i)
				{
					realCoeff[i] = buffer.data() + numResonators * i * 6;
					imagCoeff[i] = buffer.data() + numResonators * i * 6 + numResonators;
					realState[i] = buffer.data() + numResonators * i * 6 + numResonators * 2;
					imagState[i] = buffer.data() + numResonators * i * 6 + numResonators * 3;
					real[i] = buffer.data() + numResonators * i * 6 + numResonators * 4;
					imag[i] = buffer.data() + numResonators * i * 6 + numResonators * 5;
				}


				lowpass = buffer.data() + numResonators * 2 * 6 + numResonators * 6;

			}

			int i = 0;
			if (vSize == 1)
			{
				auto const bandWidth = minWindowSize;
				auto const Q = vectorQ / bandWidth;
				for (int z = 0; z < numVectors; ++z)
				{
					auto const theta = mappedRads[i] + (z - (numVectors - 1) >> 1) * Q;

					auto const coeffs = dsp::filters::design<dsp::filters::type::resonator, 1, double>(theta);

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
				lowpass[i] = Math::expDecay(bandWidth);
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

						auto const coeffs = dsp::filters::design<dsp::filters::type::resonator, 1, double>(theta);

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

					lowpass[i] = cpl::Math::expDecay(bandWidth);

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

	public:
		Scalar * real[Vectors];
		Scalar * imag[Vectors];
	private:
		static const std::size_t numVectors = Vectors;
		Scalar * realCoeff[Vectors];
		Scalar * imagCoeff[Vectors];
		Scalar * realState[Vectors];
		Scalar * imagState[Vectors];

		Scalar * lowpass;
		std::size_t numFilters;
		double maxWindowSize;
		double minWindowSize;
		double vectorQ;
		std::vector<Scalar, cpl::AlignmentAllocator<Scalar, 32u>> buffer;

	};

};
#endif