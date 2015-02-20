/*************************************************************************************

	Audio Programming Environment VST. 
		
		VST is a trademark of Steinberg Media Technologies GmbH.

    Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:Misc.cpp
		
		Implementation of Misc.h

*************************************************************************************/

#include "MacroConstants.h"
#include "CCtrlEditSpace.h"
#include "NewStuffAndLook.h"
#include "GUIUtils.h"
#include <exception>
#include <typeinfo>
#include "SysStats.h"

namespace cpl
{
	void CCtrlEditSpace::valueChanged(const CBaseControl * ctrl)
	{
		resetToControl();
	}

	CCtrlEditSpace::CCtrlEditSpace(cpl::CBaseControl * parent)
		: parentControl(parent), hasBeenInitialized(false), exitAfterAnimation(false), inputValueWasValid(false),
		toolTip("Control Edit Space: Interface for editing the values of controls precisely."),
		expanderButton(new CTriangleButton()),
		compactWidth(100),
		compactHeight(25),
		fullWidth(200),
		fullHeight(100),
		compactMode(true)

	{

		iconSucces.setImage(CVectorResource::renderSVGToImage("icons/svg/succestick.svg", juce::Rectangle<int>(15, 15), cpl::GetColour(cpl::ColourEntry::success)));
		iconError.setImage(CVectorResource::renderSVGToImage("icons/svg/errorcross.svg", juce::Rectangle<int>(15, 15), cpl::GetColour(cpl::ColourEntry::error)));
		iconSucces.setOpaque(false);
		iconSucces.setAlpha(0);
		iconError.setOpaque(false);
		iconError.setAlpha(0);
		addAndMakeVisible(iconSucces);
		addAndMakeVisible(iconError);
		if (!parent)
		{
			throw std::runtime_error(
				std::string(__func__) + std::string(": Null pointer passed to constructor - trying to control non-existant control")
			);
		}
		//addAndMakeVisible(intValueLabel);
		setBounds(0, 0, compactWidth, compactHeight);
		addAndMakeVisible(fmtValueLabel);
		fmtValueLabel.setEditable(true);
		fmtValueLabel.setFont(CLookAndFeel_CPL::defaultLook().getStdFont());
/*		fmtValueLabel.setColour(fmtValueLabel.textColourId, colourAuxFont);
		fmtValueLabel.setColour(juce::TextEditor::focusedOutlineColourId, colourAux);
		fmtValueLabel.setColour(juce::TextEditor::outlineColourId, colourActivated);
		fmtValueLabel.setColour(juce::TextEditor::textColourId, colourAuxFont);
		fmtValueLabel.setColour(juce::TextEditor::highlightedTextColourId, colourSelFont);
		fmtValueLabel.setColour(CaretComponent::caretColourId, colourAux.brighter(0.2f));*/
		fmtValueLabel.addListener(this);

		//intValueLabel.setFont(cpl::systemFont); EDIT_TYPE_NEWFONTS
		intValueLabel.setEditable(true);
		intValueLabel.setFont(CLookAndFeel_CPL::defaultLook().getStdFont());
		/*intValueLabel.setColour(fmtValueLabel.textColourId, colourAuxFont);
		intValueLabel.setColour(juce::TextEditor::focusedOutlineColourId, colourAux);
		intValueLabel.setColour(juce::TextEditor::outlineColourId, colourActivated);
		intValueLabel.setColour(juce::TextEditor::textColourId, colourAuxFont);
		intValueLabel.setColour(juce::TextEditor::highlightedTextColourId, colourSelFont);
		intValueLabel.setColour(CaretComponent::caretColourId, colourAux.brighter(0.2f));*/
		intValueLabel.addListener(this);

		addChildComponent(intValueLabel);
		//setWantsKeyboardFocus(true);
		parentControl->bAddPassiveChangeListener(this);
		addAndMakeVisible(errorVisualizer);
		getAnimator().addChangeListener(this);
		addAndMakeVisible(expanderButton.get());
		expanderButton->addListener(this);

	}


