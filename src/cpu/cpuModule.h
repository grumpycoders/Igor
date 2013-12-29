#pragma once

#include "Igor.h"

class c_cpu_module;

class c_cpu_state
{
public:
};

// Holds the result of one analyze step (ie, one instruction).
// Each CPU must derive from it
// It is passed back to the CPU for query
class c_cpu_analyse_result
{
public:
	u64 m_startOfInstruction;
	u8 m_instructionSize;

};

struct s_igorDatabase;

enum e_analyzeResult
{
	stop_analysis,
	continue_analysis,
};

class IgorAnalysisManager;

struct s_analyzeState
{
	// input
	u64 m_PC;
	c_cpu_module* pCpu;
	c_cpu_state* pCpuState;
    IgorAnalysisManager* pAnalysis;

	// output
	c_cpu_analyse_result* m_cpu_analyse_result;

	e_analyzeResult m_analyzeResult;
};

class c_cpu_module
{
public:
	virtual igor_result analyze(s_analyzeState* pState) = 0;
	virtual igor_result printInstruction(s_analyzeState* pState, Balau::String& outputString) = 0;
	virtual c_cpu_analyse_result* allocateCpuSpecificAnalyseResult() = 0;
};
