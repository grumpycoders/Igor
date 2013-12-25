#include "IgorAPI.h"
#include "IgorDatabase.h"

igor_result igor_create_section(s_igorDatabase* pDatabase, u64 virtualAddress, u64 size, igor_section_handle& sectionHandle)
{
	sectionHandle = pDatabase->m_sections.size();
	s_igorSection* pNewSection = new s_igorSection;
	pDatabase->m_sections.push_back(pNewSection);

	pNewSection->m_virtualAddress = virtualAddress;
	pNewSection->m_size = size;

	return IGOR_SUCCESS;
}

igor_result igor_set_section_option(s_igorDatabase* pDatabase, igor_section_handle sectionHandle, e_igor_section_option option)
{
	s_igorSection* pSection = pDatabase->m_sections[sectionHandle];

	pSection->m_option |= option;

	return IGOR_SUCCESS;
}

igor_result igor_load_section_data(s_igorDatabase* pDatabase, igor_section_handle sectionHandle, BFile reader, u64 size)
{
	s_igorSection* pSection = pDatabase->m_sections[sectionHandle];

	pSection->m_rawData = new u8[size];
	reader->read(pSection->m_rawData, size);
	pSection->m_rawDataSize = size;

	return IGOR_SUCCESS;
}