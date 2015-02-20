#ifndef _FILTERDESIGN_H
	#define _FILTERDESIGN_H
	#include "../dsp.h"
	#include <complex>

	namespace cpl
	{
		namespace dsp
		{
			namespace filters
			{

				enum class type
				{
					butterworth,
					chebyshev1,
					chebyshev2,
					elliptic
				};
				
				enum class kind
				{
					resonator,
					bandpass,
					bandstop,
					lowpass,
					highpass
					
				};

				template<typename T>
					T prewarp(T omega, T integrationStep = 1.0)
					{
						return (2.0/integrationStep) * std::tan(omega * integrationStep * 0.5);
					}
				
				
				template<kind f, size_t order = 1, typename T = double>
					struct coefficients
					{
						typedef T Scalar;
						std::complex<Scalar> c[order];
					};

				template<kind f, size_t order, typename T>
					coefficients<f, order, T> design(T stuff);


				template<>
					coefficients<kind::resonator> design(double rads)
					{
						coefficients<kind::resonator> ret;
						// prewarp
						auto const g = std::tan(rads / 2.0);
						// intermediate sine and cosine terms
						// trig^2(t) = 1 - trig(t -pi/2)^2
						auto const z = 2.0 / (1.0 + g * g);

						ret.c[0] = std::complex<double>(z - 1.0, z * g);

						return ret;
					}




			};
		};
	};

#endif