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
 
	file:CKnobSlider.cpp
 
		Source code for CKnobSlider.h
 
*************************************************************************************/

#include "CKnobSlider.h"
#include "CKnobSliderEditor.h"
#include "../simd/simd_consts.h"


namespace cpl
{

	std::unique_ptr<CCtrlEditSpace> CKnobSlider::bCreateEditSpace()
	{
		if (isEditSpacesAllowed)
		{
			return std::unique_ptr<CCtrlEditSpace>(new CKnobSliderEditor(this));
		}
		else
			return nullptr;
	}

	CKnobSlider::CKnobSlider(const std::string & name, ControlType typeToUse)
		: juce::Slider("CKnobSlider")
		, CBaseControl(this)
		, title(name)
		, type(typeToUse)
		, oldStyle(Slider::SliderStyle::RotaryVerticalDrag)
		, laggedValue(0)
	{
		// IF you change the range, please change the scaling back to what is found in the comments.
		setRange(0.0, 1.0);
		isEditSpacesAllowed = true;
		addListener(this);
		setTextBoxStyle(NoTextBox, 0, 0, 0);
		setIsKnob(true);
		enableTooltip(true);
		setVisible(true);
		setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
		setPopupMenuEnabled(true);

	}



	bool CKnobSlider::bStringToValue(const std::string & valueString, iCtrlPrec_t & val) const
	{
		switch (type)
		{
		case pct:
		{
			int intValue = 0;
			if (sscanf(valueString.c_str(), "%d", &intValue) > 0)
			{
				val = cpl::Math::confineTo<iCtrlPrec_t>(iCtrlPrec_t(intValue) / 100.0, 0.0, 1.0);
				return true;
			}
			break;
		}
		case hz:
		{
			double fVal = 0;
			char * endPtr = nullptr;
			fVal = std::strtod(valueString.c_str(), &endPtr);
			if (endPtr > valueString.c_str()) // can only happen if number was parsed
			{
				val = cpl::Math::confineTo<iCtrlPrec_t>(fVal, 0.0, hzLimit) / hzLimit;
				return true;
			}
			break;
		}
		case db:
		{
			double fVal = 0;
			char * endPtr = nullptr;
			fVal = std::strtod(valueString.c_str(), &endPtr);
			if (endPtr > valueString.c_str()) // can only happen if number was parsed
			{
				val = cpl::Math::confineTo<iCtrlPrec_t>(std::pow(10, fVal / 20), 0.0, 1.0);
				return true;
			}
			break;
		}
		case ft:
		{
			double fVal = 0;
			char * endPtr = nullptr;
			fVal = std::strtod(valueString.c_str(), &endPtr);
			if (endPtr > valueString.c_str()) // can only happen if number was parsed
			{
				val = cpl::Math::confineTo<iCtrlPrec_t>(fVal, 0.0, 1.0);
				return true;
			}
			break;
		}
		case ms:
		{
			double fVal = 0;
			char * endPtr = nullptr;
			fVal = std::strtod(valueString.c_str(), &endPtr);
			if (endPtr > valueString.c_str()) // can only happen if number was parsed
			{
				val = cpl::Math::confineTo<iCtrlPrec_t>(fVal / msLimit, 0.0, 1.0);
				return true;
			}
			break;
		}
		};
		return false;
	}
	bool CKnobSlider::bValueToString(std::string & valueString, iCtrlPrec_t val) const
	{
		char buf[100];
		switch (type)
		{
		case pct:
			sprintf_s(buf, "%d %%", cpl::Math::round<int>(val * 100));
			valueString = buf;
			return true;
		case hz:
			sprintf_s(buf, "%.1f Hz", val * hzLimit);
			valueString = buf;
			return true;
		case db:
			if (val == iCtrlPrec_t(0.0))
				sprintf_s(buf, "-oo dB");
			else
				sprintf_s(buf, "%.3f dB", 20 * log10(val));
			valueString = buf;
			return true;
		case ft:
			sprintf_s(buf, "%.3f", val);
			valueString = buf;
			return true;
		case ms:
			sprintf_s(buf, "%d ms", cpl::Math::round<int>(val * msLimit));
			valueString = buf;
			return true;
		};
		return false;
	}

