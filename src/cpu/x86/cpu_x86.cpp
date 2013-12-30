#include <map>

#include "IgorDatabase.h"
#include "cpu_x86.h"
#include "cpu_x86_opcodes.h"

#include <Printer.h>
using namespace Balau;

const char* registerName[5][8] =
{
	{ "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH" }, // 8bit
	{ "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" }, // 16bits
	{ "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI" }, // 32 bits
	{ "MM0", "MM1", "MM2", "MM3", "MM4", "MM5", "MM6", "MM7" }, // 64 bits
	{ "XMM0", "XMM1", "XMM2", "XMM3", "XMM4", "XMM5", "XMM6", "XMM7" }, // 128 bits
};

igor_result c_cpu_x86::analyze(s_analyzeState* pState)
{
	c_cpu_x86_state* pX86State = (c_cpu_x86_state*)pState->pCpuState;

	if (pX86State == NULL)
	{
		// grab the default state
		pX86State = &m_defaultState;
		pState->pCpuState = pX86State;
	}

	c_x86_analyse_result& result = *(c_x86_analyse_result*)pState->m_cpu_analyse_result;
	result.reset();

	result.m_startOfInstruction = pState->m_PC;
	result.m_instructionSize = 0;

	pState->m_cpu_analyse_result = &result;

	// handle prefixes
	bool bIsPrefix = false;
	u8 currentByte = 0;
	do 
	{
		bIsPrefix = false;
        if (pState->pAnalysis->readU8(pState->m_PC++, currentByte) != IGOR_SUCCESS)
		{
			return IGOR_FAILURE;
		}

		switch (currentByte)
		{
		case 0x64:
			bIsPrefix = true;
			result.m_segmentOverride = c_x86_analyse_result::SEGMENT_OVERRIDE_FS;
			break;
		case 0x66:
			bIsPrefix = true;
			result.m_sizeOverride = true;
			break;
		case 0xF0:
			bIsPrefix = true;
			result.m_lockPrefix = true;
			break;
		case 0xF2:
			bIsPrefix = true;
			result.m_repPrefixF2 = true;
			break;
		case 0xF3:
			bIsPrefix = true;
			result.m_repPrefixF3 = true;
			break;
		default:
			break;
		}
	} while (bIsPrefix);

	if (x86_opcode_table[currentByte] == NULL)
	{
		pState->m_PC = result.m_startOfInstruction;
		return IGOR_FAILURE;
	}

	try {
		if (x86_opcode_table[currentByte](pState, pX86State, currentByte) != IGOR_SUCCESS)
		{
			pState->m_PC = result.m_startOfInstruction;
			return IGOR_FAILURE;
		}
	}
	catch (X86AnalysisException & e) {
		pState->m_PC = result.m_startOfInstruction;
		return IGOR_FAILURE;
	}

	result.m_instructionSize = pState->m_PC - result.m_startOfInstruction;

	return IGOR_SUCCESS;
}

const char* c_cpu_x86::getRegisterName(e_operandSize size, u8 regIndex, bool sizeOverride)
{
	if (size == e_operandSize::OPERAND_Default)
		size = OPERAND_32bit;

	return registerName[(int)size][regIndex];
}

#define MKNAME(inst) std::make_pair(INST_X86_##inst, #inst)

static const std::map<e_x86_mnemonic, const char *> s_mnemonics {
    MKNAME(MOV),    MKNAME(CALL),   MKNAME(JMP),    MKNAME(PUSH),
    MKNAME(POP),    MKNAME(SUB),    MKNAME(AND),    MKNAME(CMP),
    MKNAME(JZ),     MKNAME(JNZ),    MKNAME(TEST),   MKNAME(NOT),
    MKNAME(XOR),    MKNAME(LEA),    MKNAME(INC),    MKNAME(DEC),
    MKNAME(LEAVE),  MKNAME(RETN),   MKNAME(OR),     MKNAME(SHL),
    MKNAME(SHR),    MKNAME(JO),     MKNAME(JNO),    MKNAME(JB),
    MKNAME(JNB),    MKNAME(JBE),    MKNAME(JNBE),   MKNAME(JS),
    MKNAME(JNS),    MKNAME(JP),     MKNAME(JNP),    MKNAME(JL),
    MKNAME(JNL),    MKNAME(JLE),    MKNAME(JNLE),   MKNAME(ADD),
	MKNAME(SETZ),	MKNAME(MOVZX),	MKNAME(CMPXCHG),MKNAME(INT),
	MKNAME(XCHG),	MKNAME(STOSD),	MKNAME(MOVSB),	MKNAME(MOVSD),
	MKNAME(DIV),	MKNAME(IMUL),

	MKNAME(PXOR),	MKNAME(MOVQ),	MKNAME(MOVDQA),	MKNAME(MOVUPS),
	MKNAME(MOVSS)
};

