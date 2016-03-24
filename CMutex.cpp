/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2016 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:CMutex.cpp
		
		Implementation of some basic routines to avoid cyclic dependencies.

*************************************************************************************/
#include "CMutex.h"

namespace cpl
{
	int AlertUserAboutMutex()
	{
		return Misc::MsgBox("Deadlock detected in spinlock: Protected resource is not released after max interval. "
			"Wait again (try again), release resource (continue) - can create async issues - or exit (cancel)?",
			programInfo.name,
			Misc::MsgStyle::sConTryCancel | Misc::MsgIcon::iStop,
			NULL,
			true);
	}
}