#pragma once

#include "Igor.h"

struct s_igorDatabase;

// Public API
igor_result igor_create_section(s_igorDatabase* pDatabase, igorAddress virtualAddress, u64 size, igor_section_handle& outputSectionHandle);
igor_result igor_set_section_name(igor_section_handle sectionHandle, const Balau::String& sectionName);

enum e_igor_section_option
{
	IGOR_SECTION_OPTION_CODE = 1<<0,
	IGOR_SECTION_OPTION_EXECUTE = 1<<1,
	IGOR_SECTION_OPTION_READ = 1<<2,
};
igor_result igor_set_section_option(s_igorDatabase* pDatabase, igor_section_handle sectionHandle, e_igor_section_option option);

igor_result igor_load_section_data(s_igorDatabase* pDatabase, igor_section_handle sectionHandle, BFile reader, u64 size);

// Private
struct s_igorSection
{
	s_igorSection() :
		m_virtualAddress(0),
		m_size(0),
		m_rawData(nullptr),
		m_rawDataSize(0),
		m_option(0),
		m_instructionSize(nullptr)
	{

	}

	~s_igorSection()
	{
		if (m_instructionSize)
			free(m_instructionSize);
	}

	void createInstructionArray()
	{
		m_instructionSize = (u8*)malloc(m_size);
		memset(m_instructionSize, 0, m_size);
	}

	igorAddress m_virtualAddress;
	u64 m_size;
	u8* m_rawData;
	u64 m_rawDataSize;
	u64 m_option;

	u8* m_instructionSize;// Temporary. Size of instructions. 0 means unknown if it's an instruction or not.
};

class SectionCompare {
public:
    bool operator()(const s_igorSection * x, const s_igorSection * y) const {
        return x->m_virtualAddress < y->m_virtualAddress;
    }
};
