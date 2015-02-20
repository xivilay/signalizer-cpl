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
 
 file:ComponentContainers.cpp
 
	Source code for componentcontainers.h
 
 *************************************************************************************/

#include "ComponentContainers.h"
#include <stdexcept>
#include "Mathext.h"

namespace cpl
{
	
	/*********************************************************************************************
	 
		class CControlContainer
	 
	 *********************************************************************************************/
	CControlContainer::CControlContainer(juce::Component * viewToControl)
		: base(viewToControl), isVertical(false), stdWidth(80), stdHeight(stdWidth), just(juce::Justification::topLeft), isNested(true), ort(top)
	{
		
	}
	/*********************************************************************************************/		
	void CControlContainer::setOrientation(Orientation newOrientation)
	{
		ort = newOrientation;
		isVertical = ((ort == left) || (ort == right));
	}
	/*********************************************************************************************/		
	CControlContainer::~CControlContainer()
	{
		for(auto & item : controls)
			if(item.owned)
				delete item.ref;
	}
	/*********************************************************************************************/		
	void CControlContainer::setNested(bool isNested)
	{
		this->isNested = isNested;
	}
	/*********************************************************************************************/		
	void CControlContainer::expand()
	{
		if(base)
			// expand based on orientation
			base->setSize(base->getWidth() + stdWidth * !isVertical, base->getHeight() + stdHeight * isVertical);
	}
	/*********************************************************************************************/		
	void CControlContainer::addControl(CBaseControl * newControl, bool takeOwnership)
	{
		if(!newControl)
			return;
		
		int coord, x, y;
		coord = x = y = 0;
		
		for(auto & c : controls)
		{
			coord += isVertical ? c.ref->bGetSize().getWidth() : c.ref->bGetSize().getWidth();
		}
		
		if(!controls.empty())
			coord += 5; // gives 5 extra space between controls 
		
		controls.push_back(ControlRef(newControl, takeOwnership));
		
		if(base)
		{
			base->addAndMakeVisible(newControl->bGetView());
			
			// adjust width
			if((20 + coord + newControl->bGetSize().getWidth()) >= base->getWidth()) // expand our container
				base->setSize(20 + coord + newControl->bGetSize().getWidth(), base->getHeight());
			
			if(newControl->bGetSize().getHeight() > (base->getHeight() - (isNested ? 25 : 0)))
			{
				auto diff = newControl->bGetSize().getHeight() - (base->getHeight() - (isNested ? 25 : 0));
				switch(ort)
				{
					case top:
						// at top we dont care about y-alignment since increasing height works as expected
						base->setSize(base->getWidth(), newControl->bGetSize().getHeight() + (isNested ? 25 : 0));
						break;
					case bottom:
						// when we're at bottom we need to offset the y to make space, else we grow out of view size
						base->setBounds(base->getX(), base->getY() - diff, base->getWidth(), newControl->bGetSize().getHeight() + (isNested ? 20 : 0));
						break;
				}
				//base->setSize(base->getWidth(), 20 + newControl->bGetSize().getHeight());
			}
			coord += isVertical ? base->getY() : base->getX();
			
			
		}
		if(isVertical)
		{
			y = coord;
		}
		else
		{
			x = coord;
		}
		y += isNested ? 15 : 5;
		x += isNested ? 10 : 10;
		
		newControl->bSetPos(x, y);
	}

	/*********************************************************************************************
	 
		class CControlPanel
	 
	 *********************************************************************************************/

	CControlPanel::CControlPanel()
		: CBaseControl(this), CControlContainer(this), accessor(Colours::darkorange), oldHeight(-1), collapsed(false)
	{
		addAndMakeVisible(accessor);
		accessor.setSize(15, 15);
		accessor.addListener(this);
		setNested(false);
	}
	/*********************************************************************************************/
	void CControlPanel::setName(std::string name)
	{
		this->name = name;
	}
	/*********************************************************************************************/
	int CControlPanel::getPanelOffset()
	{
		return accessor.getHeight();
	}
	/*********************************************************************************************/
	void CControlPanel::addControl(CBaseControl * newControl, bool takeOwnership)
	{
		//setBounds(getX(), getY() - 20, getWidth(), getHeight());
		// we overload this member to grow our view if added controls nest, or take up too much space.
		CControlContainer::addControl(newControl, takeOwnership);
		const auto & bounds = newControl->bGetSize();
		if((bounds.getY() + bounds.getHeight()) > (getHeight()))
		{
			auto newHeight = bounds.getY() + bounds.getHeight();
			auto newYDelta = newHeight - getHeight();
			oldHeight = newHeight;
			switch(ort)
			{
				case top:
					setBounds(getX(), getY(), getWidth(), newHeight);
					break;
				case bottom:
					setBounds(getX(), getY() - newYDelta, getWidth(), newHeight);
					break;
			}
			
		}
	}
	/*********************************************************************************************/
	void CControlPanel::resizeAccordingly()
	{
		for(auto & item : controls)
		{

			const auto & bounds = item.ref->bGetSize();
			if((bounds.getY() + bounds.getHeight() + 10) > (getHeight()))
			{
				auto newHeight = bounds.getY() + bounds.getHeight() + 10;
				auto newYDelta = newHeight - getHeight();
				oldHeight = newHeight;
				switch(ort)
				{
					case top:
						setBounds(getX(), getY(), getWidth(), newHeight);
						break;
					case bottom:
						setBounds(getX(), getY() - newYDelta, getWidth(), newHeight);
						break;
				}
			}
		}
	}
	/*********************************************************************************************/
	void CControlPanel::setOrientation(CControlContainer::Orientation newOrientation)
	{
		switch (newOrientation)
		{
			case top:
				accessor.setSwitch(!collapsed);
				break;
			case bottom:
				accessor.setSwitch(collapsed);
				break;
			default:
				break;
		}
		CControlContainer::setOrientation(newOrientation);
	}
	/*********************************************************************************************/
	iCtrlPrec_t CControlPanel::bGetValue() const
	{
		return collapsed ? 1.f : 0.f;
	}
	/*********************************************************************************************/
	void CControlPanel::bSetValue(iCtrlPrec_t param)
	{
		if (param > 0.5f)
		{
			if (!collapsed)
			{
				accessor.flip();
				onValueChange();
			}
		}
		else
			if (collapsed)
			{
				accessor.flip();
				onValueChange();
			}
	}

