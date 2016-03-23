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
 
	file:CInputControl.cpp
 
		Source code for CInputControl.h
 
*************************************************************************************/

#include "CInputControl.h"

namespace cpl
{
	/*********************************************************************************************

		CComboBox - constructor

	*********************************************************************************************/
	CInputControl::CInputControl(const std::string & name)
		: CBaseControl(this), title(name)
	{
		isEditSpacesAllowed = false;
		initialize();
	}
	/*********************************************************************************************/
	void CInputControl::initialize()
	{
		setSize(ControlSize::Rectangle.width, ControlSize::Rectangle.height);
		addAndMakeVisible(box);
		enableTooltip(true);
		box.addListener(this);
		box.setEditable(true);
		box.setFont(CLookAndFeel_CPL::defaultLook().getStdFont());
		addAndMakeVisible(errorVisualizer);
	}

	void CInputControl::labelTextChanged(Label *labelThatHasChanged)
	{
		if (labelThatHasChanged == &box)
		{
			bForceEvent();
		}
	}

	/*********************************************************************************************/
	void CInputControl::resized()
	{
		stringBounds = CRect(5, 0, getWidth(), std::min(20, getHeight() / 2));
		box.setBounds(0, stringBounds.getBottom(), getWidth(), getHeight() - stringBounds.getHeight());
		errorVisualizer.setBounds(getBounds().withPosition(0, 0));
	
	}
	void CInputControl::bSetTitle(const std::string & newTitle)
	{
		title = newTitle;
	}
	std::string CInputControl::bGetTitle() const
	{
		return title.toStdString();
	}
	/*********************************************************************************************/
	void CInputControl::paint(juce::Graphics & g)
	{
		//g.setFont(systemFont.withHeight(TextSize::normalText)); EDIT_TYPE_NEWFONTS
		g.setFont(TextSize::normalText);
		g.setColour(cpl::GetColour(cpl::ColourEntry::ControlText));
		g.drawFittedText(title, stringBounds, juce::Justification::centredLeft, 1, 1);
		g.setColour(cpl::GetColour(cpl::ColourEntry::Deactivated));
		g.fillRect(box.getBounds());

	}
	CInputControl::~CInputControl()
	{
		notifyDestruction();
	}

	void CInputControl::indicateSuccess()
	{
		if (getAnimator().isAnimating(&errorVisualizer))
		{
			// already animating this
			// TODO: ensure reference doesn't die.
			GUIUtils::FutureMainEvent(500, [&]() { indicateSuccess(); }, this);
		}

		errorVisualizer.borderColour = cpl::GetColour(cpl::ColourEntry::Success);
		errorVisualizer.borderSize = 4.f;
		errorVisualizer.setAlpha(1.f);
		errorVisualizer.isActive = true;
		errorVisualizer.repaint();

		getAnimator().animateComponent(&errorVisualizer, errorVisualizer.getBounds(), 0.f, 300, false, 1.0, 1.0);

	}
	void CInputControl::indicateError()
	{
		if (getAnimator().isAnimating(&errorVisualizer))
		{
			// already animating this
			// TODO: ensure reference doesn't die.
			GUIUtils::FutureMainEvent(500, [&]() { indicateError(); }, this);
		}

		errorVisualizer.borderColour = cpl::GetColour(cpl::ColourEntry::Error);
		errorVisualizer.borderSize = 4.f;
		errorVisualizer.setAlpha(1.f);
		errorVisualizer.isActive = true;
		errorVisualizer.repaint();

		getAnimator().animateComponent(&errorVisualizer, errorVisualizer.getBounds(), 0.f, 300, false, 1.0, 1.0);
	}
	/*********************************************************************************************/
	void CInputControl::setInputValue(const std::string & inputValue, bool sync)
	{
		box.setText(inputValue, sync ? juce::NotificationType::sendNotificationSync : juce::sendNotificationAsync);
	}

	void CInputControl::setInputValueInternal(const std::string & inputValue)
	{
		box.setText(inputValue, juce::NotificationType::dontSendNotification);
	}

	std::string CInputControl::getInputValue() const
	{
		return box.getText().toStdString();
	}

	void CInputControl::changeListenerCallback(ChangeBroadcaster *source)
	{
		auto & animator = juce::Desktop::getInstance().getAnimator();
		if (source == &animator)
		{
			if (!animator.isAnimating(&errorVisualizer))
			{
				// animation finished, hide component
				errorVisualizer.isActive = false;

			}
		}
	}
};
