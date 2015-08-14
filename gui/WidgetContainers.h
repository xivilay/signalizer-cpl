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
				: suggestedWidth(), suggestedHeight()
			{

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

			void arrange(bool fromResized = true)
			{
				// the general amount of separation between elements
				const std::size_t sepY(5), sepX(5);
				// current x,y offsets
				std::size_t offX(0), offY(0);
				suggestedHeight = 0;
				suggestedWidth = 0;
				std::size_t maxHeightInPrevRow = 0;
				for (std::size_t y = 0; y < controls.size(); ++y)
				{
					maxHeightInPrevRow = 0;
					offX = 0;
					offY += sepY;

					for (std::size_t x = 0; x < controls[y].size(); ++x)
					{
						// get max height for next row
						auto bounds = controls[y][x].first->bGetView()->getBounds();
						maxHeightInPrevRow = std::max<std::size_t>(maxHeightInPrevRow, bounds.getHeight());
						offX += sepX;

						controls[y][x].first->bGetView()->setTopLeftPosition(offX, offY);
						offX += bounds.getWidth();
					}

					offY += maxHeightInPrevRow;

					suggestedWidth = std::max(suggestedWidth, offX);
				}
				suggestedHeight += offY;
				suggestedHeight += sepY;
				suggestedWidth += sepX;
				if (!fromResized)
				{
					setSize(suggestedWidth, suggestedHeight);
				}
			}

			void addControl(cpl::CBaseControl * c, std::size_t row, bool takeOwnerShip = false)
			{
				if (!c)
					return;
				// check if it already exists

				controls.resize(std::max(row + 1, controls.size()));

				for (unsigned x = 0; x < controls[row].size(); ++x)
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
			std::size_t suggestedHeight, suggestedWidth;
		};
	};

#endif