#ifndef _FILTERDESIGN_H
	#define _FILTERDESIGN_H

	#include "../dsp.h"
	#include "../mathext.h"
	#include <complex>

	namespace cpl
	{
		namespace dsp
		{
			namespace filters
			{

				enum class type
				{
					resonator, 
					oscillator,
					butterworth
				};

				template<type f, size_t order, typename T>
					struct coefficients
					{
						typedef T Scalar;
						Scalar gain;
						std::complex<Scalar> c[order];
					};

				//template<type f, size_t order, typename T> 
				//	coefficients<f, order, T> design(T stuff);


				template<type f, size_t order, typename T>
					typename std::enable_if < f == type::oscillator, coefficients<type::resonator, 1, T>>::type
						design(T rads)
					{
						coefficients<type::oscillator, 1, T> ret;

						auto const g = std::tan(rads / 2.0);
						auto const z = 2.0 / (1.0 + g * g);

						ret.c[0] = std::complex<T>(z - 1.0, z * g);
						ret.gain = (T)1.0;
						return ret;
					}

				template<type f, size_t order, typename T>
					typename std::enable_if < f == type::resonator && order == 1,  coefficients<type::resonator, order, T>>::type
						design(T rads, T bandWidth, T QinDBs)
					{
						coefficients<type::resonator, order, T> ret;

						auto const Bq = (3 / QinDBs) * M_E / 12.0;
						auto const r = std::exp(Bq * -bandWidth / 2.0);
						auto const gain = (1.0 - r);

						ret.c[0] = std::polar(r, rads);
						ret.gain = gain;
						return ret;
					}

				template<type f, size_t order, typename T>
					typename std::enable_if < f == type::resonator && order == 3,  coefficients<type::resonator, order, T>>::type
						design(T rads, T bandWidth, T QinDBs)
					{
						coefficients<type::resonator, order, T> ret;

						auto const Bq = (3 / QinDBs) * M_E / 12.0;
						auto const r = std::exp(Bq * -bandWidth / 2.0);
						auto const gain = 1.0 / (1.0 - r);

						for (int z = 0; z < order; ++z)
						{
							ret.c[z] = std::polar(r, rads + Math::mapAroundZero<signed>(z, order) * Bq * bandWidth);
						}

						ret.gain = gain;
						return ret;
					}


			};
		};
	};

#endif