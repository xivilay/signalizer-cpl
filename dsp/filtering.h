#ifndef _FILTERDESIGN_H
	#define _FILTERDESIGN_H

	#include "../dsp.h"
	#include "../mathext.h"
	#include "../simd.h"
	#include <complex>

	namespace cpl
	{
		namespace dsp
		{
			namespace filters
			{


				template<class ArbitraryVector>
					inline auto convolve(const ArbitraryVector & f, std::size_t fSize, const ArbitraryVector & g, std::size_t gSize, std::ptrdiff_t n)
						->	unq_typeof(f[0])
					{
						using namespace cpl::simd;
						typedef val_typeof(f[0]) value_type;

						std::ptrdiff_t m = -gSize;
						std::size_t M = gSize;

						// scalar codepath for small sets
						if (gSize < 32)
						{
							for (; (n + m) < 0; ++n)
							{



							}


						}
						else
						{
							

						}

					}

			};
		};
	};

#endif