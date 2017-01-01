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
 
	file:CComboBox.h
 
		A combobox.
 
*************************************************************************************/

#ifndef CPL_CVALUECOMBOBOX_H
	#define CPL_CVALUECOMBOBOX_H

	#include "ControlBase.h"
	#include "ValueControl.h"

	namespace cpl
	{

		class CValueComboBox
			: public juce::Component
			, private juce::ComboBox::Listener
			, public ValueControl<ValueEntityBase, CompleteValue<LinearRange<ValueT>, BasicFormatter<ValueT>>>
		{

		public:
			CValueComboBox(ValueEntityBase * valueToReferTo = nullptr, bool takeOwnerShip = false);

			// overrides
			virtual void bSetTitle(const std::string & newTitle) override;
			virtual std::string bGetTitle() const override;
	
			virtual void paint(juce::Graphics & g) override;

			int getZeroBasedSelIndex() const;
			
			const std::string & valueFor(std::size_t idx) const noexcept
			{
				return values.at(idx);
			}

			template<typename T>
				T getZeroBasedSelIndex() const
				{
					return (T)(valueObject->getTransformer().transform(valueObject->getNormalizedValue()));
				}
			
			template<typename T>
				void setZeroBasedIndex(T input)
				{
					valueObject->setNormalizedValue(valueObject->getTransformer().normalize(static_cast<ValueT>(input)));
				}

			bool setEnabledStateFor(const std::string & idx, bool toggle);
			bool setEnabledStateFor(std::size_t idx, bool toggle);



		protected:

			virtual void onValueObjectChange(ValueEntityListener * sender, ValueEntityBase * value) override;

			std::size_t indexOfValue(const std::string & idx) const noexcept;

			void setZeroBasedSelIndex(int index);
			// overrides
			virtual void resized() override;
			virtual void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
			virtual void baseControlValueChanged() override;
			virtual bool queryResetOk() override;

		private:
			// list of |-seperated values
			virtual void setValues(const std::vector<std::string> & values);

			void initialize();

			// data
			std::vector<std::string> values;
			juce::ComboBox box;
			juce::String title;
			juce::Rectangle<int> stringBounds;
		};
	};
#endif