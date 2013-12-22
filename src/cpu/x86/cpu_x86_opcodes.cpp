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
	u8 RM = MOD_REG_RM & 7; \
	s32 offset = 0; \
	e_mod mod = MOD_DIRECT; \
	switch (MOD) \
	{\
	case 0:\
		mod = MOD_INDIRECT;\
		break;\
	case 1:\
	{\
			  mod = MOD_INDIRECT_ADD_8;\
			  s8 offsetS8 = 0;\
			  if (pState->pDataBase->readS8(pState->m_PC++, offsetS8) != IGOR_SUCCESS)\
			  {\
				  return IGOR_FAILURE;\
			  }\
			  \
			  offset = offsetS8;\
			  \
			  break;\
	}\
	case 3:\
		mod = MOD_DIRECT;\
		break;\
	default:\
		Failure("Unhandled MOD");\
	}

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

	switch (currentByte)
	{
	case 0xE9:
		{
			if (pState->pDataBase->readS32(pState->m_PC, jumpTarget) != IGOR_SUCCESS)
			{
				return IGOR_FAILURE;
			}
			pState->m_PC += 4;
			break;
		}
	case 0xEB:
		{
			s8 jumpTargetS8;
			if (pState->pDataBase->readS8(pState->m_PC++, jumpTargetS8) != IGOR_SUCCESS)
			{
				return IGOR_FAILURE;
			}
			jumpTarget = jumpTargetS8;
			break;
		}
	}
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
	case 0x89:
		{
			GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)REG, mod, offset);
			x86_analyse_result->m_operands[1].setAsRegister(operandSize, (e_register)RM);
			break;
		}
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
	case 0xA3:
		{
			u32 target = 0;
			if (pState->pDataBase->readU32(pState->m_PC, target) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;

			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsAddress(target, true); 
			x86_analyse_result->m_operands[1].setAsRegister(OPERAND_32bit, REG_EAX);

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

igor_result x86_opcode_lea(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_LEA;

	GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)REG);
	x86_analyse_result->m_operands[1].setAsRegister(operandSize, (e_register)RM, mod, offset);

	return IGOR_SUCCESS;
}


igor_result x86_opcode_cmp(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_CMP;

	GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	switch (currentByte)
	{
	case 0x39:
		{
			u32 target = 0;
			if (pState->pDataBase->readU32(pState->m_PC, target) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;
		
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsAddress(target, 1);
			x86_analyse_result->m_operands[1].setAsRegister(operandSize, (e_register)RM);
			break;
		}
	case 0x3B:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)REG);
		x86_analyse_result->m_operands[1].setAsRegister(operandSize, (e_register)RM);
		break;
	default:
		Failure("x86_opcode_cmp");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_test(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_TEST;

	GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)REG);
	x86_analyse_result->m_operands[1].setAsRegister(operandSize, (e_register)RM);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_xor(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_XOR;

	GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)REG);
	x86_analyse_result->m_operands[1].setAsRegister(operandSize, (e_register)RM);

	return IGOR_SUCCESS;
}



igor_result x86_opcode_push(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_PUSH;
	e_operandSize operandSize = getOperandSize(pX86State);

	switch (currentByte)
	{
	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
	case 0x54:
	case 0x55:
	case 0x56:
	case 0x57:
		x86_analyse_result->m_numOperands = 1;
		x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)(currentByte & 7));
		break;
	case 0x68:
		{
			u32 immediate = 0;
			if (pState->pDataBase->readU32(pState->m_PC, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;

			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsImmediate(IMMEDIATE_U32, immediate);
			break;
		}
	case 0x6A:
		{
			u8 immediate = 0;
			if (pState->pDataBase->readU8(pState->m_PC++, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;

			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsImmediate(IMMEDIATE_U8, immediate);
			break;
		}
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_jz(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_JZ;

	s8 jumpTargetS8 = 0;
	if (pState->pDataBase->readS8(pState->m_PC++, jumpTargetS8) != IGOR_SUCCESS)
	{
		return IGOR_FAILURE;
	}

	u64 jumpTarget = pState->m_PC + jumpTargetS8;

	IgorAnalysis::igor_add_code_analysis_task(jumpTarget);

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsAddress(jumpTarget);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_jnz(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_JNZ;

	s8 jumpTargetS8 = 0;
	if (pState->pDataBase->readS8(pState->m_PC++, jumpTargetS8) != IGOR_SUCCESS)
	{
		return IGOR_FAILURE;
	}

	u64 jumpTarget = pState->m_PC + jumpTargetS8;

	IgorAnalysis::igor_add_code_analysis_task(jumpTarget);

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsAddress(jumpTarget);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_F7(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	switch (REG)
	{
		case 2:
		{
			x86_analyse_result->m_mnemonic = INST_X86_NOT;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)RM);
			break;
		}
	default:
		Failure("Unhandled x86_opcode_F7");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_FF(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	switch (REG)
	{
		case 2:
		{
			s32 jumpTarget = 0;
			if (pState->pDataBase->readS32(pState->m_PC, jumpTarget) != IGOR_SUCCESS)
			{
				return IGOR_FAILURE;
			}
			pState->m_PC += 4;

			x86_analyse_result->m_mnemonic = INST_X86_CALL;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsAddress(jumpTarget, true);
			break;
		}
	default:
		Failure("Unhandled x86_opcode_F7");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_83(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	switch (REG)
	{
		case 4:
		{
			u8 immediate = 0;
			if (pState->pDataBase->readU8(pState->m_PC++, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;

			x86_analyse_result->m_mnemonic = INST_X86_AND;
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)RM, mod, offset);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U8, immediate);
			break;
		}
		case 5:
		{
			u8 immediate = 0;
			if (pState->pDataBase->readU8(pState->m_PC++, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;

			x86_analyse_result->m_mnemonic = INST_X86_SUB;
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)RM, mod, offset);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U8, immediate);
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
	/* 0x33 */ &x86_opcode_xor,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	/* 39 */ &x86_opcode_cmp,
	NULL,
	/* 3B */ &x86_opcode_cmp,
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
	/* 0x68 */ &x86_opcode_push,
	NULL,
	/* 0x6A */ &x86_opcode_push,
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
	/* 0x74 */ &x86_opcode_jz,
	/* 0x75 */ &x86_opcode_jnz,
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
	/* 0x85 */ &x86_opcode_test,
	NULL,
	NULL,
	NULL,
	/* 0x89 */ &x86_opcode_mov,
	NULL,
	/* 0x8B */ &x86_opcode_mov,
	NULL,
	/* 0x8D */ &x86_opcode_lea,
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
	/* A3 */ &x86_opcode_mov,
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
	/* EB */ &x86_opcode_jmp,
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
	/* 0xF7 */ &x86_opcode_F7,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	/* 0xFF */ &x86_opcode_FF,
};