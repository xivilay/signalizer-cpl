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
 
	file:CValueInputControl.h
 
		A simple CBaseControl widget that contains a text field with a title, and controls
		a value.
 
*************************************************************************************/

#ifndef CPL_CVALUEINPUTCONTROL_H
	#define CPL_CVALUEINPUTCONTROL_H

	#include "ControlBase.h"
	#include "ValueControl.h"
	#include <string>
	#include <vector>
	#include "../BuildingBlocks.h"

	namespace cpl
	{
		/*********************************************************************************************

			CValueControl - an extended knob that shows a list of values instead.

		*********************************************************************************************/
		class CValueInputControl
		: 
			public juce::Component,
			protected juce::Label::Listener,
			protected juce::ChangeListener,
			public ValueControl<ValueEntityBase, CompleteValue<LinearRange<ValueT>, BasicFormatter<ValueT>>>,
			protected DestructionNotifier
		{

		public:

			CValueInputControl(ValueEntityBase * valueToReferTo = nullptr, bool takeOwnerShip = false);

			// overrides
			virtual void bSetTitle(const std::string & newTitle) override;
			virtual std::string bGetTitle() const override;
	
			virtual void paint(juce::Graphics & g) override;

			virtual void indicateSuccess();
			virtual void indicateError();
			~CValueInputControl();
		protected:
			juce::ComponentAnimator & getAnimator() { return juce::Desktop::getInstance().getAnimator(); }
			virtual void changeListenerCallback(ChangeBroadcaster *source) override;
			virtual void labelTextChanged(Label *labelThatHasChanged) override;
			virtual void onValueObjectChange(ValueEntityListener * sender, ValueEntityBase * value) override;
			virtual void CValueInputControl::baseControlValueChanged() override;
			// overrides
			virtual void resized() override;

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