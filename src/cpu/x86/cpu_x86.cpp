#include <map>

#include "IgorDatabase.h"
#include "IgorSession.h"
#include "cpu_x86.h"
#include "cpu_x86_opcodes.h"

#include <Printer.h>
using namespace Balau;

// this matches e_registerMode:
/*
REGISTER_r8 = 0,
REGISTER_r16 = 1,
REGISTER_r32 = 2,
REGISTER_r64 = 3,
REGISTER_mm = 4,
REGISTER_xmm = 5,
*/

const char* registerName32bitsMode[6][8] =
{
	{ "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH" }, // 8bit
	{ "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" }, // 16bits
	{ "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI" }, // 32 bits
	{ "", "", "", "", "", "", "", "" },
	{ "MM0", "MM1", "MM2", "MM3", "MM4", "MM5", "MM6", "MM7" }, // mm
	{ "XMM0", "XMM1", "XMM2", "XMM3", "XMM4", "XMM5", "XMM6", "XMM7" }, // xmm
};

const char* registerName64bitsREXMode[6][16] =
{
	{	"AL",	"CL",	"DL",	"BL",	"SPL",	"BPL",	"SIL",	"DIL",	"R8B",	"R9B",	"R10B",	"R11B",	"R12B",	"R13B",	"R14B",	"R15B"}, // 8bit
	{	"AX",	"CX",	"DX",	"BX",	"SP",	"BP",	"SI",	"DI",	"R8W",	"R9W",	"R10W",	"R11W",	"R12W",	"R13W",	"R14W",	"R15W"}, // 16bits
	{	"EAX",	"ECX",	"EDX",	"EBX",	"ESP",	"EBP",	"ESI",	"EDI",	"R8D",	"R9D",	"R10D",	"R11D",	"R12D",	"R13D",	"R14D",	"R15D"}, // 32 bits
	{	"RAX",	"RCX",	"RDX",	"RBX",	"RSP",	"RBP",	"RSI",	"RDI",	"R8",	"R9",	"R10",	"R11",	"R12",	"R13",	"R14",	"R15"}, // 64 bits
	{	"MM0",	"MM1",	"MM2",	"MM3",	"MM4",	"MM5",	"MM6",	"MM7",	"MM8",	"MM9",	"MM10",	"MM11",	"MM12",	"MM13",	"MM14",	"MM15" }, // mm
	{	"XMM0",	"XMM1",	"XMM2",	"XMM3",	"XMM4",	"XMM5",	"XMM6",	"XMM7",	"XMM8",	"XMM9",	"XMM10","XMM11","XMM12","XMM13","XMM14","XMM15"}, // xmm
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

	pState->m_cpu_analyse_result = &result;

	// handle prefixes
	bool bIsPrefix = false;
	u8 currentByte = 0;
	do 
	{
		bIsPrefix = false;
        if (pState->pSession->readU8(pState->m_PC++, currentByte) != IGOR_SUCCESS)
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

		if (pX86State->m_executionMode == c_cpu_x86_state::_64bits)
		{
			switch (currentByte)
			{
				case 0x48:
					bIsPrefix = true;
					result.m_Prefix64Bit_43 = true;
					break;
				default:
					break;
			}
		}
	} while (bIsPrefix);

	if (x86_opcode_table[currentByte] == NULL)
	{
		Printer::log(M_INFO, "Unknown instruction byte %02x at %08llX", currentByte, result.m_startOfInstruction);
		pState->m_PC = result.m_startOfInstruction;
		return IGOR_FAILURE;
	}

	try {
		if (x86_opcode_table[currentByte](pState, pX86State, currentByte) != IGOR_SUCCESS)
		{
			Printer::log(M_INFO, "Failed decoding instruction byte %02x at %08llX", currentByte, result.m_startOfInstruction.offset);
			pState->m_PC = result.m_startOfInstruction;
			return IGOR_FAILURE;
		}
	}
	catch (X86AnalysisException & e) {
        Printer::log(M_INFO, "Exception while decoding instruction byte %02x at %08llX", currentByte, result.m_startOfInstruction.offset);
		pState->m_PC = result.m_startOfInstruction;
		return IGOR_FAILURE;
	}

	result.m_instructionSize = pState->m_PC - result.m_startOfInstruction;

	return IGOR_SUCCESS;
}

