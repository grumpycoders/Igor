#include "IgorDatabase.h"
#include "cpu_x86.h"

igor_result c_cpu_x86::analyze(s_analyzeState* pState)
{
	c_cpu_x86_state* pX86State = (c_cpu_x86_state*)pState->pCpuState;

	u8 currentByte = 0;
	if(pState->pDataBase->readByte(pState->m_PC, currentByte) != IGOR_SUCCESS)
	{
		return IGOR_FAILURE;
	}

	switch(currentByte)
	{
	case 0x26:
	case 0x2E:
	case 0x36:
	case 0x3E:
	case 0xF2:
	case 0xF3:
		Failure("x86 prefix");
		break;

	default:
		break;
	}

	// opcode
	switch(currentByte)
	{
	case 0xE8:
		pState->m_mnemonic = "call";
		break;
	}

	return IGOR_SUCCESS;
}