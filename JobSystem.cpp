/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2023 Janus Lynggaard Thorborg (www.jthorborg.com)

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

	file:JobSystem.cpp

		Implementation of JobSystem.h

*************************************************************************************/

#include "JobSystem.h"
#include <thread>

namespace cpl
{
	JobSystem& JobSystem::getShared()
	{
		static JobSystem shared(std::thread::hardware_concurrency() > 3 ? (std::thread::hardware_concurrency() / 2) - 1 : 1);

		return shared;
	}
}