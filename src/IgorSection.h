#pragma once

#include "Igor.h"

// Public API
igor_result igor_create_section(u64 virtualAddress, u64 size, igor_section_handle& outputSectionHandle);
igor_result igor_set_section_name(igor_section_handle sectionHandle, const Balau::String& sectionName);

enum e_igor_section_option
{
	IGOR_SECTION_OPTION_CODE = 1<<0,
	IGOR_SECTION_OPTION_EXECUTE = 1<<1,
	IGOR_SECTION_OPTION_READ = 1<<2,
};
igor_result igor_set_section_option(igor_section_handle sectionHandle, e_igor_section_option option);

igor_result igor_load_section_data(igor_section_handle sectionHandle, BFile reader, u64 size);

// Private
struct s_igorSection
{
	s_igorSection() :
		m_virtualAddress(0),
		m_size(0),
		m_rawData(nullptr),
		m_rawDataSize(0),
		m_option(0)
	{

	}

	u64 m_virtualAddress;
	u64 m_size;
	u8* m_rawData;
	u64 m_rawDataSize;
	u64 m_option;

	//ALIGNED_CLASS_ALLOCATOR(16);
};