	juce::Rectangle<int> CKnobSlider::getTextRect() const
	{
		if (getHeight() >= ControlSize::Square.height)
		{
			return CRect(0, static_cast<int>(getHeight() * 0.75), getWidth(), static_cast<int>(getHeight() * 0.25));
		}
		else
		{
			auto sideKnobLength = ControlSize::Square.height / 2;
			return CRect(sideKnobLength + 5, getHeight() / 2, getWidth() - (sideKnobLength + 5), getHeight() / 2);
		}
	}
	juce::Rectangle<int> CKnobSlider::getTitleRect() const
	{
		if (getHeight() >= ControlSize::Square.height)
		{
			return CRect(getWidth(), static_cast<int>(getHeight() * 0.25));
		}
		else
		{
			auto sideKnobLength = ControlSize::Square.height / 2;
			return CRect(sideKnobLength + 5, 0, getWidth() - (sideKnobLength + 5), getHeight() / 2);
		}
	}

	void CKnobSlider::computePaths()
	{
		const float radius = jmin(getWidth() / 2, getHeight() / 2) - 1.0f;
		const float centreX = getHeight() * 0.5f;
		const float centreY = centreX;
		const float rx = centreX - radius;
		const float ry = centreY - radius;
		const float rw = radius * 2.0f;
		const float rotaryStartAngle = 2 * simd::consts<float>::pi * -0.4f;
		const float rotaryEndAngle = 2 * simd::consts<float>::pi * 0.4f;
		const float angle = static_cast<float>(
			bGetValue() * (rotaryEndAngle - rotaryStartAngle) + rotaryStartAngle
		);
		const float thickness = 0.7f;

		// pie fill
		pie.clear();
		pie.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, angle, thickness * 0.9f);

		// pointer
		const float innerRadius = radius * 0.2f;
		const float pointerHeight = innerRadius * thickness;
		const float pointerLengthScale = 0.5f;

		pointer.clear();

