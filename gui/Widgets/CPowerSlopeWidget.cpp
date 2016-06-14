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

	static const double 
		kMinDbs = -32, 
		kMaxDbs = 32,
		kBaseMin = 2,
		kBaseMax = 10,
		kPivotMin = 10,
		kPivotMax = 20000;

	CPowerSlopeWidget::CPowerSlopeWidget()
		: CBaseControl(this)
		, transformedBase(0)
		, transformedPivot(0)
		, transformedSlope(0)
		, a(0)
		, b(0)
	{
		enableTooltip(true);
		addAndMakeVisible(layout);
		initUI();
		bSetIsDefaultResettable(true);
	}

	CPowerSlopeWidget::PowerFunction CPowerSlopeWidget::derive()
	{
		return{ a.load(std::memory_order_acquire), b.load(std::memory_order_acquire) };
	}


	void CPowerSlopeWidget::valueChanged(const CBaseControl * ctrl)
	{
		auto value = ctrl->bGetValue();

		if (ctrl == &kbase)
		{
			transformedBase = Math::UnityScale::linear(value, kBaseMin, kBaseMax);
		}
		else if (ctrl == &kpivot)
		{
			transformedPivot = Math::UnityScale::exp(value, kPivotMin, kPivotMax);
		}
		else if (ctrl == &kslope)
		{
			transformedSlope = std::pow(10, Math::UnityScale::linear(value, kMinDbs, kMaxDbs) / 20);
		}

		a.store(std::log(transformedSlope) / std::log(transformedBase), std::memory_order_release);
		b.store(1.0 / std::pow(transformedPivot, a.load(std::memory_order_relaxed)), std::memory_order_release);

		bForceEvent();
	}

	bool CPowerSlopeWidget::stringToValue(const CBaseControl * ctrl, const std::string & buffer, iCtrlPrec_t & value)
	{
		iCtrlPrec_t interpretedValue;

		if (ctrl == &kbase)
		{
			if (lexicalConversion(buffer, interpretedValue))
			{
				interpretedValue = Math::UnityScale::Inv::linear(interpretedValue, kBaseMin, kBaseMax);
				value = Math::confineTo(interpretedValue, 0, 1);
				return true;
			}
		}
		else if (ctrl == &kpivot)
		{
			if (lexicalConversion(buffer, interpretedValue))
			{
				interpretedValue = Math::UnityScale::Inv::exp(interpretedValue, kPivotMin, kPivotMax);
				value = Math::confineTo(interpretedValue, 0, 1);
				return true;
			}
		}
		else if (ctrl == &kslope)
		{
			if (lexicalConversion(buffer, interpretedValue))
			{
				interpretedValue = Math::UnityScale::Inv::linear(interpretedValue, kMinDbs, kMaxDbs);
				value = Math::confineTo(interpretedValue, 0, 1);
				return true;
			}
		}

		return false;
	}

	bool CPowerSlopeWidget::valueToString(const CBaseControl * ctrl, std::string & buffer, iCtrlPrec_t value)
	{
		char buf[100];

		if (ctrl == &kbase)
		{
			sprintf_s(buf, "%.2f", Math::UnityScale::linear(value, kBaseMin, kBaseMax));
		}
		else if (ctrl == &kpivot)
		{
			sprintf_s(buf, "%.2f", Math::UnityScale::exp(value, kPivotMin, kPivotMax));
		}
		else if (ctrl == &kslope)
		{
			sprintf_s(buf, "%.2f dB", Math::UnityScale::linear(value, kMinDbs, kMaxDbs));
		}

		buffer = buf;
		return true;
	}

	void CPowerSlopeWidget::onObjectDestruction(const ObjectProxy & destroyedObject)
	{

	}

	void CPowerSlopeWidget::paint(juce::Graphics & g)
	{

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

		for (auto c : { &kbase, &kslope, &kpivot })
		{
			c->bAddChangeListener(this);
			c->bAddFormatter(this);
		}

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
