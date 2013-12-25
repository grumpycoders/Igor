#include "IgorDatabase.h"
#include "cpu_x86.h"
#include "cpu_x86_opcodes.h"

igor_result x86_opcode_setz(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_SETZ;

	s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

	x86_analyse_result->m_numOperands = 1;
	x86_analyse_result->m_operands[0].setAsRegister((e_register)modRegRm.getRM(), OPERAND_8bit);

	return IGOR_SUCCESS;
}

igor_result x86_opcode_movzx(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	x86_analyse_result->m_mnemonic = INST_X86_MOVZX;

	s_mod_reg_rm modRegRm = GET_MOD_REG_RM(pState);

	switch (currentByte)
	{
	case 0xB6:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegister(modRegRm.getREG());
		x86_analyse_result->m_operands[1].setAsRegisterRM(modRegRm, OPERAND_8bit);
		break;
	case 0xB7:
		x86_analyse_result->m_numOperands = 2;
		x86_analyse_result->m_operands[0].setAsRegister(modRegRm.getREG());
		x86_analyse_result->m_operands[1].setAsRegisterRM(modRegRm);
		break;
	default:
		X86_DECODER_FAILURE("x86_opcode_movzx");
	}

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

    pState->pAnalysis->igor_add_code_analysis_task(x86_analyse_result->m_operands[0].m_address.m_addressValue);

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
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
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
	NULL,
	NULL,
	NULL,
	NULL,
	/* 0x94 */ &x86_opcode_setz,
	NULL,
	NULL,
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
	/* 0xB6 */ &x86_opcode_movzx,
	/* 0xB7 */ &x86_opcode_movzx,
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