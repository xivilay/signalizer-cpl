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
 
	 file:GraphicComponents.h
		
		Wrappers and graphic classes that can be used for controls, images, etc.
	
 *************************************************************************************/

#ifndef CPL_GRAPHICCOMPONENTS_H
	#define CPL_GRAPHICCOMPONENTS_H

	#include "../Common.h"
	#include <cstdint>
	#include <string>
	#include "CBaseControl.h"
	#include "../CMutex.h"

	namespace cpl
	{
	
		/*********************************************************************************************

			Textlabel interface

		*********************************************************************************************/
		class CTextLabel : public juce::Component
		{
		protected:
			juce::String text;
			float size;
			CColour colour;
			juce::Justification just;
		public:
			CTextLabel();
			void setFontSize(float newSize);
			void setColour(CColour newColour);
			virtual void setText(const std::string & newText);
			virtual void paint(juce::Graphics & g) override;
			void setPos(int x, int y);
			void setJustification(juce::Justification j) { just = j; }
		};
		/*********************************************************************************************

			Greenlinetester. Derive from this if you are uncertain that you are getting painted -
			Will draw a green line.

		*********************************************************************************************/
		class CGreenLineTester : public juce::Component
		{
			void paint(juce::Graphics & g)
			{
				g.setColour(juce::Colours::green);
				g.drawLine(juce::Line<float>(0.f, 0.f, static_cast<float>(getWidth()), static_cast<float>(getHeight())),1);
				g.setColour(juce::Colours::blue);
				g.drawRect(getBounds().withZeroOrigin().toFloat(), 0.5f);
			}
		};
		/*********************************************************************************************

			Name says it all. Holds a virtual container of larger size, that is scrollable.

		*********************************************************************************************/
		class CScrollableContainer 
			: public juce::Component
			, public CBaseControl
			, private juce::ScrollBar::Listener
		{
		protected:
			juce::ScrollBar * scb;
			Component * virtualContainer;
			const juce::Image * background;
		public:
			CScrollableContainer();
			void bSetSize(const CRect & in);
			int getVirtualHeight();
			void setVirtualHeight(int height);
			void bSetValue(iCtrlPrec_t newVal);
			iCtrlPrec_t bGetValue();
			void setBackground(const juce::Image * b) { background = b;}
			void setBackground(const juce::Image & b) {	background = &b; }
			juce::ScrollBar * getSCB() { return scb; }
			juce::Component * getVContainer() { return virtualContainer; }
			void scrollBarMoved(juce::ScrollBar * b, double newRange) override;
			virtual void paint(juce::Graphics & g) override;
			virtual ~CScrollableContainer() CPL_llvm_DummyNoExcept;

		};
		/*********************************************************************************************

			Same as CTextLabel, however is protected using a mutex

		*********************************************************************************************/
		class CTextControl : public CTextLabel, public CBaseControl
		{
		public:
			CTextControl();
			void bSetText(const std::string & newText) override;
			std::string bGetText() const override;
			void paint(juce::Graphics & g) override;

		};


	};
#endif