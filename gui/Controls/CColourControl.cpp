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
 
	file:CColourControl.cpp
 
		Source code for CColourControl.h
 
 *************************************************************************************/

#include "CColourControl.h"
#include "CKnobSliderEditor.h"
#include "../CCtrlEditSpace.h"

namespace cpl
{
	
	juce::Colour ColourFromPixelARGB(juce::PixelARGB pixel)
	{
		return juce::Colour(pixel.getRed(), pixel.getGreen(), pixel.getBlue(), pixel.getAlpha());
	}
	
	const char * ColourToneTypes[] = 
	{
		"RGB",
		"ARGB",
		"GreyTone",
		"Red",
		"Green", 
		"Blue"
	};

	class CustomColourSelector
	:
		public juce::ColourSelector
	{
	public:
		CustomColourSelector(int flags = (showAlphaChannel | showColourAtTop | showSliders | showColourspace),
			int edgeGap = 4,
			int gapAroundColourSpaceComponent = 7)
			: juce::ColourSelector(flags, edgeGap, gapAroundColourSpaceComponent)
		{

		}

		void shrinkLabels()
		{
			// default juce::Slider layouts have enourmous labels
			// compared to the actual slider.
			// for this widget, we only need to display two characters,
			// so we shrink them a little bit.

			char * names[] = { "r", "b", "g", "a" };

			for (int i = 0; i < getNumChildComponents(); ++i)
			{
				auto currentChild = getChildComponent(i);
				if (juce::Slider * s = dynamic_cast<juce::Slider*>(currentChild))
				{
					auto currentWidth = s->getTextBoxWidth();
					auto currentHeight = s->getTextBoxWidth();
					auto currentPos = s->getTextBoxPosition();
					s->setTextBoxStyle(currentPos, false, currentWidth / 3, currentHeight);
					s->setName(names[i]);
					s->setLookAndFeel(&cpl::CLookAndFeel_CPL::defaultLook());
				}
			}
		}

	};

	class ColourEditor 
	: 
		public CKnobSliderEditor
	{
	public:
		ColourEditor(CColourControl * parentControl)
			: CKnobSliderEditor(parentControl), parent(parentControl), selector(15, 5, 5), 
			recursionFlagWeChanged(false), recursionFlagTheyChanged(false)
		{
			oldHeight = fullHeight;
			oldWidth = fullWidth;
			fullWidth = oldWidth + extraWidth;
			fullHeight = oldHeight + extraHeight;
			selector.addChangeListener(this);

			selector.setCurrentColour(parent->getControlColour());
			toolTip = "Colour editor space - adjust ARGB values of controls precisely.";

			setOpaque(false);
			//addAndMakeVisible(selector);
		}

		virtual void resized() override
		{
			selector.setBounds(1, oldHeight, fullWidth - elementHeight - 3, extraHeight );
			CKnobSliderEditor::resized();
		}

		virtual void changeListenerCallback(ChangeBroadcaster *source) override
		{
			if (recursionFlagTheyChanged || recursionFlagWeChanged)
			{
				recursionFlagWeChanged = recursionFlagTheyChanged = false;
			}
			else if (source == &selector)
			{
				recursionFlagWeChanged = true;
				parent->setControlColour(selector.getCurrentColour());
			}
			CKnobSliderEditor::changeListenerCallback(source);
		}

		virtual void valueChanged(const CBaseControl * ctrl) override
		{
			if (recursionFlagTheyChanged || recursionFlagWeChanged)
			{
				recursionFlagWeChanged = recursionFlagTheyChanged = false;
			}
			else if (!recursionFlagWeChanged)
			{
				recursionFlagTheyChanged = true;
				selector.setCurrentColour(parent->getControlColour());
			}
			CKnobSliderEditor::valueChanged(ctrl);
		}

		virtual void setMode(bool newMode)
		{
			if (!newMode)
			{
				selector.setCurrentColour(parent->getControlColour());
				addAndMakeVisible(&selector);
				selector.shrinkLabels();
			}
			else
			{
				removeChildComponent(&selector);
			}
			CKnobSliderEditor::setMode(newMode);
		}
	private:
		static const int extraHeight = 180;
		static const int extraWidth = 10;
		bool recursionFlagWeChanged;
		bool recursionFlagTheyChanged;
		int oldHeight, oldWidth;
		CColourControl * parent;
		CustomColourSelector selector;
	};

	std::unique_ptr<CCtrlEditSpace> CColourControl::bCreateEditSpace()
	{
		if (bGetEditSpacesAllowed())
			return std::unique_ptr<CCtrlEditSpace>(new ColourEditor(this));
		else
			return nullptr;
	}

