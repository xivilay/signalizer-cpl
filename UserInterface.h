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
 
	 file:UserInterface.h
	 
		Middleware-stuff interface, that lets the user interact.
 
 *************************************************************************************/

#ifndef _USERINTERFACE_H
	#define _USERINTERFACE_H

	#include "Common.h"
	#include <vector>

	namespace cpl
	{
		/*********************************************************************************************
	 
			components inheriting from mouselistener needs
			composition of listeners to avoid diamond problem.
	 
		 *********************************************************************************************/
		struct MouseDelegate : public juce::MouseListener
		{
		public:
			struct MouseCallBack
			{
				virtual void cbMouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {};
				virtual void cbMouseDoubleClick(const juce::MouseEvent& event) {};
				virtual void cbMouseDrag(const juce::MouseEvent& event) {};
				virtual void cbMouseUp(const juce::MouseEvent& event) {};
				virtual void cbMouseDown(const juce::MouseEvent& event) {};
			};
			MouseDelegate(MouseCallBack & list) : listener(list) {};
			void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
			{
				listener.cbMouseWheelMove(event, wheel);
			}
			void mouseDoubleClick(const juce::MouseEvent& event) override
			{
				listener.cbMouseDoubleClick(event);
			}
			void mouseDrag(const juce::MouseEvent& event) override
			{
				listener.cbMouseDrag(event);
			}
			void mouseUp(const juce::MouseEvent& event) override
			{
				listener.cbMouseUp(event);
			}
			void mouseDown(const juce::MouseEvent& event) override
			{
				listener.cbMouseDown(event);
			}

		private:
			MouseCallBack & listener;
		};
	};
#endif