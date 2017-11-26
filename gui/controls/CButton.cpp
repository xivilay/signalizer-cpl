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

	file:CButton.cpp

		Source code of CButton.h

*************************************************************************************/

#include "CButton.h"

namespace cpl
{
	/*********************************************************************************************

	CButton

	*********************************************************************************************/
	CButton::CButton(ValueEntityBase * value, bool takeOwnership)
		: juce::Button("CButton")
		, Base(this, value, takeOwnership)
		, toggle(false)
	{
		/* if (value)
		{
			std::string toggled, untoggled;
			if (value->getFormatter().format(0, untoggled) && value->getFormatter().format(1, toggled))
			{
				setTexts(toggled, untoggled);
			}
		} */
		setSize(ControlSize::Rectangle.width, ControlSize::Rectangle.height / 2);
		enableTooltip(true);

		setToggleState(getValueReference().getNormalizedValue() > 0.5 ? true : false, juce::NotificationType::dontSendNotification);
	}

	CButton::~CButton() {};

	void CButton::onValueObjectChange(ValueEntityListener * sender, ValueEntityBase * object)
	{
		setToggleState(object->getNormalizedValue() > 0.5 ? true : false, juce::NotificationType::dontSendNotification);
	}

	std::string CButton::bGetTitle() const
	{
		return texts[0].toStdString();
	}

	void CButton::bSetTitle(std::string input)
	{
		texts[0] = std::move(input);
	}

	void CButton::setSingleText(std::string input)
	{
		setTexts(input, input);
	}

	void CButton::setToggleable(bool isAble)
	{
		setClickingTogglesState(toggle = isAble);
	}

	void CButton::setTexts(std::string untoggled, std::string toggled)
	{
		texts[0] = std::move(untoggled);
		texts[1] = std::move(toggled);
		repaint();
	}

	void CButton::setUntoggledText(std::string newText)
	{
		this->texts[0] = std::move(newText);
		repaint();
	}

	void CButton::setToggledText(std::string newText)
	{
		this->texts[1] = std::move(newText);
		repaint();
	}

	void CButton::clicked()
	{
		valueObject->setNormalizedValue(getToggleState() ? 1.0f : 0.0f);
	}


	void CButton::paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
	{
		const float cornerSize = 5.5f;
		const int lengthToCorner = static_cast<int>(ceil(cornerSize / 2));
		juce::Colour c = cpl::GetColour(ColourEntry::Activated).brighter(0.1f);
		auto bias = 0.0f;
		const bool isPressed = isButtonDown || getToggleState();
		// button becomes darker if pressed
		if (isButtonDown)
			bias -= 0.4f;
		else if (getToggleState())
			bias -= 0.3f;
		// button becomes brighter if hovered
		if (isMouseOverButton)
			bias += 0.1f;

		juce::Colour fill(c.withMultipliedBrightness(0.7f + bias));
		juce::Colour lightShadow(c.withMultipliedBrightness(1.1f + 0.65f * bias));
		juce::Colour darkShadow(c.withMultipliedBrightness(0.25f * (1.f + bias)));

		juce::ColourGradient gradient(!isPressed ? fill.brighter(0.15f) : fill.darker(0.15f), 0.f,
			0.f, !isPressed ? fill.darker(0.15f) : fill.brighter(0.2f), (float)getWidth(), (float)getHeight(), false);

		if (isPressed)
		{
			// draw fill
			//g.setColour(fill);
			g.setGradientFill(gradient);
			g.fillRoundedRectangle(1.f, 1.f, getWidth() - 2.f, getHeight() - 2.f, 3.f);

			// draw sunken shadow
			g.setColour(darkShadow);
			g.drawLine(1.f, (float)lengthToCorner, 1.f, (float)getHeight() - lengthToCorner, 1.f);
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
			g.drawRoundedRectangle(0.2f, 0.2f, getWidth() - 0.5f, getHeight() - 0.5f, 5, 0.7f);

		}
		else
		{
			// draw filling colour
			//g.setColour(fill);
			g.setGradientFill(gradient);
			g.fillRoundedRectangle(1.5f, 1.5f, getWidth() - 1.7f, getHeight() - 2.2f, 3.7f);

			// draw light shadow
			g.setColour(lightShadow);
			g.drawLine(1.f, (float)lengthToCorner, 1.f, (float)getHeight() - (lengthToCorner), 2.f);
			g.drawLine((float)lengthToCorner, 1.f, (float)getWidth() - (lengthToCorner), 1.f, 2.f);
			//g.setColour(c.withMultipliedBrightness(1.f + bias));
			// draw corner outline
			g.drawLine(1.f, (float)lengthToCorner, (float)lengthToCorner, 2.f);
			// draw black outline
			g.setColour(juce::Colours::black);
			g.drawRoundedRectangle(0.2f, 0.2f, getWidth() - 0.5f, getHeight() - 0.5f, 5.f, 0.7f);

		}
		g.setFont(cpl::TextSize::smallText);
		g.setColour(cpl::GetColour(ColourEntry::ControlText));
		//juce::Font lol("Consolas", cpl::TextSize::normalText, Font::plain);

		//g.setFont(lol);


		auto & text = texts[1].length() ? (toggle ? texts[getToggleState()] : texts[0]) : texts[0];
		if (isButtonDown)
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
};
