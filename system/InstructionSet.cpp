#include "InstructionSet.h"

namespace cpl
{
	#ifndef CPL_WINDOWS
		//  GCC Inline Assembly
		// creds: http://stackoverflow.com/questions/6121792/how-to-check-if-a-cpu-supports-the-sse3-instruction-set
		void cpuid(int CPUInfo[4],int InfoType)
		{
			__asm__ __volatile__ (
								  "cpuid":
								  "=a" (CPUInfo[0]),
								  "=b" (CPUInfo[1]),
								  "=c" (CPUInfo[2]),
								  "=d" (CPUInfo[3]) :
								  "a" (InfoType), "c" (0)
								  );
		}
		
		void cpuidex(int CPUInfo[4],int InfoType, int SubFunctionID)
		{
			__asm__	__volatile__ ("movl $0, %%ecx" : "=g" (SubFunctionID));
			__asm__ __volatile__ (
								  "cpuid":
								  "=a" (CPUInfo[0]),
								  "=b" (CPUInfo[1]),
								  "=c" (CPUInfo[2]),
								  "=d" (CPUInfo[3]) :
								  "a" (InfoType), "c" (0)
								  );
		}
	#else
	
		void cpuid(int CPUInfo[4],int InfoType)
		{
			return __cpuid(CPUInfo, InfoType);
		};
		
		void cpuidex(int CPUInfo[4],int InfoType, int SubFunctionID)
		{
			return __cpuidex(CPUInfo, InfoType, SubFunctionID);
		};
	
	#endif
	namespace msdn
	{
		const InstructionSet::InstructionSet_Internal InstructionSet::CPU_Rep;
		
	};
		
}