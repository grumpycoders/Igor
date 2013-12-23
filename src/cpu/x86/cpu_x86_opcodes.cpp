#include "IgorDatabase.h"
#include "cpu_x86.h"
#include "cpu_x86_opcodes.h"

#include <Printer.h>
using namespace Balau;

#if defined(DEBUG) && defined(_MSC_VER)
#define X86_DECODER_FAILURE(errorString) \
	__debugbreak(); \
	throw X86AnalysisException(errorString);
#else
#define X86_DECODER_FAILURE(errorString) \
	throw X86AnalysisException(errorString);
#endif

s_mod_reg_rm GET_MOD_REG_RM(s_analyzeState* pState)
{
	s_mod_reg_rm resultModRegRm;

	if (pState->pDataBase->readByte(pState->m_PC++, resultModRegRm.RAW_VALUE) != IGOR_SUCCESS)
	{
		throw X86AnalysisException("Failed to read GET_MOD_REG_RM!");
	}
	u8 MOD = (resultModRegRm.RAW_VALUE >> 6) & 3;
	u8 REG = (resultModRegRm.RAW_VALUE >> 3) & 7;
	u8 RM = resultModRegRm.RAW_VALUE & 7;

	resultModRegRm.offset = 0;

	if ((MOD != 3) && (RM == 4))
	{
		if (pState->pDataBase->readU8(pState->m_PC++, resultModRegRm.SIB) != IGOR_SUCCESS)
		{
			throw X86AnalysisException("Error in GET_MOD_REG_RM");
		}
	}

	switch (MOD)
	{
	case 0:
		if (RM == 5)
		{
			if (pState->pDataBase->readS32(pState->m_PC, resultModRegRm.offset) != IGOR_SUCCESS)
			{
				throw X86AnalysisException("Error in GET_MOD_REG_RM");
			}
			pState->m_PC += 4;
		}
		break;
	case 1:
	{
		s8 offsetS8 = 0;
		if (pState->pDataBase->readS8(pState->m_PC++, offsetS8) != IGOR_SUCCESS)
		{
			throw X86AnalysisException("Error in GET_MOD_REG_RM");
		}

		resultModRegRm.offset = offsetS8;
		break;
	}
	case 3:
		//mod = MOD_DIRECT;
		break;
	default:
		throw X86AnalysisException("Error in GET_MOD_REG_RM");
	}

	return resultModRegRm;
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

igor_result x86_opcode_leave(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_LEAVE;

	return IGOR_SUCCESS;
}

igor_result x86_opcode_retn(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_RETN;

	pState->m_analyzeResult = stop_analysis;

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

igor_result x86_opcode_C1(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	u8 variation = modRegRm.getREGRaw();

	switch (variation)
	{
	case 4:
		{
			u8 immediate;
			if (pState->pDataBase->readU8(pState->m_PC++, immediate) != IGOR_SUCCESS)
			{
				return IGOR_FAILURE;
			}
			x86_analyse_result->m_mnemonic = INST_X86_SHL;
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterRM(operandSize, modRegRm);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U8, immediate);
			break;
		}
		case 5:
		{
			u8 immediate;
			if (pState->pDataBase->readU8(pState->m_PC++, immediate) != IGOR_SUCCESS)
			{
				return IGOR_FAILURE;
			}
			x86_analyse_result->m_mnemonic = INST_X86_SHR;
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterRM(operandSize, modRegRm);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U8, immediate);
			break;
		}
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_C1");
	}

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
			s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterRM(operandSize, modRegRm);
			x86_analyse_result->m_operands[1].setAsRegister(operandSize, modRegRm.getREG());
			break;
		}
	case 0x8A:
		{
			s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(OPERAND_8bit, REG_AL);
			x86_analyse_result->m_operands[1].setAsRegisterRM(operandSize, modRegRm);
			break;
		}
	case 0x8B:
		{
			s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(operandSize, modRegRm.getREG());
			x86_analyse_result->m_operands[1].setAsRegisterRM(operandSize, modRegRm);
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
			x86_analyse_result->m_operands[1].setAsAddress(target);

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
	case 0xC7:
		{
			s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

			u32 target = 0;
			if (pState->pDataBase->readU32(pState->m_PC, target) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;

			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterRM(operandSize, modRegRm);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U32, target);
			break;
		}
	default:
		X86_DECODER_FAILURE("Unhandled case in x86_opcode_mov");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_lea(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_LEA;

	s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegister(operandSize, modRegRm.getREG());
	x86_analyse_result->m_operands[1].setAsRegisterRM(operandSize, modRegRm);

	return IGOR_SUCCESS;
}


igor_result x86_opcode_cmp(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_CMP;

	e_operandSize operandSize = getOperandSize(pX86State);

	switch (currentByte)
	{
	case 0x39:
		{
			s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterRM(operandSize, modRegRm);
			x86_analyse_result->m_operands[1].setAsRegister(operandSize, modRegRm.getREG());
			break;
		}
	case 0x3A:
		{
			s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(OPERAND_8bit, modRegRm.getREG());
			x86_analyse_result->m_operands[1].setAsRegisterRM(OPERAND_8bit, modRegRm);
			break;
		}
	case 0x3B:
		{
			s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(operandSize, modRegRm.getREG());
			x86_analyse_result->m_operands[1].setAsRegisterRM(operandSize, modRegRm);
			break;
		}
	case 0x3C:
		{
			u8 immediate = 0;
			if (pState->pDataBase->readU8(pState->m_PC++, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;

			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(OPERAND_8bit, REG_AL);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U8, immediate);
			break;
		}
	default:
		X86_DECODER_FAILURE("x86_opcode_cmp");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_test(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_TEST;

	s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegister(operandSize, modRegRm.getREG());
	x86_analyse_result->m_operands[1].setAsRegisterRM(operandSize, modRegRm);

	return IGOR_SUCCESS;
}


igor_result x86_opcode_sub(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_SUB;

	e_operandSize operandSize = getOperandSize(pX86State);

	switch (currentByte)
	{
	case 0x2B:
		{
			s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(operandSize, modRegRm.getREG());
			x86_analyse_result->m_operands[1].setAsRegisterRM(operandSize, modRegRm);
			break;
		}
	case 0x2D:
		{
			u32 immediate = 0;
			if (pState->pDataBase->readU32(pState->m_PC, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;

			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(operandSize, REG_EAX);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U32, immediate);
			break;
		}
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_or(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_OR;

	e_operandSize operandSize = getOperandSize(pX86State);

	switch (currentByte)
	{
	case 0x0B:
		{
			s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(operandSize, modRegRm.getREG());
			x86_analyse_result->m_operands[1].setAsRegisterRM(operandSize, modRegRm);
			break;
		}
	case 0x0D:
		{
			u32 immediate = 0;
			if (pState->pDataBase->readU32(pState->m_PC, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(operandSize, REG_EAX);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U32, immediate);
			break;
		}
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_or");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_xor(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_XOR;

	s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	switch (currentByte)
	{
	case 0x31:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterRM(operandSize, modRegRm);
		x86_analyse_result->m_operands[1].setAsRegister(operandSize, modRegRm.getREG());
		break;
	case 0x33:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegister(operandSize, modRegRm.getREG());
		x86_analyse_result->m_operands[1].setAsRegisterRM(operandSize, modRegRm);
		break;
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_xor");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_inc(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_INC;
	e_operandSize operandSize = getOperandSize(pX86State);

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)(currentByte & 7));

	return IGOR_SUCCESS;
}

igor_result x86_opcode_pop(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_POP;
	e_operandSize operandSize = getOperandSize(pX86State);

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsRegister(operandSize, (e_register)(currentByte & 7));

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
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_push");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_j_varients(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	
	switch (currentByte)
	{
	case 0x70:
		x86_analyse_result->m_mnemonic = INST_X86_JO;
		break;
	case 0x71:
		x86_analyse_result->m_mnemonic = INST_X86_JNO;
		break;
	case 0x72:
		x86_analyse_result->m_mnemonic = INST_X86_JB;
		break;
	case 0x73:
		x86_analyse_result->m_mnemonic = INST_X86_JNB;
		break;
	case 0x74:
		x86_analyse_result->m_mnemonic = INST_X86_JZ;
		break;
	case 0x75:
		x86_analyse_result->m_mnemonic = INST_X86_JNZ;
		break;
	case 0x76:
		x86_analyse_result->m_mnemonic = INST_X86_JBE;
		break;
	case 0x77:
		x86_analyse_result->m_mnemonic = INST_X86_JNBE;
		break;
	case 0x78:
		x86_analyse_result->m_mnemonic = INST_X86_JS;
		break;
	case 0x79:
		x86_analyse_result->m_mnemonic = INST_X86_JNS;
		break;
	case 0x7A:
		x86_analyse_result->m_mnemonic = INST_X86_JP;
		break;
	case 0x7B:
		x86_analyse_result->m_mnemonic = INST_X86_JNP;
		break;
	case 0x7C:
		x86_analyse_result->m_mnemonic = INST_X86_JL;
		break;
	case 0x7D:
		x86_analyse_result->m_mnemonic = INST_X86_JNL;
		break;
	case 0x7E:
		x86_analyse_result->m_mnemonic = INST_X86_JLE;
		break;
	case 0x7F:
		x86_analyse_result->m_mnemonic = INST_X86_JNLE;
		break;
	}

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
	s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	switch (modRegRm.getREGRaw())
	{
		case 2:
		{
			x86_analyse_result->m_mnemonic = INST_X86_NOT;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsRegisterRM(operandSize, modRegRm);
			break;
		}
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_F7");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_FF(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	u8 variation = modRegRm.getREGRaw();

	switch (variation)
	{
		case 2:
		{
			x86_analyse_result->m_mnemonic = INST_X86_CALL;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsRegisterRM(operandSize, modRegRm);
			break;
		}
		case 4:
		{
			x86_analyse_result->m_mnemonic = INST_X86_JMP;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsRegisterRM(operandSize, modRegRm);
			pState->m_analyzeResult = stop_analysis;
			break;
		}
		case 6:
		{
			x86_analyse_result->m_mnemonic = INST_X86_PUSH;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsRegisterRM(operandSize, modRegRm);
			break;
		}
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_F7");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_83(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

	e_operandSize operandSize = getOperandSize(pX86State);

	u8 immediate = 0;
	if (pState->pDataBase->readU8(pState->m_PC++, immediate) != IGOR_SUCCESS)
		return IGOR_FAILURE;

	u8 variation = modRegRm.getREGRaw();

	switch (variation)
	{
		case 0:
			x86_analyse_result->m_mnemonic = INST_X86_ADD;
			break;
		case 4:
			x86_analyse_result->m_mnemonic = INST_X86_AND;
			break;
		case 5:
			x86_analyse_result->m_mnemonic = INST_X86_SUB;
			break;
		default:
			X86_DECODER_FAILURE("x86_opcode_83");
	}

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegisterRM(operandSize, modRegRm);
	x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U8, immediate);

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
	/* 0x0B */ &x86_opcode_or,
	NULL,
	/* 0x0D */ &x86_opcode_or,
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
	/* 0x2B */ &x86_opcode_sub,
	NULL,
	/* 0x2D */ &x86_opcode_sub,
	NULL,
	NULL,

	// 0x30
	NULL,
	/* 0x31 */ &x86_opcode_xor,
	NULL,
	/* 0x33 */ &x86_opcode_xor,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	/* 39 */ &x86_opcode_cmp,
	/* 3A */ &x86_opcode_cmp,
	/* 3B */ &x86_opcode_cmp,
	/* 3C */ &x86_opcode_cmp,
	NULL,
	NULL,
	NULL,

	/* 0x40 */ &x86_opcode_inc,
	/* 0x41 */ &x86_opcode_inc,
	/* 0x42 */ &x86_opcode_inc,
	/* 0x43 */ &x86_opcode_inc,
	/* 0x44 */ &x86_opcode_inc,
	/* 0x45 */ &x86_opcode_inc,
	/* 0x46 */ &x86_opcode_inc,
	/* 0x47 */ &x86_opcode_inc,
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
	/* 0x58 */ &x86_opcode_pop,
	/* 0x59 */ &x86_opcode_pop,
	/* 0x5A */ &x86_opcode_pop,
	/* 0x5B */ &x86_opcode_pop,
	/* 0x5C */ &x86_opcode_pop,
	/* 0x5D */ &x86_opcode_pop,
	/* 0x5E */ &x86_opcode_pop,
	/* 0x5F */ &x86_opcode_pop,

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

	/* 0x70 */ &x86_opcode_j_varients,
	/* 0x71 */ &x86_opcode_j_varients,
	/* 0x72 */ &x86_opcode_j_varients,
	/* 0x73 */ &x86_opcode_j_varients,
	/* 0x74 */ &x86_opcode_j_varients,
	/* 0x75 */ &x86_opcode_j_varients,
	/* 0x76 */ &x86_opcode_j_varients,
	/* 0x77 */ &x86_opcode_j_varients, 
	/* 0x78 */ &x86_opcode_j_varients, 
	/* 0x79 */ &x86_opcode_j_varients, 
	/* 0x7A */ &x86_opcode_j_varients, 
	/* 0x7B */ &x86_opcode_j_varients, 
	/* 0x7C */ &x86_opcode_j_varients, 
	/* 0x7D */ &x86_opcode_j_varients, 
	/* 0x7E */ &x86_opcode_j_varients, 
	/* 0x7F */ &x86_opcode_j_varients, 

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
	/* 0x8A */ &x86_opcode_mov,
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
	/* C1 */ &x86_opcode_C1,
	NULL,
	/* C3 */ &x86_opcode_retn,
	NULL,
	NULL,
	NULL,
	/* C7 */ &x86_opcode_mov,
	NULL,
	/* C9 */ &x86_opcode_leave,
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