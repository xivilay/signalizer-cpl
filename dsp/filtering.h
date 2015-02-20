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

	file:filtering.h
	
		filter algorithms.

*************************************************************************************/

#ifndef _FILTERING_H
	#define _FILTERING_H

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