#include "IgorAPI.h"
#include "IgorDatabase.h"

igor_result igor_create_section(u64 virtualAddress, u64 size, igor_section_handle& sectionHandle)
{
	s_igorDatabase* pDatabase = getCurrentIgorDatabase();

	sectionHandle = pDatabase->m_sections.getSize();
	s_igorSection* pNewSection = pDatabase->m_sections.allocate(1);

	pNewSection->m_virtualAddress = virtualAddress;
	pNewSection->m_size = size;

	return IGOR_SUCCESS;
}

igor_result igor_set_section_option(igor_section_handle sectionHandle, e_igor_section_option option)
{
	s_igorDatabase* pDatabase = getCurrentIgorDatabase();
	s_igorSection* pSection = pDatabase->m_sections.getElementPtr(sectionHandle);

	pSection->m_option |= option;

	return IGOR_SUCCESS;
}

igor_result igor_load_section_data(igor_section_handle sectionHandle, BFile reader, u64 size)
{
	s_igorDatabase* pDatabase = getCurrentIgorDatabase();
	s_igorSection* pSection = pDatabase->m_sections.getElementPtr(sectionHandle);

	pSection->m_rawData = new u8[size];
	reader->read(pSection->m_rawData, size);
	pSection->m_rawDataSize = size;

	return IGOR_SUCCESS;
}