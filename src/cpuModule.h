#pragma once

#include "Igor.h"

class c_cpu_state
{
public:
};

struct s_igorDatabase;

struct s_analyzeState
{
	u64 m_PC;
	c_cpu_state* pCpuState;
	s_igorDatabase* pDataBase;

	// output
	Balau::String m_mnemonic;
};

class c_cpu_module
{
public:
	virtual igor_result analyze(s_analyzeState* pState) = 0;
};