void c_cpu_x86::generateReferences(s_analyzeState* pState)
{
    c_x86_analyse_result& result = *(c_x86_analyse_result*)pState->m_cpu_analyse_result;

    // generate references
    for (int i = 0; i < result.m_numOperands; i++)
    {
        generateReferences(pState, i);
    }
}

const char* c_cpu_x86::getRegisterName(s_analyzeState* pState, s_registerDefinition definition, u8 regIndex, bool sizeOverride)
{
	c_cpu_x86_state* pX86State = (c_cpu_x86_state*)pState->pCpuState;

	if (pX86State->m_executionMode == c_cpu_x86_state::_64bits)
	{
		return registerName64bitsREXMode[definition.m_mode][regIndex];
	}
	else
	{
		return registerName32bitsMode[definition.m_mode][regIndex];
	}

	return NULL;
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
	MKNAME(DIV), MKNAME(IMUL), MKNAME(FINIT), MKNAME(PUSHF),
	MKNAME(POPF), MKNAME(NOP), MKNAME(CPUID), MKNAME(MOVSX),
    MKNAME(SETNZ),
    MKNAME(CLD),
    MKNAME(ROL),
    MKNAME(ROR),
    MKNAME(RCL),
    MKNAME(RCR),
    MKNAME(SAL),
    MKNAME(SAR),
    MKNAME(FILD),
    MKNAME(FSTP),
    MKNAME(FLD),
    MKNAME(FSUB),
    MKNAME(FLDZ),
    MKNAME(FUCOMPP),
    MKNAME(FNSTSW),
    MKNAME(FXCH),
    MKNAME(FADDP),
    MKNAME(FDIVP),
    MKNAME(FDIV),
    MKNAME(FSTCW),
    MKNAME(FLDCW),
    MKNAME(FISTP),

	MKNAME(PXOR),	MKNAME(MOVQ),	MKNAME(MOVDQA),	MKNAME(MOVUPS),
	MKNAME(MOVSS), 
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

igor_result c_cpu_x86::getMnemonic(s_analyzeState* pState, Balau::String& outputString)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;

    const char* mnemonicString = getMnemonicName(x86_analyse_result->m_mnemonic);

    //instructionString.set("0x%08llX: ", x86_analyse_result->m_startOfInstruction);

    if (x86_analyse_result->m_lockPrefix)
    {
        outputString.append("LOCK ");
    }

    if (x86_analyse_result->m_repPrefixF3)
    {
        outputString.append("REP ");
    }

    outputString.append("%s", mnemonicString);

    return IGOR_SUCCESS;
}

int c_cpu_x86::getNumOperands(s_analyzeState* pState)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;

    return x86_analyse_result->m_numOperands;
}

void c_cpu_x86::generateReferences(s_analyzeState* pState, int operandIndex)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
    s_x86_operand* pOperand = &x86_analyse_result->m_operands[operandIndex];

	switch (pOperand->m_type)
	{
	case s_x86_operand::type_register:
		break;
	case s_x86_operand::type_registerRM:
	{
		if (pOperand->m_registerRM.m_mod_reg_rm.getMod() == MOD_DIRECT)
		{
		}
		else
		{
			u8 RMIndex = pOperand->m_registerRM.m_mod_reg_rm.getRM();
			if (RMIndex == 4)
			{
				// use SIB
				u8 SIB_BASE = pOperand->m_registerRM.m_mod_reg_rm.getSIBBase();

				if (pOperand->m_registerRM.m_mod_reg_rm.getSIBIndex() == 5)
				{
					EAssert(pOperand->m_registerRM.m_mod_reg_rm.getSIBIndex() != 4, "bad scaled index");
					u8 SIB_SCALE = pOperand->m_registerRM.m_mod_reg_rm.getSIBScale();
					u8 SIB_INDEX = pOperand->m_registerRM.m_mod_reg_rm.getSIBIndex();
					u8 multiplier = 1 << SIB_SCALE;

                    pState->pSession->addReference(igorAddress(pOperand->m_registerRM.m_mod_reg_rm.offset), pState->m_cpu_analyse_result->m_startOfInstruction);
				}
			}
			else
			if ((RMIndex == 5) && pOperand->m_registerRM.m_mod_reg_rm.getMod() == MOD_INDIRECT)
			{
                pState->pSession->addReference(igorAddress(pOperand->m_registerRM.m_mod_reg_rm.offset), pState->m_cpu_analyse_result->m_startOfInstruction);
			}
			else
			{
				//operandString.set("[%s%+d]", getRegisterName(pOperand->m_registerRM.m_operandSize, RMIndex), pOperand->m_registerRM.m_mod_reg_rm.offset);
			}
		}
		break;
	}
    case s_x86_operand::type_registerST:
        break;
	case s_x86_operand::type_immediate:
	{
//		operandString.set("%s%Xh", segmentString, pOperand->m_immediate.m_immediateValue);
		break;
	}
	case s_x86_operand::type_address:
	{
        pState->pSession->addReference(igorAddress(pOperand->m_address.m_addressValue), pState->m_cpu_analyse_result->m_startOfInstruction);
		break;
	}
	default:
		Failure("Bad operand type");
	}
}

