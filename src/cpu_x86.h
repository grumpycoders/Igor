#ifndef __CPU_X86_H__
#define __CPU_X86_H__

#include "cpuModule.h"

class c_cpu_x86_state : public c_cpu_state
{
public:
	enum e_executionMode
	{
		undefined,
		_16bits,
		_32bits,
		_64bits,
	};

	e_executionMode m_executionMode;
};

class c_cpu_x86 : public c_cpu_module
{
public:
	igor_result analyze(s_analyzeState* pState);
};

#endif