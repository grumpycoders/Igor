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

	u8 currentByte = 0;
	if (pState->pDataBase->readByte(pState->m_PC, currentByte) != IGOR_SUCCESS)
	{
		return IGOR_FAILURE;
	}

	c_x86_analyse_result result;

	result.m_PC = pState->m_PC;
	result.m_mnemonic = INST_X86_UNDEF;
	result.m_numOperands = 0;

	pState->m_cpu_analyse_result = &result;

	pState->m_PC++;

	if (x86_opcode_table[currentByte] == NULL)
	{
		return IGOR_FAILURE;
	}

	if (x86_opcode_table[currentByte](pState, pX86State, currentByte) != IGOR_SUCCESS)
	{
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
	case INST_X86_TEST:
		return "TEST";
	case INST_X86_NOT:
		return "NOT";
	case INST_X86_XOR:
		return "XOR";
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
	instructionString.set("0x%08llX: %s", x86_analyse_result->m_PC, mnemonicString);

	for (int i = 0; i < x86_analyse_result->m_numOperands; i++)
	{
		Balau::String operandString;
		s_x86_operand* pOperand = &x86_analyse_result->m_operands[i];

		switch (pOperand->m_type)
		{
		case s_x86_operand::type_register:
			if (pOperand->m_register.m_mod == MOD_DIRECT)
				operandString.set("%s", getRegisterName(pOperand->m_register.m_operandSize, pOperand->m_register.m_registerIndex));
			else
				operandString.set("[%s%+d]", getRegisterName(pOperand->m_register.m_operandSize, pOperand->m_register.m_registerIndex), pOperand->m_register.m_offset);
				
			break;
		case s_x86_operand::type_immediate:
			operandString.set("%Xh", pOperand->m_immediate.m_immediateValue);
			break;
		case s_x86_operand::type_address:
			if (pOperand->m_address.m_dereference)
			{
				operandString.set("[0x%08llX]", pOperand->m_address.m_addressValue);
			}
			else
			{
				operandString.set("0x%08llX", pOperand->m_address.m_addressValue);
			}
			break;
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

