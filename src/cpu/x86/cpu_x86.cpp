#include "IgorDatabase.h"
#include "cpu_x86.h"
#include "cpu_x86_opcodes.h"

const char* registerName[3][8] =
{
	{ "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH" }, // 8bit
	{ "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" }, // 16bits
	{ "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI" }, // 32 bits
};

igor_result c_cpu_x86::analyze(s_analyzeState* pState)
{
	c_cpu_x86_state* pX86State = (c_cpu_x86_state*)pState->pCpuState;

	if (pX86State == NULL)
	{
		// grab the default state
		pX86State = &m_defaultState;
	}

	u8 currentByte = 0;
	if (pState->pDataBase->readByte(pState->m_PC, currentByte) != IGOR_SUCCESS)
	{
		return IGOR_FAILURE;
	}

	pState->m_PC++;

	if (x86_opcode_table[currentByte] == NULL)
	{
		return IGOR_FAILURE;
	}

	return x86_opcode_table[currentByte](pState, pX86State, currentByte);
}

const char* c_cpu_x86::getRegisterName(e_operandSize size, u8 regIndex)
{
	return registerName[(int)size][regIndex];
}