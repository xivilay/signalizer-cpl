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

#include "CKnobSlider.h"
#include "CKnobSliderEditor.h"
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


	/*********************************************************************************************

	CKnobSlider

	*********************************************************************************************/
	CKnobSlider::CKnobSlider(const std::string & name, ControlType typeToUse)
		: Slider("CKnobSlider"), knobGraphics(CResourceManager::instance().getImage("knob.png")), CBaseControl(this), title(name), type(typeToUse)
	{
		Slider::setRange(0.0, 1.0);
		isEditSpacesAllowed = true;
		this->addListener(this);
		numFrames = knobGraphics.getHeight() / knobGraphics.getWidth();
		sideLength = knobGraphics.getWidth();
		setTextBoxStyle(NoTextBox, 0, 0, 0);
		setIsKnob(true);
		enableTooltip(true);
		setVisible(true);
		setPopupMenuEnabled(true);
	}


	CKnobSlider::~CKnobSlider()
	{

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

	void CKnobSlider::paint(juce::Graphics& g)
	{
		if (isKnob)
		{
			auto size = ControlSize::Square.height;
			auto quarterSize = size / 4;
			auto sideKnobLength = ControlSize::Square.height / 2;
			// quantize
			int value = Math::round<int>((getValue() - getMinimum()) / (getMaximum() - getMinimum()) * (numFrames - 1));
			g.setFont(TextSize::smallerText);
			//g.setFont(systemFont.withHeight()); EDIT_TYPE_NEWFONTS
			g.setColour(cpl::GetColour(cpl::ColourEntry::ctrltext));

			// if the size is greater or equal to standard square height, we draw the knob in the middle, and the text above and bottom.
			if (getHeight() >= ControlSize::Square.height)
			{
				g.drawImage(knobGraphics, quarterSize, quarterSize, sideKnobLength, sideKnobLength, 0, value * sideLength, sideLength, sideLength);
				g.drawText(bGetTitle(), getTitleRect(), juce::Justification::horizontallyCentred, false);
				g.drawText(bGetText(), getTextRect(), juce::Justification::centred, false);
			}
			else
			{
				g.drawImage(knobGraphics, 0, 0, sideKnobLength, sideKnobLength, 0, value * sideLength, sideLength, sideLength);
				g.drawText(bGetTitle(), getTitleRect(), juce::Justification::centredLeft, false);
				g.drawText(bGetText(), getTextRect(), juce::Justification::centredLeft, false);
			}



		}
		else
		{
			
			g.fillAll(cpl::GetColour(cpl::ColourEntry::activated).darker(0.6f));


			auto bounds = getBounds();
			auto rem = CRect(1, 1, bounds.getWidth() - 2, bounds.getHeight() - 2);
			g.setColour(cpl::GetColour(cpl::ColourEntry::activated).darker(0.1f));
			g.fillRect(rem.withLeft(cpl::Math::round<int>(rem.getX() + rem.getWidth() * bGetValue())));

			g.setColour(cpl::GetColour(cpl::ColourEntry::separator));
			//g.drawRect(getBounds().toFloat());
			g.setFont(TextSize::largeText);
			//g.setFont(systemFont.withHeight(TextSize::largeText)); EDIT_TYPE_NEWFONTS
			g.setColour(cpl::GetColour(cpl::ColourEntry::auxfont));
			if (isMouseOverOrDragging())
			{
				g.drawText(bGetText(), getBounds().withPosition(5, 0), juce::Justification::centredLeft);
			}
			else
			{
				g.drawText(bGetTitle(), getBounds().withPosition(5, 0), juce::Justification::centredLeft);
			}

		}
	}

	iCtrlPrec_t CKnobSlider::bGetValue() const
	{
		return static_cast<iCtrlPrec_t>((getValue() - getMinimum()) / (getMaximum() - getMinimum()));
	}
	void CKnobSlider::save(CSerializer::Archiver & ar, long long int version)
	{
		ar << bGetValue();
		ar << isKnob;
		ar << getVelocityBasedMode();
		ar << getMouseDragSensitivity();
		ar << getSliderStyle();
	}
	void CKnobSlider::load(CSerializer::Builder & ar, long long int version)
	{
		iCtrlPrec_t value(0);
		bool vel(false);
		int sens(0);
		SliderStyle style;
		ar >> value;
		ar >> isKnob;
		ar >> vel;
		ar >> sens;
		ar >> style;
		setIsKnob(isKnob);
		this->setVelocityBasedMode(vel);
		this->setMouseDragSensitivity(sens);
		this->setSliderStyle(style);
		bSetValue(value, true);
	}

	void CKnobSlider::bSetText(const std::string & in)
	{
		text = in;
	}
	void CKnobSlider::bSetInternal(iCtrlPrec_t newValue)
	{
		setValue(newValue * (getMaximum() - getMinimum()) + getMinimum(), dontSendNotification);
	}
	void CKnobSlider::bSetTitle(const std::string & in)
	{
		title = in;
	}
	void CKnobSlider::bSetValue(iCtrlPrec_t newValue, bool sync)
	{
		setValue(newValue * (getMaximum() - getMinimum()) + getMinimum(), 
			sync ? juce::NotificationType::sendNotificationSync : juce::NotificationType::sendNotification);
	}	
	void CKnobSlider::setCtrlType(ControlType newType)
	{
		if(newType > 0 && newType <= ControlType::ms)
			type = newType;
	}

	void CKnobSlider::setIsKnob(bool shouldBeKnob)
	{
		isKnob = !!shouldBeKnob;
		if (isKnob)
		{
			setSize(ControlSize::Rectangle.width, ControlSize::Rectangle.height);//setSize(ControlSize::Square.width, ControlSize::Square.height);
			this->setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
		}
		else
		{
			setSize(ControlSize::Rectangle.width, ControlSize::Rectangle.height);
			this->setSliderStyle(Slider::SliderStyle::LinearHorizontal);
		}
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
