#pragma once

#include "cpu/cpuModule.h"
#include "Exceptions.h"

class c_cpu_x86;

class X86AnalysisException : public Balau::GeneralException {
public:
    X86AnalysisException(Balau::String fn) : GeneralException(fn) { }
protected:
    X86AnalysisException() { }
};

class c_cpu_x86_state : public c_cpu_state
{
public:
    enum e_executionMode
    {
        undefined,
        _16bits,
        _32bits,
        _64bits,
    };

    c_cpu_x86_state() :
        m_executionMode(undefined)
    {

    }

    e_executionMode m_executionMode;
};

enum e_x86_mnemonic
{
    INST_X86_UNDEF,

    INST_X86_MOV,
    INST_X86_CALL,
    INST_X86_JMP,
    INST_X86_PUSH,
    INST_X86_POP,
    INST_X86_SUB,
    INST_X86_AND,
    INST_X86_CMP,
    INST_X86_JZ,
    INST_X86_JNZ,
    INST_X86_TEST,
    INST_X86_NOT,
    INST_X86_XOR,
    INST_X86_LEA,
    INST_X86_INC,
    INST_X86_DEC,
    INST_X86_LEAVE,
    INST_X86_RETN,
    INST_X86_OR,
    INST_X86_ROL,
    INST_X86_ROR,
    INST_X86_RCL,
    INST_X86_RCR,
    INST_X86_SAL,
    INST_X86_SAR,
    INST_X86_SHL,
    INST_X86_SHR,
    INST_X86_JO,
    INST_X86_JNO,
    INST_X86_JB,
    INST_X86_JNB,
    INST_X86_JBE,
    INST_X86_JNBE,
    INST_X86_JS,
    INST_X86_JNS,
    INST_X86_JP,
    INST_X86_JNP,
    INST_X86_JL,
    INST_X86_JNL,
    INST_X86_JLE,
    INST_X86_JNLE,
    INST_X86_ADD,
    INST_X86_SETZ,
    INST_X86_SETNZ,
    INST_X86_MOVZX,
    INST_X86_MOVSX,
    INST_X86_CMPXCHG,
    INST_X86_INT,
    INST_X86_XCHG,
    INST_X86_STOSD,
    INST_X86_MOVSD,
    INST_X86_MOVSB,
    INST_X86_DIV,
    INST_X86_IMUL,
    INST_X86_FINIT,
    INST_X86_PUSHF,
    INST_X86_POPF,
    INST_X86_NOP,
    INST_X86_CPUID,
    INST_X86_CLD,
    INST_X86_FILD,
    INST_X86_FSTP,
    INST_X86_FLD,
    INST_X86_FSUB,
    INST_X86_FLDZ,
    INST_X86_FUCOMPP,
    INST_X86_FNSTSW,
    INST_X86_FXCH,
    INST_X86_FADDP,
    INST_X86_FDIVP,
    INST_X86_FDIV,
    INST_X86_FSTCW,
    INST_X86_FLDCW,
    INST_X86_FISTP,

    INST_X86_PXOR,
    INST_X86_MOVQ,
    INST_X86_MOVDQA,
    INST_X86_MOVUPS,
    INST_X86_MOVSS,
};

// !!!! this has to match the register list registerName in cpu_x86.cpp
enum e_register
{
    // 8 bit
    REG_AL = 0,
    REG_CL = 1,
    REG_DL = 2,
    REG_BL = 3,
    REG_AH = 4,
    REG_CH = 5,
    REG_DH = 6,
    REG_BH = 7,

    // 16 bit
    REG_AX = 0,
    REG_CX = 1,
    REG_DX = 2,
    REG_BX = 3,
    REG_SP = 4,
    REG_BP = 5,
    REG_SI = 6,
    REG_DI = 7,

    // 32 bit
    REG_EAX = 0,
    REG_ECX = 1,
    REG_EDX = 2,
    REG_EBX = 3,
    REG_ESP = 4,
    REG_EBP = 5,
    REG_ESI = 6,
    REG_EDI = 7,
};

enum e_registerMode
{
    REGISTER_r8 = 0,
    REGISTER_r16 = 1,
    REGISTER_r32 = 2,
    REGISTER_r64 = 3,
    REGISTER_mm = 4,
    REGISTER_xmm = 5,
};

enum e_operandSize
{
    OPERAND_Default = -2,
    OPERAND_Unkown = -1,

    OPERAND_8bit = 0,
    OPERAND_16bit = 1,
    OPERAND_32bit = 2,
    OPERAND_64bit = 3,
    OPERAND_128bit = 4,
};

#define OPERAND_SIZE_8 (1<<e_operandSize::OPERAND_8bit)
#define OPERAND_SIZE_16 (1<<e_operandSize::OPERAND_16bit)
#define OPERAND_SIZE_32 (1<<e_operandSize::OPERAND_32bit)
#define OPERAND_SIZE_64 (1<<e_operandSize::OPERAND_64bit)
#define OPERAND_SIZE_128 (1<<e_operandSize::OPERAND_128bit)

