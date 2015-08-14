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

	file:CColourControl.h

		A widget that can display a colour and allow the user to choose a new.
		Note the explicit use of juce::PixelARGB - avoid representing colours
		through integers (platforms has binary inconsistensies).
 
*************************************************************************************/

#ifndef _CPRESETWIDGET_H
	#define _CPRESETWIDGET_H

	#include "../common.h"
	#include "ControlBase.h"
	#include "CButton.h"
	#include "CComboBox.h"

	namespace cpl
	{

		class CPresetWidget
		:
			public juce::Component,
			public CBaseControl,
			protected CBaseControl::PassiveListener
		{
		public:

			CPresetWidget(CSerializer::Serializable * contentToBeSerialized, const std::string & uniqueName);

			// overrides
			virtual void valueChanged(const cpl::CBaseControl * c) override;
			virtual void onValueChange() override;
			virtual void onObjectDestruction(const ObjectProxy & object) override;
			// api
			const std::string & getName() const noexcept;
			bool setSelectedPreset(juce::File location);
			const std::vector<std::string> & getPresets();

			void updatePresetList();

		protected:
			std::string presetWithoutExtension(juce::File preset);
			std::string fullPathToPreset(const std::string &);
			void initControls();

			// data

			CButton kloadPreset, ksavePreset;
			CComboBox kpresetList;
			MatrixSection layout;
			CSerializer::Serializable * parent;
			std::string name;
			std::string ext;
		};

	};

#endif