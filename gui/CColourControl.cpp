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
#include "../CCtrlEditSpace.h"

namespace cpl
{
	
	juce::Colour ColourFromPixelARGB(juce::PixelARGB pixel)
	{
		return juce::Colour(pixel.getRed(), pixel.getGreen(), pixel.getBlue(), pixel.getAlpha());
	}
	
	
	
	const char * argbTypes[] = 
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
			for (int i = 0; i < getNumChildComponents(); ++i)
			{
				auto currentChild = getChildComponent(i);
				if (juce::Slider * s = dynamic_cast<juce::Slider*>(currentChild))
				{
					auto currentWidth = s->getTextBoxWidth();
					auto currentHeight = s->getTextBoxWidth();
					auto currentPos = s->getTextBoxPosition();
					s->setTextBoxStyle(currentPos, false, currentWidth / 3, currentHeight);
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

			addAndMakeVisible(argbSelector);
			selector.addChangeListener(this);

			selector.setCurrentColour(ColourFromPixelARGB(parent->getControlColour()));
			toolTip = "Colour editor space - adjust ARGB values of controls precisely.";

			juce::StringArray choices;
			for (const char * str : argbTypes)
			{
				choices.add(str);
			}
			argbSelector.addItemList(choices, 1);
			auto type = parent->getType();
			if (type != parent->SingleChannel)
			{
				argbSelector.setSelectedId(type + 1, juce::dontSendNotification);
			}
			else
			{
				argbSelector.setSelectedId(type + 1 + parent->getChannel(), juce::dontSendNotification);
			}
			argbSelector.addListener(this);
			setOpaque(false);
			//addAndMakeVisible(selector);
		}

		virtual void resized() override
		{
			argbSelector.setBounds(1, oldHeight, fullWidth - elementHeight - 3, elementHeight);
			auto bounds = argbSelector.getBounds();
			selector.setBounds(1, bounds.getBottom(), fullWidth - elementHeight - 3, extraHeight - bounds.getHeight());
			CKnobSliderEditor::resized();
		}

		virtual juce::String bGetToolTipForChild(const juce::Component * child) const override
		{
			if (child == &argbSelector || argbSelector.isParentOf(child))
			{
				return "Set which components of the colour the control adjusts.";
			}
			return CKnobSliderEditor::bGetToolTipForChild(child);
		}
		virtual void comboBoxChanged(juce::ComboBox * boxThatChanged) override
		{
			if (boxThatChanged == &argbSelector)
			{
				auto id = argbSelector.getSelectedId() - 1;
				if (id > 2)
				{
					id -= 2;
					parent->setChannel(id - 1);
					parent->setType(parent->SingleChannel);
					
				}
				else
					parent->setType((CColourControl::ColourType)(id));

				animateSucces(&argbSelector);
			}
			CKnobSliderEditor::comboBoxChanged(boxThatChanged);
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
				
				auto colour = selector.getCurrentColour();
				juce::PixelARGB p(colour.getAlpha(), colour.getRed(), colour.getGreen(), colour.getBlue());
				
				//std::cout << std::hex << "Component: broadcasting " << p.getARGB() << std::endl;
				parent->setControlColour(p);
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
				selector.setCurrentColour(ColourFromPixelARGB(parent->getControlColour()));
			}
			CKnobSliderEditor::valueChanged(ctrl);
		}
		virtual void setMode(bool newMode)
		{
			if (!newMode)
			{
				selector.setCurrentColour(ColourFromPixelARGB(parent->getControlColour()));
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
		static const int extraHeight = 210;
		static const int extraWidth = 10;
		bool recursionFlagWeChanged;
		bool recursionFlagTheyChanged;
		int oldHeight, oldWidth;
		CColourControl * parent;
		CustomColourSelector selector;
		juce::ComboBox argbSelector;
	};

	std::unique_ptr<CCtrlEditSpace> CColourControl::bCreateEditSpace()
	{
		if (isEditSpacesAllowed)
			return std::unique_ptr<CCtrlEditSpace>(new ColourEditor(this));
		else
			return nullptr;
	}

	// endian-less pixel interface.
	// slow and a PITA but the only solution!
	struct ARGBPixel
	{
		struct
		{
			// components
			//std::uint8_t a, r, g, b; <- big endian
			std::uint8_t b, g, r, a;
		} c;
		
		juce::PixelARGB toPixelARGB() { return juce::PixelARGB(c.a, c.r, c.g, c.b); }
		void fromPixelARGB(juce::PixelARGB p) { c.b = p.getBlue(); c.g = p.getGreen(); c. r = p.getRed(); c.a = p.getAlpha(); }
		ARGBPixel() : c() {}
		#ifdef CPL_MSVC
			ARGBPixel(std::uint8_t red, std::uint8_t green, std::uint8_t blue) 
			{
				c.a = 0;
				c.r = red;
				c.g = green;
				c.b = blue;
			}
			ARGBPixel(std::uint8_t alpha, std::uint8_t red, std::uint8_t green, std::uint8_t blue)
			{
				c.r = red;
				c.g = green;
				c.b = blue;
				c.a = alpha;
			}
		#else
			ARGBPixel(std::uint8_t red, std::uint8_t green, std::uint8_t blue) : c{ red, green, blue, 0} {}
			ARGBPixel(std::uint8_t alpha, std::uint8_t red, std::uint8_t green, std::uint8_t blue) : c{ red, green, blue, alpha} {}
		#endif
	};
	
	/*
	 
	 How to fix this:
		All setting / getting of colours happens through juce::PixelARGB / juce::Colour.
			Internally (including serialization) happens through ARGBPixel which is the same
			across endianness and platforms.
	 
	 
	 */
	

	/*********************************************************************************************

	CKnobEx

	*********************************************************************************************/
	CColourControl::CColourControl(const std::string & name, ColourType typeToUse)
		: CKnobSlider(name), colourType(typeToUse), colour(0xFF, 0, 0, 0), channel(Channels::Red)
	{
		isEditSpacesAllowed = true;
		onValueChange();
	}
	/*********************************************************************************************/

	void CColourControl::save(CSerializer::Archiver & ar, long long int version)
	{
		CKnobSlider::save(ar, version);
		
		std::uint8_t a = colour.getAlpha(), r = colour.getRed(), g = colour.getGreen(), b = colour.getBlue();
		ar << a; ar << r; ar << g; ar << b;
		ar << getType();
	}
	void CColourControl::load(CSerializer::Builder & ar, long long int version)
	{
		CKnobSlider::load(ar, version);
		decltype(getType()) newType;
		std::uint8_t a = colour.getAlpha(), r = colour.getRed(), g = colour.getGreen(), b = colour.getBlue();
		ar >> a; ar >> r; ar >> g; ar >> b;
		ar >> newType;
		setType(newType);
		colour = juce::PixelARGB(a, r, g, b);
		setControlColour(colour);
	}

	void CColourControl::onValueChange()
	{
		colour = floatToInt(bGetValue());
		
		//std::cout << "final colour = " << colour.getARGB() << std::endl;
	}
	void CColourControl::setType(ColourType typeToUse)
	{
		colourType = typeToUse;
		bSetValue(intToFloat(colour));
	}
	CColourControl::ColourType CColourControl::getType() const
	{
		return colourType;
	}
	void CColourControl::setChannel(int newChannel)
	{
		channel = newChannel % 3;
	}
	int CColourControl::getChannel() const
	{
		return channel;
	}
	
#if JUCE_BIG_ENDIAN
	#error rewrite the two lower functions!
#endif
	
	juce::PixelARGB CColourControl::floatToInt(iCtrlPrec_t val) const
	{

		switch (colourType) 
		{
		case RGB:
		{

			std::uint32_t intensity = static_cast<std::uint32_t>(val * 0xFFFFFF);
			
			std::uint8_t red, green, blue;
			red = std::uint8_t((intensity & 0xFF0000) >> 16);
			green = std::uint8_t((intensity & 0xFF00) >> 8);
			blue = std::uint8_t((intensity & 0xFF));
			
			
			juce::PixelARGB newRet(colour.getAlpha(), red, green, blue);

			return newRet;
		}
		case ARGB:
		{
			// copy alpha and preserve it, even though we are
			// in RGB mode.
			
			std::size_t intensity = static_cast<std::size_t>(val * 0xFFFFFFFF);
			
			std::uint8_t red, green, blue, alpha;
			alpha = std::uint8_t((intensity & 0xFF000000) >> 24);
			red = std::uint8_t((intensity & 0xFF0000) >> 16);
			green = std::uint8_t((intensity & 0xFF00) >> 8);
			blue = std::uint8_t((intensity & 0xFF));

			
			juce::PixelARGB newRet(alpha, red, green, blue);
			
			return newRet.getARGB();
		}
		case SingleChannel:
		{
			auto colourCopy = colour;
			switch(channel)
			{
				case Red:	colourCopy.getRed() = static_cast<std::uint8_t>(val * 0xFF); break;
				case Green: colourCopy.getGreen() = static_cast<std::uint8_t>(val * 0xFF); break;
				case Blue:	colourCopy.getBlue() = static_cast<std::uint8_t>(val * 0xFF); break;
				case Alpha: colourCopy.getAlpha() = static_cast<std::uint8_t>(val * 0xFF); break;
			}
			return colourCopy;
		}
		case GreyTone:
		{
			auto intensity = static_cast<std::uint8_t>(val * 0xFF);
			juce::PixelARGB newRet(colour.getAlpha(), intensity, intensity, intensity);
			
			return newRet;
		}
		default:
			return juce::PixelARGB(static_cast<std::uint32_t>(val * 0xFFFFFFFF));
		};
	}

	iCtrlPrec_t CColourControl::intToFloat(juce::PixelARGB val) const
	{
		switch (colourType)
		{
		case RGB:
		{
			std::uint32_t components = std::uint32_t(val.getARGB()) & std::uint32_t(0xFFFFFF);
				
			return double(components) / (double)0xFFFFFF;
		}
		case ARGB:
			return double(val.getARGB()) / (double)0xFFFFFFFF;
		case SingleChannel:
		{
			std::uint8_t intensity = 0;
			switch(channel)
			{
				case Red:	intensity = colour.getRed(); break;
				case Green: intensity = colour.getGreen(); break;
				case Blue:	intensity = colour.getBlue(); break;
				case Alpha: intensity = colour.getAlpha(); break;
			}

			return intensity / (double)0xFF;
		}
		case GreyTone:
		{
			double sum = val.getRed() + val.getGreen() + val.getBlue();
			return sum / (3 * 0xFF);
		}
		default:
			return 0;
		};
	}
	juce::PixelARGB CColourControl::getControlColour()
	{
		return colour;
	}
	juce::Colour CColourControl::getControlColourAsColour()
	{
		return ColourFromPixelARGB(colour);
	}
	void CColourControl::setControlColour(juce::PixelARGB newColour)
	{

		auto floatingRepresentation = intToFloat(newColour);
		colour = floatToInt(floatingRepresentation);
		//std::cout << "Value " << std::hex << newColour.getARGB()  << "set to " <<  colour.getARGB() << std::endl;

		getSlider().setValue(
			floatingRepresentation * (getSlider().getMaximum() - getSlider().getMinimum()) + getSlider().getMinimum(), 
			juce::NotificationType::sendNotificationSync);
	}
	bool CColourControl::bStringToValue(const std::string & valueString, iCtrlPrec_t & value) const
	{
		juce::Colour g;
		g.getARGB();
		std::uint32_t result(0);
		char * endPtr = nullptr;
		result = static_cast<std::uint32_t>(strtoul(valueString.c_str(), &endPtr, 0));
		if (endPtr > valueString.c_str())
		{
			value = intToFloat(result);
			return true;
		}
		return false;
	}

	bool CColourControl::bValueToString(std::string & valueString, iCtrlPrec_t value) const
	{
		char text[30];
		sprintf_s(text, "0x%.8X", floatToInt(value).getARGB());
		valueString = text;
		return true;
	}

	void CColourControl::paint(juce::Graphics & g)
	{
		CKnobSlider::paint(g);
		g.setColour(ColourFromPixelARGB(colour));
		auto b = getTextRect().toFloat();

		g.fillRoundedRectangle(b.withTrimmedRight(5).withTrimmedBottom(2), 5.f);




	}

};
