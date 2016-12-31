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
 
	file:CPowerSlopeWidget.h
 
		A widget that can design a power function used for dsp slopes.
 
*************************************************************************************/

#ifndef CPL_CPOWERSLOPEWIDGET_H
	#define CPL_CPOWERSLOPEWIDGET_H
	#include "WidgetBase.h"
	#include "../../rendering/Graphics.h"

	namespace cpl
	{

		class CPowerSlopeWidget
		: 
			public juce::Component,
			public ValueControl<PowerSlopeValue, CompletePowerSlopeValue>
		{

		public:

			CPowerSlopeWidget(PowerSlopeValue * value = nullptr, bool takeOwnership = false);

		protected:

			virtual void onControlSerialization(CSerializer::Archiver & ar, Version version) override;
			virtual void onControlDeserialization(CSerializer::Builder & ar, Version version) override;

			void initUI();

			cpl::CValueKnobSlider kbase, kpivot, kslope;
			cpl::MatrixSection layout;
		};
	};
#endif