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
 
	file:CPowerSlopeWidget.cpp
		
		Implementation of CPowerSlopeWidget.h
	
*************************************************************************************/

#include "CPowerSlopeWidget.h"
#include "../../LexicalConversion.h"

namespace cpl
{
	CPowerSlopeWidget::CPowerSlopeWidget(PowerSlopeValue * value, bool takeOwnership)
		: ValueControl<PowerSlopeValue, CompletePowerSlopeValue>(this, value, takeOwnership)
		, kbase(&getValueReference().getValueIndex(PowerSlopeValue::Index::Base), false)
		, kslope(&getValueReference().getValueIndex(PowerSlopeValue::Index::Slope), false)
		, kpivot(&getValueReference().getValueIndex(PowerSlopeValue::Index::Pivot), false)
	{
		enableTooltip(true);
		addAndMakeVisible(layout);
		initUI();
		bSetIsDefaultResettable(true);
	}


	void CPowerSlopeWidget::onControlSerialization(CSerializer::Archiver & ar, Version version)
	{
		ar << kbase;
		ar << kslope;
		ar << kpivot;
	}

	void CPowerSlopeWidget::onControlDeserialization(CSerializer::Builder & ar, Version version)
	{
		ar >> kbase;
		ar >> kslope;
		ar >> kpivot;
	}

	void CPowerSlopeWidget::initUI()
	{
		kbase.bSetTitle("Slope base");
		kslope.bSetTitle("Slope value");
		kpivot.bSetTitle("Slope pivot");

		kbase.bSetDescription("The base (or distance) from the start where the function equals the slope; common values are 2 for octaves, or 10 for decades");
		kslope.bSetDescription("A scale for the value of the function after base * pivot progress");
		kpivot.bSetDescription("The center of the power function, where the function equals 1");

		bSetDescription("A widget that can design a DSP power slope function in the form of y = b * a^x"); 

		layout.addControl(&kbase, 0);
		layout.addControl(&kslope, 1);
		layout.addControl(&kpivot, 0);

		auto && size = layout.getSuggestedSize();
		setSize(size.first, size.second);

		// default values.
		kbase.bInterpretAndSet("2", true);
		kslope.bInterpretAndSet("0", true);
		kpivot.bInterpretAndSet("1000", true);

		kbase.bForceEvent();
		kslope.bForceEvent();
		kpivot.bForceEvent();
	}

};
