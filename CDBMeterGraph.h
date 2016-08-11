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

#ifndef CPL_CDBMETERGRAPH_H
	#define CPL_CDBMETERGRAPH_H

	#include <vector>
	#include "mathext.h"
	#include "Utility.h"

	namespace cpl
	{

		class CDBMeterGraph
		{
		public:

			/*
				allows the graph to show linear or logarithmic divisions
			*/
			enum Scaling
			{
				Logarithmic, 
				Linear
				
			};

			struct DBMarker
			{
				double coord;
				double fraction;
				double dbVal;
			};

			CDBMeterGraph(double lDbs = -60, double uDbs = 6, int numMaxDivisions = 10)
				: lowerDbs(-60), upperDbs(6), numMaxDivisions(numMaxDivisions)
			{
				setLowerDbs(lDbs);
				setUpperDbs(uDbs);
			}


			void setBounds(const Utility::Bounds<double> newBounds)
			{
				bounds = newBounds;
			}

			void setLowerDbs(double ldbs)
			{
				lowerDbs = ldbs;
			}

			void setDivisionLimit(double minPixelsForDiv)
			{
				numMaxDivisions = std::max(1.0, bounds.dist() / minPixelsForDiv);
			}

			void setUpperDbs(double udbs)
			{
				upperDbs = udbs;
			}

			void compileDivisions()
			{
				double scaleTable[] = { 1, 3, 5, 9, 12, 15, 20, 30 };
				int size = std::extent<decltype(scaleTable)>::value;

				auto getIncrement = [&](int level)
				{

					if (level < 0)
					{
						return std::pow(2, level);
					}
					else if (level >= size)
					{
						return (1.5 + 0.5 * (level - size)) * scaleTable[size - 1];
					}
					else
					{
						return scaleTable[level];
					}
				};

				divisions.clear();
				auto diff = std::abs(upperDbs - lowerDbs);
				auto sign = upperDbs > lowerDbs ? 1 : lowerDbs > upperDbs ? -1 : 0;

				if (sign == 0)
					return;

				bool selected = false;
				auto index = 0;
				int count = 0;
				double inc = 0;
				int numLines = 0;

				while (!selected)
				{
					inc = getIncrement(index);
					double incz1 = getIncrement(index - 1);
					int numLinesz1 = static_cast<int>(diff / incz1);
					numLines = static_cast<int>(diff / inc);

					if (numLines > numMaxDivisions)
						index++;
					else if (numLinesz1 > numMaxDivisions)
						break;
					else
						index--;

					count++;

					// TODO: converge problem?
					if (count > 20)
						break;
				}

	
				auto lowest = std::min(upperDbs, lowerDbs);
				auto highest = std::max(upperDbs, lowerDbs);

				auto current = cpl::Math::roundToNextMultiplier(lowest, inc);

				while (current <= highest)
				{
					double fraction = 1.0 - cpl::Math::UnityScale::Inv::linear(current, lowerDbs, upperDbs);
					divisions.push_back({ fraction * bounds.dist(), fraction, current });

					current += inc;
				}
			}

			const std::vector<DBMarker> & getDivisions() { return divisions; }

		private:
			double numMaxDivisions; // divisions of the graph, ie. 10, 20, 30,
			double lowerFrac, upperFrac;
			double lowerDbs, upperDbs;
			Utility::Bounds<double> bounds;
			std::vector<DBMarker> divisions;
			Scaling scaling;
		};

	};
#endif