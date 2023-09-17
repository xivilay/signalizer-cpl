/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2023 Janus Lynggaard Thorborg (www.jthorborg.com)

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

	file:ChannelMatrix.h

		A 2D array for audio data, internally rectangular.

*************************************************************************************/

#ifndef CPL_CHANNELMATRIX_H
#define CPL_CHANNELMATRIX_H

#include <vector>
#include "../Types.h"
#include "../dsp.h"

namespace cpl
{
	template<typename T>
	class ChannelMatrix
	{
	public:

		void resizeChannels(std::size_t length)
		{
			auxBuffers.resize(length);
		}

		void softBufferResize(std::size_t length)
		{
			auto newSize = length * auxBuffers.size();
			auxData.resize(std::max(newSize, auxData.size()));

			for (std::size_t i = 0; i < auxBuffers.size(); ++i)
				auxBuffers[i] = auxData.data() + length * i;

			bufferLength = length;
		}

		void copy(const float* const* buffers, std::size_t channelMatrixOffset, std::size_t numBuffers)
		{
			for (std::size_t i = 0; i < numBuffers; ++i)
			{
				std::memcpy(auxBuffers[i + channelMatrixOffset], buffers[i], bufferLength * sizeof(float));
			}
		}

		void accumulate(const float* const* buffers, std::size_t index, std::size_t numBuffers, float start, float end)
		{
			const auto delta = end - start;

			for (std::size_t i = 0; i < numBuffers; ++i)
			{
				for (std::size_t n = 0; n < bufferLength; ++n)
				{
					const float progress = n / float(bufferLength - 1);
					auxBuffers[i + index][n] += buffers[i][n] * (start + progress * delta);
				}
			}
		}

		void clear(std::size_t index, std::size_t numBuffers)
		{
			for (std::size_t i = 0; i < numBuffers; ++i)
			{
				std::memset(auxBuffers[i + index], 0, bufferLength * sizeof(float));
			}
		}

		void clear()
		{
			std::memset(auxData.data(), 0, auxData.size() * sizeof(float));
		}

		void copyResample(const float* buffer, std::size_t index, std::size_t numSamples)
		{
			if (numSamples == bufferLength)
				return copy(&buffer, index, 1);

			auto ratio = (double)numSamples / bufferLength;
			double x = 0;

			cpl::Types::fsint_t samples = static_cast<cpl::Types::fsint_t>(numSamples);

			for (std::size_t i = 0; i < bufferLength; ++i)
			{
				auxBuffers[index][i] = cpl::dsp::linearFilter<float>(buffer, samples, x);
				x += ratio;
			}
		}

		T* operator [] (std::size_t index) const { return auxBuffers[index]; }
		T** data() noexcept { return auxBuffers.data(); }
		std::size_t size() const noexcept { return auxBuffers.size(); }

	private:
		std::size_t bufferLength = 0;
		std::vector<T> auxData;
		std::vector<T*> auxBuffers;

	};

};


#endif