igor_result c_cpu_x86::getOperand(s_analyzeState* pState, int operandIndex, Balau::String& operandString, bool bUseColor)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	c_cpu_x86_state* pX86State = (c_cpu_x86_state*)pState->pCpuState;

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

    s_x86_operand* pOperand = &x86_analyse_result->m_operands[operandIndex];

	switch (pOperand->m_type)
	{
	case s_x86_operand::type_register:
		operandString.set("%s", getRegisterName(pState, pOperand->m_register.m_registerDefinition, pOperand->m_register.m_registerIndex, x86_analyse_result->m_sizeOverride));
		break;
	case s_x86_operand::type_registerRM:
	{
		if (pOperand->m_registerRM.m_mod_reg_rm.getMod() == MOD_DIRECT)
		{
			operandString.set("%s", getRegisterName(pState, pOperand->m_registerRM.m_registerDefinition, pOperand->m_registerRM.m_mod_reg_rm.getRM()));
		}
		else
		{
			u8 RMIndex = pOperand->m_registerRM.m_mod_reg_rm.getRM();
			if (RMIndex == 4)
			{
				// use SIB
				u8 SIB_BASE = pOperand->m_registerRM.m_mod_reg_rm.getSIBBase();
				const char* baseString = getRegisterName(pState, pOperand->m_registerRM.m_registerDefinition, SIB_BASE);

				if (pOperand->m_registerRM.m_mod_reg_rm.getSIBIndex() == 5)
				{
					EAssert(pOperand->m_registerRM.m_mod_reg_rm.getSIBIndex() != 4, "bad scaled index");
					u8 SIB_SCALE = pOperand->m_registerRM.m_mod_reg_rm.getSIBScale();
					u8 SIB_INDEX = pOperand->m_registerRM.m_mod_reg_rm.getSIBIndex();
					u8 multiplier = 1 << SIB_SCALE;

					const char* indexString = getRegisterName(pState, pOperand->m_registerRM.m_registerDefinition, SIB_INDEX);

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

						const char* indexString = getRegisterName(pState, pOperand->m_registerRM.m_registerDefinition, SIB_INDEX);

						operandString.set("[%s+%s*%d+%d]", baseString, indexString, multiplier, pOperand->m_registerRM.m_mod_reg_rm.offset);
					}
				}
			}
			else
			if ((RMIndex == 5) && pOperand->m_registerRM.m_mod_reg_rm.getMod() == MOD_INDIRECT)
			{
				igorAddress effectiveAddress = igorAddress(pOperand->m_registerRM.m_mod_reg_rm.offset);

				if (pX86State->m_executionMode == c_cpu_x86_state::_64bits)
				{
					effectiveAddress += x86_analyse_result->m_startOfInstruction + x86_analyse_result->m_instructionSize;
				}

				Balau::String symbolName;
				if (pState->pSession->getSymbolName(effectiveAddress, symbolName))
				{
					operandString.set("%s[%s%s%s]", segmentString, startColor(KNOWN_SYMBOL, bUseColor), symbolName.to_charp(), finishColor(KNOWN_SYMBOL, bUseColor));
				}
				else
				{
					operandString.set("%s[0x%08llX]", segmentString, effectiveAddress.offset);
				}
			}
			else
			{
				operandString.set("[%s%+d]", getRegisterName(pState, pOperand->m_registerRM.m_registerDefinition, RMIndex), pOperand->m_registerRM.m_mod_reg_rm.offset);
			}
		}
		break;
	}
    case s_x86_operand::type_registerST:
        operandString.set("st(%d)", pOperand->m_registerST.m_registerIndex);
        break;
	case s_x86_operand::type_immediate:
	{
		operandString.set("%s%Xh", segmentString, pOperand->m_immediate.m_immediateValue);
		break;
	}
	case s_x86_operand::type_address:
	{
		Balau::String addressString;

        Balau::String symbolName;
        if (pState->pSession->getSymbolName(igorAddress(pOperand->m_address.m_addressValue), symbolName))
        {
            if (pOperand->m_address.m_dereference)
            {
                operandString.set("%s[%s%s%s]", segmentString, startColor(KNOWN_SYMBOL, bUseColor), symbolName.to_charp(), finishColor(KNOWN_SYMBOL, bUseColor));
            }
            else
            {
                operandString.set("%s%s%s%s", segmentString, startColor(KNOWN_SYMBOL, bUseColor), symbolName.to_charp(), finishColor(KNOWN_SYMBOL, bUseColor));
            }
        }
        else
        {
            if (pOperand->m_address.m_dereference)
            {
                operandString.set("%s[0x%08llX]", segmentString, pOperand->m_address.m_addressValue);
            }
            else
            {
                operandString.set("%s0x%08llX", segmentString, pOperand->m_address.m_addressValue);
            }
        }
		break;
	}
	default:
		Failure("Bad operand type");
	}

    return IGOR_SUCCESS;
}

