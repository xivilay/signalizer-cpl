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

	file:CDBMeterGraph.h

		Provides functionality for automatically computing decibel divisions in a view,
		with granularity based upon pixel length.

*************************************************************************************/

#ifndef CPL_AXISTOOLS_H
#define CPL_AXISTOOLS_H

#include <iterator>
#include "DBMeterAxis.h"
#include "FrequencyAxis.h"

namespace cpl
{
	namespace special
	{
		template<typename T, typename Vector>
		inline auto SuitableAxisDivision(const Vector & scaleTable, T desiredDivisions, T axisLength) -> decltype(std::end(scaleTable), T())
		{
			int size = static_cast<int>(std::end(scaleTable) - std::begin(scaleTable));

			auto getIncrement = [&](int level)
			{

				if (level < 0)
				{
					return static_cast<T>(std::pow(2, level));
				}
				else if (level >= size)
				{
					// scale by magnitude difference
					return static_cast<T>(scaleTable[level % size] * std::pow(scaleTable[size - 1], level / size));
				}
				else
				{
					return static_cast<T>(scaleTable[level]);
				}
			};

			int index = 0;
			std::size_t count = 0;

			std::size_t numLines = 0;

			T currentValue(1);

			for (;;)
			{
				currentValue = getIncrement(index);
				double incz1 = getIncrement(index - 1);
				std::size_t numLinesz1 = static_cast<int>(axisLength / incz1);
				numLines = static_cast<int>(axisLength / currentValue);

				if (numLines > desiredDivisions)
					index++;
				else if (numLinesz1 > desiredDivisions)
					break;
				else
					index--;

				count++;

				// TODO: converge problem? - also, provide inital estimate
				if (count > 20)
					break;
			}

			return currentValue;
		}
	}

};
#endif
