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
		m_executionMode(_32bits)
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
	INST_X86_MOVZX,

	INST_X86_PXOR,
	INST_X86_MOVQ,
	INST_X86_MOVDQA,
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

enum e_operandSize
{
	OPERAND_Default = -2,
	OPERAND_Unkown = -1,
	OPERAND_8bit = 0,
	OPERAND_16bit = 1,
	OPERAND_32bit = 2,
	OPERAND_MM_64 = 3,
	OPERAND_XMM_128 = 4,

	OPERAND_16_32,
	OPERAND_XMM_64_128,
};

enum e_immediateSize
{
	IMMEDIATE_S8,
	IMMEDIATE_U8,
	IMMEDIATE_S16,
	IMMEDIATE_U16,
	IMMEDIATE_S32,
	IMMEDIATE_U32,
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
	s32 offset;

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
		type_immediate,
		type_address,
	} m_type;

	union
	{
		struct
		{
			e_operandSize m_operandSize;
			u16 m_registerIndex;
		} m_register;

		struct
		{
			e_operandSize m_operandSize;
			s_mod_reg_rm m_mod_reg_rm;
		} m_registerRM;

		struct
		{
			e_immediateSize m_immediateSize;
			u64 m_immediateValue; // will need to extent that at some point for 128bit
		} m_immediate;
		
		struct  
		{
			u8 m_segment;
			u64 m_addressValue;
			bool m_dereference;
		} m_address;
	};

	void setAsRegister(e_register registerIndex, e_operandSize size = OPERAND_Default)
	{
		m_type = type_register;
		m_register.m_operandSize = size;
		m_register.m_registerIndex = registerIndex;
	}

	void setAsRegisterRM(s_mod_reg_rm modRegRm, e_operandSize size = OPERAND_Default)
	{
		m_type = type_registerRM;
		m_registerRM.m_operandSize = size;
		m_registerRM.m_mod_reg_rm = modRegRm;
	}

	void setAsRegisterRM(s_analyzeState* pState, e_operandSize size = OPERAND_16_32);
	void setAsRegisterR(s_analyzeState* pState, e_operandSize size = OPERAND_16_32);

	void setAsRegisterRM_XMM(s_analyzeState* pState, e_operandSize size = OPERAND_XMM_64_128);
	void setAsRegisterR_XMM(s_analyzeState* pState, e_operandSize size = OPERAND_XMM_64_128);

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

	void setAsAddressRel(s_analyzeState* pState, e_operandSize size = OPERAND_16_32, bool dereference = false);

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
		m_startOfInstruction = -1;
		m_mnemonic = INST_X86_UNDEF;
		m_numOperands = 0;
		m_segmentOverride = SEGMENT_OVERRIDE_NONE;
		m_sizeOverride = false;
		memset(&m_mod_reg_rm, 0xCD, sizeof(s_mod_reg_rm));
	}

	e_operandSize getAlternateOperandSize(e_operandSize inputSize)
	{
		if (inputSize == OPERAND_16_32)
		{
			return OPERAND_16bit;
		}
		if (inputSize == OPERAND_XMM_64_128)
		{
			return OPERAND_XMM_128;
		}

		return inputSize;
	}

	e_operandSize getDefaultOperandSize(e_operandSize inputSize)
	{
		if (inputSize == OPERAND_16_32)
		{
			return OPERAND_32bit;
		}
		if (inputSize == OPERAND_XMM_64_128)
		{
			return OPERAND_MM_64;
		}

		return inputSize;
	}

	e_x86_mnemonic m_mnemonic;
	u8 m_numOperands;
	s_x86_operand m_operands[X86_MAX_OPERAND];
	e_segment_override m_segmentOverride;
	bool m_sizeOverride;
	s_mod_reg_rm m_mod_reg_rm;
};

class c_cpu_x86 : public c_cpu_module
{
public:

	igor_result analyze(s_analyzeState* pState);
	c_cpu_analyse_result* allocateCpuSpecificAnalyseResult(){ return new c_x86_analyse_result; }

	const char* getRegisterName(e_operandSize size, u8 regIndex, bool sizeOverride = false);
	const char* getMnemonicName(e_x86_mnemonic mnemonic);

	void printInstruction(c_cpu_analyse_result* result);

private:

	c_cpu_x86_state m_defaultState;
};
