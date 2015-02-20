/*************************************************************************************
 
 cpl - cross-platform library - v. 0.1.0.
 
 Copyright (C) 2015 Janus Lynggaard Thorborg [LightBridge Studios]
 
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
 
 file:ComponentContainers.inl
 
	Source code for componentcontainers.h (templated inlines)
 
 *************************************************************************************/
#ifdef _COMPONENTCONTAINERS_H

	namespace cpl
	{
		/*********************************************************************************************

		class CButtonGroup

		*********************************************************************************************/
		template<class Button>
		void CButtonGroup<Button>::buttonClicked(juce::Button * b)
		{
			return;
			if (buttonType * button = dynamic_cast<buttonType *>(b))
			{
				for (unsigned i = 0; i < buttons.size(); ++i)
					if (button == buttons[i])
					{
						indexChanged(i);
						break;
					}

			}
			else
			{
				// templated argument not derived from juce::Button and cpl::CBaseControl
				throw std::runtime_error("CMultiButton::buttonClicked argument not derived from juce::Buttom or cpl::CBaseControl; bad user code");
			}
		}
		/*********************************************************************************************/
		template<class Button>
		bool CButtonGroup<Button>::valueChanged(CBaseControl* button)
		{

			for (int i = 0; i < buttons.size(); ++i)
			{
				if (button == buttons[i])
				{
					if (type & radio)
					{
						value = indexToFloat(i);
						//toggleIndex(i);
						if (i != toggledIndex && cb) // only send off event if a button is deselected
							cb->buttonDeselected(this, toggledIndex);
						if (i != toggledIndex && cb)
							cb->buttonSelected(this, i);
						toggledIndex = i;

					}
					else
					{
						bool isOn = button->bGetValue() > 0.5f;
						if (cb)
						{
							if (isOn)
								cb->buttonSelected(this, i);
							else
								cb->buttonDeselected(this, i);
						}
					}
					break;
				}
			}

			return false;

		}
		/*********************************************************************************************/
		template<class Button>
		void CButtonGroup<Button>::indexChanged(int index)
		{

			if (!(type & polyToggle))
			{
				untoggle(true, toggledIndex);
				value = indexToFloat(index);

			}
			auto & button = buttons.at(index);
			button->setEnabled(false);
			toggledIndex = index;

		}
		/*********************************************************************************************/
		template<class Button>
		CButtonGroup<Button>::CButtonGroup(const std::vector<std::string> & names, CMultiButtonCallback * cb, int behaviour)
			: GroupComponent("", ""), CBaseControl(this), value(0), toggledIndex(-1), type(behaviour), cb(cb)
		{
			setColour(outlineColourId, juce::Colours::orange.withMultipliedBrightness(0.5));
			setColour(textColourId, juce::Colours::orange.withMultipliedBrightness(0.7));
			auto size = (int)names.size();
			setSize(100, 25 + size * 17);
			for (int i = 0; i < size; ++i)
			{
				int block = (getHeight() - 25) / names.size();
				int xpoint = block * i + 17;
				auto button = new buttonType(names[i]);
				button->setBounds(10, xpoint, getWidth() - 20, 17);
				button->bSetListener(this);
				button->setToggleable(true);
				buttons.push_back(button);
				addAndMakeVisible(button);
			}

			if (type & radio)
			{
				for (auto button : buttons)
				{
					button->setRadioGroupId(1, juce::NotificationType::dontSendNotification);

				}
			}
			//toggleIndex(0);
		}
		/*********************************************************************************************/
		template<class Button>
		CButtonGroup<Button>::~CButtonGroup()
		{
			for (auto & b : buttons)
				delete b;
		}
		/*********************************************************************************************/
		template<class Button>
		std::string CButtonGroup<Button>::getButtonName(int index)
		{
			return buttons.at(index)->bGetTitle();
		}
		/*********************************************************************************************/
		template<class Button>
		std::size_t CButtonGroup<Button>::getNumButtons()
		{
			return buttons.size();
		}
		/*********************************************************************************************/
		template<class Button>
		typename CButtonGroup<Button>::buttonType & CButtonGroup<Button>::getButton(int index)
		{
			return *buttons.at(index);
		}
		/*********************************************************************************************/
		template<class Button>
		void CButtonGroup<Button>::untoggle(bool notify, int index)
		{
			if (index < 0 && toggledIndex < 0)
				return;
			auto actualIndex = type & polyToggle || index != -1 ? index : toggledIndex;
			auto & button = buttons.at(actualIndex);
			notify ? button->bSetValue(0) : button->bSetInternal(0);
		}
		/*********************************************************************************************/
		template<class Button>
		void CButtonGroup<Button>::toggleIndex(unsigned index, bool notify)
		{
			if (index == toggledIndex && buttons.at(index)->bGetValue() > 0.f)
				return;
			/*if (!(type & polyToggle)) // old radio behaviour
			{
				untoggle(notify);
				value = indexToFloat(index);

			}*/
			auto & button = buttons.at(index);
			notify ? button->bSetValue(1) : button->bSetInternal(1);
			toggledIndex = index;

		}
		/*********************************************************************************************/
		template<class Button>
		iCtrlPrec_t CButtonGroup<Button>::bGetValue() const
		{
			return value;
		}
		template<class Button>
		int CButtonGroup<Button>::getToggledIndex()
		{
			return toggledIndex;
		}
		/*********************************************************************************************/
		template<class Button>
		int CButtonGroup<Button>::floatToIndex(float val)
		{
			// factors works with ones, offsets are zero indexed though.
			return cpl::Math::round<int>(val * ((buttons.size() ? buttons.size() : 1) - 1));
		}
		/*********************************************************************************************/
		template<class Button>
		float CButtonGroup<Button>::indexToFloat(int index)
		{

			return buttons.size() ? (float)(index) / (buttons.size() - 1) : 0.0f;
		}
		/*********************************************************************************************/
		template<class Button>
		void CButtonGroup<Button>::bSetValue(iCtrlPrec_t val)
		{

			return toggleIndex(floatToIndex(val));
		}
		/*********************************************************************************************/
		template<class Button>
		void CButtonGroup<Button>::bSetInternal(iCtrlPrec_t val)
		{
			return toggleIndex(floatToIndex(val), false);
		}
	};

#endif