#include <set>
#include <AtStartExit.h>
#include "IgorDatabase.h"

#include "IgorMemory.h"

#undef max

s_igorDatabase::s_igorDatabase()
{
}

s_igorDatabase::~s_igorDatabase()
{
    for (auto & cpu : m_cpu_modules)
        delete cpu;
    for (auto & section : m_sections)
        delete section;
}

igor_result s_igorDatabase::igor_add_cpu(c_cpu_module* pCpuModule, igor_cpu_handle& outputCpuHandle)
{
    size_t current = m_cpu_modules.size();
    AAssert(current < std::numeric_limits<igor_cpu_handle>::max(), "Too many CPUs!");
    outputCpuHandle = (igor_cpu_handle) current;
    m_cpu_modules.push_back(pCpuModule);

    return IGOR_SUCCESS;
}

c_cpu_module* s_igorDatabase::getCpuForAddress(igorAddress PC)
{
    if (m_cpu_modules.size())
    {
        return m_cpu_modules[0];
    }
    
    return NULL;
}

c_cpu_state* s_igorDatabase::getCpuStateForAddress(igorAddress PC)
{
    return NULL;
}

s_igorSection* s_igorDatabase::findSectionFromAddress(igorAddress address)
{
    Balau::ScopeLock sl(m_lock);
    for(auto & i : m_sections)
    {
        s_igorSection* pSection = i;

        if((pSection->m_virtualAddress <= address.offset) && (pSection->m_virtualAddress + pSection->m_size > address.offset))
        {
            return pSection;
        }
    }

    return NULL;
}

igor_result s_igorDatabase::readS64(igorAddress address, s64& output)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(address);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    igorLinearAddress rawOffset = address - igorAddress(m_sessionId, pSection->m_virtualAddress, pSection->m_id);

    if (rawOffset > pSection->m_rawDataSize)
    {
        return IGOR_FAILURE;
    }

    output = *(s64*)(pSection->m_rawData + rawOffset);

    return IGOR_SUCCESS;
}

igor_result s_igorDatabase::readU64(igorAddress address, u64& output)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(address);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    igorLinearAddress rawOffset = address - igorAddress(m_sessionId, pSection->m_virtualAddress, pSection->m_id);

    if (rawOffset > pSection->m_rawDataSize)
    {
        return IGOR_FAILURE;
    }

    output = *(u64*)(pSection->m_rawData + rawOffset);

    return IGOR_SUCCESS;

}

igor_result s_igorDatabase::readS32(igorAddress address, s32& output)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(address);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    igorLinearAddress rawOffset = address - igorAddress(m_sessionId, pSection->m_virtualAddress, pSection->m_id);

    if (rawOffset > pSection->m_rawDataSize)
    {
        return IGOR_FAILURE;
    }

    output = *(s32*)(pSection->m_rawData + rawOffset);

    return IGOR_SUCCESS;
}

igor_result s_igorDatabase::readU32(igorAddress address, u32& output)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(address);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    igorLinearAddress rawOffset = address - igorAddress(m_sessionId, pSection->m_virtualAddress, pSection->m_id);

    if (rawOffset > pSection->m_rawDataSize)
    {
        return IGOR_FAILURE;
    }

    output = *(u32*)(pSection->m_rawData + rawOffset);

    return IGOR_SUCCESS;

}

igor_result s_igorDatabase::readS16(igorAddress address, s16& output)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(address);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    igorLinearAddress rawOffset = address - igorAddress(m_sessionId, pSection->m_virtualAddress, pSection->m_id);

    if (rawOffset > pSection->m_rawDataSize)
    {
        return IGOR_FAILURE;
    }

    output = *(s16*)(pSection->m_rawData + rawOffset);

    return IGOR_SUCCESS;

}

igor_result s_igorDatabase::readU16(igorAddress address, u16& output)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(address);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    igorLinearAddress rawOffset = address - igorAddress(m_sessionId, pSection->m_virtualAddress, pSection->m_id);

    if (rawOffset > pSection->m_rawDataSize)
    {
        return IGOR_FAILURE;
    }

    output = *(u16*)(pSection->m_rawData + rawOffset);

    return IGOR_SUCCESS;

}

