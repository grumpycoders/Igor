#pragma once

#include "Igor.h"

struct s_igorDatabase;

// Public API
igor_result igor_set_section_name(igor_segment_handle segmentHandle, const Balau::String& sectionName);

enum e_igor_section_option
{
    IGOR_SECTION_OPTION_CODE = 1<<0,
    IGOR_SECTION_OPTION_EXECUTE = 1<<1,
    IGOR_SECTION_OPTION_READ = 1<<2,
};
igor_result igor_set_section_option(s_igorDatabase* pDatabase, igor_segment_handle segmentHandle, e_igor_section_option option);

igor_result igor_load_section_data(s_igorDatabase* pDatabase, igor_segment_handle segmentHandle, BFile reader, u64 size);

// Private
struct s_igorSegment
{
    s_igorSegment() :
        m_virtualAddress(0),
        m_size(0),
        m_rawData(nullptr),
        m_rawDataSize(0),
        m_option(0),
        m_instructionSize(nullptr)
    {

    }

    ~s_igorSegment()
    {
        if (m_instructionSize)
            free(m_instructionSize);
    }

    void createInstructionArray()
    {
        m_instructionSize = (u8*)malloc(m_size);
        memset(m_instructionSize, 0, m_size);
    }

	Balau::String m_name;
    igorLinearAddress m_virtualAddress;
    u64 m_size;
    u8* m_rawData;
    u64 m_rawDataSize;
    u64 m_option;

    u8* m_instructionSize;// Temporary. Size of instructions. 0 means unknown if it's an instruction or not.

    igor_segment_handle m_handle = 0;
};

class SectionCompare {
public:
    bool operator()(const s_igorSegment * x, const s_igorSegment * y) const {
        return x->m_virtualAddress < y->m_virtualAddress;
    }
};
