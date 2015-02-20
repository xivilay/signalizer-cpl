/*************************************************************************************

	Audio Programming Environment VST. 
		
		VST is a trademark of Steinberg Media Technologies GmbH.

    Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:Misc.cpp
		
		Implementation of Misc.h

*************************************************************************************/
#include "CBaseControl.h"
#include "CCtrlEditSpace.h"

namespace cpl
{

	std::unique_ptr<CCtrlEditSpace> CBaseControl::bCreateEditSpace()
	{
		if (isEditSpacesAllowed)
			return std::unique_ptr<CCtrlEditSpace>(new CCtrlEditSpace(this));
		else
			return nullptr;
	}
	/*CSerializer::Archiver & operator << (CSerializer::Archiver & left, CBaseControl * right)
	{
		right->save(left, left.getMasterVersion());
		return left;
	}
	CSerializer::Builder & operator >> (CSerializer::Builder & left, CBaseControl * right)
	{
		right->load(left, left.getMasterVersion());
		return left;
	}*/
};