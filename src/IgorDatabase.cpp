#include "IgorDatabase.h"

igor_result s_igorDatabase::igor_add_cpu(c_cpu_module* pCpuModule, igor_cpu_handle& outputCpuHandle)
{
	outputCpuHandle = m_cpu_modules.size();
	m_cpu_modules.push_back(pCpuModule);

	return IGOR_SUCCESS;
}

c_cpu_module* s_igorDatabase::getCpuForAddress(u64 PC)
{
	return m_cpu_modules[0];
}

c_cpu_state* s_igorDatabase::getCpuStateForAddress(u64 PC)
{
	return NULL;
}

s_igorSection* s_igorDatabase::findSectionFromAddress(u64 address)
{
	for(int i=0; i<m_sections.size(); i++)
	{
		s_igorSection* pSection = m_sections[i];

		if((pSection->m_virtualAddress <= address) && (pSection->m_virtualAddress + pSection->m_size > address))
		{
			return pSection;
		}
	}

	return NULL;
}

igor_result s_igorDatabase::readS32(u64 address, s32& output)
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

igor_result s_igorDatabase::readU32(u64 address, u32& output)
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

igor_result s_igorDatabase::readS16(u64 address, s16& output)
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

igor_result s_igorDatabase::readU16(u64 address, u16& output)
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

igor_result s_igorDatabase::readS8(u64 address, s8& output)
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

igor_result s_igorDatabase::readU8(u64 address, u8& output)
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

igor_result s_igorDatabase::create_section(u64 virtualAddress, u64 size, igor_section_handle& sectionHandle)
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

int s_igorDatabase::readString(u64 address, Balau::String& outputString)
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

igor_result s_igorDatabase::declare_symbolType(u64 virtualAddress, e_symbolType type)
{
	return IGOR_SUCCESS;
}

igor_result s_igorDatabase::declare_variable(u64 virtualAddress, e_baseTypes type)
{
	s_symbolDefinition& symbol = m_symbolMap[virtualAddress];

	symbol.m_type = SYMBOL_VARIABLE;
	symbol.m_variable.initAs(type);

	return IGOR_SUCCESS;
}

igor_result s_igorDatabase::declare_name(u64 virtualAddress, Balau::String name)
{
	s_symbolDefinition& symbol = m_symbolMap[virtualAddress];

	symbol.m_name = name;

	return IGOR_SUCCESS;
}

s_igorDatabase::s_symbolDefinition* s_igorDatabase::get_Symbol(u64 virtualAddress)
{
	auto t = m_symbolMap.find(virtualAddress);
	if (t == m_symbolMap.end())
		return NULL;
	return &t->second;
}

u64 s_igorDatabase::findSymbol(const char* symbolName)
{
	// this is going to be very slow...

	for (auto i = m_symbolMap.begin(); i != m_symbolMap.end(); i++)
	{
		if (i->second.m_name.compare(symbolName) == 0)
		{
			return i->first;
		}
	}

	return -1;
}