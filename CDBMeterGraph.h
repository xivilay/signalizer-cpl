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

			CDBMeterGraph(int lDbs = -60, int uDbs = 6, int numMaxDivisions = 10)
				: lowerDbs(-60), upperDbs(6), numMaxDivisions(numMaxDivisions)
			{
				setLowerDbs(lDbs);
				setUpperDbs(uDbs);
			}

			template<bool scale = true, bool testInfinity = false>
				double transform(double fraction)
				{
					using namespace cpl::Math;
					if (scale)
					{
						switch (scaling)
						{
						case Linear:
							return UnityScale::Inv::linear<double>(fraction, lowerFrac, upperFrac) * bounds.dist();
						case Logarithmic:
							if (testInfinity)
							{
								double point = UnityScale::Inv::exp<double>(fraction, lowerFrac, upperFrac);
								if (std::isinf(point))
									return 0;
								else
									return point * bounds.dist();
							}
							else
								return point * bounds.dist();
						}
					}
					else
					{
						switch (scaling)
						{
						case Linear:
							return UnityScale::Inv::linear<double>(fraction, lowerFrac, upperFrac);
						case Logarithmic:
							if (testInfinity)
							{
								double point = UnityScale::Inv::exp<double>(fraction, lowerFrac, upperFrac);
								if (std::isinf(point))
									return 0;
								else
									return point;
							}
							else
								return point;
						}
					}
				}

			template<class Vector, bool aligned = false, bool scale = true, bool testInfinity = false>
				void transformRangef(Vector & fractions, std::size_t size)
				{

					// the formulae used here is the inverted exponential function:
					// log10(y / _min) / log10(_max / _min);
					// except we use the natural logarithm here, which incurs no change

					std::size_t remainer = size % 4; // quadruple-vectorized

					double sizeFraction = 1.0 / std::log(upperFrac / lowerFrac);

					Types::v4sf invDivisor = _mm_set1_ps(static_cast<float>(sizeFraction));
					Types::v4sf minDivisor = _mm_set1_ps(static_cast<float>(1.0 / lowerFrac));
					Types::v4sf scale = _mm_set1_ps(static_cast<float>(bounds.dist()));
					Types::v4sf input;

					for (Types::fint_t i = 0; i < size; i += 4)
					{
						// yes we have some conditionals in this loop, but this really should be removed
						// by any compiler
						if (aligned)
							input = _mm_load_ps(&fractions[i]);
						else
							input = _mm_loadu_ps(&fractions[i]);
						input = _mm_mul_ps(input, minDivisor);
						input = cpl::sse::log_ps(input);
						input = _mm_mul_ps(input, invDivisor);
						if (scale)
							input = _mm_mul_ps(input, scale);
						if (aligned)
							_mm_store_ps(&fractions[i], input);
						else
							_mm_storeu_ps(&fractions[i], input);
					}
					// handle remainder
					for (Types::fint_t i = size - remainder; i < size; ++i)
					{
						fractions[i] = std::log(fractions[i] / lowerFrac) * sizeFraction;

					}
				}

			void setBounds(const Utility::Bounds<double> newBounds)
			{
				bounds = newBounds;
				scaleDivisions();
			}

			void setLowerDbs(int ldbs)
			{
				if (ldbs > (upperDbs - 3))
					return;
				lowerDbs = ldbs;
				compileDivisions();
			}

			void setUpperDbs(int udbs)
			{
				if (lowerDbs + 3 > udbs)
					return;
				upperDbs = udbs;
				compileDivisions();
			}
			void compileDivisions()
			{
				using namespace cpl::Math;
				int dBPerMarker = 6;
				signed int numMarkers = static_cast<signed int>(upperDbs - lowerDbs) / dBPerMarker;
				while (numMarkers > numMaxDivisions)
				{
					dBPerMarker += 6;
					numMarkers = static_cast<signed int>(upperDbs - lowerDbs) / dBPerMarker;
				}
				int i;
				for (i = 0; i < numMarkers; ++i)
				{
					// the next successive 6 dB marker
					auto const currentDBMarker = static_cast<signed int>((upperDbs - i * dBPerMarker) - (int(upperDbs) % dBPerMarker));

					auto const point = UnityScale::Inv::exp<double>(dbToFraction<double>(currentDBMarker), lowerFrac, upperFrac);
					if (point > 1)
						break;
					divisions[i].fraction = point;
					divisions[i].dbVal = currentDBMarker;
				}
				divisions.resize(i > 0 ? i - 1 : 0);
				scaleDivisions();
			}
			void scaleDivisions()
			{
				for (auto & div : divisions)
					div.coord = (1 - div.fraction) * bounds.dist();
			}
			const std::vector<DBMarker> & getDivisions() { return divisions; }

		private:
			const int numMaxDivisions; // divisions of the graph, ie. 10, 20, 30,
			double lowerFrac, upperFrac;
			int lowerDbs, upperDbs;
			Utility::Bounds<double> bounds;
			std::vector<DBMarker> divisions;
			Scaling scaling;
		};

	};
#endif