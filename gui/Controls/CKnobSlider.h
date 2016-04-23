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
 
	file:CKnobSlider.h

		Base class of all knobsliders.
 
*************************************************************************************/

#ifndef CPL_CKNOBSLIDER_H
	#define CPL_CKNOBSLIDER_H

	#include "../Common.h"
	#include "../CBaseControl.h"

	namespace cpl
	{
		/*********************************************************************************************

			Basic slider / knob interface.

		*********************************************************************************************/
		class CKnobSlider 
		: 
			public juce::Slider,
			public CBaseControl
		{

		public:
			// some basic functionality types.
			enum ControlType 
			{
				pct, // 0 .. 100%
				hz, // 0 .. 8000 Hz
				db, // -oo .. 0 dB
				ft, // 0.0 .. 1.0
				ms // 0 .. 1000 ms
			};

			const float hzLimit = 8000.0f;
			const int msLimit = 1000;

			CKnobSlider(const std::string & name = "", ControlType typeToRepresent = ControlType::pct);

			// overrides
			virtual iCtrlPrec_t bGetValue() const override;
			virtual void bSetInternal(iCtrlPrec_t) override;
			virtual void bSetText(const std::string & in) override;
			virtual void bSetTitle(const std::string & in) override;
			virtual void bSetValue(iCtrlPrec_t newValue, bool sync = false) override;
			virtual void bRedraw() override;
			virtual std::string bGetText() const override;
			virtual std::string bGetTitle() const override;
			virtual void onValueChange() override;
			virtual void paint(juce::Graphics& g) override;
			virtual std::unique_ptr<CCtrlEditSpace> bCreateEditSpace() override;
			virtual void serialize(CSerializer::Archiver & ar, Version version) override;
			virtual void deserialize(CSerializer::Builder & ar, Version version) override;


			// new functions
			virtual void setCtrlType(ControlType newType);
			virtual void setIsKnob(bool shouldBeKnob);
			virtual bool getIsKnob() const;
			virtual juce::Rectangle<int> getTextRect() const;
			virtual juce::Rectangle<int> getTitleRect() const;
		protected:
			virtual bool bStringToValue(const std::string & valueString, iCtrlPrec_t & val) const override;
			virtual bool bValueToString(std::string & valueString, iCtrlPrec_t val) const override;

			// true if this is displayed as a knob, otherwise it is a slider.
			bool isKnob;
			void computePaths();
			juce::Slider & getSlider() { return *this; }
			iCtrlPrec_t laggedValue;

		private:

			juce::Path pie, pointer;

			juce::Slider::SliderStyle oldStyle;
			ControlType type;
			std::string title, text;
		};
	};
#endif