		pointer.addRectangle(
			-pointerHeight * 0.5f + 2.f + (1 - pointerLengthScale) * radius * thickness,
			-pointerHeight * 0.5f,
			pointerLengthScale * radius * thickness,
			innerRadius * thickness);
		auto trans = AffineTransform::rotation(angle - simd::consts<float>::pi * 0.5f).translated(centreX, centreY);
		pointer.applyTransform(trans);
		
	}

	void CKnobSlider::paint(juce::Graphics& g)
	{
		auto newValue = bGetValue();

		if (isKnob)
		{
			if (newValue != laggedValue)
				computePaths();

			// code based on the v2 juce knob

			const bool isMouseOver = isMouseOverOrDragging() && isEnabled();
			const float thickness = 0.7f;

			// main fill
			g.setColour(GetColour(ColourEntry::Deactivated));
			g.fillEllipse(juce::Rectangle<float>(getHeight(), getHeight()));

			// center fill
			g.setColour(GetColour(ColourEntry::Separator));
			g.fillEllipse(juce::Rectangle<float>(getHeight(), getHeight()).reduced(getHeight() * (1 - thickness * 1.1f)));

			g.setColour(GetColour(ColourEntry::SelectedText)
				.withMultipliedBrightness(isMouseOver ? 0.8f : 0.7f));

			g.fillPath(pie);

			g.setColour(GetColour(ColourEntry::ControlText)
				.withMultipliedBrightness(isMouseOver ? 1.0f : 0.8f));

			g.fillPath(pointer);

			g.setFont(TextSize::smallerText);
			g.setColour(cpl::GetColour(cpl::ColourEntry::ControlText));

			// text
			g.drawText(bGetTitle(), getTitleRect(), juce::Justification::centredLeft, false);
			g.drawText(bGetText(), getTextRect(), juce::Justification::centredLeft, false);
		}
		else
		{
			
			g.fillAll(cpl::GetColour(cpl::ColourEntry::Activated).darker(0.6f));


			auto bounds = getBounds();
			auto rem = CRect(1, 1, bounds.getWidth() - 2, bounds.getHeight() - 2);
			g.setColour(cpl::GetColour(cpl::ColourEntry::Activated).darker(0.1f));
			g.fillRect(rem.withLeft(cpl::Math::round<int>(rem.getX() + rem.getWidth() * bGetValue())));

			g.setColour(cpl::GetColour(cpl::ColourEntry::Separator));
			//g.drawRect(getBounds().toFloat());
			g.setFont(TextSize::largeText);
			//g.setFont(systemFont.withHeight(TextSize::largeText)); EDIT_TYPE_NEWFONTS
			g.setColour(cpl::GetColour(cpl::ColourEntry::AuxillaryText));
			if (isMouseOverOrDragging())
			{
				g.drawText(bGetText(), getBounds().withPosition(5, 0), juce::Justification::centredLeft);
			}
			else
			{
				g.drawText(bGetTitle(), getBounds().withPosition(5, 0), juce::Justification::centredLeft);
			}

		}

		laggedValue = bGetValue();
	}

	iCtrlPrec_t CKnobSlider::bGetValue() const
	{
		// (getValue() - getMinimum()) / (getMaximum() - getMinimum())
		return static_cast<iCtrlPrec_t>(getValue());
	}
	void CKnobSlider::serialize(CSerializer::Archiver & ar, Version version)
	{
		ar << bGetValue();
		ar << isKnob;
		ar << getVelocityBasedMode();
		ar << getMouseDragSensitivity();
		ar << getSliderStyle();
	}
	void CKnobSlider::deserialize(CSerializer::Builder & ar, Version version)
	{
		iCtrlPrec_t value(0);
		bool vel(false);
		int sens(0);
		Slider::SliderStyle style;
		ar >> value;
		ar >> isKnob;
		ar >> vel;
		ar >> sens;
		ar >> style;
		setIsKnob(isKnob);
		setVelocityBasedMode(vel);
		setMouseDragSensitivity(sens);
		setSliderStyle(style);
		bSetValue(value, true);
	}

	void CKnobSlider::bSetText(const std::string & in)
	{
		text = in;
	}
	void CKnobSlider::bSetInternal(iCtrlPrec_t newValue)
	{
		// newValue * (getMaximum() - getMinimum()) + getMinimum()
		setValue(newValue, dontSendNotification);
	}
	void CKnobSlider::bSetTitle(const std::string & in)
	{
		title = in;
	}
	void CKnobSlider::bSetValue(iCtrlPrec_t newValue, bool sync)
	{
		// newValue * (getMaximum() - getMinimum()) + getMinimum()
		setValue(newValue, sync ? juce::NotificationType::sendNotificationSync : juce::NotificationType::sendNotification);
	}
	void CKnobSlider::bRedraw()
	{
		bFormatValue(text, bGetValue());
		bSetText(text);
		repaint();
	}
	void CKnobSlider::setCtrlType(ControlType newType)
	{
		if(newType > 0 && newType <= ControlType::ms)
			type = newType;
	}

	void CKnobSlider::setIsKnob(bool shouldBeKnob)
	{
		setSize(ControlSize::Rectangle.width, ControlSize::Rectangle.height);//setSize(ControlSize::Square.width, ControlSize::Square.height);

		if (shouldBeKnob && !isKnob)
		{
			setSliderStyle(oldStyle);
		}
		else
		{
			oldStyle = getSliderStyle();
			setSliderStyle(Slider::SliderStyle::LinearHorizontal);
		}

		isKnob = !!shouldBeKnob;
	}

	bool CKnobSlider::getIsKnob() const
	{
		return isKnob;
	}

	std::string CKnobSlider::bGetTitle() const
	{
		return title;
	}
	std::string CKnobSlider::bGetText() const
	{
		return text;
	}
	void CKnobSlider::onValueChange()
	{
		bFormatValue(text, bGetValue());
		bSetText(text);
	}

};
