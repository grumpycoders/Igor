#pragma once

#include "Igor.h"

class c_cpu_module;

class c_cpu_state
{
public:
};

struct s_igorDatabase;

enum e_analyzeResult
{
	stop_analysis,
	continue_analysis,
};

struct s_analyzeState
{
	// input
	u64 m_PC;
	c_cpu_module* pCpu;
	c_cpu_state* pCpuState;
	s_igorDatabase* pDataBase;

	// output
	u32 m_mnemonic;
	e_analyzeResult m_analyzeResult;
};

class c_cpu_module
{
public:
	virtual igor_result analyze(s_analyzeState* pState) = 0;
};
