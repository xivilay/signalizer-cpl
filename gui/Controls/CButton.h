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
 
	file:CButton.h
 
		A simple button that shows some text. Has ability to be 'sticky'.
 
*************************************************************************************/

#ifndef CPL_CBUTTON_H
	#define CPL_CBUTTON_H

	#include "ControlBase.h"
	#include <string>

	namespace cpl
	{

		class CButton : public juce::Button, public CBaseControl
		{
			juce::String texts[2];
			bool toggle;
		public:
			CButton(const std::string & text, const std::string & textToggled = "");
			CButton();
			~CButton();

			void setToggleable(bool isAble);
			void bSetInternal(iCtrlPrec_t newValue) override;
			void bSetValue(iCtrlPrec_t newValue, bool sync = false) override;
			iCtrlPrec_t bGetValue() const override;
			void setUntoggledText(const std::string &);
			void setToggledText(const std::string &);
			std::string bGetTitle() const override;
			void bSetTitle(const std::string &) override;

		private:
			void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown);

		};
	};
#endif