#include "IgorAPI.h"
#include "IgorDatabase.h"

igor_result igor_set_section_option(s_igorDatabase* pDatabase, igor_section_handle sectionHandle, e_igor_section_option option)
{
	return pDatabase->set_section_option(sectionHandle, option);
}

igor_result igor_load_section_data(s_igorDatabase* pDatabase, igor_section_handle sectionHandle, BFile reader, u64 size)
{
	return pDatabase->load_section_data(sectionHandle, reader, size);
}