igor_result s_igorDatabase::readS8(igorAddress address, s8& output)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(address);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    igorLinearAddress rawOffset = address - igorAddress(m_sessionId, pSection->m_virtualAddress, pSection->m_id);

    if (rawOffset > pSection->m_rawDataSize)
    {
        return IGOR_FAILURE;
    }

    output = *(s8*)(pSection->m_rawData + rawOffset);

    return IGOR_SUCCESS;

}

igor_result s_igorDatabase::readU8(igorAddress address, u8& output)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(address);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    igorLinearAddress rawOffset = address - igorAddress(m_sessionId, pSection->m_virtualAddress, pSection->m_id);

    if (rawOffset > pSection->m_rawDataSize)
    {
        return IGOR_FAILURE;
    }

    output = *(u8*)(pSection->m_rawData + rawOffset);

    return IGOR_SUCCESS;

}

igor_result s_igorDatabase::create_section(igorLinearAddress virtualAddress, u64 size, igor_section_handle& sectionHandle)
{
    Balau::ScopeLock sl(m_lock);
    size_t current = m_sections.size();
    AAssert(current < std::numeric_limits<igor_section_handle>::max(), "Too many sections!");
    sectionHandle = (igor_section_handle) current;
    s_igorSection* pNewSection = new s_igorSection;
    m_sections.push_back(pNewSection);

    pNewSection->m_virtualAddress = virtualAddress;
    pNewSection->m_size = size;
    pNewSection->m_id = sectionHandle;

    return IGOR_SUCCESS;
}

igor_result s_igorDatabase::set_section_option(igor_section_handle sectionHandle, e_igor_section_option option)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = m_sections[sectionHandle];

    pSection->m_option |= option;

    return IGOR_SUCCESS;
}

igor_result s_igorDatabase::load_section_data(igor_section_handle sectionHandle, BFile reader, u64 size)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = m_sections[sectionHandle];

    pSection->m_rawData = new u8[size];
    ssize_t r = reader->read(pSection->m_rawData, size);
    AAssert(r == size, "Couldn't read %" PRIu64 " bytes, got %zi instead", size, r);
    pSection->m_rawDataSize = size;

    return IGOR_SUCCESS;
}

igor_result s_igorDatabase::load_section_data(igor_section_handle sectionHandle, const void* data, u64 size)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = m_sections[sectionHandle];

    pSection->m_rawData = new u8[size];
    memcpy(pSection->m_rawData, data, size);
    pSection->m_rawDataSize = size;

    return IGOR_SUCCESS;
}

int s_igorDatabase::readString(igorAddress address, Balau::String& outputString)
{
    Balau::ScopeLock sl(m_lock);
    int length = 0;
    s8 currentChar;
    for (;;)
    {
        currentChar = readS8(address++);

        if (!currentChar)
            break;
        length++;
        outputString.append("%c", currentChar);

    } while (currentChar);

    return length;
}

igor_result s_igorDatabase::declare_symbolType(igorAddress virtualAddress, e_symbolType type)
{
    return IGOR_SUCCESS;
}

igor_result s_igorDatabase::declare_variable(igorAddress virtualAddress, e_baseTypes type)
{
    Balau::ScopeLock sl(m_lock);
    s_symbolDefinition& symbol = m_symbolMap[virtualAddress];

    symbol.m_type = SYMBOL_VARIABLE;
    symbol.m_variable.initAs(type);

    return IGOR_SUCCESS;
}

igor_result s_igorDatabase::declare_name(igorAddress virtualAddress, Balau::String name)
{
    Balau::ScopeLock sl(m_lock);
    s_symbolDefinition& symbol = m_symbolMap[virtualAddress];

    symbol.m_name = name;

    return IGOR_SUCCESS;
}

