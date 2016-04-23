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
			public CBaseControl,
			public juce::Component,
			public CBaseControl::PassiveListener,
			public CBaseControl::ValueFormatter
		{

		public:

			struct PowerFunction
			{
				double a, b;
			};

			CPowerSlopeWidget();

			/// <summary>
			/// Safe and wait-free from any thread
			/// </summary>
			/// <returns></returns>
			PowerFunction derive();
			void paint(juce::Graphics & g) override;

			virtual void valueChanged(const CBaseControl * ctrl) override;
			virtual void onObjectDestruction(const ObjectProxy & destroyedObject) override;
			virtual bool stringToValue(const CBaseControl * ctrl, const std::string & buffer, iCtrlPrec_t & value) override;
			virtual bool valueToString(const CBaseControl * ctrl, std::string & buffer, iCtrlPrec_t value) override;


		protected:

			virtual void onControlSerialization(CSerializer::Archiver & ar, Version version) override;
			virtual void onControlDeserialization(CSerializer::Builder & ar, Version version) override;

			void initUI();

			std::atomic<iCtrlPrec_t> a, b;
			double transformedBase, transformedPivot, transformedSlope;
			cpl::CKnobSlider kbase, kpivot, kslope;
			cpl::MatrixSection layout;
		};
	};
#endif