	juce::String CCtrlEditSpace::bGetToolTip() const
	{
		return toolTip;
	}

	inline int getBorder(int size, int maxSize)
	{

	}

	void CCtrlEditSpace::paint(juce::Graphics & g)
	{

		if (!hasBeenInitialized)
		{
			hasBeenInitialized = true;
			createSimpleViewEditor();
		}

		g.fillAll(cpl::GetColour(cpl::ColourEntry::deactivated));
		g.setColour(cpl::GetColour(cpl::ColourEntry::separator));
		g.drawVerticalLine(getWidth() - (compactHeight - 1), 0.f, (float)getHeight() - 1);

		if (!compactMode)
		{
			g.setColour(cpl::GetColour(cpl::ColourEntry::separator));
			g.drawHorizontalLine(elementHeight, 0.f, getWidth() - 1.f);
			// draw title of control
			//g.setFont(cpl::systemFont); EDIT_TYPE_NEWFONTS
			g.setFont(TextSize::normalText);
			g.setColour(cpl::GetColour(cpl::ColourEntry::auxfont));
			auto titleRect = getBounds().withPosition(5, 1).withHeight(elementHeight);
			g.drawText("Editing ", titleRect.withRight(50), juce::Justification::centredLeft);
			g.setColour(cpl::GetColour(cpl::ColourEntry::selfont));
			g.drawText(typeid(*parentControl).name(), titleRect.withLeft(52), juce::Justification::centredLeft);

			// draw insides, starting with internal value
			auto elementPos = titleRect.withY(titleRect.getY() + elementHeight);
			g.setColour(cpl::GetColour(cpl::ColourEntry::auxfont));
			g.drawText("Semantic value:", elementPos, juce::Justification::centredLeft);

			g.drawText("Internal value:", elementPos.withY(cpl::Math::round<int>(elementPos.getY() + elementHeight * 1.5)), juce::Justification::centredLeft);
		}
		g.setColour(cpl::GetColour(cpl::ColourEntry::separator));
		g.drawRect(0, 0, getWidth(), getHeight());

	}

	void CCtrlEditSpace::resized()
	{

		expanderButton->setBounds
		(	
			centerRectInsideRegion
			(
				getBounds().withPosition(0, 0).withLeft(getWidth() - (compactHeight)).withBottom(compactHeight - 1),
				10,
				7.5
			)
		);

		if (!expanderButton->getToggleState())
		{
			fmtValueLabel.setBounds(1, 1, getWidth() - compactHeight, elementHeight);
			intValueLabel.setVisible(false);
		}
		else
		{
			auto badCodingStyleConst = elementHeight - 5;
			fmtValueLabel.setBounds(5, badCodingStyleConst + elementHeight, getWidth() - (compactHeight + 5), elementHeight);
			intValueLabel.setBounds(5, fmtValueLabel.getBounds().getY() + badCodingStyleConst * 2, getWidth() - (compactHeight + 5), elementHeight);


			iconError.setBounds(getWidth() - 20, intValueLabel.getBounds().getY() + 3, 15, 15);

			intValueLabel.setVisible(true);
			
		}
		errorVisualizer.setBounds(getBounds().withPosition(0, 0));
		fmtValueLabel.grabKeyboardFocus();
	}

