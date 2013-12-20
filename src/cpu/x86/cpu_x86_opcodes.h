#pragma once

typedef igor_result(*t_x86_opcode)(s_analyzeState* pState, c_cpu_x86_state* pX86State, u8 currentByte);
extern const t_x86_opcode x86_opcode_table[0x100];
