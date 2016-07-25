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

igor_result s_igorSegment::setRawData(u64 offset, u8 dataValue)
{
    if (offset > m_size)
        return IGOR_FAILURE;

    if (m_rawData == NULL)
    {
        m_rawData = new u8[m_size];
        memset(m_rawData, 0xCC, m_size);
        m_rawDataSize = m_size;
    }

    m_rawData[offset] = dataValue;

    return IGOR_SUCCESS;
}