	void CCtrlEditSpace::createSimpleViewEditor()
	{
		if (compactMode && isVisible())
		{
			fmtValueLabel.showEditor();
		}
	}
	void CCtrlEditSpace::editorShown(Label * curLabel, TextEditor & editor)
	{
		if (curLabel == &fmtValueLabel)
			editor.addListener(this);
		if (curLabel == &intValueLabel)
			editor.addListener(this);
	}
	void CCtrlEditSpace::animateSucces(juce::Component * objectThatWasModified)
	{
		if (getAnimator().isAnimating(&errorVisualizer))
		{
			// already animating this, try again in some time
			GUIUtils::FutureMainEvent(500, [&]() { animateSucces(objectThatWasModified); });
		}
		if (compactMode)
		{
			exitAfterAnimation = compactMode;
			inputValueWasValid = true;
			errorVisualizer.borderColour = cpl::GetColour(cpl::ColourEntry::success);
			errorVisualizer.borderSize = 4.f;
			errorVisualizer.setAlpha(1.f);
			errorVisualizer.isActive = true;
			errorVisualizer.repaint();

			getAnimator().animateComponent(&errorVisualizer, errorVisualizer.getBounds(), 0.f, 300, false, 1.0, 1.0);
		}
		else
		{
			inputValueWasValid = true;
			iconSucces.setAlpha(1.f);
			iconSucces.setBounds(getWidth() - 20, objectThatWasModified->getBounds().getY() + 3, 15, 15);
			getAnimator().animateComponent(&iconSucces, iconSucces.getBounds(), 0.f, 300, false, 1.0, 1.0);
		}
	}

	void CCtrlEditSpace::buttonClicked(juce::Button * b)
	{
		if (b == expanderButton.get())
		{
			compactMode = !expanderButton->getToggleState();
			setMode(compactMode);
		}
		fmtValueLabel.showEditor();
		repaint();
	}


	void CCtrlEditSpace::animateError(juce::Component * objectThatWasModified)
	{
		if (getAnimator().isAnimating(&errorVisualizer))
		{
			// already animating this, try again in some time
			GUIUtils::FutureMainEvent(500, [&]() { animateError(objectThatWasModified); });
		}
		if (compactMode)
		{
			exitAfterAnimation = inputValueWasValid = false;
			errorVisualizer.borderColour = cpl::GetColour(ColourEntry::error);
			errorVisualizer.borderSize = 4.f;
			errorVisualizer.setAlpha(1.f);
			errorVisualizer.isActive = true;
			errorVisualizer.repaint();

			getAnimator().animateComponent(&errorVisualizer, errorVisualizer.getBounds(), 0.f, 300, false, 1.0, 1.0);
		}
		else
		{
			inputValueWasValid = true;
			iconError.setAlpha(1.f);
			iconError.setBounds(getWidth() - 20, objectThatWasModified->getBounds().getY() + 3, 15, 15);
			getAnimator().animateComponent(&iconError, iconError.getBounds(), 0.f, 300, false, 1.0, 1.0);
		}
	}

	void CCtrlEditSpace::textEditorReturnKeyPressed(TextEditor & editor)
	{
		if (&editor == fmtValueLabel.getCurrentTextEditor())
		{
			// this is where we try to interpret a formatted value
			// to an internal range.

			// note: we use the value from the editor, because the label may not
			// have been updated yet.
			if (inputValueWasValid = interpretAndSet(editor.getText().toStdString()))
			{
				animateSucces(&fmtValueLabel);
			}
			else
				animateError(&fmtValueLabel);
		}
		else if (&editor == intValueLabel.getCurrentTextEditor())
		{
			// here we try to map an input string to [0, 1] range.
			// the cbasecontrol provides a static method for this
			iCtrlPrec_t val(0);
			auto succes = CBaseControl::bMapStringToInternal(editor.getText().toStdString(), val);


			if (succes)
			{
				parentControl->bSetValue(val);
				animateSucces(&intValueLabel);
			}
			else
				animateError(&intValueLabel);

		}
	}
	iCtrlPrec_t CCtrlEditSpace::interpretString(const std::string & value)
	{
		iCtrlPrec_t ret(0);
		parentControl->bInterpret(value, ret);
		return ret;
	}
	bool CCtrlEditSpace::interpretAndSet(const std::string & value)
	{
		return parentControl->bInterpretAndSet(value, false);
	}
	std::string CCtrlEditSpace::getStringFrom(iCtrlPrec_t value)
	{
		std::string ret;
		if (parentControl->bFormatValue(ret, cpl::Math::confineTo<iCtrlPrec_t>(value, 0.0, 1.0)))
			return ret;
		else
			return "<error>";
	}
	std::string CCtrlEditSpace::getValueString()
	{
		return getStringFrom(parentControl->bGetValue());
	}
	void CCtrlEditSpace::setInternal(iCtrlPrec_t value)
	{
		parentControl->bSetInternal(cpl::Math::confineTo<iCtrlPrec_t>(value, 0.0, 1.0));
	}
	iCtrlPrec_t CCtrlEditSpace::getValue()
	{
		return parentControl->bGetValue();
	}



