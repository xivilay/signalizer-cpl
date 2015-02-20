/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

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

	file:SysStats.h

		Gives access to stats of the system, like supported cpu instruction sets
		and such.

*************************************************************************************/


#ifndef _SYSSTATS_H
	#define _SYSSTATS_H
	#include "MacroConstants.h"
	#include "InstructionSet.h"

	#ifdef __CPP11__
		#include <thread>
	#endif

	#include "PlatformSpecific.h"
	

	namespace cpl
	{
		namespace SysStats
		{


			class CProcessorInfo
			{
			public:
				enum Archs
				{
					MMX = 1,
					SSE = 1 << 1,
					SSE2 = 1 << 2,
					SSE3 = 1 << 3,
					SSE4 = 1 << 4,
					AVX = 1 << 5,
					AVX2 = 1 << 6,
					FMA = 1 << 7
				};
				/*
					Certain processors like Intel use hyperthreading
					to actually increase performance, when using more threads
					than available cores.
				*/
				std::size_t getNumOptimalThreads() const
				{
					auto numCores = getNumCores();
					#if 0
						if (msdn::InstructionSet::isIntel)
							return numCores + numCores - 1;
						else
							return numCores;
					#else
						return numCores > 1 ? numCores - 1 : numCores;
					#endif
				}
				/*
					http://stackoverflow.com/a/150971/1287254
				*/
				std::size_t getNumCores() const
				{
					#ifdef __CPP11__
						return std::thread::hardware_concurrency();
					#else
						#ifdef __WINDOWS__
							SYSTEM_INFO sysinfo;
							GetSystemInfo( &sysinfo );
							return sysinfo.dwNumberOfProcessors;
						#else
							return sysconf( _SC_NPROCESSORS_ONLN );
						#endif
					#endif
				}

				double getMHz() const
				{
					return frequency;
				}

				static const CProcessorInfo & instance()
				{
					static const CProcessorInfo _this;
					return _this;
				}

				static const std::string getName()
				{
					return msdn::InstructionSet::Vendor() + msdn::InstructionSet::Brand();
				}

				bool test(Archs arch) const
				{
					return (narchs & arch) ? true : false;
				}

			private:

				CProcessorInfo()
					: narchs(0)
				{
					collectInfo();
				}


				void collectInfo()
				{
					narchs = 0;
					if (msdn::InstructionSet::AVX())
						narchs |= Archs::AVX;
					if (msdn::InstructionSet::AVX2())
						narchs |= Archs::AVX2;
					if (msdn::InstructionSet::FMA())
						narchs |= Archs::FMA;
					if (msdn::InstructionSet::SSE())
						narchs |= Archs::SSE;
					if (msdn::InstructionSet::SSE2())
						narchs |= Archs::SSE2;
					if (msdn::InstructionSet::SSE3())
						narchs |= Archs::SSE3;
					if (msdn::InstructionSet::SSE41())
						narchs |= Archs::SSE4;
					if (msdn::InstructionSet::MMX())
						narchs |= Archs::MMX;


					#ifdef __WINDOWS__
						HKEY hKey;
						DWORD dwMHz;
						DWORD dwSize = sizeof(DWORD);

						RegOpenKeyEx(HKEY_LOCAL_MACHINE,
							_T("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
							0,
							KEY_READ,
							&hKey);

						RegQueryValueEx(hKey, _T("~MHz"), NULL, NULL, (LPBYTE)&dwMHz, &dwSize);
						frequency = dwMHz;
					#else
						#warning dirty fix here
						frequency = 2700;
					#endif
				}

				std::size_t narchs;
				double frequency;

			};



		};
	};

#endif