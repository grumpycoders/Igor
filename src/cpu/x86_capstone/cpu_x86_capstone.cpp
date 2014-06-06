#include "IgorDatabase.h"
#include "IgorSession.h"

#include "cpu_x86_capstone.h"

extern "C" {
    const char *X86_reg_name(csh handle, unsigned int reg);
}


c_cpu_x86_capstone::c_cpu_x86_capstone(cs_mode mode)
{
    m_csMode = mode;
    cs_open(CS_ARCH_X86, mode, &m_csHandle);
    cs_option(m_csHandle, CS_OPT_DETAIL, 1);
}

c_cpu_x86_capstone::~c_cpu_x86_capstone()
{

}

Balau::String c_cpu_x86_capstone::getTag() const
{
    switch (m_csMode)
    {
    case CS_MODE_32:
        return "capstone_i386";
        break;
    case CS_MODE_64:
        return "capstone_x86_64";
        break;
    }

    Failure("unknown cpu");
    return "";
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
        cs_insn* pCurrentInstruction = &insn[0];

        pState->m_cpu_analyse_result->m_startOfInstruction = pState->m_PC;
        pState->m_cpu_analyse_result->m_instructionSize = pCurrentInstruction->size;
        pState->m_PC += pCurrentInstruction->size;

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
                if (strcmp(pCurrentInstruction->mnemonic, flowFollow[i]) == 0)
                {
                    if (pCurrentInstruction->detail->x86.op_count == 1)
                    {
                        if (pCurrentInstruction->detail->x86.operands[0].type == X86_OP_IMM)
                        {
                            igorAddress targetAddress(pState->pSession, pCurrentInstruction->detail->x86.operands[0].imm, -1);
                            pState->pSession->add_code_analysis_task(targetAddress);
                        }
                        /*else if (pCurrentInstruction->detail->x86.operands[0].type == X86_OP_MEM)
                        {
                            if (pCurrentInstruction->detail->x86.operands[0].mem.base == X86_REG_RIP)
                            {
                                EAssert(pCurrentInstruction->detail->x86.operands[0].mem.index == 0);
                                EAssert(pCurrentInstruction->detail->x86.operands[0].mem.scale == 1);

                                pState->pSession->add_code_analysis_task(igorAddress(pCurrentInstruction->address + pCurrentInstruction->detail->x86.operands[0].mem.disp));
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
                if (strcmp(pCurrentInstruction->mnemonic, endOfFlow[i]) == 0)
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

        cs_insn* pCurrentInstruction = &insn[0];
        /*
        for (int i = 0; i < sizeof(flowControl) / sizeof(flowControl[0]); i++)
        {
            if (strcmp(pCurrentInstruction->mnemonic, flowControl[i]) == 0)
            {
                mnemonicColor = c_cpu_module::MNEMONIC_FLOW_CONTROL;

                if (pCurrentInstruction->detail->x86.op_count == 1)
                {
                    if (pCurrentInstruction->detail->x86.operands[0].type == X86_OP_MEM)
                    {
                        if (pCurrentInstruction->detail->x86.operands[0].mem.base == X86_REG_RIP)
                        {
                            EAssert(pCurrentInstruction->detail->x86.operands[0].mem.index == 0, "Wrong index");
                            EAssert(pCurrentInstruction->detail->x86.operands[0].mem.scale == 1, "Wrong scale");

                            igorAddress effectiveAddress(pCurrentInstruction->address + pCurrentInstruction->size + pCurrentInstruction->detail->x86.operands[0].mem.disp);

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
        }*/

        outputString.append("%s%s%s ", startColor(mnemonicColor), pCurrentInstruction->mnemonic, finishColor(mnemonicColor));
        /*
        for (int operandIndex = 0; operandIndex < pCurrentInstruction->detail->x86.op_count; operandIndex++)
        {
            if (operandIndex != 0)
            {
                outputString.append(", ");
            }
            cs_x86_op* pOperand = &pCurrentInstruction->detail->x86.operands[operandIndex];

            switch (pOperand->type)
            {
                case X86_OP_REG:
                {
                    const char* regName = X86_reg_name(m_csHandle, pOperand->reg);

                    outputString.append("%s%s%s", startColor(c_cpu_module::OPERAND_REGISTER), regName, finishColor(c_cpu_module::OPERAND_REGISTER));

                    break;
                }

                case X86_OP_IMM:
                {
                    outputString.append("%s0x%llX%s", startColor(c_cpu_module::OPERAND_IMMEDIATE), pOperand->imm, finishColor(c_cpu_module::OPERAND_IMMEDIATE));

                    break;
                }

                case X86_OP_MEM:
                {
                    break;
                }

                default:
                    Failure("Unimplemented operand type");
            }
        }
        

        outputString.append("       ");
        */
        outputString.append(pCurrentInstruction->op_str);
        //outputString.append(commentString);

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

namespace {

class c_cpu_x86_capstone_factory : public c_cpu_factory
{
    virtual c_cpu_module* maybeCreateCpu(const Balau::String & cpuString) override
    {
        if (cpuString == "capstone_i386")
        {
            return new c_cpu_x86_capstone(CS_MODE_32);
        }
        else if (cpuString == "capstone_x86_64")
        {
            return new c_cpu_x86_capstone(CS_MODE_64);
        }

        return NULL;
    }
};

};

static c_cpu_x86_capstone_factory s_cpu_x86_capstone_factory;
