#pragma once

#include <vector>
#include <map>

#include <Task.h>

#include "Igor.h"

#include "IgorAPI.h"
#include "IgorSection.h"
#include "cpu/cpuModule.h"

typedef int igor_cpu_handle;

struct s_analysisRequest
{
    s_analysisRequest(u64 PC) : m_pc(PC) { }
	u64 m_pc;
};

struct s_igorDatabase
{
	// Not sure about that stuff yet. Kind of making it up as I go
	enum e_baseTypes
	{
		TYPE_U8,
		TYPE_S8,
		TYPE_U16,
		TYPE_S16,
		TYPE_U32,
		TYPE_S32,

		TYPE_STRUCT,
	};

	struct s_variableType
	{
		e_baseTypes m_baseType;
		u32 m_arraySize; // if 0, not an array. What would array of 1 mean?
		u32 m_structHandle; // get you to the structure definition;

		void initAs(e_baseTypes baseType)
		{
			m_baseType = baseType;
			m_arraySize = 0;
			m_structHandle = -1;
		}
	};

	enum e_symbolType
	{
		SYMBOL_UNKOWN, // Default "unknown" state

		SYMBOL_VARIABLE,

		SYMBOL_FUNCTION,
		SYMBOL_IMPORT_FUNCTION,
		SYMBOL_STRING,
	};

	struct s_symbolDefinition
	{
		e_symbolType m_type;
		Balau::String m_name;
		// Balau::String m_comment;
		union
		{
			s_variableType m_variable;
		};

		s_symbolDefinition() :
			m_type(SYMBOL_UNKOWN)
		{

		}
	};

	std::map<u64, s_symbolDefinition> m_symbolMap;

	// a map of the name of all symbols. Should the name of a symbol be included in the global m_symbolTypeMap?
	// Technically, only symbol could have name...
	//std::map<u64, Balau::String> m_stringMap;

	// How do we define a structure in Igor?
	struct s_structure
	{
		struct s_element
		{
			u32 offset;
			s_variableType m_type;
		};
		Balau::String m_name;
		std::vector<s_element> m_elements;
	};
	std::vector<s_structure> m_structures;

	std::vector<c_cpu_module*> m_cpu_modules;
	Balau::TQueue<s_analysisRequest> m_analysisRequests;
	std::vector<s_igorSection*> m_sections;

    igor_result igor_add_cpu(c_cpu_module* pCpuModule, igor_cpu_handle& outputCpuHandle);
    
    c_cpu_module* getCpuForAddress(u64 PC);
	c_cpu_state* getCpuStateForAddress(u64 PC);

	igor_result readS32(u64 address, s32& output);
	igor_result readU32(u64 address, u32& output);
	igor_result readS16(u64 address, s16& output);
	igor_result readU16(u64 address, u16& output);
	igor_result readS8(u64 address, s8& output);
	igor_result readU8(u64 address, u8& output);

	u64 findSymbol(const char* symbolName);

	s32 readS32(u64 address)
	{
		s32 output;
		readS32(address, output);
		return output;
	}

	u32 readU32(u64 address)
	{
		u32 output;
		readU32(address, output);
		return output;
	}

	s16 readS16(u64 address)
	{
		s16 output;
		readS16(address, output);
		return output;
	}

	u16 readU16(u64 address)
	{
		u16 output;
		readU16(address, output);
		return output;
	}

	s8 readS8(u64 address)
	{
		s8 output;
		readS8(address, output);
		return output;
	}

	u8 readU8(u64 address)
	{
		u8 output;
		readU8(address, output);
		return output;
	}

	int readString(u64 address, Balau::String& outputString);

	igor_result create_section(u64 virtualAddress, u64 size, igor_section_handle& outputSectionHandle);
	igor_result set_section_name(const Balau::String& sectionName);
	igor_result set_section_option(igor_section_handle sectionHandle, e_igor_section_option option);
	igor_result load_section_data(igor_section_handle sectionHandle, BFile reader, u64 size);

	igor_result declare_name(u64 virtualAddress, Balau::String name);
	igor_result declare_symbolType(u64 virtualAddress, e_symbolType type);
	igor_result declare_variable(u64 virtualAddress, e_baseTypes type);

    igor_result flag_address_as_u32(u64 virtualAddress);
    igor_result flag_address_as_instruction(u64 virtualAddress, u8 instructionSize);


    igor_result is_address_flagged_as_code(u64 virtualAddress);
    u64 get_next_valid_address_before(u64 virtualAddress);
    u64 get_next_valid_address_after(u64 virtualAddress);

	igorAddress getEntryPoint();
	igor_section_handle getSectionFromAddress(igorAddress virtualAddress);
	igorAddress getSectionStartVirtualAddress(igor_section_handle sectionHandle);
	u64 getSectionSize(igor_section_handle sectionHandle);

	igorAddress m_entryPoint;

private:
	s_igorSection* findSectionFromAddress(u64 address);
	s_symbolDefinition* get_Symbol(u64 virtualAddress);
};
