#include "IgorDatabase.h"
#include "cpu_x86.h"
#include "cpu_x86_opcodes.h"

#include <Printer.h>
using namespace Balau;

s_mod_reg_rm GET_MOD_REG_RM(s_analyzeState* pState)
{
	s_mod_reg_rm resultModRegRm;

	if (pState->pSession->readU8(pState->m_PC++, resultModRegRm.RAW_VALUE) != IGOR_SUCCESS)
	{
		throw X86AnalysisException("Failed to read GET_MOD_REG_RM!");
	}
	u8 MOD = (resultModRegRm.RAW_VALUE >> 6) & 3;
	u8 REG = (resultModRegRm.RAW_VALUE >> 3) & 7;
	u8 RM = resultModRegRm.RAW_VALUE & 7;

	resultModRegRm.offset = 0;

	if ((MOD != 3) && (RM == 4))
	{
        if (pState->pSession->readU8(pState->m_PC++, resultModRegRm.SIB) != IGOR_SUCCESS)
		{
			throw X86AnalysisException("Error in GET_MOD_REG_RM");
		}
	}

	switch (MOD)
	{
	case 0:
		if ((RM == 5) || (resultModRegRm.getSIBBase() == 5))
		{
			s32 offset;
            if (pState->pSession->readS32(pState->m_PC, offset) != IGOR_SUCCESS)
			{
				throw X86AnalysisException("Error in GET_MOD_REG_RM");
			}
			resultModRegRm.offset = offset;
			pState->m_PC += 4;
		}
		break;
	case 1:
	{
		s8 offsetS8 = 0;
        if (pState->pSession->readS8(pState->m_PC++, offsetS8) != IGOR_SUCCESS)
		{
			throw X86AnalysisException("Error in GET_MOD_REG_RM");
		}

		resultModRegRm.offset = offsetS8;
		break;
	}
	case 2:
	{
		s32 offsetS32 = 0;
        if (pState->pSession->readS32(pState->m_PC, offsetS32) != IGOR_SUCCESS)
		{
			throw X86AnalysisException("Error in GET_MOD_REG_RM");
		}
		pState->m_PC += 4;
		resultModRegRm.offset = offsetS32;
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
/*
e_operandSize getOperandSize(s_analyzeState* pState)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	c_cpu_x86_state* pX86State = (c_cpu_x86_state*)pState->pCpuState;

	e_operandSize operandSize = OPERAND_Unkown;

	if (x86_analyse_result->m_sizeOverride)
	{
		switch (pX86State->m_executionMode)
		{
		case c_cpu_x86_state::_16bits:
			Failure("override in 16bit?")
			break;
		case c_cpu_x86_state::_32bits:
			operandSize = OPERAND_16bit;
			break;
		default:
			Failure("Bad state in getOperandSize");
			break;
		}
	}
	else
	{
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
	}

	return operandSize;
}
*/
igor_result x86_opcode_call(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_CALL;

	s32 jumpTarget = 0;
    if (pState->pSession->readS32(pState->m_PC, jumpTarget) != IGOR_SUCCESS)
	{
		return IGOR_FAILURE;
	}

	pState->m_PC += 4;
    igorAddress jumpTargetAddress = pState->m_PC;
    jumpTargetAddress += jumpTarget;

    pState->pSession->add_code_analysis_task(jumpTargetAddress);

	x86_analyse_result->m_numOperands = 1;
    x86_analyse_result->m_operands[0].setAsAddress(jumpTargetAddress.offset);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_leave(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_LEAVE;

	return IGOR_SUCCESS;
}

igor_result x86_opcode_int(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_INT;

	x86_analyse_result->m_numOperands = 1;

	switch (currentByte)
	{
	case 0xCC:
		x86_analyse_result->m_operands[0].setAsImmediate(IMMEDIATE_U8, (u8)3);
		break;
	case 0xCD:
		x86_analyse_result->m_operands[0].setAsImmediate(pState, OPERAND_8bit);
		break;
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_int");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_d9(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
    x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

    u8 variation = x86_analyse_result->m_mod_reg_rm.getREGRaw();

    switch (variation)
    {
    case 1:
        x86_analyse_result->m_mnemonic = INST_X86_FXCH;
        x86_analyse_result->m_numOperands = 2;
        x86_analyse_result->m_operands[0].setAsRegisterSTi(pState);
        x86_analyse_result->m_operands[1].setAsRegisterST(pState);
        break;
    case 5:
        {
			switch (x86_analyse_result->m_mod_reg_rm.RAW_VALUE)
			{
            default:
                x86_analyse_result->m_mnemonic = INST_X86_FLDCW;
                x86_analyse_result->m_numOperands = 1;
                x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
                break;
			case 0xEE:
				x86_analyse_result->m_mnemonic = INST_X86_FLDZ;
				break;
            case 0xE8:
            case 0xE9:
            case 0xEA:
            case 0xEB:
            case 0xEC:
            case 0xED:
            case 0xEF:
				X86_DECODER_FAILURE("Unhandled subopcode in x86_opcode_d9");
			}
			break;
		}
    case 7:
        switch (x86_analyse_result->m_mod_reg_rm.RAW_VALUE)
        {
        default:
            x86_analyse_result->m_mnemonic = INST_X86_FSTCW;
            x86_analyse_result->m_numOperands = 1;
            x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
            break;
        case 0xF8:
        case 0xF9:
        case 0xFA:
        case 0xFB:
        case 0xFC:
        case 0xFD:
        case 0xFE:
        case 0xFF:
            X86_DECODER_FAILURE("Unhandled subopcode in x86_opcode_d9");
        }
        break;
    default:
        X86_DECODER_FAILURE("Unhandled x86_opcode_d9");
    }

    return IGOR_SUCCESS;
}

igor_result x86_opcode_da(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
    x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

    u8 variation = x86_analyse_result->m_mod_reg_rm.getREGRaw();

    switch (variation)
    {
    case 5:
        {
              switch (x86_analyse_result->m_mod_reg_rm.RAW_VALUE)
              {
              case 0xE9:
                  x86_analyse_result->m_mnemonic = INST_X86_FUCOMPP;
                  break;
              default:
                  X86_DECODER_FAILURE("Unhandled subopcode in x86_opcode_da");
              }
              break;
        }
    default:
        X86_DECODER_FAILURE("Unhandled x86_opcode_da");
    }

    return IGOR_SUCCESS;
}

igor_result x86_opcode_db(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	switch (x86_analyse_result->m_mod_reg_rm.getREG())
	{
	case 4:
		{
			  switch (x86_analyse_result->m_mod_reg_rm.RAW_VALUE)
			  {
			  case 0xE3:
				  x86_analyse_result->m_mnemonic = INST_X86_FINIT;
				  break;
			  default:
				  X86_DECODER_FAILURE("Unhandled subopcode in x86_opcode_db");
			  }
			  break;
		}
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_db");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_dc(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
    x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

    u8 variation = x86_analyse_result->m_mod_reg_rm.getREGRaw();

    switch (variation)
    {
    case 4:
        x86_analyse_result->m_mnemonic = INST_X86_FSUB;
        x86_analyse_result->m_numOperands = 1;
        x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
        break;
    case 6:
        x86_analyse_result->m_mnemonic = INST_X86_FDIV;
        x86_analyse_result->m_numOperands = 1;
        x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
        break;
    default:
        X86_DECODER_FAILURE("Unhandled x86_opcode_dc");
    }

    return IGOR_SUCCESS;
}

igor_result x86_opcode_dd(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
    x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

    u8 variation = x86_analyse_result->m_mod_reg_rm.getREGRaw();

    switch (variation)
    {
    case 0:
        x86_analyse_result->m_mnemonic = INST_X86_FLD;
        x86_analyse_result->m_numOperands = 1;
        x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
        break;
    case 3:
        x86_analyse_result->m_mnemonic = INST_X86_FSTP;
        x86_analyse_result->m_numOperands = 1;
        x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
        break;
    default:
        X86_DECODER_FAILURE("Unhandled x86_opcode_dd");
    }

    return IGOR_SUCCESS;
}

igor_result x86_opcode_de(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
    x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

    u8 variation = x86_analyse_result->m_mod_reg_rm.getREGRaw();

    switch (variation)
    {
    case 0:
        x86_analyse_result->m_mnemonic = INST_X86_FADDP;
        x86_analyse_result->m_numOperands = 2;
        x86_analyse_result->m_operands[0].setAsRegisterSTi(pState);
        x86_analyse_result->m_operands[1].setAsRegisterST(pState);
        break;
    case 7:
        x86_analyse_result->m_mnemonic = INST_X86_FDIVP;
        x86_analyse_result->m_numOperands = 2;
        x86_analyse_result->m_operands[0].setAsRegisterSTi(pState);
        x86_analyse_result->m_operands[1].setAsRegisterST(pState);
        break;
    default:
        X86_DECODER_FAILURE("Unhandled x86_opcode_de");
    }

    return IGOR_SUCCESS;
}

igor_result x86_opcode_df(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
    x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

    u8 variation = x86_analyse_result->m_mod_reg_rm.getREGRaw();

    switch (variation)
    {
    case 0:
        x86_analyse_result->m_mnemonic = INST_X86_FILD;
        x86_analyse_result->m_numOperands = 1;
        x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
        break;
    case 3:
        x86_analyse_result->m_mnemonic = INST_X86_FISTP;
        x86_analyse_result->m_numOperands = 2;
        x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
        x86_analyse_result->m_operands[1].setAsRegisterST(pState);
        break;
    case 4:
        switch (x86_analyse_result->m_mod_reg_rm.RAW_VALUE)
        {
        case 0xE0:
            x86_analyse_result->m_mnemonic = INST_X86_FNSTSW;
            x86_analyse_result->m_numOperands = 1;
            x86_analyse_result->m_operands[0].setAsRegister(pState, REG_AX, OPERAND_16bit);
            break;
        default:
            X86_DECODER_FAILURE("Unhandled subopcode in x86_opcode_df");
        }
        break;
    default:
        X86_DECODER_FAILURE("Unhandled x86_opcode_df");
    }

    return IGOR_SUCCESS;
}

igor_result x86_opcode_retn(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_RETN;

	if (currentByte == 0xC2)
	{
		u16 immediate = 0;
        if (pState->pSession->readU16(pState->m_PC, immediate) != IGOR_SUCCESS)
			return IGOR_FAILURE;
		pState->m_PC += 4;

		x86_analyse_result->m_numOperands = 1;
		x86_analyse_result->m_operands[0].setAsImmediate(IMMEDIATE_U16, immediate);
	}

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
            if (pState->pSession->readS32(pState->m_PC, jumpTarget) != IGOR_SUCCESS)
			{
				return IGOR_FAILURE;
			}
			pState->m_PC += 4;
			break;
		}
	case 0xEB:
		{
			s8 jumpTargetS8;
            if (pState->pSession->readS8(pState->m_PC++, jumpTargetS8) != IGOR_SUCCESS)
			{
				return IGOR_FAILURE;
			}
			jumpTarget = jumpTargetS8;
			break;
		}
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_jmp");
	}
	igorAddress jumpTargetAddress = pState->m_PC;
    jumpTargetAddress += jumpTarget;

    pState->pSession->add_code_analysis_task(jumpTargetAddress);

	x86_analyse_result->m_numOperands = 1;
    x86_analyse_result->m_operands[0].setAsAddress(jumpTargetAddress.offset);

	pState->m_analyzeResult = stop_analysis;

	return IGOR_SUCCESS;
}

igor_result x86_opcode_C1(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
    x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

    x86_analyse_result->m_numOperands = 2;
    x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
    x86_analyse_result->m_operands[1].setAsImmediate(pState, OPERAND_8bit);

    u8 variation = x86_analyse_result->m_mod_reg_rm.getREGRaw();

	switch (variation)
	{
    case 0:
        x86_analyse_result->m_mnemonic = INST_X86_ROL;
        break;
    case 1:
        x86_analyse_result->m_mnemonic = INST_X86_ROR;
        break;
    case 2:
        x86_analyse_result->m_mnemonic = INST_X86_RCL;
        break;
    case 3:
        x86_analyse_result->m_mnemonic = INST_X86_RCR;
        break;
    case 4:
		x86_analyse_result->m_mnemonic = INST_X86_SHL;
		break;
    case 5:
        x86_analyse_result->m_mnemonic = INST_X86_SHR;
        break;
    case 6:
        x86_analyse_result->m_mnemonic = INST_X86_SAL;
        break;
    case 7:
        x86_analyse_result->m_mnemonic = INST_X86_SAR;
        break;
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_C1");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_xchg(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_XCHG;

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegisterR(pState);
	x86_analyse_result->m_operands[1].setAsRegisterRM(pState);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_stosd(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_STOSD;

	return IGOR_SUCCESS;
}

igor_result x86_opcode_movsd(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_MOVSD;

	return IGOR_SUCCESS;
}

igor_result x86_opcode_movsb(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_MOVSB;

	return IGOR_SUCCESS;
}

igor_result x86_opcode_mov(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_MOV;
	
	switch (currentByte)
	{
	case 0x88:
		{
			x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterRM(pState, OPERAND_8bit);
			x86_analyse_result->m_operands[1].setAsRegisterR(pState, OPERAND_8bit);
			break;
		}
	case 0x89:
		{
			x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
			x86_analyse_result->m_operands[1].setAsRegisterR(pState);
			break;
		}
	case 0x8A:
		{
			x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(pState, REG_AL, OPERAND_8bit);
			x86_analyse_result->m_operands[1].setAsRegisterRM(pState);
			break;
		}
	case 0x8B:
		{
			x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterR(pState);
			x86_analyse_result->m_operands[1].setAsRegisterRM(pState);
			break;
		}
	case 0xA1:
		{
		 	u32 target = 0;
            if (pState->pSession->readU32(pState->m_PC, target) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;
            igorAddress targetAddress;
            targetAddress.offset = target;

			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(pState, REG_EAX, OPERAND_16_32);
            x86_analyse_result->m_operands[1].setAsAddress(targetAddress.offset);

			break;
		}
	case 0xA3:
		{
			u32 target = 0;
            if (pState->pSession->readU32(pState->m_PC, target) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;
            igorAddress targetAddress;
            targetAddress.offset = target;

			x86_analyse_result->m_numOperands = 2;
            x86_analyse_result->m_operands[0].setAsAddress(targetAddress.offset, true);
			x86_analyse_result->m_operands[1].setAsRegister(pState, REG_EAX, OPERAND_16_32);

			break;
		}
	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
	case 0xB4:
	case 0xB5:
	case 0xB6:
	case 0xB7:
		{
			u8 registerIdx = currentByte & 7;
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(pState, (e_register)registerIdx, OPERAND_8bit);
			x86_analyse_result->m_operands[1].setAsImmediate(pState, OPERAND_8bit);
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
			u8 registerIdx = currentByte & 7;
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(pState, (e_register)registerIdx, OPERAND_16_32);
			x86_analyse_result->m_operands[1].setAsImmediate(pState, OPERAND_16_32);
			break;
		}
	case 0xC6:
		{
			x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterR(pState, OPERAND_8bit);
			x86_analyse_result->m_operands[1].setAsImmediate(pState, OPERAND_8bit);
			break;
		}
	case 0xC7:
		{
			s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

			u32 target = 0;
            if (pState->pSession->readU32(pState->m_PC, target) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;

			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterRM(modRegRm);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U32, target);
			break;
		}
	default:
		X86_DECODER_FAILURE("Unhandled case in x86_opcode_mov");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_pushf(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_PUSHF;
	return IGOR_SUCCESS;
}

igor_result x86_opcode_popf(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_POPF;
	return IGOR_SUCCESS;
}

igor_result x86_opcode_nop(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_NOP;
	return IGOR_SUCCESS;
}

igor_result x86_opcode_lea(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_LEA;

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegisterR(pState);
	x86_analyse_result->m_operands[1].setAsRegisterRM(pState);

	return IGOR_SUCCESS;
}


igor_result x86_opcode_cmp(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_CMP;

	switch (currentByte)
	{
    case 0x38:
        x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
        x86_analyse_result->m_numOperands = 2;
        x86_analyse_result->m_operands[0].setAsRegisterRM(pState, OPERAND_8bit);
        x86_analyse_result->m_operands[1].setAsRegisterR(pState, OPERAND_8bit);
        break;
	case 0x39:
		x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
		x86_analyse_result->m_operands[1].setAsRegisterR(pState);
		break;
	case 0x3A:
		x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState, OPERAND_8bit);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState, OPERAND_8bit);
		break;
	case 0x3B:
		x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState);
		break;
	case 0x3C:
		{
			u8 immediate = 0;
            if (pState->pSession->readU8(pState->m_PC++, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;

			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(pState, REG_AL, OPERAND_8bit);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U8, immediate);
			break;
		}
	case 0x3D:
	{
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegister(pState, REG_EAX, OPERAND_16_32);
		x86_analyse_result->m_operands[1].setAsImmediate(pState, OPERAND_16_32);
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

	switch (currentByte)
	{
	case 0x84:
		x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState, OPERAND_8bit);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState, OPERAND_8bit);
		break;
	case 0x85:
		x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState);
		break;
	case 0xA9:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegister(pState, REG_EAX, OPERAND_16_32);
		x86_analyse_result->m_operands[1].setAsImmediate(pState, OPERAND_16_32);
		break;
	default:
		X86_DECODER_FAILURE("x86_opcode_test");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_add(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_ADD;

	switch (currentByte)
	{
	case 0x01:
		x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterRM(pState, OPERAND_16_32);
		x86_analyse_result->m_operands[1].setAsRegisterR(pState, OPERAND_16_32);
		break;
	case 0x03:
		x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState);
		break;
	case 0x04:
		x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState, OPERAND_8bit);
		x86_analyse_result->m_operands[1].setAsImmediate(pState, OPERAND_8bit);
		break;
    case 0x05:
        x86_analyse_result->m_numOperands = 2;
        x86_analyse_result->m_operands[0].setAsRegister(pState, REG_EAX, OPERAND_16_32);
        x86_analyse_result->m_operands[1].setAsImmediate(pState, OPERAND_16_32);
        break;
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_add");
	}
	return IGOR_SUCCESS;
}

igor_result x86_opcode_sub(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_SUB;

	switch (currentByte)
	{
    case 0x29:
        x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
        x86_analyse_result->m_numOperands = 2;
        x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
        x86_analyse_result->m_operands[1].setAsRegisterR(pState);
        break;
	case 0x2B:
		{
			x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterR(pState);
			x86_analyse_result->m_operands[1].setAsRegisterRM(pState);
			break;
		}
	case 0x2D:
		{
			u32 immediate = 0;
            if (pState->pSession->readU32(pState->m_PC, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;

			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(pState, REG_EAX, OPERAND_16_32);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U32, immediate);
			break;
		}
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_sub");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_or(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_OR;

	switch (currentByte)
	{
    case 0x09:
        x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
        x86_analyse_result->m_numOperands = 2;
        x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
        x86_analyse_result->m_operands[1].setAsRegisterR(pState);
        break;
	case 0x0B:
		x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState);
		break;
	case 0x0C:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegister(pState, REG_AL, OPERAND_8bit);
		x86_analyse_result->m_operands[1].setAsImmediate(pState, OPERAND_8bit);
		break;
	case 0x0D:
		{
			u32 immediate = 0;
            if (pState->pSession->readU32(pState->m_PC, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegister(pState, REG_EAX);
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

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	switch (currentByte)
	{
	case 0x31:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
		x86_analyse_result->m_operands[1].setAsRegisterR(pState);
		break;
	case 0x32:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState, OPERAND_8bit);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState, OPERAND_8bit);
		break;
	case 0x33:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState);
		break;
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_xor");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_imul(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_IMUL;

	switch (currentByte)
	{
	case 0x69:
		x86_analyse_result->m_numOperands = 3;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState);
		x86_analyse_result->m_operands[2].setAsImmediate(pState, OPERAND_16_32);
		break;
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_imul");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_and(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_AND;

	switch (currentByte)
	{
    case 0x23:
        x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
        x86_analyse_result->m_numOperands = 2;
        x86_analyse_result->m_operands[0].setAsRegisterR(pState);
        x86_analyse_result->m_operands[1].setAsRegisterRM(pState);
        break;
	case 0x25:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegister(pState, REG_EAX);
		x86_analyse_result->m_operands[1].setAsImmediate(pState);
		break;
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_and");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_inc(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_INC;

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsRegister(pState, (e_register)(currentByte & 7));

	return IGOR_SUCCESS;
}

igor_result x86_opcode_dec(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_DEC;

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsRegister(pState, (e_register)(currentByte & 7));

	return IGOR_SUCCESS;
}

igor_result x86_opcode_pop(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_POP;
	

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsRegister(pState, (e_register)(currentByte & 7));

	return IGOR_SUCCESS;
}

igor_result x86_opcode_push(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_PUSH;
	

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
		x86_analyse_result->m_operands[0].setAsRegister(pState, (e_register)(currentByte & 7));
		break;
	case 0x68:
		{
			u32 immediate = 0;
            if (pState->pSession->readU32(pState->m_PC, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;
			pState->m_PC += 4;

			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsImmediate(IMMEDIATE_U32, immediate);
			break;
		}
	case 0x6A:
		{
			u8 immediate = 0;
            if (pState->pSession->readU8(pState->m_PC++, immediate) != IGOR_SUCCESS)
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
    if (pState->pSession->readS8(pState->m_PC++, jumpTargetS8) != IGOR_SUCCESS)
	{
		return IGOR_FAILURE;
	}

    igorAddress jumpTarget = pState->m_PC;
    jumpTarget += jumpTargetS8;

	pState->pSession->add_code_analysis_task(jumpTarget);

	x86_analyse_result->m_numOperands = 1;
    x86_analyse_result->m_operands[0].setAsAddress(jumpTarget.offset);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_F6(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

	u8 variation = modRegRm.getREGRaw();

	switch (variation)
	{
		case 0:
		{
			u8 immediate = 0;
            if (pState->pSession->readU8(pState->m_PC++, immediate) != IGOR_SUCCESS)
				return IGOR_FAILURE;

			x86_analyse_result->m_mnemonic = INST_X86_TEST;
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterRM(modRegRm);
			x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U8, immediate);
			break;
		}
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_F6");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_F7(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	u8 variation = x86_analyse_result->m_mod_reg_rm.getREGRaw();

	switch (variation)
	{
		case 0:
			x86_analyse_result->m_mnemonic = INST_X86_TEST;
			x86_analyse_result->m_numOperands = 2;
			x86_analyse_result->m_operands[0].setAsRegisterRM(pState, OPERAND_16_32);
			x86_analyse_result->m_operands[1].setAsImmediate(pState, OPERAND_16_32);
			break;
		case 2:
			x86_analyse_result->m_mnemonic = INST_X86_NOT;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
			break;
		case 6:
			x86_analyse_result->m_mnemonic = INST_X86_DIV;
			x86_analyse_result->m_numOperands = 3;
			x86_analyse_result->m_operands[0].setAsRegister(pState, REG_EDX, OPERAND_16_32);
			x86_analyse_result->m_operands[1].setAsRegister(pState, REG_EAX, OPERAND_16_32);
			x86_analyse_result->m_operands[2].setAsRegisterRM(pState);
			break;
		default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_F7");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_cld(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
    x86_analyse_result->m_mnemonic = INST_X86_CLD;
    return IGOR_SUCCESS;
}

igor_result x86_opcode_FF(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	u8 variation = x86_analyse_result->m_mod_reg_rm.getREGRaw();

	switch (variation)
	{
		case 0:
		{
			x86_analyse_result->m_mnemonic = INST_X86_INC;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
			break;
		}
		case 1:
		{
			x86_analyse_result->m_mnemonic = INST_X86_DEC;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
			break;
		}
		case 2:
		{
			x86_analyse_result->m_mnemonic = INST_X86_CALL;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
			break;
		}
		case 4:
		{
			x86_analyse_result->m_mnemonic = INST_X86_JMP;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
			pState->m_analyzeResult = stop_analysis;
			break;
		}
		case 6:
		{
			x86_analyse_result->m_mnemonic = INST_X86_PUSH;
			x86_analyse_result->m_numOperands = 1;
			x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
			break;
		}
	default:
		X86_DECODER_FAILURE("Unhandled x86_opcode_F7");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_80(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	u8 immediate = 0;
    if (pState->pSession->readU8(pState->m_PC, immediate) != IGOR_SUCCESS)
		return IGOR_FAILURE;
	pState->m_PC += 1;

	u8 variation = x86_analyse_result->m_mod_reg_rm.getREGRaw();

	switch (variation)
	{
	case 0:
		x86_analyse_result->m_mnemonic = INST_X86_ADD;
		break;
	case 1:
		x86_analyse_result->m_mnemonic = INST_X86_OR;
		break;
	case 4:
		x86_analyse_result->m_mnemonic = INST_X86_AND;
		break;
	case 5:
		x86_analyse_result->m_mnemonic = INST_X86_SUB;
		break;
	case 7:
		x86_analyse_result->m_mnemonic = INST_X86_CMP;
		break;
	default:
		X86_DECODER_FAILURE("x86_opcode_80");
	}

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegisterRM(pState, OPERAND_8bit);
	x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U8, immediate);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_81(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	u32 immediate = 0;
    if (pState->pSession->readU32(pState->m_PC, immediate) != IGOR_SUCCESS)
		return IGOR_FAILURE;
	pState->m_PC += 4;

	u8 variation = x86_analyse_result->m_mod_reg_rm.getREGRaw();

	switch (variation)
	{
	case 0:
		x86_analyse_result->m_mnemonic = INST_X86_ADD;
		break;
	case 1:
		x86_analyse_result->m_mnemonic = INST_X86_OR;
		break;
	case 4:
		x86_analyse_result->m_mnemonic = INST_X86_AND;
		break;
	case 5:
		x86_analyse_result->m_mnemonic = INST_X86_SUB;
		break;
	case 6:
		x86_analyse_result->m_mnemonic = INST_X86_XOR;
		break;
	case 7:
		x86_analyse_result->m_mnemonic = INST_X86_CMP;
		break;
	default:
		X86_DECODER_FAILURE("x86_opcode_81");
	}

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegisterRM(pState);
	x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U32, immediate);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_83(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

	u8 immediate = 0;
    if (pState->pSession->readU8(pState->m_PC++, immediate) != IGOR_SUCCESS)
		return IGOR_FAILURE;

	u8 variation = modRegRm.getREGRaw();

	switch (variation)
	{
		case 0:
			x86_analyse_result->m_mnemonic = INST_X86_ADD;
			break;
		case 1:
			x86_analyse_result->m_mnemonic = INST_X86_OR;
			break;
		case 4:
			x86_analyse_result->m_mnemonic = INST_X86_AND;
			break;
		case 5:
			x86_analyse_result->m_mnemonic = INST_X86_SUB;
			break;
        case 6:
            x86_analyse_result->m_mnemonic = INST_X86_XOR;
            break;
		case 7:
			x86_analyse_result->m_mnemonic = INST_X86_CMP;
			break;
		default:
			X86_DECODER_FAILURE("x86_opcode_83");
	}

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegisterRM(modRegRm);
	x86_analyse_result->m_operands[1].setAsImmediate(IMMEDIATE_U8, immediate);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_F(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	u8 currentByteF = 0;

    if (pState->pSession->readU8(pState->m_PC++, currentByteF) != IGOR_SUCCESS)
	{
        Printer::log(M_INFO, "Unknown extended instruction byte %02x at %08llX", currentByte, pState->m_cpu_analyse_result->m_startOfInstruction.offset);
		return IGOR_FAILURE;
	}

	if (x86_opcode_table_0xf[currentByteF] == NULL)
	{
        Printer::log(M_INFO, "Failed extended instruction byte %02x at %08llX", currentByte, pState->m_cpu_analyse_result->m_startOfInstruction.offset);
		return IGOR_FAILURE;
	}

	return x86_opcode_table_0xf[currentByteF](pState, pX86State, currentByteF);
}

const t_x86_opcode x86_opcode_table[0x100] =
{
	// 0x00
	NULL,
	/* 0x01 */ &x86_opcode_add,
	NULL,
	/* 0x03 */ &x86_opcode_add,
	/* 0x04 */ &x86_opcode_add,
    /* 0x05 */ &x86_opcode_add,
	NULL,
	NULL,
	NULL,
    /* 0x09 */ &x86_opcode_or,
	NULL,
	/* 0x0B */ &x86_opcode_or,
	/* 0x0C */ &x86_opcode_or,
	/* 0x0D */ &x86_opcode_or,
	NULL,
	/* 0x0F */ &x86_opcode_F,

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
    /* 0x23 */ &x86_opcode_and,
	NULL,
	/* 0x25 */ &x86_opcode_and,
	NULL,
	NULL,
	NULL,
    /* 0x29 */ &x86_opcode_sub,
	NULL,
	/* 0x2B */ &x86_opcode_sub,
	NULL,
	/* 0x2D */ &x86_opcode_sub,
	NULL,
	NULL,

	// 0x30
	NULL,
	/* 0x31 */ &x86_opcode_xor,
	/* 0x32 */ &x86_opcode_xor,
	/* 0x33 */ &x86_opcode_xor,
	NULL,
	NULL,
	NULL,
	NULL,
    /* 0x38 */ &x86_opcode_cmp,
	/* 0x39 */ &x86_opcode_cmp,
	/* 0x3A */ &x86_opcode_cmp,
	/* 0x3B */ &x86_opcode_cmp,
	/* 0x3C */ &x86_opcode_cmp,
	/* 0x3D */ &x86_opcode_cmp,
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
	/* 0x48 */ &x86_opcode_dec,
	/* 0x49 */ &x86_opcode_dec,
	/* 0x4A */ &x86_opcode_dec,
	/* 0x4B */ &x86_opcode_dec,
	/* 0x4C */ &x86_opcode_dec,
	/* 0x4D */ &x86_opcode_dec,
	/* 0x4E */ &x86_opcode_dec,
	/* 0x4F */ &x86_opcode_dec,

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
	/* 0x69 */ &x86_opcode_imul,
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
	/* 0x80 */ &x86_opcode_80,
	/* 0x81 */ &x86_opcode_81,
	NULL,
	/* 0x83 */ &x86_opcode_83,
	/* 0x84 */ &x86_opcode_test,
	/* 0x85 */ &x86_opcode_test,
	NULL,
	/* 0x87 */ &x86_opcode_xchg,
	/* 0x88 */ &x86_opcode_mov,
	/* 0x89 */ &x86_opcode_mov,
	/* 0x8A */ &x86_opcode_mov,
	/* 0x8B */ &x86_opcode_mov,
	NULL,
	/* 0x8D */ &x86_opcode_lea,
	NULL,
	NULL,

	// 0x90
	/* 0x90 */ &x86_opcode_nop,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	/* 0x9C */ &x86_opcode_pushf,
	/* 0x9D */ &x86_opcode_popf,
	NULL,
	NULL,

	// 0xA0
	NULL,
	/* A1 */ &x86_opcode_mov,
	NULL,
	/* A3 */ &x86_opcode_mov,
	/* A4 */ &x86_opcode_movsb,
	/* A5 */ &x86_opcode_movsd,
	NULL,
	NULL,
	NULL,
	/* A9 */ &x86_opcode_test,
	NULL,
	/* AB */ &x86_opcode_stosd,
	NULL,
	NULL,
	NULL,
	NULL,

	// 0xB0
	/* B0 */ &x86_opcode_mov,
	/* B1 */ &x86_opcode_mov,
	/* B2 */ &x86_opcode_mov,
	/* B3 */ &x86_opcode_mov,
	/* B4 */ &x86_opcode_mov,
	/* B5 */ &x86_opcode_mov,
	/* B6 */ &x86_opcode_mov,
	/* B7 */ &x86_opcode_mov,
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
	/* C2 */ &x86_opcode_retn,
	/* C3 */ &x86_opcode_retn,
	NULL,
	NULL,
	/* C6 */ &x86_opcode_mov,
	/* C7 */ &x86_opcode_mov,
	NULL,
	/* C9 */ &x86_opcode_leave,
	NULL,
	NULL,
	/* CC */ &x86_opcode_int,
	/* CD */ &x86_opcode_int,
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
    /* 0xD9*/ &x86_opcode_d9,
    /* 0xDA*/ &x86_opcode_da,
	/* 0xDB*/ &x86_opcode_db,
    /* 0xDC*/ &x86_opcode_dc,
    /* 0xDD*/ &x86_opcode_dd,
    /* 0xDE*/ &x86_opcode_de,
    /* 0xDF*/ &x86_opcode_df,

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
	/* 0xF6 */ &x86_opcode_F6,
	/* 0xF7 */ &x86_opcode_F7,
	NULL,
	NULL,
	NULL,
	NULL,
    /* 0xFC */ &x86_opcode_cld,
	NULL,
	NULL,
	/* 0xFF */ &x86_opcode_FF,
};
