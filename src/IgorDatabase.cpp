#include <set>
#include "IgorDatabase.h"

igor_result s_igorDatabase::igor_add_cpu(c_cpu_module* pCpuModule, igor_cpu_handle& outputCpuHandle)
{
	outputCpuHandle = m_cpu_modules.size();
	m_cpu_modules.push_back(pCpuModule);

	return IGOR_SUCCESS;
}

c_cpu_module* s_igorDatabase::getCpuForAddress(igorAddress PC)
{
	return m_cpu_modules[0];
}

c_cpu_state* s_igorDatabase::getCpuStateForAddress(igorAddress PC)
{
	return NULL;
}

s_igorSection* s_igorDatabase::findSectionFromAddress(igorAddress address)
{
	for(auto & i : m_sections)
	{
		s_igorSection* pSection = i;

		if((pSection->m_virtualAddress <= address) && (pSection->m_virtualAddress + pSection->m_size > address))
		{
			return pSection;
		}
	}

	return NULL;
}

igor_result s_igorDatabase::readS32(igorAddress address, s32& output)
{
	s_igorSection* pSection = findSectionFromAddress(address);

	if (pSection == NULL)
	{
		return IGOR_FAILURE;
	}

	u64 rawOffset = address - pSection->m_virtualAddress;

	if (rawOffset > pSection->m_rawDataSize)
	{
		return IGOR_FAILURE;
	}

	output = *(s32*)(pSection->m_rawData + rawOffset);

	return IGOR_SUCCESS;
}

igor_result s_igorDatabase::readU32(igorAddress address, u32& output)
{
	s_igorSection* pSection = findSectionFromAddress(address);

	if (pSection == NULL)
	{
		return IGOR_FAILURE;
	}

	u64 rawOffset = address - pSection->m_virtualAddress;

	if (rawOffset > pSection->m_rawDataSize)
	{
		return IGOR_FAILURE;
	}

	output = *(u32*)(pSection->m_rawData + rawOffset);

	return IGOR_SUCCESS;

}

igor_result s_igorDatabase::readS16(igorAddress address, s16& output)
{
	s_igorSection* pSection = findSectionFromAddress(address);

	if (pSection == NULL)
	{
		return IGOR_FAILURE;
	}

	u64 rawOffset = address - pSection->m_virtualAddress;

	if (rawOffset > pSection->m_rawDataSize)
	{
		return IGOR_FAILURE;
	}

	output = *(s16*)(pSection->m_rawData + rawOffset);

	return IGOR_SUCCESS;

}

igor_result s_igorDatabase::readU16(igorAddress address, u16& output)
{
	s_igorSection* pSection = findSectionFromAddress(address);

	if (pSection == NULL)
	{
		return IGOR_FAILURE;
	}

	u64 rawOffset = address - pSection->m_virtualAddress;

	if (rawOffset > pSection->m_rawDataSize)
	{
		return IGOR_FAILURE;
	}

	output = *(u16*)(pSection->m_rawData + rawOffset);

	return IGOR_SUCCESS;

}

igor_result s_igorDatabase::readS8(igorAddress address, s8& output)
{
	s_igorSection* pSection = findSectionFromAddress(address);

	if (pSection == NULL)
	{
		return IGOR_FAILURE;
	}

	u64 rawOffset = address - pSection->m_virtualAddress;

	if (rawOffset > pSection->m_rawDataSize)
	{
		return IGOR_FAILURE;
	}

	output = *(s8*)(pSection->m_rawData + rawOffset);

	return IGOR_SUCCESS;

}

igor_result s_igorDatabase::readU8(igorAddress address, u8& output)
{
	s_igorSection* pSection = findSectionFromAddress(address);

	if (pSection == NULL)
	{
		return IGOR_FAILURE;
	}

	u64 rawOffset = address - pSection->m_virtualAddress;

	if (rawOffset > pSection->m_rawDataSize)
	{
		return IGOR_FAILURE;
	}

	output = *(u8*)(pSection->m_rawData + rawOffset);

	return IGOR_SUCCESS;

}

igor_result s_igorDatabase::create_section(igorAddress virtualAddress, u64 size, igor_section_handle& sectionHandle)
{
	sectionHandle = m_sections.size();
	s_igorSection* pNewSection = new s_igorSection;
	m_sections.push_back(pNewSection);

	pNewSection->m_virtualAddress = virtualAddress;
	pNewSection->m_size = size;

	return IGOR_SUCCESS;
}

igor_result s_igorDatabase::set_section_option(igor_section_handle sectionHandle, e_igor_section_option option)
{
	s_igorSection* pSection = m_sections[sectionHandle];

	pSection->m_option |= option;

	return IGOR_SUCCESS;
}

igor_result s_igorDatabase::load_section_data(igor_section_handle sectionHandle, BFile reader, u64 size)
{
	s_igorSection* pSection = m_sections[sectionHandle];

	pSection->m_rawData = new u8[size];
	reader->read(pSection->m_rawData, size);
	pSection->m_rawDataSize = size;

	return IGOR_SUCCESS;
}

