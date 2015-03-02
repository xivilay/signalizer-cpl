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

#ifndef _COMPONENTCONTAINERS_H
	#define _COMPONENTCONTAINERS_H

	#include "GraphicComponents.h"
	#include <vector>

	namespace cpl
	{
		/*********************************************************************************************
	 
			CButtonGroup - holds a set of buttons that can be toggled to radio functionality.
			The templated argument is the type of button it holds. Must derive from cpl::CBaseControl
	 
		 *********************************************************************************************/
		template<class Button>
			class CButtonGroup
			:
				public juce::GroupComponent,
				public cpl::CBaseControl,
				protected cpl::CBaseControl::Listener
			{

			public:
				// types
				enum Behaviour
				{
					radio = 2,
					mustBeToggled = 2,
					polyToggle = 4
				};
			
				typedef Button buttonType;
			
				class CMultiButtonCallback
				{
				public:
					virtual void buttonSelected(cpl::CBaseControl * c, int index) = 0;
					virtual void buttonDeselected(cpl::CBaseControl * c, int index) = 0;
					virtual ~CMultiButtonCallback() {};
				};

				// construction
				CButtonGroup(const std::vector<std::string> & names, CMultiButtonCallback * cb = nullptr, int behaviour = radio);
				~CButtonGroup();
				// methods
				std::string getButtonName(int index);
				buttonType & getButton(int index);
				void untoggle(bool notify = true, int index = -1);
				void toggleIndex(unsigned index, bool notify = true);
				void suggestSize(int length, int height);
				int floatToIndex(float val);
				float indexToFloat(int index);
				// overrides
				void bSetValue(iCtrlPrec_t val) override;
				void bSetInternal(iCtrlPrec_t val) override;
				iCtrlPrec_t bGetValue() const override;
				int getToggledIndex();
				std::size_t getNumButtons();
			private:

				int type;
				float value;
				signed int toggledIndex;
				std::vector<buttonType *> buttons;
				CMultiButtonCallback * cb;
			
				void buttonClicked(juce::Button * b) override;
				bool valueChanged(CBaseControl* button) override;
				void indexChanged(int index);
			};
	
		/*********************************************************************************************
	 
			class CControlContainer - base class for containers. Will resize its host based on it's 
			orientation and holds references to added controls.
	 
		 *********************************************************************************************/
		class CControlContainer
		{
		
		public:
		
			enum Orientation
			{
				top, bottom, left, right
			
			};
		
			struct ControlRef
			{
				CBaseControl * ref;
				bool owned;
				ControlRef(CBaseControl * r, bool o) : ref(r), owned(o) {}
			};
		
			CControlContainer(juce::Component * viewToControl);
			virtual ~CControlContainer();		

			virtual void setOrientation(Orientation newOrientation);
			void setNested(bool isNested);
			void expand();
			virtual void addControl(CBaseControl * newControl, bool takeOwnership = false);
		
		protected:
		
			std::vector<ControlRef> controls;
			bool isVertical; // expands vertically or horizontally
			bool isNested; // adds a larger offset from top to fit better inside other grouped controls.
			int stdWidth, stdHeight;
			juce::Component * base;
			juce::Justification just; // ignore for now, always left/top-aligned
			Orientation ort;
		};

		class CControlGroup : public juce::GroupComponent, public CBaseControl, public CControlContainer
		{

		public:

			CControlGroup()
				: GroupComponent("KnobGroup"), CBaseControl(this), CControlContainer(this)
			{
				setSize(100, 100);
				setColour(outlineColourId, juce::Colours::orange.withMultipliedBrightness(0.5f));
				setColour(textColourId, juce::Colours::orange.withMultipliedBrightness(0.7f));
				//setText("Knobs");
			}
		};

		class CControlPanel : public juce::Component,  public CBaseControl, public CControlContainer
		{
		public:

			CControlPanel();
		
			void setName(std::string name);
			int getPanelOffset();
			void resizeAccordingly();
		
			void setOrientation(CControlContainer::Orientation newOrientation) override;
			void onValueChange() override;
			virtual void addControl(CBaseControl * newControl, bool takeOwnership = false) override;
			void resized() override;
			void paint(juce::Graphics & g) override;
			iCtrlPrec_t bGetValue() const override;
			void bSetValue(iCtrlPrec_t param, bool sync = false) override;
		private:

			class TriangleButton : public juce::Button
			{
				friend class CControlPanel;
			public:
				void clicked() override;
				void flip();
				void setSwitch(bool bOn);
				TriangleButton(juce::Colour colour);
				void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;
			private:
				juce::Colour baseColour;
				bool orientation;
			};

			TriangleButton accessor;
			int oldHeight;
			bool collapsed;
			juce::String name;

		};
	
	};
	/*********************************************************************************************

		Inline templates

	*********************************************************************************************/
	#include "ComponentContainers.inl"

#endif