#define OPERAND_SIZE_16_32 (OPERAND_SIZE_16 | OPERAND_SIZE_32)
#define OPERAND_SIZE_16_32_64 (OPERAND_SIZE_16 | OPERAND_SIZE_32 | OPERAND_SIZE_64)
#define OPERAND_SIZE_64_16 (OPERAND_SIZE_64 | OPERAND_SIZE_16)
#define OPERAND_SIZE_64_128 (OPERAND_SIZE_64 | OPERAND_SIZE_128)

#define OPERAND_REG (1 << (16+0))
#define OPERAND_REG_MM (1 << (16+1))
#define OPERAND_REG_XMM (1 << (16+2))

typedef u32 operandDefinition;

#define OPERAND_8 (operandDefinition)(OPERAND_REG | OPERAND_SIZE_8)
#define OPERAND_16_32 (operandDefinition)(OPERAND_REG | OPERAND_SIZE_16_32)
#define OPERAND_16_32_64 (operandDefinition)(OPERAND_REG |OPERAND_SIZE_16_32_64)
#define OPERAND_64_16 (operandDefinition)(OPERAND_REG |OPERAND_SIZE_64_16)
#define OPERAND_XMM_m32 (operandDefinition)(OPERAND_REG_MM | OPERAND_SIZE_32)
#define OPERAND_XMM_m64 (operandDefinition)(OPERAND_REG_MM | OPERAND_SIZE_64)
#define OPERAND_XMM_64_128 (operandDefinition)(OPERAND_REG_XMM | OPERAND_SIZE_64_128)

struct s_registerDefinition
{
    void init(s_analyzeState* pState, operandDefinition inputDefinition);

    e_operandSize m_size;
    e_registerMode m_mode;
};

enum e_immediateSize
{
    IMMEDIATE_S8,
    IMMEDIATE_U8,
    IMMEDIATE_S16,
    IMMEDIATE_U16,
    IMMEDIATE_S32,
    IMMEDIATE_U32,
    IMMEDIATE_U64,
    IMMEDIATE_S64,
};

enum e_mod
{
    MOD_INDIRECT = 0,
    MOD_INDIRECT_ADD_8 = 1,
    MOD_INDIRECT_ADD_32 = 2,
    MOD_DIRECT,
};

struct s_mod_reg_rm
{
    u8 RAW_VALUE;
    u8 SIB;
    s64 offset;

    u8 getREGRaw()
    {
        return ((RAW_VALUE >> 3) & 7);
    }
    e_register getREG()
    {
        return (e_register)((RAW_VALUE >> 3) & 7);
    }

    e_mod getMod()
    {
        return (e_mod)((RAW_VALUE >> 6) & 3);
    }
    u8 getRM()
    {
        return RAW_VALUE & 7;
    }

    u8 getSIBScale()
    {
        return (SIB >> 6) & 3;
    }

    u8 getSIBIndex()
    {
        return (SIB >> 3) & 7;
    }

    u8 getSIBBase()
    {
        return (SIB >> 0) & 7;
    }
};

struct s_x86_operand
{
    enum type
    {
        type_register,
        type_registerRM,
        type_registerST,
        type_immediate,
        type_address,
    } m_type;

    struct register_t
    {
        s_registerDefinition m_registerDefinition;
        operandDefinition m_operandSize;
        u16 m_registerIndex;
    };

    struct registerRM_t
    {
        s_registerDefinition m_registerDefinition;
        operandDefinition m_operandSize;
        s_mod_reg_rm m_mod_reg_rm;
    };

    struct registerST_t
    {
        u8 m_registerIndex;
    };

    struct immediate_t
    {
        e_immediateSize m_immediateSize;
        u64 m_immediateValue; // will need to extent that at some point for 128bit
    };

    struct address_t
    {
        u8 m_segment;
        u64 m_addressValue;
        bool m_dereference;
    };
    
    union
    {
        register_t m_register;
        registerRM_t m_registerRM;
        registerST_t m_registerST;
        immediate_t m_immediate;
        address_t m_address;
    };

    operandDefinition computeOperandSize(s_analyzeState* pState, operandDefinition size);

    void setAsRegister(s_analyzeState* pState, e_register registerIndex, operandDefinition size = OPERAND_16_32_64);

    void setAsRegisterRM(s_analyzeState* pState, s_mod_reg_rm modRegRm, operandDefinition size = OPERAND_16_32_64)
    {
        m_type = type_registerRM;
        m_registerRM.m_registerDefinition.init(pState, size);
        m_registerRM.m_operandSize = size;
        m_registerRM.m_mod_reg_rm = modRegRm;
    }

    void setAsRegisterRM(s_analyzeState* pState, operandDefinition definition = OPERAND_16_32_64);
    void setAsRegisterR(s_analyzeState* pState, operandDefinition definition = OPERAND_16_32_64);
    void setAsRegisterRM_XMM(s_analyzeState* pState, operandDefinition size = OPERAND_XMM_64_128);
    void setAsRegisterR_XMM(s_analyzeState* pState, operandDefinition size = OPERAND_XMM_64_128);