bool s_igorDatabase::getSymbolName(igorAddress address, Balau::String& name)
{
    Balau::ScopeLock sl(m_lock);
    s_symbolDefinition* pSymbolDef = get_Symbol(address);

    if (pSymbolDef == NULL)
        return false;

    name = pSymbolDef->m_name;

    return true;
}

s_igorDatabase::s_symbolDefinition* s_igorDatabase::get_Symbol(igorAddress virtualAddress)
{
    Balau::ScopeLock sl(m_lock);
    auto t = m_symbolMap.find(virtualAddress);
    if (t == m_symbolMap.end())
        return NULL;
    return &t->second;
}

igorAddress s_igorDatabase::findSymbol(const char* symbolName)
{
    // this is going to be very slow...
    Balau::ScopeLock sl(m_lock);

    for (auto & i : m_symbolMap)
    {
        if (i.second.m_name.compare(symbolName) == 0)
        {
            return i.first;
        }
    }

    return igorAddress();
}

igor_result s_igorDatabase::flag_address_as_u32(igorAddress virtualAddress)
{
    // TODO!

    return IGOR_SUCCESS;
}

igor_result s_igorDatabase::flag_address_as_instruction(igorAddress virtualAddress, u8 instructionSize)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(virtualAddress);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    if ((virtualAddress.offset + instructionSize) > pSection->m_virtualAddress + pSection->m_size)
    {
        return IGOR_FAILURE;
    }

    if (pSection->m_instructionSize == nullptr)
    {
        pSection->createInstructionArray();
    }

    u8* pInstructionSize = &pSection->m_instructionSize[(virtualAddress.offset - pSection->m_virtualAddress)];

    if (*pInstructionSize) // already an instruction there
    {
        return IGOR_FAILURE;
    }

    *pInstructionSize = instructionSize;
    memset(pInstructionSize + 1, 0xFF, instructionSize - 1);

    return IGOR_SUCCESS;
}

igor_result s_igorDatabase::is_address_flagged_as_code(igorAddress virtualAddress)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(virtualAddress);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    if (pSection->m_instructionSize == nullptr)
    {
        return IGOR_FAILURE;
    }

    u8* pInstructionSize = &pSection->m_instructionSize[(virtualAddress.offset - pSection->m_virtualAddress)];

    if (*pInstructionSize)
    {
        return IGOR_SUCCESS;
    }
    else
    {
        return IGOR_FAILURE;
    }
}

igorAddress s_igorDatabase::get_next_valid_address_before(igorAddress virtualAddress)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(virtualAddress);

    if (pSection == NULL)
    {
        return virtualAddress;
    }

    if (pSection->m_instructionSize == nullptr)
    {
        return virtualAddress;
    }

    while (pSection->m_instructionSize[(virtualAddress.offset - pSection->m_virtualAddress)] == 0xFF)
    {
        virtualAddress--;
    }

    return virtualAddress;
}

igorAddress s_igorDatabase::get_next_valid_address_after(igorAddress virtualAddress)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = findSectionFromAddress(virtualAddress);

    if (pSection == NULL)
    {
        return virtualAddress;
    }

    if (pSection->m_instructionSize == nullptr)
    {
        return virtualAddress;
    }

    while (pSection->m_instructionSize[(virtualAddress.offset - pSection->m_virtualAddress)] == 0xFF)
    {
        virtualAddress++;
    }

    return virtualAddress;
}

igorAddress s_igorDatabase::getEntryPoint()
{
    Balau::ScopeLock sl(m_lock);
    return m_entryPoint;
}

s_igorSection* s_igorDatabase::getSection(igor_section_handle sectionHandle)
{
    Balau::ScopeLock sl(m_lock);
    return m_sections[sectionHandle];
}

