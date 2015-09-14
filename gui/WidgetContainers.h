/*************************************************************************************
 
 cpl - cross-platform library - v. 0.1.0.
 
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
 
 file:DesignBase.h
 
	Contains the methods, classes and data that encompass the standard 'design', 
	look or feel needed to give an uniform look.
 
 *************************************************************************************/

#ifndef _WIDGETCONTAINERS_H
	#define _WIDGETCONTAINERS_H

	#include "../Common.h"
	#include "../CBaseControl.h"

	namespace cpl
	{
		typedef juce::Component DummyComponent;

		class MatrixSection
		:
			public DummyComponent
		{
		public:

			MatrixSection()
				: suggestedWidth(), suggestedHeight(), spaceAfterLargest(false), xSpacing(5), ySpacing(5)
			{

			}

			void setSpacesAfterLargestElement(bool trigger = true) noexcept
			{
				spaceAfterLargest = trigger;
			}

			cpl::CBaseControl * operator [](const std::string & name) const
			{
				for (unsigned y = 0; y < controls.size(); ++y)
				{
					for (unsigned x = 0; x < controls[y].size(); ++x)
					{
						if (cpl::CBaseControl * c = controls[y][x].first)
						{
							if (c->bGetTitle() == name)
								return c;
						}
					}
				}
				return nullptr;
			}

			void resized() override
			{
				arrange();
			}

			void setXSpacing(int spacing) { xSpacing = spacing; }
			void setYSpacing(int spacing) { ySpacing = spacing;}
			int getXSpacing() const noexcept { return xSpacing; }
			int getySpacing()  const noexcept { return ySpacing; }

			void arrange(bool fromResized = true)
			{
				// the general amount of separation between elements
				const int sepY(ySpacing), sepX(xSpacing);
				// current x,y offsets
				int offX(5), offY(0);
				suggestedHeight = 0;
				suggestedWidth = 0;
				int maxHeightInPrevRow = 0;
				for (std::size_t y = 0; y < controls.size(); ++y)
				{
					maxHeightInPrevRow = 0;
					offX = 0;

					// go from left to right, ie. iterate through rows.
					for (std::size_t x = 0; x < controls[y].size(); ++x)
					{
						// get max height for next row
						auto bounds = controls[y][x].first->bGetView()->getBounds();
						maxHeightInPrevRow =
							spaceAfterLargest ? std::max<int>(maxHeightInPrevRow, bounds.getHeight()) : bounds.getHeight();
						// if we don't space vertically after the highest control in the last row,
						// we position ourselves after the bottom of the control above us (if there is one).
						auto const ypos = (spaceAfterLargest || y == 0) ? offY : controls[y - 1][x].first->bGetView()->getBottom() + sepY;
						controls[y][x].first->bGetView()->setTopLeftPosition(offX, ypos);
						offX += sepX;
						offX += bounds.getWidth();
					}

					offY += sepY;

					offY += maxHeightInPrevRow;

					suggestedWidth = std::max(suggestedWidth, offX);
				}

				if (spaceAfterLargest)
				{
					suggestedHeight += offY;
					suggestedHeight += sepY;
				}
				else
				{
					// if we dont space after largest, we have to scan through columns to see the actual bottom
					int bot = controls.size() ? controls.size() - 1 : 0;
					int maxBot = 0;
					for (std::size_t x = 0; x < controls[bot].size(); x++)
					{
						maxBot = std::max(maxBot, (int) controls[bot][x].first->bGetView()->getBottom());
					}

					suggestedHeight = maxBot + sepY;
				}

				suggestedWidth += sepX;
				if (!fromResized)
				{
					setSize(suggestedWidth, suggestedHeight);
				}
			}

			void addControl(cpl::CBaseControl * c, int row, bool takeOwnerShip = false)
			{
				if (!c)
					return;
				// check if it already exists

				controls.resize(std::max(row + 1, (int) controls.size()));

				for (std::size_t x = 0; x < controls[row].size(); ++x)
				{
					if (c == controls[row][x].first)
					{
						return;
					}
				}
				// okay, add it.
				controls[row].emplace_back(c, takeOwnerShip);
				addAndMakeVisible(c->bGetView());
				arrange(false);
			}

			std::pair<int, int> getSuggestedSize()
			{
				return std::make_pair(suggestedWidth, suggestedHeight);
			}
			virtual ~MatrixSection()
			{
				for (auto & row : controls)
				{
					for (auto & col : row)
					{
						// second == takeOwnerShip
						if (col.second)
							delete col.first;
					}
				}
			}
		private:
			std::vector<std::vector<std::pair<cpl::CBaseControl *, bool>>> controls;
			int suggestedHeight, suggestedWidth;
			int xSpacing, ySpacing;
			bool spaceAfterLargest;
		};
	};

#endif