igor_result c_cpu_x86::printInstruction(s_analyzeState* pState, Balau::String& instructionString, bool bUseColor)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;

    getMnemonic(pState, instructionString);

	for (int i = 0; i < x86_analyse_result->m_numOperands; i++)
	{
        Balau::String operandString;
        getOperand(pState, i, operandString, bUseColor);
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

void s_x86_operand::setAsRegisterRM(s_analyzeState* pState, operandDefinition size)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	m_type = type_registerRM;

	m_registerRM.m_registerDefinition.init(pState, size);
	m_registerRM.m_mod_reg_rm = x86_analyse_result->m_mod_reg_rm;
}

void s_x86_operand::setAsRegisterSTi(s_analyzeState* pState)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
    m_type = type_registerST;

    m_registerST.m_registerIndex = x86_analyse_result->m_mod_reg_rm.getRM();
}

void s_x86_operand::setAsRegisterST(s_analyzeState* pState)
{
    c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
    m_type = type_registerST;

    m_registerST.m_registerIndex = 0;
}

void s_registerDefinition::init(s_analyzeState* pState, operandDefinition inputDefinition)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	c_cpu_x86_state* pX86State = (c_cpu_x86_state*)pState->pCpuState;

	switch (inputDefinition & 0x0000FFFF)
	{
		case OPERAND_SIZE_8:
			m_size = OPERAND_8bit;
			break;
		case OPERAND_SIZE_16_32:
			// figure out if we are in 16, 32
			if (x86_analyse_result->m_sizeOverride)
			{
				m_size = OPERAND_16bit;
			}
			else
			{
				m_size = OPERAND_32bit;
			}
			break;
		case OPERAND_SIZE_16_32_64:
			// figure out if we are in 16, 32 or 64 bit
			if (x86_analyse_result->m_sizeOverride)
			{
				m_size = OPERAND_16bit;
			}
			else if (x86_analyse_result->m_Prefix64Bit_43)
			{
				EAssert(pX86State->m_executionMode == c_cpu_x86_state::_64bits);
				m_size = OPERAND_64bit;
			}
			else
			{
				m_size = OPERAND_32bit;
			}
			break;
		case OPERAND_SIZE_64_16:
			if (x86_analyse_result->m_sizeOverride)
			{
				m_size = OPERAND_16bit;
			}
			else
			{
				m_size = OPERAND_64bit;
			}
			break;
		default:
			Failure("");
	}
	switch (inputDefinition & 0xFFFF0000)
	{
		case OPERAND_REG:
			if (m_size == OPERAND_8bit)
				m_mode = REGISTER_r8;
			else if (m_size == OPERAND_16bit)
				m_mode = REGISTER_r16;
			else if (m_size == OPERAND_32bit)
				m_mode = REGISTER_r32;
			else
				m_mode = REGISTER_r64;
			break;
		default:
			Failure("");
	}
}

