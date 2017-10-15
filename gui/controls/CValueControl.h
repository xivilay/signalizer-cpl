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

	file:CValueControl.h

		A knob that slides through a list of values

*************************************************************************************/

#ifndef CPL_CVALUECONTROL_H
#define CPL_CVALUECONTROL_H

#include "CKnobSlider.h"
#include <vector>

namespace cpl
{
	/*********************************************************************************************

		CValueControl - an extended knob that shows a list of values instead.

	*********************************************************************************************/
	class CValueControl
		:
		public CKnobSlider
	{

	public:
		// 'C' constructor
		// 'values' is a list of |-seperated values
		CValueControl(const std::string & name, const std::string & values, const std::string & unit);
		CValueControl(const std::string & name, const std::vector<std::string> & values, const std::string & unit);
		CValueControl();

		// list of |-seperated values
		virtual void setValues(const std::string & values);
		virtual void setValues(const std::vector<std::string> & values);
		virtual void setUnit(const std::string & unit);

	protected:
		// overrides
		virtual bool bStringToValue(const std::string & valueString, iCtrlPrec_t & val) const override;
		virtual bool bValueToString(std::string & valueString, iCtrlPrec_t val) const override;
		//virtual void baseControlValueChanged() override;

		// data
		std::vector<std::string> values;
		std::string unit;
	};
};
#endif