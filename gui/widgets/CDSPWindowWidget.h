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

#ifndef CDSPWINDOWWIDGET_H
#define CDSPWINDOWWIDGET_H

#include "WidgetBase.h"
#include "../../dsp/DSPWindows.h"

namespace cpl
{

	class CDSPWindowWidget
		: public juce::Component
		, public ValueControl<WindowDesignValue, CompleteWindowDesign>
	{
	public:

		enum ChoiceOptions
		{
			All,
			FiniteDFTWindows
		};

		/// <summary>
		/// 
		/// </summary>
		CDSPWindowWidget(WindowDesignValue * value = nullptr, bool takeOwnership = false);

		void setWindowOptions(ChoiceOptions c);


	protected:

		virtual void onValueObjectChange(ValueEntityListener * sender, ValueEntityBase * value) override;

		virtual void onControlSerialization(CSerializer::Archiver & ar, Version version) override;
		virtual void onControlDeserialization(CSerializer::Builder & ar, Version version) override;

		class WindowAnalyzer : public juce::Component
		{
		public:

			WindowAnalyzer(CDSPWindowWidget & parent);
			void paint(juce::Graphics & g) override;

		private:
			CDSPWindowWidget & p;
		};

		virtual void resized() override;
		void initControls();

		// data
		CValueComboBox kwindowList;
		CValueComboBox ksymmetryList;
		CValueKnobSlider kalpha, kbeta;
		MatrixSection layout;
		WindowAnalyzer analyzer;

	};

};

#endif