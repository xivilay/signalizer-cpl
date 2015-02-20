#ifndef CLOWPASS_H
	#define CLOWPASS_H
	#include <cmath>

	namespace cpl
	{
		class CLowPass
		{
			double coef[9];
			double state[4];
			double omega; //peak freq
			double gi; //peak mag

		public:

			CLowPass()
				: omega(400 / 8000.0), gi(0.01), state()
			{
				calculateCoeffs();
			}

			void setCutoff(double hertz)
			{
				omega = hertz / 8000;
				calculateCoeffs();
			}

			void setResonance(double res)
			{
				gi = res;
				calculateCoeffs();
			}
			template<typename Ty>
				inline float process(Ty sIn)
				{

					double sOut = coef[0] * sIn + state[0];

					state[0] = coef[1] * sIn + coef[5] * sOut + state[1];
					state[1] = coef[2] * sIn + coef[6] * sOut + state[2];
					state[2] = coef[3] * sIn + coef[7] * sOut + state[3];
					state[3] = coef[4] * sIn + coef[8] * sOut;

					return static_cast<Ty>(sOut);

				}

			void calculateCoeffs()
			{
				double k, p, q, a;
				double a0, a1, a2, a3, a4;
				double g = 0.5 + gi * 10;
				omega = omega == 0.0f ? 0.001 : omega;
				k = (4.0*g - 3.0) / (g + 1.0);
				p = 1.0 - 0.25*k; p *= p;

				// LP:
				a = 1.0 / (std::tan(0.5*omega)*(1.0 + p));
				p = 1.0 + a;
				q = 1.0 - a;

				a0 = 1.0 / (k + p*p*p*p);
				a1 = 4.0*(k + p*p*p*q);
				a2 = 6.0*(k + p*p*q*q);
				a3 = 4.0*(k + p*q*q*q);
				a4 = (k + q*q*q*q);
				p = a0*(k + 1.0);

				coef[0] = p;
				coef[1] = 4.0*p;
				coef[2] = 6.0*p;
				coef[3] = 4.0*p;
				coef[4] = p;
				coef[5] = -a1*a0;
				coef[6] = -a2*a0;
				coef[7] = -a3*a0;
				coef[8] = -a4*a0;

			}
		};
	};

#endif