#undef MKNAME

const char* c_cpu_x86::getMnemonicName(e_x86_mnemonic mnemonic)
{
    auto t = s_mnemonics.find(mnemonic);

    if (t == s_mnemonics.end()) {
        Failure("Unknown x86 mnemonic in c_cpu_x86::getMnemonicName");
        return NULL;
    }

    return t->second;
}

igor_result c_cpu_x86::printInstruction(s_analyzeState* pState, Balau::String& instructionString)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;

	const char* mnemonicString = getMnemonicName(x86_analyse_result->m_mnemonic);
	
	instructionString.set("0x%08llX: ", x86_analyse_result->m_startOfInstruction);

	if (x86_analyse_result->m_lockPrefix)
	{
		instructionString.append("LOCK ");
	}

	if (x86_analyse_result->m_repPrefixF3)
	{
		instructionString.append("REP ");
	}

	instructionString.append("%s", mnemonicString);

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
			operandString.set("%s", getRegisterName(pOperand->m_register.m_operandSize, pOperand->m_register.m_registerIndex, x86_analyse_result->m_sizeOverride));
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

					if (pOperand->m_registerRM.m_mod_reg_rm.getMod() == 0)
					{
						u8 SIB_SCALE = pOperand->m_registerRM.m_mod_reg_rm.getSIBScale();
						u8 SIB_INDEX = pOperand->m_registerRM.m_mod_reg_rm.getSIBIndex();
						u8 multiplier = 1 << SIB_SCALE;

						const char* indexString = getRegisterName(pOperand->m_registerRM.m_operandSize, SIB_INDEX);

						operandString.set("0x%08llX[%s*%d]", pOperand->m_registerRM.m_mod_reg_rm.offset, indexString, multiplier);
					}
					else
					{
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

	return IGOR_SUCCESS;
}

void s_x86_operand::setAsRegisterRM(s_analyzeState* pState, e_operandSize size)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	m_type = type_registerRM;

	if (x86_analyse_result->m_mod_reg_rm.getMod() == MOD_DIRECT)
	{
		if (x86_analyse_result->m_sizeOverride)
		{
			m_registerRM.m_operandSize = x86_analyse_result->getAlternateOperandSize(size);
		}
		else
		{
			m_registerRM.m_operandSize = x86_analyse_result->getDefaultOperandSize(size);
		}
	}
	else
	{
		// should do the address override here
		m_registerRM.m_operandSize = x86_analyse_result->getDefaultAddressSize();
	}
	
	m_registerRM.m_mod_reg_rm = x86_analyse_result->m_mod_reg_rm;
}

void s_x86_operand::setAsRegister(s_analyzeState* pState, e_register registerIndex, e_operandSize size)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	m_type = type_register;

	if (x86_analyse_result->m_sizeOverride)
	{
		m_register.m_operandSize = x86_analyse_result->getAlternateOperandSize(size);
	}
	else
	{
		m_register.m_operandSize = x86_analyse_result->getDefaultOperandSize(size);
	}

	m_register.m_registerIndex = registerIndex;
}

void s_x86_operand::setAsRegisterR(s_analyzeState* pState, e_operandSize size)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	m_type = type_register;

	if (x86_analyse_result->m_sizeOverride)
	{
		m_registerRM.m_operandSize = x86_analyse_result->getAlternateOperandSize(size);
	}
	else
	{
		m_registerRM.m_operandSize = x86_analyse_result->getDefaultOperandSize(size);
	}

	m_register.m_registerIndex = x86_analyse_result->m_mod_reg_rm.getREG();
}

