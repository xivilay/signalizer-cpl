/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:CMutex.h
		
		Provides an interface for easily locking objects through RAII so long as
		they publically derive from CMutex::Lockable.
		Uses an special spinlock that yields the thread instead of busy-waiting.
		time-outs after a specified interval, and asks the user what to do, as well
		as providing debugger breakpoints in case of locks.

*************************************************************************************/

#ifndef _CDBMETER_H
	#define _CDBMETER_H
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
				if (ldbs > (upperDbs - 3))
					return;
				lowerDbs = ldbs;
			}

			void setDivisionLimit(double minPixelsForDiv)
			{
				numMaxDivisions = std::max(1.0, bounds.dist() / minPixelsForDiv);
			}

			void setUpperDbs(double udbs)
			{
				if (lowerDbs + 3 > udbs)
					return;
				upperDbs = udbs;
			}

			void compileDivisions()
			{
				double dynamicRange = upperDbs - lowerDbs;
				divisions.clear();
				double flooredMultiplier = std::floor(0.5 + (dynamicRange / numMaxDivisions) / 3.0);
				// will create divions each 1 db or succesive multipliers of 3
				double dbsPerMarker = std::max(1.0, flooredMultiplier * 3);

				double current = cpl::Math::roundToNextMultiplier(lowerDbs, dbsPerMarker);

				while (current <= upperDbs)
				{
					double fraction = 1.0 - cpl::Math::UnityScale::Inv::linear(current, lowerDbs, upperDbs);
					divisions.push_back({ fraction * bounds.dist(), fraction, current });

					current += dbsPerMarker;

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