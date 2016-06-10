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
 
	file:CValueKnobSlider.h

		A self-contained slider with defined value semantics.
 
*************************************************************************************/

#ifndef CPL_CVALUEKNOBSLIDER_H
	#define CPL_CVALUEKNOBSLIDER_H

	#include "CKnobSlider.h"
	#include "ControlBase.h"

	namespace cpl
	{
		/*********************************************************************************************

			Basic slider / knob interface.

		*********************************************************************************************/
		class CValueKnobSlider 
			: public CKnobSlider
			, private ValueEntityBase::ValueEntityListener
		{
		public:


			CValueKnobSlider(ValueEntityBase * valueToReferTo, bool takeOwnerShip = false);

			void setValueReference(ValueEntityBase * valueToReferTo, bool takeOwnerShip = false);

			// overrides
			virtual iCtrlPrec_t bGetValue() const override;
			virtual void bSetInternal(iCtrlPrec_t) override;
			virtual void bSetValue(iCtrlPrec_t newValue, bool sync = false) override;
			//virtual std::unique_ptr<CCtrlEditSpace> bCreateEditSpace() override;

		protected:

			virtual void onValueChange() override;
			virtual void paint(juce::Graphics& g) override;

			virtual void onControlSerialization(CSerializer::Archiver & ar, Version version) override;
			virtual void onControlDeserialization(CSerializer::Builder & ar, Version version) override;

			virtual bool bStringToValue(const std::string & valueString, iCtrlPrec_t & val) const override;
			virtual bool bValueToString(std::string & valueString, iCtrlPrec_t val) const override;

			virtual void valueEntityChanged(ValueEntityListener * sender, ValueEntityBase * value) override;

		private:

			std::unique_ptr<ValueEntityBase, Utility::MaybeDelete<ValueEntityBase>> valueObject;
		};
	};
#endif