	void CCtrlEditSpace::visibilityChanged()
	{

		fmtValueLabel.showEditor();

		resetToControl();

	}

	void CCtrlEditSpace::resetToControl()
	{
		std::string fmtText, intText;
		auto val = parentControl->bGetValue();
		CBaseControl::bMapIntValueToString(intText, val);
		parentControl->bFormatValue(fmtText, val);
		
		fmtValueLabel.setText(fmtText, juce::NotificationType::dontSendNotification);
		intValueLabel.setText(intText, juce::NotificationType::dontSendNotification);
	}

	bool CCtrlEditSpace::grabFocus()
	{
		fmtValueLabel.showEditor();
		fmtValueLabel.grabKeyboardFocus();
		return true;
	}

	void CCtrlEditSpace::looseFocus()
	{
		
		unfocusAllComponents();
	}

	void CCtrlEditSpace::setMaximumSize(int width, int height)
	{
		maximumSize.setXY(width, height);
	}
	void CCtrlEditSpace::setMode(bool shouldBeCompact)
	{
		if (shouldBeCompact)
		{
			compactMode = true;
			setSize(compactWidth, compactHeight);
		}
		else
		{
			compactMode = false;
			setSize(fullWidth, fullHeight);
		}

	}
	juce::String CCtrlEditSpace::bGetToolTipForChild(const Component * c) const
	{
		if (GUIUtils::ViewContains(&intValueLabel, c))
		{
			return "The internal 0.0 to 1.0 range of all controls.";
		}
		else if (GUIUtils::ViewContains(&fmtValueLabel, c))
		{
			return "What the internal value represents.";
		}
		return juce::String::empty;
	}
	void CCtrlEditSpace::onObjectDestruction(const CBaseControl::ObjectProxy & ctrl)
	{

		if (ctrl == parentControl)
		{
			// if the control ceases to exists, we have no meaningful existance.
			// attempt suicide here.
			parentControl = nullptr;
			delete this;
		}
	}

	CCtrlEditSpace::~CCtrlEditSpace() 
	{
		// warning - the control may have been deleted at this point
		// if so, the callback on onDeathDestruction sets the parentControl pointer to zero.
		if(parentControl)
			parentControl->bRemovePassiveChangeListener(this);

		getAnimator().removeChangeListener(this);
	}
	CBaseControl * CCtrlEditSpace::getBaseControl()
	{
		return parentControl;
	}
	void CCtrlEditSpace::labelTextChanged(juce::Label *labelThatHasChanged)
	{
		if (labelThatHasChanged == &fmtValueLabel)
		{
			auto const & editText = fmtValueLabel.getText();
			if (editText.length())
				interpretAndSet(editText.toStdString());

		}
	}

	void CCtrlEditSpace::changeListenerCallback(ChangeBroadcaster *source)
	{
		auto & animator = juce::Desktop::getInstance().getAnimator();
		if (source == &animator)
		{
			if (!animator.isAnimating(&errorVisualizer))
			{
				// animation finished, hide component
				errorVisualizer.isActive = false;
				if (inputValueWasValid && exitAfterAnimation)
					delete this;

			}
		}
	}
};