#pragma once

#include "Igor.h"

class c_PELoader
{
public:
	int loadPE(BFile reader);
	int loadOptionalHeader386(BFile reader);
	int loadOptionalHeader64(BFile reader);

	// IMAGE_FILE_HEADER
	u16 m_Machine;
	u16 m_NumberOfSections;
	u32 m_TimeDateStamp;
	u32 m_PointerToSymbolTable;
	u32 m_NumberOfSymbols;
	u16 m_SizeOfOptionalHeader;
	u16 m_Characteristics;

	// IMAGE_OPTIONAL_HEADER64
	u64 m_ImageBase;
	u64 m_EntryPoint;
};
