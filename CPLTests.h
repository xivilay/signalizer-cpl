/*************************************************************************************
 
	 cpl - cross-platform library - v. 0.3.0.
	 
	 Copyright (C) 2015 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:CPLTests.h

		Access to automated performance, integrity and precision tests of CPL.

*************************************************************************************/

#ifndef CPLTESTS_H
	#define CPLTESTS_H

	namespace cpl
	{

		enum class DiagnosticLevel
		{
			None, 
			Errors,
			Warnings,
			Info = Warnings,
			All
		};

		bool CAudioStreamTest(std::size_t emulatedBufferSize = 64, double sampleRate = 44100, DiagnosticLevel lvl = DiagnosticLevel::Warnings);

	};

#endif