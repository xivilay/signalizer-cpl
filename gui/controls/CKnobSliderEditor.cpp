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
 
	file:CKnobSliderEditor.cpp
 
		Source code for CKnobSliderEditor.h
 
*************************************************************************************/

#include "CKnobSliderEditor.h"
#include "CKnobSlider.h"

namespace cpl
{

	static const char * knobTypes[] =
	{
		"Rotable knob",
		"Slider"
	};

	CKnobSliderEditor::CKnobSliderEditor(CKnobSlider * parentToControl)
		: CCtrlEditSpace(parentToControl), parent(parentToControl)
	{
		oldHeight = fullHeight;
		fullHeight += extraHeight;
		iface.addListener(this);
		int counter = 1;
		for (const auto & str : knobTypes)
		{
			iface.addItem(str, counter++);
		}

		iface.setSelectedId(!parent->getIsKnob() + 1, juce::dontSendNotification);

	}
	juce::String CKnobSliderEditor::bGetToolTipForChild(const juce::Component * c) const
	{
		if (GUIUtils::ViewContains(&iface, c))
		{
			return "Change between visual representations of the control.";
		}
		return CCtrlEditSpace::bGetToolTipForChild(c);
	}
	void CKnobSliderEditor::resized()
	{
		iface.setBounds(1, oldHeight, fullWidth - (elementHeight + 4), elementHeight);
		CCtrlEditSpace::resized();
	}
	void CKnobSliderEditor::comboBoxChanged(juce::ComboBox * boxThatChanged)
	{
		if (boxThatChanged == &iface)
		{
			parent->setIsKnob(!(iface.getSelectedId() - 1 > 0 ? true : false));
			animateSucces(&iface);
		}
	}
	void CKnobSliderEditor::setMode(bool newMode)
	{
		if (!newMode)
		{
			addAndMakeVisible(&iface);
		}
		else
		{
			removeChildComponent(&iface);
		}
		CCtrlEditSpace::setMode(newMode);
	}

};
