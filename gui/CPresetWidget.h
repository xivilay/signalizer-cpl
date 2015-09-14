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

			enum Setup
			{
				/// <summary>
				/// Only has a load/save preset buttons.
				/// </summary>
				Minimal = 0x1,
				/// <summary>
				/// In addition to minimal, has load/save default presets.
				/// </summary>
				WithDefault = 0x2
			};

			/// <summary>
			/// 
			/// </summary>
			/// <param name="contentToBeSerialized">
			/// The object to be changed when the user interacts with the widget
			/// </param>
			/// <param name="uniqueName">
			/// The unique name/ID that identifies the parent. This will be a part of the filename and file,
			/// ensures only this name can load presets saved with that name.
			/// </param>
			CPresetWidget(CSerializer::Serializable * contentToBeSerialized, const std::string & uniqueName, Setup s = Minimal);

			// overrides
			virtual void valueChanged(const cpl::CBaseControl * c) override;
			virtual void onValueChange() override;
			virtual void onObjectDestruction(const ObjectProxy & object) override;
			// api
			const std::string & getName() const noexcept;
			/// <summary>
			/// Tries to apply a preset from a file.
			/// </summary>
			/// <param name="location"></param>
			/// <returns></returns>
			bool setSelectedPreset(juce::File location);
			const std::vector<std::string> & getPresets();
			/// <summary>
			/// Tries to load the default preset, if any. Also fails if WithDefault isn't set.
			/// </summary>
			/// <returns></returns>
			bool loadDefaultPreset();
			void updatePresetList();

		protected:
			std::string presetWithoutExtension(juce::File preset);
			std::string fullPathToPreset(const std::string &);
			void setDisplayedPreset(juce::File location);


			void initControls();

			// data

			CButton kloadPreset, ksavePreset, kloadDefault, ksaveDefault;
			CComboBox kpresetList;
			MatrixSection layout;
			CSerializer::Serializable * parent;
			std::string name;
			std::string ext;
			Setup layoutSetup;
		};

	};

#endif