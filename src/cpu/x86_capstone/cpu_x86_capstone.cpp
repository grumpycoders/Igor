#include "IgorDatabase.h"
#include "IgorSession.h"

#include "cpu_x86_capstone.h"

extern "C" {
	int strcasecmp(const char *f1, const char *f2);
}

int strcasecmp(const char *f1, const char *f2)
{
	Failure("strcasecmp not implemented");
	return 0;
}

c_cpu_x86_capstone::c_cpu_x86_capstone()
{
	cs_open(CS_ARCH_X86, CS_MODE_64, &m_csHandle);
}

c_cpu_x86_capstone::~c_cpu_x86_capstone()
{

}

igor_result c_cpu_x86_capstone::analyze(s_analyzeState* pState)
{
	u8 buffer[256];
	for (int i = 0; i < 256; i++)
	{
		buffer[i] = pState->pSession->readU8(pState->m_PC + i);
	}
	cs_insn *insn;
	int count = cs_disasm_ex(m_csHandle, buffer, 256, pState->m_PC.offset, 0, &insn);

	if (count)
	{
		pState->m_cpu_analyse_result->m_startOfInstruction.offset = insn[0].address;
		pState->m_cpu_analyse_result->m_instructionSize = insn[0].size;
		pState->m_PC += insn[0].size;

		cs_free(insn, count);

		return IGOR_SUCCESS;
	}

	return IGOR_FAILURE;
}

igor_result c_cpu_x86_capstone::printInstruction(s_analyzeState* pState, Balau::String& outputString, bool bUseColor)
{
	u8 buffer[256];
	for (int i = 0; i < pState->m_cpu_analyse_result->m_instructionSize; i++)
	{
		buffer[i] = pState->pSession->readU8(pState->m_cpu_analyse_result->m_startOfInstruction + i);
	}
	cs_insn *insn;
	int count = cs_disasm_ex(m_csHandle, buffer, pState->m_cpu_analyse_result->m_instructionSize, pState->m_cpu_analyse_result->m_startOfInstruction.offset, 0, &insn);

	if (count)
	{
		outputString.append("%s %s", insn[0].mnemonic, insn[0].op_str);

		cs_free(insn, count);

		return IGOR_SUCCESS;
	}

	return IGOR_FAILURE;
}

void c_cpu_x86_capstone::printInstruction(c_cpu_analyse_result* result)
{

}

igor_result c_cpu_x86_capstone::getMnemonic(s_analyzeState* pState, Balau::String& outputString)
{
	return IGOR_SUCCESS;
}

int c_cpu_x86_capstone::getNumOperands(s_analyzeState* pState)
{
	return 0;
}

igor_result c_cpu_x86_capstone::getOperand(s_analyzeState* pState, int operandIndex, Balau::String& outputString, bool bUseColor)
{
	return IGOR_SUCCESS;
}

void c_cpu_x86_capstone::generateReferences(s_analyzeState* pState)
{

}