operandDefinition s_x86_operand::computeOperandSize(s_analyzeState* pState, operandDefinition size)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;

	if (x86_analyse_result->m_sizeOverride)
	{
		return x86_analyse_result->getAlternateOperandSize(size);
	}
	else
	{
		c_cpu_x86_state* pX86State = (c_cpu_x86_state*)pState->pCpuState;

		if (pX86State->m_executionMode == c_cpu_x86_state::_64bits)
		{
			if (x86_analyse_result->m_Prefix64Bit_43)
			{
				return x86_analyse_result->getAlternate2OperandSize(size);
			}
			else
			{
				return x86_analyse_result->getDefaultOperandSize(size);
			}
		}
		else
		{
			return x86_analyse_result->getDefaultOperandSize(size);
		}
	}
}

void s_x86_operand::setAsRegister(s_analyzeState* pState, e_register registerIndex, operandDefinition size)
{
	m_type = type_register;
	m_register.m_registerDefinition.init(pState, size);
	m_register.m_operandSize = computeOperandSize(pState, size);
	m_register.m_registerIndex = registerIndex;
}

void s_x86_operand::setAsRegisterR(s_analyzeState* pState, operandDefinition size)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	m_type = type_register;
	m_registerRM.m_registerDefinition.init(pState, size);
	m_registerRM.m_operandSize = computeOperandSize(pState, size);
	m_register.m_registerIndex = x86_analyse_result->m_mod_reg_rm.getREG();
}

void s_x86_operand::setAsRegisterRM_XMM(s_analyzeState* pState, operandDefinition size)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	m_type = type_registerRM;

	if (x86_analyse_result->m_mod_reg_rm.getMod() == MOD_DIRECT)
	{
		m_registerRM.m_operandSize = computeOperandSize(pState, size);
	}
	else
	{
		// should do the address override here
		m_registerRM.m_operandSize = x86_analyse_result->getDefaultOperandSize(OPERAND_16_32);
	}

	m_registerRM.m_mod_reg_rm = x86_analyse_result->m_mod_reg_rm;
}

void s_x86_operand::setAsRegisterR_XMM(s_analyzeState* pState, operandDefinition size)
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

void s_x86_operand::setAsAddressRel(s_analyzeState* pState, operandDefinition size, bool dereference)
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
            if (pState->pSession->readS32(pState->m_PC, immediate) != IGOR_SUCCESS)
				throw X86AnalysisException("Failure in setAsImmediateRel!");

			pState->m_PC += 4;

			offset = immediate;
			break;
		}
	default:
		throw X86AnalysisException("Failure in setAsImmediateRel!");
	}

    m_address.m_addressValue = pState->m_PC.offset + offset;
	m_address.m_dereference = dereference;
}

void s_x86_operand::setAsImmediate(s_analyzeState* pState, operandDefinition size)
{
	c_x86_analyse_result* x86_analyse_result = (c_x86_analyse_result*)pState->m_cpu_analyse_result;
	m_type = type_immediate;

	size = computeOperandSize(pState, size);

	u64 immediateValue = 0;

	e_immediateSize immediateSize = IMMEDIATE_U32;

	switch (size)
	{
	case OPERAND_64bit:
		{
			u64 immediate = 0;
			if (pState->pSession->readU64(pState->m_PC, immediate) != IGOR_SUCCESS)
				throw X86AnalysisException("Failure in setAsImmediate!");

			pState->m_PC += 8;

			immediateValue = immediate;
			immediateSize = IMMEDIATE_U64;
			break;
		}
	case OPERAND_32bit:
		{
			u32 immediate = 0;
            if (pState->pSession->readU32(pState->m_PC, immediate) != IGOR_SUCCESS)
				throw X86AnalysisException("Failure in setAsImmediate!");

			pState->m_PC += 4;

			immediateValue = immediate;
			immediateSize = IMMEDIATE_U32;
			break;
		}
	case OPERAND_16bit:
		{
			u16 immediate = 0;
            if (pState->pSession->readU16(pState->m_PC, immediate) != IGOR_SUCCESS)
				throw X86AnalysisException("Failure in setAsImmediate!");

			pState->m_PC += 2;

			immediateValue = immediate;
			immediateSize = IMMEDIATE_U16;
			break;
		}
	case OPERAND_8bit:
		{
			u8 immediate = 0;
            if (pState->pSession->readU8(pState->m_PC, immediate) != IGOR_SUCCESS)
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
