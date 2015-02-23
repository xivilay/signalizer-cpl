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
 
 file:ComponentContainers.cpp
 
	Source code for componentcontainers.h
 
 *************************************************************************************/

#include "CValueControl.h"
#include "../Mathext.h"

namespace cpl
{
	auto elementHeight = 15;
	auto elementWidth = 50;

	CTransformWidget::CTransformWidget()
		: CBaseControl(this), transform(1.0f)
	{

		bSetDescription("Controls an objects transformation in a visual 3D space.");
		enableTooltip(true);

		for (auto & type : labels)
		{
			for (auto & label : type)
			{
				label.addListener(this);
				label.setSelectAllWhenFocused(true);
				label.setText("1", true);
				label.setColour(TextEditor::backgroundColourId, GetColour(ColourEntry::deactivated));
				label.setColour(TextEditor::outlineColourId, GetColour(ColourEntry::separator));
				label.setColour(TextEditor::textColourId, GetColour(ColourEntry::auxfont));
				label.setScrollToShowCursor(false);
				addAndMakeVisible(label);
			}
		}
		syncEditor();
		setSize((elementWidth + 15) * 3 , elementHeight * 6);

		setViewport(getBounds());
	}

	void CTransformWidget::syncEditor()
	{
		char textBuf[200];
		for (unsigned x = 0; x < 3; ++x)
		{
			for (unsigned y = 0; y < 3; ++y)
			{
				sprintf_s(textBuf, "%.2f", transform.element(x, y));
				labels[x][y].setText(textBuf, false);
			}
		}
	}

	juce::String CTransformWidget::bGetToolTipForChild(const Component * c) const
	{
		juce::String ret = "Set the objects ";
		const char * params[] = { "position (where {0, 0, 0} is the center, and {1, 1, 1} is upper right back corner)", 
								  "rotation (in degrees)", "scale (where 1 = identity)" };
		const char * property[] = { "x-", "y-", "z-" };
		for (unsigned y = 0; y < 3; ++y)
		{

			for (unsigned x = 0; x < 3; ++x)
			{
				if (c == &labels[y][x])
				{
					ret += property[x];
					ret += params[y];
					return ret;
				}
			}
		}
		return juce::String::empty;
	}

	void CTransformWidget::inputCommand(int x, int y, const String & data)
	{
		double input;
		char * endPtr = nullptr;
		input = strtod(data.getCharPointer(), &endPtr);
		if (endPtr > data.getCharPointer())
		{
			transform.element(x, y) = (float)input;
			bForceEvent();
		}
	}

	void CTransformWidget::textEditorTextChanged(juce::TextEditor & t)
	{

	}


	void CTransformWidget::textEditorFocusLost(TextEditor & t)
	{
		for (unsigned x = 0; x < 3; ++x)
		{
			for (unsigned y = 0; y < 3; ++y)
			{
				if (&t == &labels[x][y])
				{
					inputCommand(x, y, t.getText());
					char textBuf[200];
					sprintf_s(textBuf, "%.2f", transform.element(x, y));
					t.setText(textBuf, false);
					break;
				}
			}
		}
	}
	void CTransformWidget::textEditorReturnKeyPressed(TextEditor & t)
	{
		t.unfocusAllComponents();
	}

	void CTransformWidget::mouseDown(const juce::MouseEvent & e)
	{
		if (e.eventComponent == this)
		{
			unfocusAllComponents();
		}
		Draggable3DOrientation::mouseDown(e.position);
	}

	void CTransformWidget::mouseDrag(const juce::MouseEvent & e)
	{
		Draggable3DOrientation::mouseDrag(e.position);

		auto & quat = getQuaternion();

		transform.rotation.x = quat.vector.x * 90;
		transform.rotation.y = quat.vector.y * 90;
		transform.rotation.z = quat.vector.z * 90;

		syncEditor();
	}

	void CTransformWidget::resized()
	{

		for (unsigned x = 0; x < 3; ++x)
		{
			for (unsigned y = 0; y < 3; ++y)
			{
				labels[y][x].setBounds(
					10 + x * (elementWidth + 15), 
					elementHeight + y * (elementHeight * 2), 
					elementWidth,
					elementHeight
				);
			}
		}
	}
	void CTransformWidget::paint(juce::Graphics & g)
	{

		const char * texts[] = { "x:", "y:", "z:" };
		const char * titles[] = { " - Position - ", " - Rotation - ", " - Scale - " };

		g.setFont(TextSize::normalText);
		g.setColour(GetColour(ColourEntry::auxfont));
		for (unsigned y = 0; y < 3; ++y)
		{
			g.drawText(titles[y], 0, y * (elementHeight * 2), getWidth(), elementHeight - 1, juce::Justification::centred);
		}
		g.setColour(GetColour(ColourEntry::auxfont));
		g.setFont(TextSize::smallText);
		for (unsigned y = 0; y < 3; ++y)
		{
			for (unsigned x = 0; x < 3; ++x)
			{

				g.drawText(texts[x], x * (elementWidth + 15), elementHeight + y * (elementHeight * 2), 10, elementHeight, juce::Justification::centred);
			}
		}
	}

};
