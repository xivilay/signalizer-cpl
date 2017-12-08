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

	file:CInputControl.h

		A simple CBaseControl widget that contains a text field with a title, and callbacks
		on text modifications.

*************************************************************************************/

#ifndef CPL_CINPUTCONTROL_H
#define CPL_CINPUTCONTROL_H

#include "ControlBase.h"
#include <string>
#include <vector>
#include "../BuildingBlocks.h"

namespace cpl
{
	/*********************************************************************************************

		CValueControl - an extended knob that shows a list of values instead.

	*********************************************************************************************/
	class CInputControl
		:
		public juce::Component,
		protected juce::Label::Listener,
		protected juce::ChangeListener,
		public cpl::CBaseControl,
		public DestructionNotifier
	{

	public:

		CInputControl(std::string name);
		~CInputControl();
		// list of |-seperated values
		virtual void setInputValue(const string_ref value, bool sync = true);
		virtual void setInputValueInternal(const string_ref value);
		virtual std::string getInputValue() const;

		// overrides
		virtual void bSetTitle(std::string newTitle) override;
		virtual std::string bGetTitle() const override;

		//virtual iCtrlPrec_t bGetValue() const override;
		//virtual void bSetValue(iCtrlPrec_t val, bool sync = false) override;
		//virtual void bSetInternal(iCtrlPrec_t val) override;
		virtual void paint(juce::Graphics & g) override;

		virtual void indicateSuccess();
		virtual void indicateError();

	protected:
		juce::ComponentAnimator & getAnimator() { return juce::Desktop::getInstance().getAnimator(); }
		virtual void changeListenerCallback(ChangeBroadcaster *source) override;
		virtual void labelTextChanged(Label *labelThatHasChanged) override;

		// overrides
		/*virtual bool bStringToValue(const std::string & valueString, iCtrlPrec_t & val) const override;
		virtual bool bValueToString(std::string & valueString, iCtrlPrec_t val) const override; */
		virtual void resized() override;
		//virtual void baseControlValueChanged() override;

	private:
		void initialize();
		// data
		SemanticBorder errorVisualizer;
		juce::Label box;
		juce::String title;
		juce::Rectangle<int> stringBounds;

	};
};
#endif