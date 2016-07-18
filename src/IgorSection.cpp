#include "IgorAPI.h"
#include "IgorDatabase.h"

#include "IgorMemory.h"

igor_result igor_set_section_option(s_igorDatabase* pDatabase, igor_segment_handle segmentHandle, e_igor_section_option option)
{
    return pDatabase->set_segment_option(segmentHandle, option);
}

igor_result igor_load_section_data(s_igorDatabase* pDatabase, igor_segment_handle segmentHandle, BFile reader, u64 size)
{
    return pDatabase->load_segment_data(segmentHandle, reader, size);
}
