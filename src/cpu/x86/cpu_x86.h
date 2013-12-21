#pragma once

#include "cpu/cpuModule.h"

class c_cpu_x86;

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
	INST_X86_SUB,
	INST_X86_AND,
	INST_X86_CMP,
	INST_X86_JZ,
	INST_X86_TEST,
	INST_X86_NOT,
	INST_X86_XOR,
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
	OPERAND_Unkown = -1,
	OPERAND_8bit = 0,
	OPERAND_16bit = 1,
	OPERAND_32bit = 2,
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

struct s_x86_operand
{
	enum type
	{
		type_register,
		type_immediate,
		type_address,
	} m_type;

	union
	{
		struct
		{
			e_operandSize m_operandSize;
			u16 m_registerIndex;
			e_mod m_mod;
			s32 m_offset;
		} m_register;

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

	void setAsRegister(e_operandSize size, e_register registerIndex, e_mod mod = MOD_DIRECT, s32 offset=0)
	{
		m_type = type_register;
		m_register.m_operandSize = size;
		m_register.m_registerIndex = registerIndex;
		m_register.m_mod = mod;
		m_register.m_offset = offset;
	}

	void setAsImmediate(e_immediateSize size, u8 immediateValue)
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

	void setAsAddress(u64 address, bool dereference = false)
	{
		m_type = type_address;
		m_address.m_addressValue = address;
		m_address.m_dereference = dereference;
	}
};

#define X86_MAX_OPERAND 8

class c_x86_analyse_result : public c_cpu_analyse_result
{
public:
	u64 m_PC;
	e_x86_mnemonic m_mnemonic;
	u8 m_numOperands;
	s_x86_operand m_operands[X86_MAX_OPERAND];
};

class c_cpu_x86 : public c_cpu_module
{
public:

	igor_result analyze(s_analyzeState* pState);

	const char* getRegisterName(e_operandSize size, u8 regIndex);
	const char* getMnemonicName(e_x86_mnemonic mnemonic);

	void printInstruction(c_cpu_analyse_result* result);

private:

	c_cpu_x86_state m_defaultState;
};
