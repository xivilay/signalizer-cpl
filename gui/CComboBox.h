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
 
 file:ComponentContainers.h
 
	A collection of views that can contain other views.
 
 *************************************************************************************/

#ifndef _CCOMBOBOX_H
	#define _CCOMBOBOX_H

	#include "ControlBase.h"
	#include <string>
	#include <vector>

	namespace cpl
	{
		/*********************************************************************************************

			CValueControl - an extended knob that shows a list of values instead.

		*********************************************************************************************/
		class CComboBox
		: 
			public juce::Component,
			public cpl::CBaseControl
		{

		public:
			// 'C' constructor
			// 'values' is a list of |-seperated values
			CComboBox(const std::string & name, const std::string & values);
			CComboBox(const std::string & name, const std::vector<std::string> & values);
			CComboBox();

			// list of |-seperated values
			virtual void setValues(const std::string & values);
			virtual void setValues(const std::vector<std::string> & values);

			// overrides
			virtual void bSetTitle(const std::string & newTitle) override;
			virtual std::string bGetTitle() const override;
	
			virtual iCtrlPrec_t bGetValue() const override;
			virtual void bSetValue(iCtrlPrec_t val, bool sync = false) override;
			virtual void bSetInternal(iCtrlPrec_t val) override;
			virtual void paint(juce::Graphics & g) override;

			int getZeroBasedSelIndex() const;

		protected:
			// overrides
			virtual bool bStringToValue(const std::string & valueString, iCtrlPrec_t & val) const override;
			virtual bool bValueToString(std::string & valueString, iCtrlPrec_t val) const override;
			virtual void resized() override;
			virtual void onValueChange() override;

		private:
			void initialize();

			// data
			std::vector<std::string> values;
			juce::ComboBox box;
			juce::String title;
			juce::Rectangle<int> stringBounds;
			iCtrlPrec_t internalValue;
			bool recursionFlag;
		};
	};
#endif