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

#ifndef _CCOLOURCONTROL_H
	#define _CCOLOURCONTROL_H

	#include "ControlBase.h"

	namespace cpl
	{
		/*********************************************************************************************

			CColorKnob - an automatable knob which can display a color

		*********************************************************************************************/
		class CColourControl 
		:
			public CKnobSlider
		{

		public:
			enum ColourType
			{
				RGB,
				ARGB,
				GreyTone,
				SingleChannel

			};



		public:
			CColourControl(const std::string & name = "", ColourType typeToUse = ColourType::RGB, int channel = 0);

			ColourType getType() const;
			void setType(ColourType);

			int getChannel() const;
			void setChannel(int channel);

			// overrides
			virtual void onValueChange() override;
			virtual void save(CSerializer::Archiver & ar, long long int version) override;
			virtual void load(CSerializer::Builder & ar, long long int version) override;
			virtual void paint(juce::Graphics & g) override;
			//virtual iCtrlPrec_t bGetValue() const override;
			//virtual void bSetValue(iCtrlPrec_t val) override;
			virtual bool bStringToValue(const std::string &, iCtrlPrec_t &) const override;
			virtual bool bValueToString(std::string &, iCtrlPrec_t ) const override;
			// new functions
			std::uint32_t getColour();
			void setColour(std::uint32_t newColour);
			std::uint32_t floatToInt(iCtrlPrec_t val) const;
			iCtrlPrec_t intToFloat(std::uint32_t val) const;
			virtual std::unique_ptr<CCtrlEditSpace> bCreateEditSpace() override;


		protected:
			ColourType colourType;
			int channel;
			uint32_t colour;
		};

	};

#endif