#include "elfLoader.h"
#include <Exceptions.h>
#include "IgorLocalSession.h"

using namespace Balau;

struct s_programHeader
{
    u32 m_type;
    u32 m_flags;
    u64 m_offset;
    u64 m_vaddr;
    u64 m_paddr;
    u64 m_filesz;
    u64 m_memsz;
    u64 m_align;
};

struct s_sectionHeader
{
    u32 m_name;
    u32 m_type;
    u64 m_flags;
    u64 m_addr;
    u64 m_offset;
    u64 m_size;
    u32 m_link;
    u32 m_info;
    u64 m_addressAlignement;
    u64 m_entriesSize;
};

igor_result c_elfLoader::load(BFile reader, IgorLocalSession * pSession)
{
    u32 elfMagic = reader->readBEU32().get();

    if (elfMagic != 0x7f454c46) // 0x7F ELF
    {
        return IGOR_FAILURE;
    }

    u8 elfClass = reader->readU8().get(); // 1=32bits, 2=64bits
    u8 elfData = reader->readU8().get(); // 1=little endianness, 2=big endianness
    u8 elfVersion = reader->readU8().get(); // should always be 1
    if (elfVersion != 1)
    {
        return IGOR_FAILURE;
    }
    u8 elfOSAbi = reader->readU8().get();
    u8 elfAbiVersion = reader->readU8().get();
    // skip padding
    reader->seek(7, SEEK_CUR);

    if (elfData == 2)
    {
        reader->setEndianness(Handle::BALAU_BIG_ENDIAN);
    }

    u16 elfType = reader->readU16().get(); // 1=relocatable, 2=executable, 3=shared, 4=core
    u16 elfMachine = reader->readU16().get(); // 3=x86, 0x14=PPC 0x15=PPC64 0x17=SPU
    u32 elfVersion2 = reader->readU32().get();
    if (elfVersion2 != 1)
    {
        return IGOR_FAILURE;
    }

    u64 entryPoint;
    u64 phoff;
    u64 shoff;
    bool foundEntryPointSection = false;
    uint16_t entryPointSection;
    if (elfClass == 1)
    {
        entryPoint = reader->readU32().get();
        phoff = reader->readU32().get();
        shoff = reader->readU32().get();
    }
    else
    {
        entryPoint = reader->readU64().get();
        phoff = reader->readU64().get();
        shoff = reader->readU64().get();
    }


    u32 elfFlags = reader->readU32().get();
    u16 ehSize = reader->readU16().get();
    u16 phentSize = reader->readU16().get();
    u16 phnum = reader->readU16().get();
    u16 shentsize = reader->readU16().get();
    u16 shnum = reader->readU16().get();
    u16 shstrndx = reader->readU16().get();

    if (reader->tell() != ehSize)
    {
        return IGOR_FAILURE;
    }

    // read program header
    reader->seek(phoff);
    std::vector<s_programHeader> programHeaders;
    for (u32 i = 0; i < phnum; i++)
    {
        s_programHeader programHeader;

        if (elfClass == 1)
        {
            throw GeneralException("Elf32_Phdr not implemented");
        }
        else
        {
            programHeader.m_type = reader->readU32().get();
            programHeader.m_flags = reader->readU32().get();
            programHeader.m_offset = reader->readU64().get();
            programHeader.m_vaddr = reader->readU64().get();
            programHeader.m_paddr = reader->readU64().get();
            programHeader.m_filesz = reader->readU64().get();
            programHeader.m_memsz = reader->readU64().get();
            programHeader.m_align = reader->readU64().get();
        }

        programHeaders.push_back(programHeader);
    }

    // read section header
    reader->seek(shoff);
    std::vector<s_sectionHeader> sectionHeaders;
    for (u32 i = 0; i < shnum; i++)
    {
        s_sectionHeader sectionHeader;

        if (elfClass == 1)
        {
            throw GeneralException("Elf32_Shdr not implemented");
        }
        else
        {
            sectionHeader.m_name = reader->readU32().get();
            sectionHeader.m_type = reader->readU32().get();
            sectionHeader.m_flags = reader->readU64().get();
            sectionHeader.m_addr = reader->readU64().get();
            sectionHeader.m_offset = reader->readU64().get();
            sectionHeader.m_size = reader->readU64().get();
            sectionHeader.m_link = reader->readU32().get();
            sectionHeader.m_info = reader->readU32().get();
            sectionHeader.m_addressAlignement = reader->readU64().get();
            sectionHeader.m_entriesSize = reader->readU64().get();
        }

        sectionHeaders.push_back(sectionHeader);
    }

    // load into db
    s_igorDatabase * pDataBase = pSession->getDB();

    for (u32 sectionIndex = 0; sectionIndex < shnum; sectionIndex++)
    {
        s_sectionHeader* pSectionHeader = &sectionHeaders[sectionIndex];

        // get the section name
        Balau::String sectionName;
        reader->seek(sectionHeaders[shstrndx].m_offset + pSectionHeader->m_name);
        while (char character = reader->readI8().get())
        {
            sectionName.append("%c", character);
        }

        igor_segment_handle segmentHandle;
        pDataBase->create_section(pSectionHeader->m_addr, pSectionHeader->m_size, segmentHandle);

        reader->seek(pSectionHeader->m_offset);
        pDataBase->load_section_data(segmentHandle, reader, pSectionHeader->m_size);

        igorAddress start(pSession, pSectionHeader->m_addr, segmentHandle);
        igorAddress end = start + pSectionHeader->m_size;
        igorAddress supposedEntry(pSession, entryPoint, segmentHandle);

        if ((start <= supposedEntry) && (supposedEntry < end))
        {
            entryPointSection = segmentHandle;
            foundEntryPointSection = true;
        }
    }

    EAssert(foundEntryPointSection, "Couldn't find entry point's section");
    igorAddress entryPointAddress(pSession, entryPoint, entryPointSection);
    pDataBase->m_entryPoint = entryPointAddress;

    return IGOR_SUCCESS;
}
