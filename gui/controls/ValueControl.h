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

	file:ValueControl.h

		Provides common implementations of CBaseControls and values.

*************************************************************************************/

#ifndef CPL_VALUECONTROL_H
#define CPL_VALUECONTROL_H

#include "ControlBase.h"
#include "../../Utility.h"

namespace cpl
{

	/// <summary>
	/// Concept value base: ExportedName
	/// </summary>
	template<typename ValueBase, class Substitute, typename Enable = void>
	class ValueControl;

	template<typename ValueBase, class Substitute>
	class ValueControl <ValueBase, Substitute, typename std::enable_if<std::is_base_of<ValueEntityBase, ValueBase>::value>::type>
		: public CBaseControl
		, public ValueEntityBase::ValueEntityListener
	{
	public:

		ValueControl(GraphicComponent * windowBase, ValueBase * base, bool takeOwnership = false)
			: CBaseControl(windowBase), valueObject(nullptr)
		{
			setValueReference(base, takeOwnership);
		}

		std::string bGetExportedName() override
		{
			return valueObject->getContextualName();
		}

		ValueBase & getValueReference() { return *valueObject; }

	protected:

		virtual void onControlSerialization(CSerializer::Archiver & ar, Version v) override
		{
			ar << *valueObject;
		}

		virtual void onControlDeserialization(CSerializer::Archiver & ar, Version v) override
		{
			ar >> *valueObject;
		}

		void bSetValue(iCtrlPrec_t value, bool sync = false) override
		{
			valueObject->setNormalizedValue(value);
		}

		iCtrlPrec_t bGetValue() const override
		{
			return valueObject->getNormalizedValue();
		}

		void bSetInternal(iCtrlPrec_t newValue) override
		{
			valueObject->setNormalizedValue(newValue);
		}

		bool bStringToValue(const string_ref valueString, iCtrlPrec_t & val) const override
		{
			iCtrlPrec_t tempValue;
			if (valueObject->getFormatter().interpret(valueString, tempValue))
			{
				val = valueObject->getTransformer().normalize(tempValue);
				return true;
			}
			return false;
		}

		bool bValueToString(std::string & valueString, iCtrlPrec_t val) const override
		{
			return valueObject->getFormatter().format(valueObject->getTransformer().transform(val), valueString);
		}

		virtual void onValueObjectChange(ValueEntityListener * sender, ValueEntityBase * value) {}

		void valueEntityChanged(ValueEntityListener * sender, ValueEntityBase * value) override
		{
			onValueObjectChange(sender, value);
			baseControlValueChanged();
		}

		void setValueReference(ValueBase * valueToReferTo, bool takeOwnerShip = false)
		{
			if (valueObject != nullptr)
			{
				valueObject->removeListener(this);
				valueObject.reset(nullptr);
			}

			if (valueToReferTo == nullptr)
			{
				valueToReferTo = new Substitute();
				takeOwnerShip = true;
			}

			valueObject.reset(valueToReferTo);
			valueObject.get_deleter().doDelete = takeOwnerShip;
			valueObject->addListener(this);
		}

		virtual ~ValueControl()
		{
			valueObject->removeListener(this);
		}

		std::unique_ptr<ValueBase, Utility::MaybeDelete<ValueBase>> valueObject;
	};


	template<typename ValueBase, class Substitute>
	class ValueControl<ValueBase, Substitute, typename std::enable_if<std::is_base_of<ValueGroup, ValueBase>::value>::type>
		: public CBaseControl
		, public ValueEntityBase::ValueEntityListener
	{
	public:

		ValueControl(GraphicComponent * windowBase, ValueBase * base, bool takeOwnership = false)
			: CBaseControl(windowBase), valueObject(nullptr)
		{
			setValueReference(base, takeOwnership);
		}

		std::string bGetExportedName() override
		{
			return valueObject->getContextualName();
		}

		ValueBase & getValueReference() { return *valueObject; }



	protected:

		virtual void onControlSerialization(CSerializer::Archiver & ar, Version v) override
		{
			for (std::size_t i = 0; i < valueObject->getNumValues(); i++)
				ar << valueObject->getValueIndex(i);
		}

		virtual void onControlDeserialization(CSerializer::Archiver & ar, Version v) override
		{
			for (std::size_t i = 0; i < valueObject->getNumValues(); i++)
				ar >> valueObject->getValueIndex(i);
		}


		virtual void onValueObjectChange(ValueEntityListener * sender, ValueEntityBase * value) {}

		void valueEntityChanged(ValueEntityListener * sender, ValueEntityBase * value) override
		{
			onValueObjectChange(sender, value);
			baseControlValueChanged();
		}

		void setValueReference(ValueBase * valueToReferTo, bool takeOwnerShip = false)
		{

			if (valueObject != nullptr)
			{
				const auto numValues = valueObject->getNumValues();
				for (std::size_t i = 0; i < numValues; ++i)
				{
					valueObject->getValueIndex(i).removeListener(this);
				}
				valueObject.reset(nullptr);
			}

			if (valueToReferTo == nullptr)
			{
				valueToReferTo = new Substitute();
				takeOwnerShip = true;
			}

			valueObject.reset(valueToReferTo);
			valueObject.get_deleter().doDelete = takeOwnerShip;

			const auto newNumValues = valueObject->getNumValues();
			for (std::size_t i = 0; i < newNumValues; ++i)
			{
				valueObject->getValueIndex(i).addListener(this);
			}
		}

		virtual ~ValueControl()
		{
			const auto newNumValues = valueObject->getNumValues();
			for (std::size_t i = 0; i < newNumValues; ++i)
			{
				valueObject->getValueIndex(i).removeListener(this);
			}
		}

		std::unique_ptr<ValueBase, Utility::MaybeDelete<ValueBase>> valueObject;
	};

};

#endif