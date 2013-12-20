#include "IgorDatabase.h"

s_igorDatabase g_igorDatabase;

s_igorDatabase* getCurrentIgorDatabase()
{
	return &g_igorDatabase;
}

igor_result igor_add_cpu(c_cpu_module* pCpuModule, igor_cpu_handle& outputCpuHandle)
{
	s_igorDatabase* pDatabase = getCurrentIgorDatabase();

	outputCpuHandle = pDatabase->m_cpu_modules.size();
	pDatabase->m_cpu_modules.push_back(pCpuModule);

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
	for(int i=0; i<m_sections.getSize(); i++)
	{
		s_igorSection* pSection = m_sections.getElementPtr(i);

		if((pSection->m_virtualAddress <= address) && (pSection->m_virtualAddress + pSection->m_size > address))
		{
			return pSection;
		}
	}

	return NULL;
}

igor_result s_igorDatabase::readByte(u64 address, u8& outputByte)
{
	s_igorSection* pSection = findSectionFromAddress(address);

	if(pSection == NULL)
	{
		return IGOR_FAILURE;
	}

	u64 rawOffset = address - pSection->m_virtualAddress;

	if(rawOffset > pSection->m_rawDataSize)
	{
		return IGOR_FAILURE;
	}

	outputByte = pSection->m_rawData[rawOffset];

	return IGOR_SUCCESS;
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