    void setAsRegisterSTi(s_analyzeState* pState);
    void setAsRegisterST(s_analyzeState* pState);

    void setAsImmediate(e_immediateSize size, u8 immediateValue)
    {
        m_type = type_immediate;
        m_immediate.m_immediateSize = size;
        m_immediate.m_immediateValue = immediateValue;
    }

    void setAsImmediate(e_immediateSize size, u16 immediateValue)
    {
        m_type = type_immediate;
        m_immediate.m_immediateSize = size;
        m_immediate.m_immediateValue = immediateValue;
    }

    void setAsImmediate(e_immediateSize size, u32 immediateValue)
    {
        m_type = type_immediate;
        m_immediate.m_immediateSize = size;
        m_immediate.m_immediateValue = immediateValue;
    }

    void setAsAddressRel(s_analyzeState* pState, operandDefinition size = OPERAND_16_32, bool dereference = false);
    void setAsImmediate(s_analyzeState* pState, operandDefinition size = OPERAND_16_32);

    void setAsAddress(u64 address, bool dereference = false)
    {
        m_type = type_address;
        m_address.m_addressValue = address;
        m_address.m_dereference = dereference;
    }
};

#define X86_MAX_OPERAND 4

class c_x86_analyse_result : public c_cpu_analyse_result
{
public:
    enum e_segment_override
    {
        SEGMENT_OVERRIDE_NONE,
        SEGMENT_OVERRIDE_CS,
        SEGMENT_OVERRIDE_SS,
        SEGMENT_OVERRIDE_DS,
        SEGMENT_OVERRIDE_ES,
        SEGMENT_OVERRIDE_FS,
        SEGMENT_OVERRIDE_GS,
    };

    c_x86_analyse_result()
    {
        reset();
    }

    void reset()
    {
        m_instructionSize = 0;
        m_startOfInstruction = IGOR_INVALID_ADDRESS;
        m_mnemonic = INST_X86_UNDEF;
        m_numOperands = 0;
        m_segmentOverride = SEGMENT_OVERRIDE_NONE;
        m_sizeOverride = false;
        memset(&m_mod_reg_rm, 0xCD, sizeof(s_mod_reg_rm));
        m_lockPrefix = false;
        m_repPrefixF2 = false;
        m_repPrefixF3 = false;

        m_Prefix64Bit_43 = false;
    }

    operandDefinition getAlternateOperandSize(operandDefinition inputSize)
    {
        if ((inputSize & 0xFFFF) == OPERAND_16_32)
        {
            return OPERAND_16bit;
        }
        if ((inputSize & 0xFFFF) == OPERAND_XMM_64_128)
        {
            return OPERAND_128bit;
        }

        return inputSize;
    }

    operandDefinition getAlternate2OperandSize(operandDefinition inputSize)
    {
        if ((inputSize & 0xFFFF) == OPERAND_16_32_64)
        {
            return OPERAND_64bit;
        }

        return inputSize;
    }

    operandDefinition getDefaultOperandSize(operandDefinition inputSize)
    {
        if ((inputSize & 0xFFFF) == OPERAND_16_32)
        {
            return OPERAND_32bit;
        }
        if ((inputSize & 0xFFFF) == OPERAND_XMM_64_128)
        {
            return OPERAND_64bit;
        }

        return inputSize;
    }

    operandDefinition getDefaultAddressSize()
    {
        return OPERAND_32bit;
    }

    e_x86_mnemonic m_mnemonic;
    u8 m_numOperands;
    s_x86_operand m_operands[X86_MAX_OPERAND];
    e_segment_override m_segmentOverride;
    bool m_sizeOverride;
    s_mod_reg_rm m_mod_reg_rm;
    bool m_lockPrefix;
    bool m_repPrefixF2;
    bool m_repPrefixF3;

    bool m_Prefix64Bit_43;
};

class c_cpu_x86 : public c_cpu_module
{
public:
    virtual Balau::String getTag() const override;
    igor_result analyze(s_analyzeState* pState);
    igor_result printInstruction(s_analyzeState* pState, Balau::String& outputString, bool bUseColor = false);
    c_cpu_analyse_result* allocateCpuSpecificAnalyseResult(){ return new c_x86_analyse_result; }

    const char* getRegisterName(s_analyzeState* pState, s_registerDefinition definition, u8 regIndex, bool sizeOverride = false);
    const char* getMnemonicName(e_x86_mnemonic mnemonic);

    void printInstruction(c_cpu_analyse_result* result);
    igor_result getMnemonic(s_analyzeState* pState, Balau::String& outputString);
    int getNumOperands(s_analyzeState* pState);
    igor_result getOperand(s_analyzeState* pState, int operandIndex, Balau::String& outputString, bool bUseColor = false);
    void generateReferences(s_analyzeState* pState);
    void generateReferences(s_analyzeState* pState, int operandIndex);

//private:

    c_cpu_x86_state m_defaultState;
};