int s_igorDatabase::readString(igorAddress address, Balau::String& outputString)
{
	int length = 0;
	s8 currentChar;
	do 
	{
		currentChar = readS8(address++);

		if (currentChar)
		{
			length++;
		}
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
	s_symbolDefinition& symbol = m_symbolMap[virtualAddress];

	symbol.m_type = SYMBOL_VARIABLE;
	symbol.m_variable.initAs(type);

	return IGOR_SUCCESS;
}

igor_result s_igorDatabase::declare_name(igorAddress virtualAddress, Balau::String name)
{
	s_symbolDefinition& symbol = m_symbolMap[virtualAddress];

	symbol.m_name = name;

	return IGOR_SUCCESS;
}

bool s_igorDatabase::getSymbolName(igorAddress address, Balau::String& name)
{
    s_symbolDefinition* pSymbolDef = get_Symbol(address);

    if (pSymbolDef == NULL)
        return false;

    name = pSymbolDef->m_name;

    return true;
}

s_igorDatabase::s_symbolDefinition* s_igorDatabase::get_Symbol(igorAddress virtualAddress)
{
	auto t = m_symbolMap.find(virtualAddress);
	if (t == m_symbolMap.end())
		return NULL;
	return &t->second;
}

igorAddress s_igorDatabase::findSymbol(const char* symbolName)
{
	// this is going to be very slow...

	for (auto & i : m_symbolMap)
	{
		if (i.second.m_name.compare(symbolName) == 0)
		{
			return i.first;
		}
	}

	return IGOR_INVALID_ADDRESS;
}

igor_result s_igorDatabase::flag_address_as_u32(igorAddress virtualAddress)
{
    // TODO!

    return IGOR_SUCCESS;
}

igor_result s_igorDatabase::flag_address_as_instruction(igorAddress virtualAddress, u8 instructionSize)
{
    s_igorSection* pSection = findSectionFromAddress(virtualAddress);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    if ((virtualAddress + instructionSize) > pSection->m_virtualAddress + pSection->m_size)
    {
        return IGOR_FAILURE;
    }

    if (pSection->m_instructionSize == nullptr)
    {
        pSection->createInstructionArray();
    }

    u8* pInstructionSize = &pSection->m_instructionSize[(virtualAddress - pSection->m_virtualAddress)];

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
    s_igorSection* pSection = findSectionFromAddress(virtualAddress);

    if (pSection == NULL)
    {
        return IGOR_FAILURE;
    }

    if (pSection->m_instructionSize == nullptr)
    {
        return IGOR_FAILURE;
    }

    u8* pInstructionSize = &pSection->m_instructionSize[(virtualAddress - pSection->m_virtualAddress)];

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
    s_igorSection* pSection = findSectionFromAddress(virtualAddress);

    if (pSection == NULL)
    {
        return virtualAddress;
    }

    if (pSection->m_instructionSize == nullptr)
    {
        return virtualAddress;
    }

    while (pSection->m_instructionSize[(virtualAddress - pSection->m_virtualAddress)] == 0xFF)
    {
        virtualAddress--;
    }

    return virtualAddress;
}

igorAddress s_igorDatabase::get_next_valid_address_after(igorAddress virtualAddress)
{
    s_igorSection* pSection = findSectionFromAddress(virtualAddress);

    if (pSection == NULL)
    {
        return virtualAddress;
    }

    if (pSection->m_instructionSize == nullptr)
    {
        return virtualAddress;
    }

    while (pSection->m_instructionSize[(virtualAddress - pSection->m_virtualAddress)] == 0xFF)
    {
        virtualAddress++;
    }

    return virtualAddress;
}

igorAddress s_igorDatabase::getEntryPoint()
{
	return m_entryPoint;
}

igor_section_handle s_igorDatabase::getSectionFromAddress(igorAddress virtualAddress)
{
	for (int i = 0; i<m_sections.size(); i++)
	{
		s_igorSection* pSection = m_sections[i];

        if ((pSection->m_virtualAddress <= virtualAddress) && (pSection->m_virtualAddress + pSection->m_size > virtualAddress))
		{
			return i;
		}
	}

	return -1;
}

igorAddress s_igorDatabase::getSectionStartVirtualAddress(igor_section_handle sectionHandle)
{
	s_igorSection* pSection = m_sections[sectionHandle];

    igorAddress r(pSection->m_virtualAddress);

	return r;
}

u64 s_igorDatabase::getSectionSize(igor_section_handle sectionHandle)
{
	s_igorSection* pSection = m_sections[sectionHandle];

	return pSection->m_size;
}

std::tuple<igorAddress, igorAddress, size_t> s_igorDatabase::getRanges()
{
    igorAddress start = IGOR_MAX_ADDRESS, end = IGOR_MIN_ADDRESS;
    size_t total = 0;

    for (auto & i : m_sections)
    {
        s_igorSection* pSection = i;
        igorAddress sectionStart(pSection->m_virtualAddress);
        igorAddress sectionEnd(pSection->m_virtualAddress + pSection->m_size);

        total += pSection->m_size;

        if (sectionStart < start)
            start = sectionStart;

        if (sectionEnd > end)
            end = sectionEnd;
    }

    return std::tie(start, end, total);
}

igorAddress s_igorDatabase::linearToVirtual(u64 linear) {
    std::set<s_igorSection *, SectionCompare> sections;
    for (auto & i : m_sections)
        sections.insert(i);

    u64 linearStart(0), linearEnd;
    for (auto & i : sections) {
        linearEnd = linearStart + i->m_size;
        if (linear < linearEnd) {
            return igorAddress(i->m_virtualAddress + linear - linearStart);
        }
        linearStart = linearEnd;
    }

    return IGOR_MAX_ADDRESS;
}
