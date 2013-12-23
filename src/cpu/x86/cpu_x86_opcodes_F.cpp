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

igor_result x86_opcode_mmx_sse2(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte)
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
		X86_DECODER_FAILURE("x86_opcode_mmx_sse2");
	}

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
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

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
	NULL,
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
	/* 0xEF*/ &x86_opcode_mmx_sse2,

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