/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
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

	file:CSignalTransform.h
	
		A class containing methods to calculating fourier/wavelet transforms of n-d data,
		using different combinations of (a)sync parallel / accelerated DFT's and FFTs

*************************************************************************************/

#ifndef _CPUSHFILTER_H
	#define _CPUSHFILTER_H
	
	#include "../mathext.h"
	#include "../utility.h"
	#include <xmmintrin.h>

	namespace cpl
	{
		template<typename Scalar>
			class CPeakFilter
			{
			public:
				typedef Scalar ScalarTy;

				CPeakFilter() : sampleRate(0), decay(0), history(0), pole(0), fractionateMul(1) {};

				void setSampleRate(ScalarTy sampleRate)
				{
					this->sampleRate = sampleRate;
					calculatePole();
				}
				template<typename Ty>
				void setDecayAsFraction(Ty decay, Ty fractionOfSampleRate = (Ty) 1.0)
				{
					this->decay = decay;
					fractionateMul = fractionOfSampleRate;
					calculatePole();
				}
				void setDecayAsDbs(ScalarTy decay)
				{
					this->decay = Math::dbToFraction(decay);
					calculatePole();
				}

				void calculatePole()
				{
					pole = static_cast<ScalarTy>(std::pow<double>(decay, 1.0 / (sampleRate * fractionateMul)));

				}

				ScalarTy process(ScalarTy newSample)
				{
					if (newSample > history)
						history = newSample;
					else
					{
						history *= pole;
					}
					return history;
				}

				template<class Vector>
				void processRange(Vector & oldFilters, Vector newFilters, std::size_t size)
				{
					auto remainder = size % 4;
					Types::v4sf pole = _mm_set1_ps(pole);
					Types::v4sf input, output, mask, theta;

					auto const * nf = &newFilters[0];
					auto const * of = &oldFilters[0];

					for (Types::fint_t i = 0; i < size; i += 4)
					{
						input = _mm_load_ps(nf + t);
						output = _mm_load_ps(of + t);
						
						mask = _mm_cmpge_ps(input, output); // ~0 if input > output
						output = _mm_mul_ps(output, pole); // decay old output
						input = _mm_and_ps(input, mask); // input = zero if input =< output
						output = _mm_and_ps(output, mask); // output = zero if input > output
						theta = _mm_add_ps(input, output); // output = input + output
						_mm_store_ps(of + t, theta);

					}
				}

				
				ScalarTy pole;
				double sampleRate;
				double fractionateMul;
				double decay;
				ScalarTy history;
			};


	};


#endif