	CColourControl::CColourControl(ColourValue * valueToReferTo, bool takeOwnership)
		: valueObject(nullptr)
	{
		bToggleEditSpaces(true);
		setValueReference(valueToReferTo, takeOwnership);
	}

	void CColourControl::setValueReference(ColourValue * valueToReferTo, bool takeOwnership)
	{
		bool resetAlpha = false;
		if (valueObject != nullptr)
		{
			for (int i = 0; i < 4; ++i)
				valueObject->getValueIndex((cpl::ColourValue::Index)i).removeListener(this);

			valueObject.reset(nullptr);
		}

		if (valueToReferTo == nullptr)
		{
			resetAlpha = true;
			valueToReferTo = new CompleteColour();
			takeOwnership = true;
		}

		valueObject.reset(valueToReferTo);
		valueObject.get_deleter().doDelete = takeOwnership;

		for (int i = 0; i < 4; ++i)
			valueObject->getValueIndex((cpl::ColourValue::Index)i).addListener(this);

		if(resetAlpha)
			valueObject->getValueIndex(cpl::ColourValue::A).setNormalizedValue(1.0);
	}

	/*********************************************************************************************/

	void CColourControl::onControlSerialization(CSerializer::Archiver & ar, Version version)
	{
		CKnobSlider::onControlSerialization(ar, version);
		auto colour = valueObject->getAsJuceColour();
		std::uint8_t a = colour.getAlpha(), r = colour.getRed(), g = colour.getGreen(), b = colour.getBlue();
		ar << a; ar << r; ar << g; ar << b;
		ar << std::int32_t(0);
	}
	void CColourControl::onControlDeserialization(CSerializer::Builder & ar, Version version)
	{
		CKnobSlider::onControlDeserialization(ar, version);
		std::int32_t newType;
		std::uint8_t a, r, g, b;
		ar >> a; ar >> r; ar >> g; ar >> b;
		ar >> newType;
		setControlColour(juce::Colour(r, g, b, a));
	}

	void CColourControl::onValueChange()
	{
		auto val = Slider::getValue();
		setControlColour((juce::Colour)static_cast<std::uint32_t>(val >= 1.0 ? 0xFFFFFFFF : val * 0x100000000ul));
	}

	void CColourControl::valueEntityChanged(ValueEntityListener * sender, ValueEntityBase * value)
	{
		auto colourValue = valueObject->getAsJuceColour();
		auto sliderValue = colourValue.getARGB() / double(0xFFFFFFFF);
		getSlider().setValue(sliderValue, juce::NotificationType::dontSendNotification);
		repaint();
	}

	juce::Colour CColourControl::getControlColour()
	{
		return valueObject->getAsJuceColour();
	}

	juce::Colour CColourControl::getControlColourAsColour()
	{
		return getControlColour();
	}

	void CColourControl::setControlColour(juce::Colour newColour)
	{
		valueObject->setFromJuceColour(newColour);
	}

	bool CColourControl::bStringToValue(const std::string & valueString, iCtrlPrec_t & value) const
	{
		std::uint32_t result(0);
		char * endPtr = nullptr;
		result = static_cast<std::uint32_t>(strtoul(valueString.c_str(), &endPtr, 0));
		if (endPtr > valueString.c_str())
		{
			value = result / (double)0xFFFFFFFF;
			return true;
		}
		return false;
	}			

	iCtrlPrec_t CColourControl::bGetValue() const
	{
		return valueObject->getAsJuceColour().getARGB() / (double)0xFFFFFFFF;
	}

	void CColourControl::bSetValue(iCtrlPrec_t val, bool sync) 
	{
		setControlColour((juce::Colour)static_cast<std::uint32_t>(val >= 1.0 ? 0xFFFFFFFF : val * 0x100000000ul));
	}

	bool CColourControl::bValueToString(std::string & valueString, iCtrlPrec_t value) const
	{
		char text[30];
		sprintf_s(text, "0x%.8X", static_cast<std::uint32_t>(value >= 1.0 ? 0xFFFFFFFF : value * 0x100000000ul));
		valueString = text;
		return true;
	}

	void CColourControl::paint(juce::Graphics & g)
	{
		CKnobSlider::paint(g);
		g.setColour(valueObject->getAsJuceColour());
		auto b = getTextRect().toFloat();

		g.fillRoundedRectangle(b.withTrimmedRight(5).withTrimmedBottom(2), 5.f);
	}

};
