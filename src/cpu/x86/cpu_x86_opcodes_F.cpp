#include "IgorDatabase.h"
#include "cpu_x86.h"
#include "cpu_x86_opcodes.h"

igor_result x86_opcode_F_set(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	
    switch (currentByte)
    {
    case 0x94:
        x86_analyse_result->m_mnemonic = INST_X86_SETZ;
        break;
    case 0x95:
        x86_analyse_result->m_mnemonic = INST_X86_SETNZ;
        break;
    default:
        X86_DECODER_FAILURE("x86_opcode_F_set");
    }

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsRegisterRM(pState, OPERAND_8bit);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_movzx(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_MOVZX;

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	switch (currentByte)
	{
	case 0xB6:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState, OPERAND_8bit);
		break;
	case 0xB7:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState);
		break;
	default:
		X86_DECODER_FAILURE("x86_opcode_movzx");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_movsx(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_MOVSX;

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	switch (currentByte)
	{
	case 0xBE:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState, OPERAND_8bit);
		break;
	case 0xBF:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegisterR(pState);
		x86_analyse_result->m_operands[1].setAsRegisterRM(pState, OPERAND_16bit);
		break;
	default:
		X86_DECODER_FAILURE("x86_opcode_movsx");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_F_imul(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_IMUL;

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegisterR(pState);
	x86_analyse_result->m_operands[1].setAsRegisterRM(pState);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_cpuid(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_CPUID;

	return IGOR_SUCCESS;
}

igor_result x86_opcode_cmpxchg(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_CMPXCHG;

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);

	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegister(pState, REG_EAX, OPERAND_16_32);
	x86_analyse_result->m_operands[1].setAsRegisterR(pState);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_mmx_sse2_R_RM(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegisterR_XMM(pState);
	x86_analyse_result->m_operands[1].setAsRegisterRM_XMM(pState);

	switch (currentByte)
	{
	case 0xEF:
		x86_analyse_result->m_mnemonic = INST_X86_PXOR;
		break;
	default:
		X86_DECODER_FAILURE("x86_opcode_mmx_sse2_R_RM");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_mmx_sse2_RM_R(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegisterRM_XMM(pState);
	x86_analyse_result->m_operands[1].setAsRegisterR_XMM(pState);

	switch (currentByte)
	{
	case 0x7F:
		if (x86_analyse_result->m_sizeOverride)
		{
			x86_analyse_result->m_mnemonic = INST_X86_MOVDQA;
		}
		else
		{
			x86_analyse_result->m_mnemonic = INST_X86_MOVQ;
		}
		break;
	default:
		X86_DECODER_FAILURE("x86_opcode_mmx_sse2_RM_R");
	}

	return IGOR_SUCCESS;
}

igor_result x86_opcode_F_10(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegisterRM_XMM(pState);

	EAssert(x86_analyse_result->m_sizeOverride == false, "m_sizeOverride not implemented in x86_opcode_F_10");

	if (x86_analyse_result->m_repPrefixF2)
	{
		x86_analyse_result->m_mnemonic = INST_X86_MOVSD;
		x86_analyse_result->m_operands[1].setAsRegisterRM_XMM(pState, OPERAND_XMM_m64);
	}
	else
	if (x86_analyse_result->m_repPrefixF3)
	{
		x86_analyse_result->m_mnemonic = INST_X86_MOVUPS;
		x86_analyse_result->m_operands[1].setAsRegisterRM_XMM(pState, OPERAND_XMM_m32);
	}
	else
	{
		x86_analyse_result->m_mnemonic = INST_X86_MOVSS;
	}
	x86_analyse_result->m_repPrefixF3 = false;

	return IGOR_SUCCESS;
}

igor_result x86_opcode_F_jmp(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsAddressRel(pState);

	switch (currentByte)
	{
	case 0x80:
		x86_analyse_result->m_mnemonic = INST_X86_JO;
		break;
	case 0x81:
		x86_analyse_result->m_mnemonic = INST_X86_JNO;
		break;
	case 0x82:
		x86_analyse_result->m_mnemonic = INST_X86_JB;
		break;
	case 0x83:
		x86_analyse_result->m_mnemonic = INST_X86_JNB;
		break;
	case 0x84:
		x86_analyse_result->m_mnemonic = INST_X86_JZ;
		break;
	case 0x85:
		x86_analyse_result->m_mnemonic = INST_X86_JNZ;
		break;
	case 0x86:
		x86_analyse_result->m_mnemonic = INST_X86_JBE;
		break;
	case 0x87:
		x86_analyse_result->m_mnemonic = INST_X86_JNBE;
		break;
	case 0x88:
		x86_analyse_result->m_mnemonic = INST_X86_JS;
		break;
	case 0x89:
		x86_analyse_result->m_mnemonic = INST_X86_JNS;
		break;
	case 0x8A:
		x86_analyse_result->m_mnemonic = INST_X86_JP;
		break;
	case 0x8B:
		x86_analyse_result->m_mnemonic = INST_X86_JNP;
		break;
	case 0x8C:
		x86_analyse_result->m_mnemonic = INST_X86_JL;
		break;
	case 0x8D:
		x86_analyse_result->m_mnemonic = INST_X86_JNL;
		break;
	case 0x8E:
		x86_analyse_result->m_mnemonic = INST_X86_JLE;
		break;
	case 0x8F:
		x86_analyse_result->m_mnemonic = INST_X86_JNLE;
		break;
	default:
		X86_DECODER_FAILURE("x86_opcode_mmx_sse2_RM_R");
	}

    pState->pSession->add_code_analysis_task(x86_analyse_result->m_operands[0].m_address.m_addressValue);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_movq(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;

	x86_analyse_result->m_mod_reg_rm = GET_MOD_REG_RM(pState);
	x86_analyse_result->m_numOperands = 2;
	x86_analyse_result->m_operands[0].setAsRegisterRM_XMM(pState);
	x86_analyse_result->m_operands[1].setAsRegisterR_XMM(pState);
	x86_analyse_result->m_mnemonic = INST_X86_MOVQ;

	return IGOR_SUCCESS;
}

const t_x86_opcode x86_opcode_table_0xf[0x100] =
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
	/* 0x10 */ &x86_opcode_F_10,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
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

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
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

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	/* 0x7F */ &x86_opcode_mmx_sse2_RM_R,

	/* 0x80 */ &x86_opcode_F_jmp,
	/* 0x81 */ &x86_opcode_F_jmp,
	/* 0x82 */ &x86_opcode_F_jmp,
	/* 0x83 */ &x86_opcode_F_jmp,
	/* 0x84 */ &x86_opcode_F_jmp,
	/* 0x85 */ &x86_opcode_F_jmp,
	/* 0x86 */ &x86_opcode_F_jmp,
	/* 0x87 */ &x86_opcode_F_jmp,
	/* 0x88 */ &x86_opcode_F_jmp,
	/* 0x89 */ &x86_opcode_F_jmp,
	/* 0x8A */ &x86_opcode_F_jmp,
	/* 0x8B */ &x86_opcode_F_jmp,
	/* 0x8C */ &x86_opcode_F_jmp,
	/* 0x8D */ &x86_opcode_F_jmp,
	/* 0x8E */ &x86_opcode_F_jmp,
	/* 0x8F */ &x86_opcode_F_jmp,

	// 0x90
    /* 0x90 */ &x86_opcode_F_set,
    /* 0x91 */ &x86_opcode_F_set,
    /* 0x92 */ &x86_opcode_F_set,
    /* 0x93 */ &x86_opcode_F_set,
	/* 0x94 */ &x86_opcode_F_set,
    /* 0x95 */ &x86_opcode_F_set,
    /* 0x96 */ &x86_opcode_F_set,
    /* 0x97 */ &x86_opcode_F_set,
    /* 0x98 */ &x86_opcode_F_set,
    /* 0x99 */ &x86_opcode_F_set,
    /* 0x9A */ &x86_opcode_F_set,
    /* 0x9B */ &x86_opcode_F_set,
    /* 0x9C */ &x86_opcode_F_set,
    /* 0x9D */ &x86_opcode_F_set,
    /* 0x9E */ &x86_opcode_F_set,
    /* 0x9F */ &x86_opcode_F_set,

	// 0xA0
	NULL,
	NULL,
	/* 0xA2 */ &x86_opcode_cpuid,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	/* 0xAF */ &x86_opcode_F_imul,

	// 0xB0
	NULL,
	/* 0xB1 */ &x86_opcode_cmpxchg,
	NULL,
	NULL,
	NULL,
	NULL,
	/* 0xB6 */ &x86_opcode_movzx,
	/* 0xB7 */ &x86_opcode_movzx,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	/* 0xBE */ &x86_opcode_movsx,
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
	/* 0xD6 */ &x86_opcode_movq,
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
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	/* 0xEF*/ &x86_opcode_mmx_sse2_R_RM,

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