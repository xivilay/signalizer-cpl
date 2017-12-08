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

#ifndef CPL_CCOMBOBOX_H
#define CPL_CCOMBOBOX_H

#include "ControlBase.h"
#include <string>
#include <vector>

namespace cpl
{

	class CComboBox
		: public juce::Component
		, public cpl::CBaseControl
		, private juce::ComboBox::Listener
	{

	public:
		// 'C' constructor
		// 'values' is a list of |-seperated values
		CComboBox(std::string name, const std::string_view values);
		CComboBox(std::string name, std::vector<std::string> values);
		CComboBox();

		// list of |-seperated values
		virtual void setValues(const std::string_view values);
		virtual void setValues(std::vector<std::string> values);

		// overrides
		virtual void bSetTitle(std::string newTitle) override;
		virtual std::string bGetTitle() const override;

		virtual iCtrlPrec_t bGetValue() const override;
		virtual void bSetValue(iCtrlPrec_t val, bool sync = false) override;
		virtual void bSetInternal(iCtrlPrec_t val) override;
		virtual void paint(juce::Graphics & g) override;

		int getZeroBasedSelIndex() const;

		const std::string & valueFor(std::size_t idx) const noexcept
		{
			return values.at(idx);
		}

		template<typename T>
		T getZeroBasedSelIndex() const
		{
			return (T)(box.getSelectedId() - 1);
		}

		template<typename T>
		void setZeroBasedIndex(T input)
		{
			box.setSelectedId(((int)input) - 1);
		}

		bool setEnabledStateFor(const std::string_view idx, bool toggle);
		bool setEnabledStateFor(std::size_t idx, bool toggle);
	protected:

		std::size_t indexOfValue(const std::string_view idx) const noexcept;

		void setZeroBasedSelIndex(int index);
		// overrides
		virtual bool bStringToValue(const string_ref valueString, iCtrlPrec_t & val) const override;
		virtual bool bValueToString(std::string & valueString, iCtrlPrec_t val) const override;
		virtual void resized() override;
		virtual void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
		virtual void baseControlValueChanged() override;
		virtual bool queryResetOk() override;

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