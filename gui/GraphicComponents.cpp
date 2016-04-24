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
	 
		CRenderButton
	 
	 *********************************************************************************************/
	CRenderButton::CRenderButton(const std::string & text, const std::string & textToggled)
		: juce::Button(text), CBaseControl(this), c(cpl::GetColour(ColourEntry::Activated).brighter(0.6f)), toggle(false)
	{
		
		texts[0] = text;
		enableTooltip(true);
		texts[1] = textToggled.size() ? textToggled : text;
		addListener(this);
	}
	CRenderButton::~CRenderButton() {};
	void CRenderButton::setButtonColour(juce::Colour newColour)
	{
		c = newColour;
	}
	juce::Colour CRenderButton::getButtonColour()
	{
		return c;
	}
	
	std::string CRenderButton::bGetTitle() const
	{
		return texts[getToggleState()].toStdString();
	}
	
	void CRenderButton::bSetTitle(const std::string & input)
	{
		texts[getToggleState()] = input;
	}
	
	void CRenderButton::setToggleable(bool isAble)
	{
		setClickingTogglesState(toggle = isAble);
	}
	
	void CRenderButton::bSetInternal(iCtrlPrec_t newValue)
	{
		removeListener(this);
		setToggleState(newValue > 0.1f ? true : false, juce::NotificationType::dontSendNotification);
		addListener(this);
	}
	void CRenderButton::bSetValue(iCtrlPrec_t newValue, bool sync)
	{
		setToggleState(newValue > 0.1f ? true : false, 
			sync ? juce::NotificationType::sendNotificationSync : juce::NotificationType::sendNotification);
	}
	iCtrlPrec_t CRenderButton::bGetValue() const
	{
		return getToggleState() ? 1.0f : 0.0f;
	}
	void CRenderButton::setUntoggledText(const std::string & newText)
	{
		this->texts[0] = newText;
	}
	void CRenderButton::setToggledText(const std::string & newText)
	{
		this->texts[1] = newText;
	}

	void CRenderButton::paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
	{
		const float cornerSize = 5.5f;
		const int lengthToCorner = static_cast<int>(ceil(cornerSize / 2));
		
		auto bias = 0.0f;
		const bool isPressed = isButtonDown || getToggleState();
		// button becomes darker if pressed
		if(isButtonDown)
			bias -= 0.4f;
		else if (getToggleState())
			bias -= 0.3f;
		// button becomes brighter if hovered
		if(isMouseOverButton)
			bias += 0.1f;
		
		juce::Colour fill(c.withMultipliedBrightness(0.7f + bias));
		juce::Colour lightShadow(c.withMultipliedBrightness(1.1f +  0.65f * bias));
		juce::Colour darkShadow(c.withMultipliedBrightness(0.25f * (1.f + bias)));
		
		juce::ColourGradient gradient(!isPressed ? fill.brighter(0.15f) : fill.darker(0.15f), 0.f, 
			0.f, !isPressed ? fill.darker(0.15f) : fill.brighter(0.2f), (float)getWidth(), (float)getHeight(), false);
		
		if(isPressed)
		{
			// draw fill
			//g.setColour(fill);
			g.setGradientFill(gradient);
			g.fillRoundedRectangle(1.f, 1.f, (float)getWidth() - 2, (float)getHeight() - 2, 3.f);
			
			// draw sunken shadow
			g.setColour(darkShadow);
			g.drawLine((float)1, (float)lengthToCorner, 1.f, (float)getHeight() - lengthToCorner, 1.f);
			g.drawLine((float)lengthToCorner, 1.f, (float)getWidth() - lengthToCorner, 1.f, 1.f);

			// draw light shadow
			g.setColour(lightShadow);
			g.drawVerticalLine(getWidth() - 2, (float)lengthToCorner, (float)getHeight() - lengthToCorner);
			g.drawHorizontalLine(getHeight() - 2, (float)lengthToCorner, (float)getWidth() - lengthToCorner);
			// draw corner outline

			//g.drawLine(getWidth() - lengthToCorner, getHeight() , getWidth() - 1, getHeight()  - lengthToCorner);
			g.drawLine((float)getWidth() - lengthToCorner, getHeight() - 1.5f, getWidth() - 1.5f, (float)getHeight() - lengthToCorner, 1.3f);
		
			// draw black rectangle
			g.setColour(juce::Colours::black);
			g.drawRoundedRectangle(0.2f, 0.2f, getWidth()-0.5f, getHeight()-0.5f, 5, 0.7f);
			
		}
		else
		{
			// draw filling colour
			//g.setColour(fill);
						g.setGradientFill(gradient);
			g.fillRoundedRectangle(1.5f, 1.5f, getWidth()-1.7f, getHeight()-2.2f, 3.7f);
			
			// draw light shadow
			g.setColour(lightShadow);
			g.drawLine(1.f, (float)lengthToCorner, 1.f, (float)getHeight() - (lengthToCorner), 2.f);
			g.drawLine((float)lengthToCorner, 1.f, (float)getWidth() - (lengthToCorner), 1.f, 2.f);
			//g.setColour(c.withMultipliedBrightness(1.f + bias));
			// draw corner outline
			g.drawLine(1.f, (float)lengthToCorner, (float)lengthToCorner, 2.f);
			// draw black outline
			g.setColour(juce::Colours::black);
			g.drawRoundedRectangle(0.2f, 0.2f, getWidth()-0.5f, getHeight()-0.5f, 5.f, 0.7f);

		}
		g.setFont(cpl::TextSize::smallText);
		g.setColour(cpl::GetColour(ColourEntry::ControlText));
		//juce::Font lol("Consolas", cpl::TextSize::normalText, Font::plain);

		//g.setFont(lol);


		auto & text = texts[1].length() || toggle  ? texts[getToggleState()] : texts[0];
		if(isButtonDown)
			g.drawText(text, 6, 2, getWidth() - 5, getHeight() - 2, juce::Justification::centred);
		else
			g.drawText(text, 5, 1, getWidth() - 5, getHeight() - 2, juce::Justification::centred);
		
		// draw up outline
		g.setColour(juce::Colours::black);
		
		g.drawHorizontalLine(0, (float)lengthToCorner, (float)getWidth() - lengthToCorner);
		g.drawHorizontalLine(getHeight() - 1, (float)lengthToCorner, (float)getWidth() - lengthToCorner);
		g.drawVerticalLine(0, (float)lengthToCorner, (float)getHeight() - lengthToCorner);
		g.drawVerticalLine(getWidth() - 1, (float)lengthToCorner, (float)getHeight() - lengthToCorner);
		
	}
	

	/*********************************************************************************************
	 
	 CToggle
	 
	 *********************************************************************************************/
	CToggle::CToggle()
		: CBaseControl(this), cbox(CResourceManager::instance().getImage("bmps/checkbox.png"))
	{
		addListener(this);
		setSize(ControlSize::Square.width, 20);
	}
	
	void CToggle::paint(juce::Graphics & g)
	{
		CMutex lockGuard(this);
		auto width = cbox.getWidth();
		bool toggled = getToggleState();
		g.drawImage(cbox, 0, 0, width, width, 0, toggled ? width : 0, width, width);
		g.setColour(juce::Colours::lightgoldenrodyellow);
		g.setFont(TextSize::normalText);
		g.drawText(text, CRect(width + 5, 0, getWidth() - width, width), juce::Justification::verticallyCentred | juce::Justification::left, true);
	}
	
	void CToggle::bSetText(const std::string & in)
	{
		CMutex lockGuard(this);
		text = in;
	}
	
	iCtrlPrec_t CToggle::bGetValue() const
	{
		return getToggleState();
	}
	
	void CToggle::bSetInternal(iCtrlPrec_t newValue)
	{
		removeListener(this);
		getToggleStateValue().setValue(newValue > 0.1f ? true : false);
		addListener(this);
	}
	
	void CToggle::bSetValue(iCtrlPrec_t newValue, bool sync)
	{
		setToggleState(newValue > 0.1f ? true : false, 
			sync ? juce::NotificationType::sendNotificationSync : juce::NotificationType::sendNotification);
	}
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
		CMutex lockGuard(this);
		CTextLabel::setText(newText);
	}
	std::string CTextControl::bGetText() const
	{
		//CMutex lockGuard(this);
		return text.toStdString();
	}
	void CTextControl::paint(juce::Graphics & g)
	{
		CMutex lockGuard(this);
		CTextLabel::paint(g);
	}
};