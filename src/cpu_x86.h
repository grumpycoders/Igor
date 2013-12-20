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

enum e_instructions
{
	INST_X86_MOV,

	INST_X86_CALL,
	INST_X86_JMP,
};

class c_cpu_x86 : public c_cpu_module
{
public:

	igor_result analyze(s_analyzeState* pState);
};

#endif