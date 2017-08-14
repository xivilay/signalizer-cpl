/*************************************************************************************
 
	 cpl - cross-platform library - v. 0.3.0.
	 
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

	file:SignalSanitizer.h
	
		Sanitizes a signal for floating-point non-normals.

*************************************************************************************/

#ifndef CPL_SIGNAL_SANITIZER_H
	#define CPL_SIGNAL_SANITIZER_H

	#include "../simd.h"
	#include <type_traits>
	#include "../Misc.h"
	#include "../system/SysStats.h"

	namespace cpl
	{
		namespace dsp
		{

			class SignalSanitizer
			{
			public:

				struct Results
				{
					bool hasDenormal = false;
					bool hasNaN = false;
				};

				enum Prevention : std::uint32_t
				{
					Denormal = 1 << 0,
					NaN = 1 << 2
				};


				SignalSanitizer(std::uint32_t flags = Denormal)
					: flags(flags)
				{
					if (!system::CProcessor::test(system::CProcessor::SSE))
					{
						CPL_RUNTIME_EXCEPTION("CPU doesn't support SSE!");
					}

					hardwareFlags = _mm_getcsr();
					if (flags & Denormal)
						_mm_setcsr(hardwareFlags | 0x8040); // flush to zero + denormals are zero
				}

				template<typename T, class InVector, class OutVector>
				inline typename std::enable_if<std::is_floating_point<T>::value, Results>::type process(std::size_t samples, std::size_t channels, const InVector & input, OutVector & output, T defaultValue = (T)0)
				{
					Results ret;
					if (flags & NaN)
					{
						for (std::size_t c = 0; c < channels; ++c)
						{
							for (std::size_t i = 0; i < samples; ++i)
							{
								const auto sample = input[c][i];
								const auto fpclass = std::fpclassify(sample);
								const bool good = (fpclass != FP_INFINITE && fpclass != FP_NAN);
								if (good)
								{
									output[c][i] = sample;
								}
								else
								{
									output[c][i] = defaultValue;
									ret.hasNaN = true;
								}
							}
						}
					}
					return ret;
				}


				~SignalSanitizer()
				{
					_mm_setcsr(hardwareFlags);
				}

			private:

				decltype(_mm_getcsr()) hardwareFlags;
				std::uint32_t flags;
			};


			inline bool operator == (const SignalSanitizer::Results & left, const SignalSanitizer::Results & right)
			{
				return left.hasDenormal == right.hasDenormal && left.hasNaN == right.hasNaN;
			}

			inline bool operator != (const SignalSanitizer::Results & left, const SignalSanitizer::Results & right)
			{
				return !(left == right);
			}
		};
	};
#endif