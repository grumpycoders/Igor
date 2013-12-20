#include "IgorDatabase.h"
#include "cpu_x86.h"
#include "cpu_x86_opcodes.h"

#include <Printer.h>
using namespace Balau;

#define GET_MOD_REG_RM(pState) \
u8 MOD_REG_RM = 0; \
if (pState->pDataBase->readByte(pState->m_PC++, MOD_REG_RM) != IGOR_SUCCESS) \
{ \
	return IGOR_FAILURE; \
} \
u8 MOD = (MOD_REG_RM >> 6) & 3; \
u8 REG = (MOD_REG_RM >> 3) & 7; \
u8 RM = MOD_REG_RM & 7;

e_operandSize getOperandSize(c_cpu_x86_state* pX86State)
{
	e_operandSize operandSize = OPERAND_Unkown;

	switch (pX86State->m_executionMode)
	{
	case c_cpu_x86_state::_16bits:
		operandSize = OPERAND_16bit;
		break;
	case c_cpu_x86_state::_32bits:
		operandSize = OPERAND_32bit;
		break;
	default:
		Failure("Bad state in getOperandSize");
		break;
	}

	// TODO: Here, figure out the override operand size from prefix

	return operandSize;
}

igor_result x86_opcode_call(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	pState->m_mnemonic = INST_X86_CALL;

	s32 jumpTarget = 0;
	if (pState->pDataBase->readS32(pState->m_PC, jumpTarget) != IGOR_SUCCESS)
	{
		return IGOR_FAILURE;
	}

	pState->m_PC += 4;
	jumpTarget += pState->m_PC;

	IgorAnalysis::igor_add_code_analysis_task(jumpTarget);

	Printer::log(M_INFO, "0x%08llX: CALL 0x%08llX", pState->m_PC, jumpTarget);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_jmp(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	pState->m_mnemonic = INST_X86_JMP;

	s32 jumpTarget = 0;
	if (pState->pDataBase->readS32(pState->m_PC, jumpTarget) != IGOR_SUCCESS)
	{
		return IGOR_FAILURE;
	}

	pState->m_PC += 4;
	jumpTarget += pState->m_PC;

	IgorAnalysis::igor_add_code_analysis_task(jumpTarget);

	Printer::log(M_INFO, "0x%08llX: JMP 0x%08llX", pState->m_PC, jumpTarget);

	pState->m_analyzeResult = stop_analysis;

	return IGOR_SUCCESS;
}

igor_result x86_opcode_mov(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	pState->m_mnemonic = INST_X86_MOV;

	u8 S = currentByte & 1;
	u8 X = (currentByte >> 1) & 1;

	GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	const char* operand1 = ((c_cpu_x86*)pState->pCpu)->getRegisterName(operandSize, REG);
	const char* operand2 = ((c_cpu_x86*)pState->pCpu)->getRegisterName(operandSize, RM);

	Printer::log(M_INFO, "0x%08llX: MOV %s, %s", pState->m_PC, operand1, operand2);

	return IGOR_SUCCESS;
}

const t_x86_opcode x86_opcode_table[0x100] =
{
	// 0x00
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0x10
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0x20
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0x30
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0x40
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0x50
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0x60
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0x70
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0x80
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	/* 8B */ &x86_opcode_mov,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0x90
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0xA0
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0xB0
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0xC0
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0xD0
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0xE0
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	/* E8 */ &x86_opcode_call,
	/* E9 */ &x86_opcode_jmp,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0xF0
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};