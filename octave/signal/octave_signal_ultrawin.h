#ifndef OCTAVE_SIGNAL_ULTRAWIN
#define OCTAVE_SIGNAL_ULTRAWIN
namespace octave
{
	namespace signal
	{

		template<typename T>
		struct uspv_t { T f, fp; };

		typedef enum { uswpt_Xmu, uswpt_Beta, uswpt_AttFirst, uswpt_AttLast } uswpt_t;

		bool ultraspherical_window(int n, double * w, double mu, double par, uswpt_t type, int even_norm, double *xmu_);

		bool ultraspherical_window(int n, float * w, float mu, float par, uswpt_t type, int even_norm, float *xmu_);


	};
};
#endif