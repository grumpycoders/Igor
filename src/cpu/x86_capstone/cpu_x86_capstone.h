#pragma once

#include "cpu/cpuModule.h"
#include "capstone/include/capstone.h"

class c_x86_capstone_analyse_result : public c_cpu_analyse_result
{
public:
};

class c_cpu_x86_capstone : public c_cpu_module
{
public:
	c_cpu_x86_capstone();
	virtual ~c_cpu_x86_capstone();

	igor_result analyze(s_analyzeState* pState);
	igor_result printInstruction(s_analyzeState* pState, Balau::String& outputString, bool bUseColor = false);
	c_cpu_analyse_result* allocateCpuSpecificAnalyseResult(){ return new c_x86_capstone_analyse_result; }
	
	//const char* getRegisterName(s_analyzeState* pState, s_registerDefinition definition, u8 regIndex, bool sizeOverride = false);
	//const char* getMnemonicName(e_x86_mnemonic mnemonic);

	void printInstruction(c_cpu_analyse_result* result);
	igor_result getMnemonic(s_analyzeState* pState, Balau::String& outputString);
	int getNumOperands(s_analyzeState* pState);
	igor_result getOperand(s_analyzeState* pState, int operandIndex, Balau::String& outputString, bool bUseColor = false);
	void generateReferences(s_analyzeState* pState);

	csh m_csHandle;
};
