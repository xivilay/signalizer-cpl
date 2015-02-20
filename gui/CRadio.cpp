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

#include "CComboBox.h"
#include "../Mathext.h"

namespace cpl
{
	/*********************************************************************************************

		CComboBox - constructor

	*********************************************************************************************/
	CComboBox::CComboBox(const std::string & name, const std::string & inputValues)
		: CBaseControl(this), title(name), internalValue(0.0), recursionFlag(false)
	{
		isEditSpacesAllowed = true;
		setValues(inputValues);
		initialize();
	}
	/*********************************************************************************************
	
		CComboBox - constructor

	*********************************************************************************************/
	CComboBox::CComboBox(const std::string & name, const std::vector<std::string> & inputValues)
		: CBaseControl(this), title(name), values(inputValues), internalValue(0.0), recursionFlag(false)
	{
		initialize();
	}

	/*********************************************************************************************

		CComboBox - constructor

	*********************************************************************************************/
	CComboBox::CComboBox()
		: CBaseControl(this), internalValue(0.0), recursionFlag(false)
	{
		initialize();
	}
	/*********************************************************************************************/
	void CComboBox::initialize()
	{
		setSize(ControlSize::Rectangle.width, ControlSize::Rectangle.height);
		addAndMakeVisible(box);
		box.addListener(this);
		box.setRepaintsOnMouseActivity(true);
		box.setSelectedId(1, dontSendNotification);
	}
	/*********************************************************************************************/
	void CComboBox::resized()
	{
		stringBounds = CRect(5, 0, getWidth(), std::min(20, getHeight() / 2));
		box.setBounds(0, stringBounds.getBottom(), getWidth(), getHeight() - stringBounds.getHeight());

	}
	void CComboBox::bSetTitle(const std::string & newTitle)
	{
		title = newTitle;
	}
	std::string CComboBox::bGetTitle() const
	{
		return title.toStdString();
	}
	/*********************************************************************************************/
	void CComboBox::paint(juce::Graphics & g)
	{
		//g.setFont(systemFont.withHeight(TextSize::normalText)); EDIT_TYPE_NEWFONTS
		g.setFont(TextSize::normalText);
		g.setColour(colourCtrlText);
		g.drawFittedText(title, stringBounds, juce::Justification::centredLeft, 1, 1);
	}
	/*********************************************************************************************/
	void CComboBox::setValues(const std::string & inputValues)
	{
		values.clear();
		std::size_t iter_pos(0);
		auto len = inputValues.size();
		for (std::size_t i = 0; i < len; i++)
		{
			if (inputValues[i] == '|') 
			{
				values.push_back(inputValues.substr(iter_pos, i - iter_pos));
				iter_pos = i + 1;
			}
			else if (i == (len - 1)) 
			{
				values.push_back(inputValues.substr(iter_pos, i + 1 - iter_pos));

			}
		}
		setValues(values);
	}

	int floatToInt(iCtrlPrec_t inVal, int size)
	{
		if (1 >= size)
		{
			return 0;
		}
		else
		{
			return cpl::Math::round<int>(1 + inVal * size);
		}
	}

	iCtrlPrec_t intToFloat(int idx, int size)
	{
		if (!idx)
			return 0;
		if (1 >= size)
		{
			return 0;
		}
		else
		{
			return iCtrlPrec_t(idx - 1) / (size - 1);
		}
	}

	/*********************************************************************************************/
	void CComboBox::setValues(const std::vector<std::string> & inputValues)
	{
		values = inputValues;
		auto numIDs = box.getNumItems();
		box.clear();
		juce::StringArray arr;
		for (const auto & str : inputValues)
			arr.add(str);
		box.addItemList(arr, 1);
		bSetInternal(bGetValue());
	}
	/*********************************************************************************************/
	void CComboBox::bSetValue(iCtrlPrec_t val)
	{
		box.setSelectedId(floatToInt(val, values.size()));
	}
	/*********************************************************************************************/
	void CComboBox::bSetInternal(iCtrlPrec_t val)
	{
		box.setSelectedId(floatToInt(val, values.size()), juce::dontSendNotification);
	}
	/*********************************************************************************************/
	void CComboBox::onValueChange()
	{
	}
	/*********************************************************************************************/
	iCtrlPrec_t CComboBox::bGetValue() const
	{
		return intToFloat(box.getSelectedId(), values.size());
	}
	/*********************************************************************************************/
	bool CComboBox::bValueToString(std::string & valueString, iCtrlPrec_t val) const
	{
		std::size_t idx = floatToInt(Math::confineTo<iCtrlPrec_t>(val, 0.0, 1.0), values.size() - 1);
		valueString = values[idx];
		return true;
	}

	/*********************************************************************************************/
	bool CComboBox::bStringToValue(const std::string & valueString, iCtrlPrec_t & val) const
	{
		for (std::size_t i = 0; i < values.size(); ++i)
		{
			if (values[i] == valueString)
			{
				// can never divide by zero here, since the for loop won't enter in that case
				val = iCtrlPrec_t(i) / (values.size() > 1 ? values.size() - 1 : 1);
				return true;
			}
		}
		return false;
	}
};
