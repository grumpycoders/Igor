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

#define GET_RM(pState) \
	u8 RM = 0; \
if (pState->pDataBase->readByte(pState->m_PC++, RM) != IGOR_SUCCESS) \
{ \
	return IGOR_FAILURE; \
} 

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
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_CALL;

	s32 jumpTarget = 0;
	if (pState->pDataBase->readS32(pState->m_PC, jumpTarget) != IGOR_SUCCESS)
	{
		return IGOR_FAILURE;
	}

	pState->m_PC += 4;
	jumpTarget += pState->m_PC;

	IgorAnalysis::igor_add_code_analysis_task(jumpTarget);

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsAddress(jumpTarget);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_jmp(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_JMP;

	s32 jumpTarget = 0;
	if (pState->pDataBase->readS32(pState->m_PC, jumpTarget) != IGOR_SUCCESS)
	{
		return IGOR_FAILURE;
	}

	pState->m_PC += 4;
	jumpTarget += pState->m_PC;

	IgorAnalysis::igor_add_code_analysis_task(jumpTarget);

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsAddress(jumpTarget);

	pState->m_analyzeResult = stop_analysis;

	return IGOR_SUCCESS;
}

igor_result x86_opcode_mov(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_MOV;

	u8 S = currentByte & 1;
	u8 X = (currentByte >> 1) & 1;
	e_operandSize operandSize = getOperandSize(pX86State);

	switch (currentByte)
	{
	case 0x8B:
		{
			GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)REG);
			x86_analyse_result->m_operands[1].setAsRegister(operandSize, (e_register)RM);
			break;
		}
	case 0xA1:
		{
		 	u32 target = 0;
			if (pState->pDataBase->readU32(pState->m_PC, target) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;

			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(OPERAND_32bit, REG_EAX);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U32, target);

			igor_flag_address_as_u32(target);

			break;
		}
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	case 0xBC:
	case 0xBD:
	case 0xBE:
	case 0xBF:
		{
		 	u32 target = 0;
			if (pState->pDataBase->readU32(pState->m_PC, target) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;

			u8 registerIdx = currentByte & 7;
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(OPERAND_32bit, (e_register)registerIdx);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U32, target);

			igor_flag_address_as_u32(target);

			break;
		}
	default:
		Failure("Unhandled case in x86_opcode_mov");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_push(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_PUSH;

	u8 registerIdx = currentByte & 7;

	e_operandSize operandSize = getOperandSize(pX86State);

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)registerIdx);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_83(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	s32 offset = 0;
	e_mod mod = MOD_DIRECT;

	switch (MOD)
	{
	case 0:
		// indirect addressing, do nothing
		mod = MOD_INDIRECT;
		break;
	case 1:
		// indirect addressing + 8bit
		{
			mod = MOD_INDIRECT_ADD_8;
			s8 offsetS8 = 0;
			if (pState->pDataBase->readS8(pState->m_PC++, offsetS8) != IGOR_SUCCESS)
			{
				return IGOR_FAILURE;
			}

			offset = offsetS8;

			break;
		}
	case 3:
		// direct addressing
		mod = MOD_DIRECT;
		break;
	default:
		Failure("Unhandled MOD");
	}

	switch (REG)
	{
	case 4:
	{
		s8 immediate = 0;
		if (pState->pDataBase->readS8(pState->m_PC++, immediate) != IGOR_SUCCESS)
			return IGOR_FAILURE;

		x86_analyse_result->m_mnemonic = INST_X86_AND;
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)RM, mod, offset);
		x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_S8, immediate);
		break;
	}
	case 5:
	{
		s8 immediate = 0;
		if (pState->pDataBase->readS8(pState->m_PC++, immediate) != IGOR_SUCCESS)
			return IGOR_FAILURE;

		x86_analyse_result->m_mnemonic = INST_X86_SUB;
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)RM, mod, offset);
		x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_S8, immediate);
		break;
	}
	default:
		Failure("");
	}

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
	/* 0x50 */ &x86_opcode_push,
	/* 0x51 */ &x86_opcode_push,
	/* 0x52 */ &x86_opcode_push,
	/* 0x53 */ &x86_opcode_push,
	/* 0x54 */ &x86_opcode_push,
	/* 0x55 */ &x86_opcode_push,
	/* 0x56 */ &x86_opcode_push,
	/* 0x57 */ &x86_opcode_push,
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
	/* 0x83 */ &x86_opcode_83,
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
	/* A1 */ &x86_opcode_mov,
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
	/* B8 */ &x86_opcode_mov,
	/* B9 */ &x86_opcode_mov,
	/* BA */ &x86_opcode_mov,
	/* BB */ &x86_opcode_mov,
	/* BC */ &x86_opcode_mov,
	/* BD */ &x86_opcode_mov,
	/* BE */ &x86_opcode_mov,
	/* BF */ &x86_opcode_mov,

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