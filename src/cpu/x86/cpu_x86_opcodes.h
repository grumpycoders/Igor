#pragma once

typedef igor_result(*t_x86_opcode)(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte);
extern const t_x86_opcode x86_opcode_table[0x100];
extern const t_x86_opcode x86_opcode_table_0xf[0x100];

//e_operandSize getOperandSize(s_analyzeState* pState);
s_mod_reg_rm GET_MOD_REG_RM(s_analyzeState* pState);

#if defined(DEBUG) && defined(_MSC_VER) && 0
#define X86_DECODER_FAILURE(errorString) \
    __debugbreak(); \
    throw X86AnalysisException(errorString);
#else
#define X86_DECODER_FAILURE(errorString) \
    throw X86AnalysisException(errorString);
#endif