	/*********************************************************************************************/
	void CControlPanel::onValueChange()
	{
		collapsed = !collapsed;
		enableTooltip(!collapsed);
		switch(ort)
		{
			case top:
				if(collapsed)
					setBounds(getX(), getY(), getWidth(), getPanelOffset());
				else
					setBounds(getX(), getY() + getHeight() - getPanelOffset(), getWidth(), oldHeight);
				break;
			case bottom:
				if(collapsed)
					setBounds(getX(), getY() + getHeight() - getPanelOffset(), getWidth(), getPanelOffset());
				else
					setBounds(getX(), getY() - oldHeight + getPanelOffset(), getWidth(), oldHeight);
				break;
		}
		
		
		/*
		 hide controls (no need to draw them).
		 */
		
		for(auto & c : controls)
		{
			auto view = c.ref->bGetView();
			view->setVisible(!collapsed);
		}
		// report
		bForceEvent();
	}
	/*********************************************************************************************/
	void CControlPanel::resized()
	{
		
		if(oldHeight == -1) // first time resize, assume we are given the actual fixed height. 0xc0debad
			oldHeight = getHeight();
		
		
		switch(ort)
		{
			case top:
				accessor.setBounds(-accessor.getWidth() / 2, getHeight() - getPanelOffset(), 15, 15);
				break;
			case left:
			case right:
			case bottom:
				accessor.setBounds(-accessor.getWidth() / 2, 0, 15, 15);
				break;
		}
	}
	/*********************************************************************************************/
	void CControlPanel::paint(juce::Graphics & g)
	{
		g.setColour(Colours::darkorange.withBrightness(0.5));
		
		if(collapsed)
		{
			switch(ort)
			{
				case top:
					g.drawLine(0.f, collapsed ? 0.f : (float)getHeight(), (float)getWidth(), collapsed ? 0.f : (float)getHeight(), 5.f);
					g.drawSingleLineText(name, getPanelOffset(), getPanelOffset());
					break;
				case left:
				case right:
				case bottom:
					g.drawLine(0.f, (float)getPanelOffset(), (float)getWidth(), (float)getPanelOffset(), 5.f);
					g.drawSingleLineText(name, getPanelOffset(), getPanelOffset() - 5);
					break;
			}
		}
		else
		{
			// draw bounding rectangle
			g.drawLine(0.f, 0.f, 0.f, (float)getHeight(),5.f);
			g.drawLine(0.f, 0.f, (float)getWidth(), 0.f, 5.f);
			g.drawLine((float)getWidth(), 0.f, (float)getWidth(), (float)getHeight(), 5.f);
			g.drawLine(0.f, (float)getHeight(), (float)getWidth(), (float)getHeight(), 5.f);
		}
	}
	/*********************************************************************************************/
	void CControlPanel::TriangleButton::clicked()
	{
		flip();
	}
	/*********************************************************************************************

		class CControlPanel::TriangleButton

	*********************************************************************************************/
	void CControlPanel::TriangleButton::flip()
	{
		orientation = !orientation; // flip
	}
	/*********************************************************************************************/
	void CControlPanel::TriangleButton::setSwitch(bool bOn)
	{
		orientation = bOn;
	}
	/*********************************************************************************************/
	CControlPanel::TriangleButton::TriangleButton(juce::Colour colour)
		: juce::Button("TriangleButton"), baseColour(colour), orientation(0)
	{
		setMouseCursor(juce::MouseCursor::PointingHandCursor);
	}
	/*********************************************************************************************/
	void CControlPanel::TriangleButton::paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
	{
		auto bias = 0.0f;
		// button becomes darker if pressed
		if(isButtonDown)
			bias -= 0.075f;
		// button becomes brighter if hovered
		if(isMouseOverButton)
			bias += 0.2f;
			
		juce::Path p;
			
		g.setColour(baseColour.withMultipliedBrightness(0.5f + bias));
			
		// draw triangle
		if(orientation)
		{
			p.startNewSubPath(0.f, (float)getHeight());
			p.lineTo(getWidth() / 2.f, 0.f);
			p.lineTo((float)getWidth(), (float)getHeight());
			p.lineTo(0.f, (float)getHeight());
		}
		else
		{
			p.startNewSubPath(0.f, 0.f);
			p.lineTo((float)getWidth(), 0.f);
			p.lineTo((float)getWidth() / 2.f, (float)getHeight());
			p.lineTo(0.f, 0.f);
		}
		g.fillPath(p);
	}
};
