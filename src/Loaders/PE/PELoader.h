#pragma once

#include "Igor.h"
#include "IgorAPI.h"
#include "Loaders/IgorLoader.h"

struct s_igorDatabase;
class IgorSession;

class c_PELoader : public c_IgorLoader
{
public:
    static bool isSupported(Balau::String& filename)
    {
        if (filename.strstr(".exe") != -1)
            return true;

        return false;
    }
    static c_IgorLoader* createLoader()
    {
        return new c_PELoader;
    }

    igor_result load(BFile reader, IgorLocalSession *session);
    int loadOptionalHeader386(BFile reader);
    int loadOptionalHeader64(BFile reader);

	void loadDebug(s_igorDatabase * db, BFile reader, IgorLocalSession * session);

    void loadImports(s_igorDatabase * db, BFile reader);

    // IMAGE_FILE_HEADER
    u16 m_Machine;
    u16 m_NumberOfSegments;
    u32 m_TimeDateStamp;
    u32 m_PointerToSymbolTable;
    u32 m_NumberOfSymbols;
    u16 m_SizeOfOptionalHeader;
    u16 m_Characteristics;

    // IMAGE_OPTIONAL_HEADER64
    igorLinearAddress m_ImageBase;
    u64 m_EntryPointVA;

    struct IMAGE_DATA_DIRECTORY
    {
        u32     VirtualAddress;
        u32     Size;
    } m_imageDirectory[16];

    struct s_segmentData
    {
        u8      Name[8];
        u32     Misc;
        u32     VirtualAddress;
        u32     SizeOfRawData;
        u32     PointerToRawData;
        u32     PointerToRelocations;
        u32     PointerToLinenumbers;
        u16     NumberOfRelocations;
        u16     NumberOfLinenumbers;
        u32     Characteristics;
        igor_section_handle sectionId;
    };

    std::vector<s_segmentData> m_segments;
};
