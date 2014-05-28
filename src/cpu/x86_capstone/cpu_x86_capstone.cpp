#include "IgorDatabase.h"
#include "IgorSession.h"

#include "cpu_x86_capstone.h"

c_cpu_x86_capstone::c_cpu_x86_capstone(cs_mode mode)
{
	cs_open(CS_ARCH_X86, mode, &m_csHandle);
	cs_option(m_csHandle, CS_OPT_DETAIL, 1);
}

c_cpu_x86_capstone::~c_cpu_x86_capstone()
{

}

igor_result c_cpu_x86_capstone::analyze(s_analyzeState* pState)
{
	u8 buffer[32];
	for (int i = 0; i < 32; i++)
	{
		buffer[i] = pState->pSession->readU8(pState->m_PC + i);
	}
	cs_insn *insn;
	int count = cs_disasm_ex(m_csHandle, buffer, 32, pState->m_PC.offset, 0, &insn);

	if (count)
	{
		pState->m_cpu_analyse_result->m_startOfInstruction.offset = insn[0].address;
		pState->m_cpu_analyse_result->m_instructionSize = insn[0].size;
		pState->m_PC += insn[0].size;

		// follow flow		
		{
			const char* flowFollow[] =
			{
				"call",
				"jmp",
				"jae",
				"jne",
				"je",
				"jbe",
				"jb",
			};

			for (int i = 0; i < sizeof(flowFollow) / sizeof(flowFollow[0]); i++)
			{
				if (strcmp(insn[0].mnemonic, flowFollow[i]) == 0)
				{
					if (insn[0].detail->x86.op_count == 1)
					{
						if (insn[0].detail->x86.operands[0].type == X86_OP_IMM)
						{
							pState->pSession->add_code_analysis_task(igorAddress(insn[0].detail->x86.operands[0].imm));
						}
						/*else if (insn[0].detail->x86.operands[0].type == X86_OP_MEM)
						{
							if (insn[0].detail->x86.operands[0].mem.base == X86_REG_RIP)
							{
								EAssert(insn[0].detail->x86.operands[0].mem.index == 0);
								EAssert(insn[0].detail->x86.operands[0].mem.scale == 1);

								pState->pSession->add_code_analysis_task(igorAddress(insn[0].address + insn[0].detail->x86.operands[0].mem.disp));
							}
						}*/
					}
				}
			}
		}

		// process end of flow (stop analysis)
		{
			const char* endOfFlow[] =
			{
				"jmp",
				"ret",
			};

			for (int i = 0; i < sizeof(endOfFlow) / sizeof(endOfFlow[0]); i++)
			{
				if (strcmp(insn[0].mnemonic, endOfFlow[i]) == 0)
				{
					pState->m_analyzeResult = stop_analysis;
				}
			}
		}

		cs_free(insn, count);
		return IGOR_SUCCESS;
	}

	return IGOR_FAILURE;
}

igor_result c_cpu_x86_capstone::printInstruction(s_analyzeState* pState, Balau::String& outputString, bool bUseColor)
{
	u8 buffer[32];
	for (int i = 0; i < pState->m_cpu_analyse_result->m_instructionSize; i++)
	{
		buffer[i] = pState->pSession->readU8(pState->m_cpu_analyse_result->m_startOfInstruction + i);
	}
	cs_insn *insn;
	int count = cs_disasm_ex(m_csHandle, buffer, pState->m_cpu_analyse_result->m_instructionSize, pState->m_cpu_analyse_result->m_startOfInstruction.offset, 0, &insn);

	if (count)
	{
		c_cpu_module::e_colors mnemonicColor = c_cpu_module::MNEMONIC_DEFAULT;

		Balau::String commentString;

		const char* flowControl[] =
		{
			"call",
			"jmp",
			"jae",
			"jne",
			"je",
			"jbe",
			"jb",
			"ret",
		};

		for (int i = 0; i < sizeof(flowControl) / sizeof(flowControl[0]); i++)
		{
			if (strcmp(insn[0].mnemonic, flowControl[i]) == 0)
			{
				mnemonicColor = c_cpu_module::MNEMONIC_FLOW_CONTROL;

				if (insn[0].detail->x86.op_count == 1)
				{
					if (insn[0].detail->x86.operands[0].type == X86_OP_MEM)
					{
						if (insn[0].detail->x86.operands[0].mem.base == X86_REG_RIP)
						{
							EAssert(insn[0].detail->x86.operands[0].mem.index == 0);
							EAssert(insn[0].detail->x86.operands[0].mem.scale == 1);

							igorAddress effectiveAddress(insn[0].address + insn[0].size + insn[0].detail->x86.operands[0].mem.disp);

							Balau::String symbolName;
							if (pState->pSession->getSymbolName(effectiveAddress, symbolName))
							{
								commentString.append("(%s%s%s)", startColor(KNOWN_SYMBOL), symbolName.to_charp(), finishColor(KNOWN_SYMBOL));
							}
							else
							{
								commentString.append("(0x%0llX)", effectiveAddress.offset);
							}
						}
					}
				}
			}
		}

		outputString.append("%s%s%s %s", startColor(mnemonicColor), insn[0].mnemonic, finishColor(mnemonicColor), insn[0].op_str);

		outputString.append(commentString);

		cs_free(insn, count);

		return IGOR_SUCCESS;
	}

	return IGOR_FAILURE;
}

void c_cpu_x86_capstone::printInstruction(c_cpu_analyse_result* result)
{

}

igor_result c_cpu_x86_capstone::getMnemonic(s_analyzeState* pState, Balau::String& outputString)
{
	return IGOR_SUCCESS;
}

int c_cpu_x86_capstone::getNumOperands(s_analyzeState* pState)
{
	return 0;
}

igor_result c_cpu_x86_capstone::getOperand(s_analyzeState* pState, int operandIndex, Balau::String& outputString, bool bUseColor)
{
	return IGOR_SUCCESS;
}

void c_cpu_x86_capstone::generateReferences(s_analyzeState* pState)
{

}
