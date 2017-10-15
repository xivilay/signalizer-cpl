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

	file:GraphicComponents.cpp

		Implementation of GraphicComponents.h

*************************************************************************************/

#include "GraphicComponents.h"
#include "../Misc.h"

namespace cpl
{



	/*********************************************************************************************

	 CTextLabel

	 *********************************************************************************************/
	CTextLabel::CTextLabel()
		: Component(), just(juce::Justification::centredLeft)
	{
		setSize(200, 20);
	}

	void CTextLabel::setFontSize(float newSize)
	{
		this->size = newSize;
		repaint();
	}

	void CTextLabel::setColour(CColour newColour)
	{
		this->colour = newColour;
		repaint();
	}

	void CTextLabel::setText(const std::string & newText)
	{
		text = newText;
		repaint();
	}

	void CTextLabel::paint(juce::Graphics & g)
	{
		g.setFont(size);
		g.setColour(colour);
		g.drawText(text, CRect(0, 0, getWidth(), getHeight()), just, false);
	}

	void CTextLabel::setPos(int x, int y)
	{
		setCentrePosition(x + getWidth() / 2, y + getHeight() / 2);
	}
	/*********************************************************************************************

	 CScrollableContainer

	 *********************************************************************************************/
	CScrollableContainer::CScrollableContainer()
		: Component("CScrollableLineContainer"), CBaseControl(this)
	{
		virtualContainer = new Component();
		addAndMakeVisible(virtualContainer);
		scb = new juce::ScrollBar(true);
		scb->addListener(this);
		scb->setColour(juce::ScrollBar::ColourIds::trackColourId, juce::Colours::lightsteelblue);
		addAndMakeVisible(scb);
	}

	void CScrollableContainer::bSetSize(const CRect & in)
	{
		setSize(in.getWidth(), in.getHeight());
		scb->setBounds(in.getWidth() - 20, 0, 20, in.getHeight());
		virtualContainer->setBounds(0, 0, in.getWidth() - scb->getWidth(), 1300);
		CBaseControl::bSetPos(in.getX(), in.getY());
	}

	void CScrollableContainer::paint(juce::Graphics & g)
	{
		if (background)
			g.drawImage(*background, 0, 0, getWidth() - scb->getWidth(), getHeight(),
				0, 0, background->getWidth(), background->getHeight());
	}

	int CScrollableContainer::getVirtualHeight()
	{
		return virtualContainer->getHeight();
	}

	void CScrollableContainer::setVirtualHeight(int height)
	{
		virtualContainer->setSize(virtualContainer->getWidth(), height);
	}

	iCtrlPrec_t CScrollableContainer::bGetValue()
	{
		double start = scb->getCurrentRangeStart();
		auto delta = 1.0 / (1 - scb->getCurrentRangeSize());
		return static_cast<float>(start * delta);
	}

	void CScrollableContainer::bSetValue(iCtrlPrec_t newVal)
	{
		double delta = 1.0 / (1 - scb->getCurrentRangeSize());
		scb->setCurrentRangeStart(newVal / delta);
	}

	void CScrollableContainer::scrollBarMoved(juce::ScrollBar * b, double newRange)
	{
		virtualContainer->setBounds(
			0,
			static_cast<signed int>(-bGetValue() * (virtualContainer->getHeight() - getHeight())),
			virtualContainer->getWidth(),
			virtualContainer->getHeight());
	}

	CScrollableContainer:: ~CScrollableContainer() noexcept
	{
		if (virtualContainer)
			delete virtualContainer;
		if (scb)
			delete scb;
	}

	/*********************************************************************************************

	 CTextControl

	 *********************************************************************************************/
	CTextControl::CTextControl()
		: CBaseControl(this)
	{

	}
	void CTextControl::bSetText(const std::string & newText)
	{
		CTextLabel::setText(newText);
	}
	std::string CTextControl::bGetText() const
	{
		return text.toStdString();
	}
	void CTextControl::paint(juce::Graphics & g)
	{
		CTextLabel::paint(g);
	}
};