void s_x86_operand::setAsRegisterRM_XMM(s_analyzeState* pState, e_operandSize size)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	m_type = type_registerRM;

	if (x86_analyse_result->m_mod_reg_rm.getMod() == MOD_DIRECT)
	{
		if (x86_analyse_result->m_sizeOverride)
		{
			m_registerRM.m_operandSize = x86_analyse_result->getAlternateOperandSize(size);
		}
		else
		{
			m_registerRM.m_operandSize = x86_analyse_result->getDefaultOperandSize(size);
		}
	}
	else
	{
		// should do the address override here
		m_registerRM.m_operandSize = x86_analyse_result->getDefaultOperandSize(OPERAND_16_32);
	}

	m_registerRM.m_mod_reg_rm = x86_analyse_result->m_mod_reg_rm;
}

void s_x86_operand::setAsRegisterR_XMM(s_analyzeState* pState, e_operandSize size)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	m_type = type_register;

	if (x86_analyse_result->m_sizeOverride)
	{
		m_registerRM.m_operandSize = x86_analyse_result->getAlternateOperandSize(size);
	}
	else
	{
		m_registerRM.m_operandSize = x86_analyse_result->getDefaultOperandSize(size);
	}

	m_register.m_registerIndex = x86_analyse_result->m_mod_reg_rm.getREG();
}

void s_x86_operand::setAsAddressRel(s_analyzeState* pState, e_operandSize size, bool dereference)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	m_type = type_address;

	if (x86_analyse_result->m_sizeOverride)
	{
		size = x86_analyse_result->getAlternateOperandSize(size);
	}
	else
	{
		size = x86_analyse_result->getDefaultOperandSize(size);
	}

	s64 offset = 0;

	switch (size)
	{
	case OPERAND_32bit:
		{
			s32 immediate = 0;
            if (pState->pAnalysis->readS32(pState->m_PC, immediate) != IGOR_SUCCESS)
				throw X86AnalysisException("Failure in setAsImmediateRel!");

			pState->m_PC += 4;

			offset = immediate;
			break;
		}
	default:
		throw X86AnalysisException("Failure in setAsImmediateRel!");
	}

	u64 address = pState->m_PC + offset;

	m_address.m_addressValue = address;
	m_address.m_dereference = dereference;
}

void s_x86_operand::setAsImmediate(s_analyzeState* pState, e_operandSize size)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	m_type = type_immediate;

	if (x86_analyse_result->m_sizeOverride)
	{
		size = x86_analyse_result->getAlternateOperandSize(size);
	}
	else
	{
		size = x86_analyse_result->getDefaultOperandSize(size);
	}

	u64 immediateValue = 0;

	e_immediateSize immediateSize = IMMEDIATE_U32;

	switch (size)
	{
	case OPERAND_32bit:
		{
			u32 immediate = 0;
            if (pState->pAnalysis->readU32(pState->m_PC, immediate) != IGOR_SUCCESS)
				throw X86AnalysisException("Failure in setAsImmediate!");

			pState->m_PC += 4;

			immediateValue = immediate;
			immediateSize = IMMEDIATE_U32;
			break;
		}
	case OPERAND_16bit:
		{
			u16 immediate = 0;
            if (pState->pAnalysis->readU16(pState->m_PC, immediate) != IGOR_SUCCESS)
				throw X86AnalysisException("Failure in setAsImmediate!");

			pState->m_PC += 2;

			immediateValue = immediate;
			immediateSize = IMMEDIATE_U16;
			break;
		}
	case OPERAND_8bit:
		{
			u8 immediate = 0;
            if (pState->pAnalysis->readU8(pState->m_PC, immediate) != IGOR_SUCCESS)
				throw X86AnalysisException("Failure in setAsImmediate!");

			pState->m_PC += 1;

			immediateValue = immediate;
			immediateSize = IMMEDIATE_U8;
			break;
		}
	default:
		throw X86AnalysisException("Failure in setAsImmediate!");
	}

	m_immediate.m_immediateSize = immediateSize;
	m_immediate.m_immediateValue = immediateValue;
}