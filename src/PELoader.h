#pragma once

#include "Igor.h"
#include "IgorAPI.h"

struct s_igorDatabase;
class IgorSession;

class c_PELoader
{
public:
	igor_result loadPE(s_igorDatabase * db, BFile reader, IgorLocalSession *);
	int loadOptionalHeader386(BFile reader);
	int loadOptionalHeader64(BFile reader);

	void loadImports(s_igorDatabase * db, BFile reader);

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

	struct IMAGE_DATA_DIRECTORY
	{
		u32     VirtualAddress;
		u32     Size;
	} m_imageDirectory[16];
};
