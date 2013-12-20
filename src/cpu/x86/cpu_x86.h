#pragma once

#include "cpu/cpuModule.h"

class c_cpu_x86;

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

	c_cpu_x86_state() :
		m_executionMode(_32bits)
	{

	}

	e_executionMode m_executionMode;
};

enum e_instructions
{
	INST_X86_MOV,

	INST_X86_CALL,
	INST_X86_JMP,
	INST_X86_PUSH,
	INST_X86_SUB,
};

enum e_operandSize
{
	OPERAND_Unkown = -1,
	OPERAND_8bit = 0,
	OPERAND_16bit = 1,
	OPERAND_32bit = 2,
};

class c_cpu_x86 : public c_cpu_module
{
public:

	igor_result analyze(s_analyzeState* pState);
	const char* getRegisterName(e_operandSize size, u8 regIndex);

private:

	c_cpu_x86_state m_defaultState;
};
