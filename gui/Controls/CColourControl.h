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

	file:CColourControl.h

		A widget that can display a colour and allow the user to choose a new.
		Note the explicit use of juce::PixelARGB - avoid representing colours
		through integers (platforms has binary inconsistensies).
 
*************************************************************************************/

#ifndef CPL_CCOLOURCONTROL_H
	#define CPL_CCOLOURCONTROL_H

	#include "ControlBase.h"
	#include "CKnobSlider.h"

	namespace cpl
	{
		/*********************************************************************************************

			CColorKnob - an automatable knob which can display a color

		*********************************************************************************************/
		class CColourControl 
		:
			public CKnobSlider,
			private ValueEntityBase::ValueEntityListener
		{


		public:

			CColourControl(ColourValue * valueToReferTo = nullptr, bool takeOwnerShip = false);


			virtual void valueChanged() override;
			virtual void startedDragging() override;
			virtual void stoppedDragging() override;

			// overrides
			virtual void baseControlValueChanged() override;
			virtual void paint(juce::Graphics & g) override;
			virtual iCtrlPrec_t bGetValue() const override;
			virtual void bSetValue(iCtrlPrec_t val, bool sync = false) override;
			virtual bool bStringToValue(const std::string &, iCtrlPrec_t &) const override;
			virtual bool bValueToString(std::string &, iCtrlPrec_t ) const override;

			// used to be different -- stored for legacy
			juce::Colour getControlColour();
			juce::Colour getControlColourAsColour();

			void setControlColour(juce::Colour newColour);

			virtual std::unique_ptr<CCtrlEditSpace> bCreateEditSpace() override;
			ColourValue & getValueReference() { return *valueObject; }

		protected:
			virtual std::string bGetExportedName() override { return valueObject->getContextualName(); }
			virtual void valueEntityChanged(ValueEntityListener * sender, ValueEntityBase * value) override;

			virtual void onControlSerialization(CSerializer::Archiver & ar, Version version) override;
			virtual void onControlDeserialization(CSerializer::Builder & ar, Version version) override;

			std::unique_ptr<ColourValue, Utility::MaybeDelete<ColourValue>> valueObject;

		private:
			void setValueReference(ColourValue * valueToReferTo, bool takeOwnerShip = false);

		};

	};

#endif