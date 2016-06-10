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
 
	file:CValueKnobSlider.cpp
 
		Source code for CValueKnobSlider.h
 
*************************************************************************************/

#include "CValueKnobSlider.h"


namespace cpl
{

	CValueKnobSlider::CValueKnobSlider(ValueEntityBase * valueToReferTo, bool takeOwnerShip = false)
		: valueObject(nullptr)
	{
		setValueReference(valueToReferTo, takeOwnerShip);
	}

	void CValueKnobSlider::setValueReference(ValueEntityBase * valueToReferTo, bool takeOwnerShip = false)
	{
		if (valueObject != nullptr)
		{
			valueObject->removeListener(this);
			valueObject.reset(nullptr);
		}

		if (valueToReferTo == nullptr)
		{
			valueToReferTo = new CompleteValue<LinearRange<iCtrlPrec_T>, BasicFormatter<iCtrlPrec_t>>();
			takeOwnerShip = true;
		}

		valueObject.reset(valueToReferTo);
		valueObject.get_deleter().doDelete = takeOwnerShip;
		valueObject->addListener(this);
	}

	bool CValueKnobSlider::bStringToValue(const std::string & valueString, iCtrlPrec_t & val) const
	{
		iCtrlPrec_t tempValue;
		if (valueObject->getFormatter().interpret(valueString, tempValue))
		{
			val = valueObject->getTransformer().normalize(tempValue);
			return true;
		}
		return false;
	}

	bool CValueKnobSlider::bValueToString(std::string & valueString, iCtrlPrec_t val) const
	{
		return valueObject->getFormatter().format(valueObject->getTransformer().transform(val), valueString);
	}


	iCtrlPrec_t CKnobSlider::bGetValue() const
	{
		return valueObject->getNormalizedValue();
	}

	void CValueKnobSlider::onControlSerialization(CSerializer::Archiver & ar, Version version)
	{
		return CKnobSlider::onControlDeserialization(ar, version);
	}

	void CValueKnobSlider::onControlDeserialization(CSerializer::Builder & ar, Version version)
	{
		return CKnobSlider::onControlDeserialization(ar, version);
	}

	void CValueKnobSlider::valueEntityChanged(ValueEntityListener * sender, ValueEntityBase * value)
	{
		setValue(newValue, juce::Notification::DontSendNotification);
		bFormatValue(text, bGetValue());
		bSetText(text);
		bForceEvent();
		repaint();
	}

	void CValueKnobSlider::bSetInternal(iCtrlPrec_t newValue)
	{
		valueObject->setNormalizedValue(newValue);
	}

	void CValueKnobSlider::bSetValue(iCtrlPrec_t newValue, bool sync)
	{
		valueObject->setNormalizedValue(newValue);
	}

	void CValueKnobSlider::onValueChange()
	{
		valueObject->setNormalizedValue(static_cast<iCtrlPrec_t>(getValue());
	}

};
