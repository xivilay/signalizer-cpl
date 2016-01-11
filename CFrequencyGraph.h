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

#ifndef _CFREQUENCYGRAPH_H
	#define _CFREQUENCYGRAPH_H

	//#define _CFREQUENCYGRAPH_DO_CHECKS

	#include <cmath>
	#include <vector>
	#include <algorithm>
	#include "mathext.h"
	#include "Utility.h"

	namespace cpl
	{



		class CFrequencyGraph
		{
		public:
			/*
				these are given seperate of small divisions so that they may
				be emphasized in the graph.
			*/
			struct MajorDivision
			{
				double coord;
				double frequency;
			};
			/*
				allows the graph to show linear or logarithmic divisions
			*/
			enum Scaling
			{
				Logarithmic, 
				Linear

			};

			CFrequencyGraph(const Utility::Bounds<double> bounds, // bounds are the actual coordinates the graph spans ('world')
				const Utility::Bounds<double> view, // view are the zoomed coords the graph should span, ie. a subset 
				double maxFrequency, // the maximum frequency of the graph to show. typically sampleRate / 2
				double startDecade = 10.0) // starting decade of the graph. for logarithmic graphs, it cannot be zero.
				: bounds(bounds), view(view), startDecade(startDecade), stopFreq(maxFrequency), scaling(Linear), minSpaceForDivision(1000000000000)
			{
				setup();
			}

			/*
				sets the bounds of the graph. may trigger a call to retransform the lines.
			*/
			void setBounds(const Utility::Bounds<double> newBounds)
			{
				bounds = newBounds;
				setup(); 
				//retransform();
				// should also recompute lines - if more space is given, more lines may be visible?
			}


			/// <summary>
			/// Sets the upper limit of the frequency graph.
			/// </summary>
			/// <param name="frequency"></param>
			void setMaxFrequency(double frequency)
			{
				if (frequency != stopFreq)
				{
					stopFreq = frequency;
					setup();
				}
			}

			/*
				allows to change the scaling of the graph.
				if a new value is given, it may trigger a call to recompute the lines.
			*/
			void setScaling(Scaling s)
			{
				if (s != scaling)
				{
					scaling = s;
					setup();
					//compileGraph();
				}
			}
			/*
				change the amount of pixels needed to trigger a recursive inclusion of divisions.
				the smaller the value, the more divisions! Capped at 1.
			*/
			void setDivisionLimit(double size)
			{
				minSpaceForDivision = std::max(1.0, size);
			}
			/*
				sets the view coordinates.
				may trigger a call to recompute lines and retransformation
			*/
			void setView(const Utility::Bounds<double> newView)
			{
				view = newView;
				setup();
				//retransform();
			}
			/*
				converts view-coordinates to world coordinates
			*/
			double viewToBounds(double viewCoordinate)
			{
				return ((viewCoordinate - view.left) / viewWidth) * boundsWidth;
			}
			/*
				converts world coordinates to view-coordinates
			*/
			double boundsToView(double boundedCoordinate)
			{
				return ((boundedCoordinate - bounds.left) / boundsWidth) * viewWidth;
			}
			/*
				returns the frequency corrosponding to a fraction of the length of the window;
				the current view scaling is taken into account.
				This would be the equivalent of mousePosition to frequency;
			*/
			double fractionToFrequency(double fraction)
			{
				// transform into world from view
				double selFreqPos = (view.left + fraction * (viewWidth)) / boundsWidth;
				return scaleFractionToFrequency(selFreqPos);
			}
			/*
				gives the fraction a scaling according to the scaling of the graph (linear/logarithmic)
				and scales this fraction into a view-coordinate
			*/
			double fractionToCoordTransformed(double fraction)
			{
				#ifdef _CFREQUENCYGRAPH_DO_CHECKS
					fraction = cpl::Math::confineTo(fraction, 0.0, 1.0);
				#endif
				return transform(transformFraction(fraction));
			}

			/*
				gives the fraction a scaling corrosponding to the scaling of the graph.
				Ie. for linear scaling, it basically returns the same fraction (although
				it may be clipped inside [0, 1], while it gives logarithmic scaling otherwise.
			*/
			double transformFraction(double fraction)
			{

				#ifdef _CFREQUENCYGRAPH_DO_CHECKS
					fraction = cpl::Math::confineTo(fraction, 0.0, 1.0);
				#endif
				
				if (scaling == Scaling::Linear)
					return fraction;

				double actualFrequency = fraction * stopFreq;
				return (std::log10(actualFrequency) - 1) * spaceForDecade;
			}

			/*
				transforms the coordinate, which is assumed to be a view-coordinate
				into a frequency
			*/
			double frequencyForCoord(double coord)
			{
				auto fraction = invTransform(coord);
				#ifdef _CFREQUENCYGRAPH_DO_CHECKS
					fraction = cpl::Math::confineTo(fraction, 0.0, 1.0);
				#endif
				return scaleFractionToFrequency(fraction);
			}

			/*
				after a compilation, lines are stored as fraction.
				if only the view is changed, but not the 'zoom'-amount,
				this can be called to retransform the lines into world
				coordinates
			*/
			void retransform() { transformLines(); }
			/*
				returns the lines that seperate the graph into common subdivisions
			*/
			const std::vector<double> & getLines() { return trans; }
			/*
				returns the lines that seperate the graph into major divisions,
				like 10, 100, 1000 etc.
				they also carry the corrosponding frequency with them.
			*/
			const std::vector<MajorDivision> & getDivisions() { return transTitles; }

			/*
				transforms a world fraction into a view-coordinate
			*/
			double transform(double fraction)
			{
				//return  view.left + ((fraction * boundsWidth) / viewWidth) * boundsWidth;

				double temp = (fraction * boundsWidth - view.left + bounds.left) / viewWidth;
				#ifdef _CFREQUENCYGRAPH_DO_CHECKS
					temp = cpl::Math::confineTo(temp, 0.0, 1.0);
				#endif
				return  bounds.left + temp * boundsWidth;
			}
			/*
				the inverse of the transform()
			*/
			double invTransform(double coord)
			{
				return ((((coord - bounds.left) * viewWidth) / boundsWidth) - bounds.left + view.left) / boundsWidth;
			}

			/*
				transforms the input to a fraction of view over the bounds (the 'world').
				ie., if the view is zoomed in, this will scale the value down.
			*/
			double scale(double in)
			{
				return in * (viewWidth / boundsWidth);
			}
			/*
				resets all current lines, and computes all possible lines based on current parameters
				and storing them. also transforms the lines, so getLines() are valid after this call. 
			*/
			void compileGraph()
			{
				// reset raw data buffer
				untrans.clear();
				titles.clear();
				lowerFreq = frequencyForCoord(bounds.left);
				higherFreq = frequencyForCoord(bounds.right);

				if (scaling == Scaling::Logarithmic)
					compileLogGraph();
				else
					compileLinearGraph();

				transformLines();
			}

		protected:

			/*
				recursive function that computes subdivisions of decades for linear scaling
				- do not call directly!
			*/
			bool compileLinearSubDecade(double offset, double step)
			{
				// the amount of space it would take to draw numDivision subdivions.
				auto const nextHigherFreq = offset + step * numDivisions;

				if (nextHigherFreq < lowerFreq)
					return true;

				double spaceForSub = (nextHigherFreq - offset) / stopFreq;
				spaceForSub *= boundsWidth;
				// due to the logarithmic nature of spacing, if the recursing fails once, everything forward will fail.
				// this is a small optimization.
				bool dontRecurse = false;
				bool printNextLine = true;

				if (spaceForSub > scale(minSpaceForDivision))
				{

					for (int i = 0; i < numDivisions; ++i)
					{
						double localOffset = i * step;
						if (offset + localOffset > nextHigherFreq)
							return false;
						if (dontRecurse || !compileLinearSubDecade(offset + localOffset, step / 10.0))
						{
							dontRecurse = true;
							if (printNextLine && i > 0) // quirky..
								saveLine((offset + localOffset) / stopFreq);
						}
						else
						{
							// a bit arcane logic here. this has to be here to prevent the last part of this
							// function overwriting the last possible subdivision.
							// another method to all this logic is sorting and removing duplicate entries in
							// this->untrans and this->titles
							printNextLine = false;
							continue;
						}
						printNextLine = true;
					}

				}
				else
				{
					return false;
				}
				// this part gets excluded if the last part of these subdivisions was printed using a recursive call
				// in that case, dont bother doing this
				if (printNextLine)
				{
					auto const coord = nextHigherFreq / stopFreq;
					saveDivision(coord, nextHigherFreq);
				}
				return true;
			}

			void compileLinearGraph()
			{

				// loop each decade

				double curDecade = lastDecade;
				double minSpace = scale(minSpaceForDivision);
				double scaledBounds = boundsWidth / stopFreq;
				while(true)
				{
					// to avoid painting smaller divisions onto major.
					bool subDivsWillBeDivisions = false;

					// check if drawing divisions of decades would be too small
					if (curDecade * scaledBounds > minSpace)
					{
						if (curDecade * scaledBounds > minSpace * 10)
							subDivsWillBeDivisions = true;

						auto currentFreq = cpl::Math::roundToNextMultiplier(lowerFreq, curDecade);

						while (currentFreq < higherFreq)
						{
							if (std::fmod(currentFreq, curDecade * 10.0) != 0.0)
							{
								saveDivision(currentFreq / stopFreq, currentFreq);

							}

							currentFreq += curDecade;

						}
						

					}
					else
					{
						break;
					}

					auto subDivSpace = curDecade / 10.0;


					// draw subdivisions of this decade.
					if (!subDivsWillBeDivisions && (subDivSpace * scaledBounds > (minSpace * 0.25))) // trigger subdivisions before major divisions
					{


						auto currentSubFreq = cpl::Math::roundToNextMultiplier(lowerFreq, subDivSpace);
						while (currentSubFreq < higherFreq)
						{
							// avoid submitting lines our 'parent' may have done
							if (std::fmod(currentSubFreq, curDecade) != 0.0)
							{
								saveLine(currentSubFreq / stopFreq);

							}
							currentSubFreq += subDivSpace;
						}

					}

					curDecade /= 10.0;
				}
			}

			void compileLogGraph()
			{
				double minStartDecade = 0, minStopDecade = 0;

				double nextLowPow10 = std::pow(10, std::ceil(std::log10(lowerFreq)));
				double nextHighPow10 = std::pow(10, std::ceil(std::log10(higherFreq)));

				minStartDecade = std::max(startDecade, nextLowPow10 / 10);
				minStopDecade = std::min(lastDecade * 10, nextHighPow10);


				// loop each decade
				for (double curDecade = minStartDecade;
				curDecade < minStopDecade;
					curDecade *= 10)
				{
					// check if drawing divisions of decades would be too small
					if (spaceForDecade * boundsWidth > scale(minSpaceForDivision))
					{
						bool printNextLine = true;
						// calculating divisions inside the powers of 10's
						for (int div = 1; div < numDivisions; ++div)
						{
							if (curDecade * (div + 2) < lowerFreq)
								continue;
							if (curDecade * (div - 1) > higherFreq)
								break;
							// see if we can print a subdivision - the function checks it itself. 
							// if it returned true, it printed the lines and we dont have to print this line,
							// since it is included.
							if (!compileLogSubDecade(curDecade * div, curDecade / 10.0))
							{
								// if curDecade is 10, and subDiv is 5, then the frequency of the current lines
								// is curDecade * subDiv = 50Hz
								double fracOffForDiv = (std::log10(div * curDecade) - 1.0) * spaceForDecade;
								// this is a lame hack to preventing these grey lines overwriting decades
								if (printNextLine && div > 1)
									saveLine(fracOffForDiv);
								else
									printNextLine = true;
							}
							else
								printNextLine = false;
						}
					}
					// mark current decade
					saveDivision((std::log10(curDecade / 10.0)) * spaceForDecade, curDecade);
				}
			}

			/*
				Maps transformed fraction to a frequency
			*/
			inline double scaleFractionToFrequency(double fraction)
			{
				if (scaling == Scaling::Logarithmic)
					return Math::UnityScale::exp(fraction, startDecade, stopFreq);
				else if (scaling == Scaling::Linear)
					return fraction * stopFreq;

				return 0.0;
			}
			void saveLine(double coord) { untrans.push_back(coord); }
			void saveDivision(double coord, double text) { titles.push_back({ coord, text }); };
			/*
				sets up coefficients and such when parameters has changed.
			*/
			void setup()
			{
				// the fractionate space for each decade, a general scaling factor
				spaceForDecade = 1.0 / (std::log10(stopFreq) - 1.0);

				// this is where to start the graph
				lastDecade = std::pow(10.0 /* startDecade? */, std::floor(std::log10(stopFreq)));
				boundsWidth = bounds.right - bounds.left;
				viewWidth = view.right - view.left;
				//minSpaceForDivision = boundsWidth * this->spaceForDecade;
			}



			/*
				recursive function that computes subdivisions of decades for logarithmic scaling
				- do not call directly!
			*/
			bool compileLogSubDecade(double offset, double step)
			{
				// TODO: still hangs sometimes
				// the amount of space it would take to draw numDivision subdivions.
				auto const nextHigherFreq = offset + step * numDivisions;

				if (nextHigherFreq < lowerFreq)
					return true;
				
				double spaceForSub = (std::log10(nextHigherFreq) - std::log10(offset)) * spaceForDecade;
				spaceForSub *= boundsWidth;
				// due to the logarithmic nature of spacing, if the recursing fails once, everything forward will fail.
				// this is a small optimization.
				bool dontRecurse = false;
				bool printNextLine = true;

				if (spaceForSub > scale(minSpaceForDivision))
				{

					for (int i = 0; i < numDivisions; ++i)
					{
						double localOffset = i * step;
						if (offset + localOffset > nextHigherFreq)
							return false;
						if (dontRecurse || !compileLogSubDecade(offset + localOffset, step / 10.0))
						{
							dontRecurse = true;
							if (printNextLine && i > 0) // quirky..
								saveLine((std::log10(offset + localOffset) - 1) * spaceForDecade);
						}
						else
						{
							// a bit arcane logic here. this has to be here to prevent the last part of this
							// function overwriting the last possible subdivision.
							// another method to all this logic is sorting and removing duplicate entries in
							// this->untrans and this->titles
							printNextLine = false;
							continue;
						}
						printNextLine = true;
					}

				}
				else
				{
					return false;
				}
				// this part gets excluded if the last part of these subdivisions was printed using a recursive call
				// in that case, dont bother doing this
				if (printNextLine)
				{
					auto const coord = (std::log10(nextHigherFreq) - 1) * spaceForDecade;
					saveDivision(coord, nextHigherFreq);
				}
				return true;
			}

			/*
				transforms the computed lines into coordinates, only storing those that are inside
				of the bounds.
			*/
			void transformLines()
			{
				trans.clear();
				transTitles.clear();

				for (unsigned i = 0; i < untrans.size(); i++)
				{
					auto const transformedCoord = transform(untrans[i]);
					if (bounds.left > transformedCoord) // not yet in scope
						continue;
					if (transformedCoord > bounds.right) // everything from this point is out of scope
						break;

					trans.push_back(transformedCoord);

				}

				std::sort(titles.begin(), titles.end(), [](MajorDivision & l, MajorDivision & r) { return l.coord < r.coord; });

				for (unsigned i = 0; i < titles.size(); i++)
				{
					auto const transformedCoord = transform(titles[i].coord);
					if (bounds.left > transformedCoord) // not yet in scope
						continue;
					if (transformedCoord > bounds.right) // everything from this point is out of scope
						break;

					transTitles.push_back({ transformedCoord, titles[i].frequency });
				}
			}


		private:
			// containers for results
			std::vector<double> untrans;
			std::vector<double> trans;
			std::vector<MajorDivision> titles;
			std::vector<MajorDivision> transTitles;
			// the bounds of the window
			Utility::Bounds<double> bounds;
			Utility::Bounds<double> view;
			// vars
			double viewWidth,
				boundsWidth,
				minSpaceForDivision,
				spaceForDecade,
				startDecade,
				lastDecade,
				stopFreq;

			double lowerFreq, higherFreq;
			const int numDivisions = 10; // divisions of the graph, ie. 10, 20, 30,
			Scaling scaling;
		};

	};
#endif