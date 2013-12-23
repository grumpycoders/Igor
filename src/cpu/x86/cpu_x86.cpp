#include "IgorDatabase.h"
#include "cpu_x86.h"
#include "cpu_x86_opcodes.h"

#include <Printer.h>
using namespace Balau;

const char* registerName[3][8] =
{
	{ "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH" }, // 8bit
	{ "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" }, // 16bits
	{ "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI" }, // 32 bits
};

igor_result c_cpu_x86::analyze(s_analyzeState* pState)
{
	c_cpu_x86_state* pX86State = (c_cpu_x86_state*)pState->pCpuState;

	if (pX86State == NULL)
	{
		// grab the default state
		pX86State = &m_defaultState;
	}

	c_x86_analyse_result result;

	result.m_startOfInstruction = pState->m_PC;

	pState->m_cpu_analyse_result = &result;

	// handle prefixes
	bool bIsPrefix = false;
	u8 currentByte = 0;
	do 
	{
		bIsPrefix = false;
		if (pState->pDataBase->readByte(pState->m_PC++, currentByte) != IGOR_SUCCESS)
		{
			return IGOR_FAILURE;
		}

		switch (currentByte)
		{
		case 0x64:
			bIsPrefix = true;
			result.m_segmentOverride = c_x86_analyse_result::SEGMENT_OVERRIDE_FS;
			break;
		default:
			break;
		}
	} while (bIsPrefix);

	if (x86_opcode_table[currentByte] == NULL)
	{
		return IGOR_FAILURE;
	}

	try {
		x86_opcode_table[currentByte](pState, pX86State, currentByte);
	}
	catch (X86AnalysisException & e) {
		return IGOR_FAILURE;
	}

	printInstruction(&result);

	return IGOR_SUCCESS;
}

const char* c_cpu_x86::getRegisterName(e_operandSize size, u8 regIndex)
{
	return registerName[(int)size][regIndex];
}

const char* c_cpu_x86::getMnemonicName(e_x86_mnemonic mnemonic)
{
	// TODO: better that a switch case

	switch (mnemonic)
	{
	case INST_X86_CALL:
		return "CALL";
	case INST_X86_JMP:
		return "JUMP";
	case INST_X86_MOV:
		return "MOV";
	case INST_X86_PUSH:
		return "PUSH";
	case INST_X86_SUB:
		return "SUB";
	case INST_X86_AND:
		return "AND";
	case INST_X86_CMP:
		return "CMP";
	case INST_X86_JZ:
		return "JZ";
	case INST_X86_JNZ:
		return "JNZ";
	case INST_X86_TEST:
		return "TEST";
	case INST_X86_NOT:
		return "NOT";
	case INST_X86_XOR:
		return "XOR";
	case INST_X86_LEA:
		return "LEA";
	case INST_X86_INC:
		return "INC";
	case INST_X86_POP:
		return "POP";
	case INST_X86_LEAVE:
		return "LEAVE";
	case INST_X86_RETN:
		return "RETN";
	default:
		Failure("Unknown x86 mnemonic in c_cpu_x86::getMnemonicName");
	}

	return NULL;
}

void c_cpu_x86::printInstruction(c_cpu_analyse_result* result)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)result;

	const char* mnemonicString = getMnemonicName(x86_analyse_result->m_mnemonic);
	
	Balau::String instructionString;
	instructionString.set("0x%08llX: %s", x86_analyse_result->m_startOfInstruction, mnemonicString);

	const char* segmentString = "";

	switch (x86_analyse_result->m_segmentOverride)
	{
	case c_x86_analyse_result::SEGMENT_OVERRIDE_NONE:
		break;
	case c_x86_analyse_result::SEGMENT_OVERRIDE_FS:
		segmentString = "fs:";
		break;
	default:
		Failure("unknown semgment override");
	}

	for (int i = 0; i < x86_analyse_result->m_numOperands; i++)
	{
		Balau::String operandString;
		s_x86_operand* pOperand = &x86_analyse_result->m_operands[i];

		switch (pOperand->m_type)
		{
		case s_x86_operand::type_register:
			operandString.set("%s", getRegisterName(pOperand->m_register.m_operandSize, pOperand->m_register.m_registerIndex));
			break;
		case s_x86_operand::type_registerRM:
		{
			if (pOperand->m_registerRM.m_mod_reg_rm.getMod() == MOD_DIRECT)
			{
				operandString.set("%s", getRegisterName(pOperand->m_registerRM.m_operandSize, pOperand->m_registerRM.m_mod_reg_rm.getRM()));
			}
			else
			{
				u8 RMIndex = pOperand->m_registerRM.m_mod_reg_rm.getRM();
				if (RMIndex == 4)
				{
					// use SIB
					u8 SIB_BASE = pOperand->m_registerRM.m_mod_reg_rm.getSIBBase();
					const char* baseString = getRegisterName(pOperand->m_registerRM.m_operandSize, SIB_BASE);

					if (pOperand->m_registerRM.m_mod_reg_rm.getSIBIndex() == 4)
					{
						operandString.set("[%s+%d]", baseString, pOperand->m_registerRM.m_mod_reg_rm.offset);
					}
					else
					{
						u8 SIB_SCALE = pOperand->m_registerRM.m_mod_reg_rm.getSIBScale();
						u8 SIB_INDEX = pOperand->m_registerRM.m_mod_reg_rm.getSIBIndex();
						u8 multiplier = 1 << SIB_SCALE;

						const char* indexString = getRegisterName(pOperand->m_registerRM.m_operandSize, SIB_INDEX);

						operandString.set("[%s+%s*%d+%d]", baseString, indexString, multiplier, pOperand->m_registerRM.m_mod_reg_rm.offset);
					}
				}
				else
				if ((RMIndex == 5) && pOperand->m_registerRM.m_mod_reg_rm.getMod() == MOD_INDIRECT)
				{
					operandString.set("%s[0x%08llX]", segmentString, pOperand->m_registerRM.m_mod_reg_rm.offset);
				}
				else
				{
					operandString.set("[%s%+d]", getRegisterName(pOperand->m_registerRM.m_operandSize, RMIndex), pOperand->m_registerRM.m_mod_reg_rm.offset);
				}
			}
			break;
		}
		case s_x86_operand::type_immediate:
		{
			operandString.set("%s%Xh", segmentString, pOperand->m_immediate.m_immediateValue);
			break;
		}
		case s_x86_operand::type_address:
		{
			Balau::String addressString;

			if (pOperand->m_address.m_dereference)
			{
				operandString.set("%s[0x%08llX]", segmentString, pOperand->m_address.m_addressValue);
			}
			else
			{
				operandString.set("%s0x%08llX", segmentString, pOperand->m_address.m_addressValue);
			}
			break;
		}
		default:
			Failure("Bad operand type");
		}

		if (i == 0)
		{
			instructionString += " ";
		}
		else
		{
			instructionString += ", ";
		}

		instructionString += operandString;
	}

	Printer::log(M_INFO, instructionString);
}