igor_section_handle s_igorDatabase::getSectionFromAddress(igorAddress virtualAddress)
{
    Balau::ScopeLock sl(m_lock);
    if (virtualAddress.m_segmentId != static_cast<igor_section_handle>(-1))
    {
        return virtualAddress.m_segmentId;
    }

    // sectionId was -1, let's search for it
    for (int i = 0; i<m_sections.size(); i++)
    {
        s_igorSection* pSection = m_sections[i];

        if ((pSection->m_virtualAddress <= virtualAddress.offset) && (pSection->m_virtualAddress + pSection->m_size > virtualAddress.offset))
        {
            return i;
        }
    }

    return -1;
}

igorAddress s_igorDatabase::getSectionStartVirtualAddress(igor_section_handle sectionHandle)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = m_sections[sectionHandle];

    igorAddress r(m_sessionId, pSection->m_virtualAddress, pSection->m_id);

    return r;
}

u64 s_igorDatabase::getSectionSize(igor_section_handle sectionHandle)
{
    Balau::ScopeLock sl(m_lock);
    s_igorSection* pSection = m_sections[sectionHandle];

    return pSection->m_size;
}

igor_result s_igorDatabase::setSectionName(igor_section_handle sectionHandle, Balau::String& name)
{
    Balau::ScopeLock sl(m_lock);
    if (s_igorSection* pSection = getSection(sectionHandle))
    {
        pSection->m_name = name;
        return IGOR_SUCCESS;
    }
    return IGOR_FAILURE;
}

igor_result s_igorDatabase::getSectionName(igor_section_handle sectionHandle, Balau::String& name)
{
    Balau::ScopeLock sl(m_lock);
    if (s_igorSection* pSection = getSection(sectionHandle))
    {
        name = pSection->m_name;
        return IGOR_SUCCESS;
    }
    return IGOR_FAILURE;
}

std::tuple<igorAddress, igorAddress, size_t> s_igorDatabase::getRanges()
{
    Balau::ScopeLock sl(m_lock);
    igorAddress start(m_sessionId, 0, 0);
    igorAddress end(m_sessionId, std::numeric_limits<igorLinearAddress>::max(), (igor_section_handle) m_sections.size() - 1);
    size_t total = 0;

    for (auto & i : m_sections)
    {
        s_igorSection* pSection = i;
        igorAddress sectionStart(m_sessionId, pSection->m_virtualAddress, i->m_id);
        igorAddress sectionEnd(m_sessionId, pSection->m_virtualAddress + pSection->m_size, i->m_id);

        total += pSection->m_size;

        if (sectionStart < start)
            start = sectionStart;

        if (sectionEnd > end)
            end = sectionEnd;
    }

    return std::tie(start, end, total);
}

igorAddress s_igorDatabase::linearToVirtual(u64 linear) {
    Balau::ScopeLock sl(m_lock);
    std::set<s_igorSection *, SectionCompare> sections;
    for (auto & i : m_sections)
        sections.insert(i);

    u64 linearStart(0), linearEnd;
    for (auto & i : sections) {
        linearEnd = linearStart + i->m_size;
        if (linear < linearEnd)
            return igorAddress(m_sessionId, i->m_virtualAddress + linear - linearStart, i->m_id);
        linearStart = linearEnd;
    }

    return igorAddress();
}

void s_igorDatabase::addReference(igorAddress referencedAddress, igorAddress referencedFrom)
{
    Balau::ScopeLock sl(m_lock);
    std::vector<igorAddress> crossReferences;
    getReferences(referencedAddress, crossReferences);

    for (int i = 0; i < crossReferences.size(); i++)
    {
        if (crossReferences[i] == referencedFrom)
            return;
    }

    m_references.insert( std::pair<igorAddress, igorAddress>(referencedAddress, referencedFrom));
}

void s_igorDatabase::getReferences(igorAddress referencedAddress, std::vector<igorAddress>& referencedFrom)
{
    Balau::ScopeLock sl(m_lock);
    std::pair<t_references::iterator, t_references::iterator> range;
    range = m_references.equal_range(referencedAddress);

    for (t_references::iterator it = range.first; it != range.second; ++it)
    {
        referencedFrom.push_back(it->second);
    }
}
