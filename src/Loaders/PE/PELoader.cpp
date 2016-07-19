#include <Input.h>

#include "PELoader.h"
#include "IgorAPI.h"
#include "IgorDatabase.h"
#include "IgorLocalSession.h"
#include "cpu/x86_llvm/cpu_x86_llvm.h"
#include "PDB/pdb.h"
#include "PDB/tpi.h"

#include "IgorMemory.h"

#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_AGGRESIVE_WS_TRIM         0x0010  // Agressively trim working set
#define IMAGE_FILE_LARGE_ADDRESS_AWARE       0x0020  // App can handle >2gb addresses
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.

#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_I386              0x014c  // Intel 386.
#define IMAGE_FILE_MACHINE_R3000             0x0162  // MIPS little-endian, 0x160 big-endian
#define IMAGE_FILE_MACHINE_R4000             0x0166  // MIPS little-endian
#define IMAGE_FILE_MACHINE_R10000            0x0168  // MIPS little-endian
#define IMAGE_FILE_MACHINE_WCEMIPSV2         0x0169  // MIPS little-endian WCE v2
#define IMAGE_FILE_MACHINE_ALPHA             0x0184  // Alpha_AXP
#define IMAGE_FILE_MACHINE_SH3               0x01a2  // SH3 little-endian
#define IMAGE_FILE_MACHINE_SH3DSP            0x01a3
#define IMAGE_FILE_MACHINE_SH3E              0x01a4  // SH3E little-endian
#define IMAGE_FILE_MACHINE_SH4               0x01a6  // SH4 little-endian
#define IMAGE_FILE_MACHINE_SH5               0x01a8  // SH5
#define IMAGE_FILE_MACHINE_ARM               0x01c0  // ARM Little-Endian
#define IMAGE_FILE_MACHINE_THUMB             0x01c2
#define IMAGE_FILE_MACHINE_AM33              0x01d3
#define IMAGE_FILE_MACHINE_POWERPC           0x01F0  // IBM PowerPC Little-Endian
#define IMAGE_FILE_MACHINE_POWERPCFP         0x01f1
#define IMAGE_FILE_MACHINE_IA64              0x0200  // Intel 64
#define IMAGE_FILE_MACHINE_MIPS16            0x0266  // MIPS
#define IMAGE_FILE_MACHINE_ALPHA64           0x0284  // ALPHA64
#define IMAGE_FILE_MACHINE_MIPSFPU           0x0366  // MIPS
#define IMAGE_FILE_MACHINE_MIPSFPU16         0x0466  // MIPS
#define IMAGE_FILE_MACHINE_AXP64             IMAGE_FILE_MACHINE_ALPHA64
#define IMAGE_FILE_MACHINE_TRICORE           0x0520  // Infineon
#define IMAGE_FILE_MACHINE_CEF               0x0CEF
#define IMAGE_FILE_MACHINE_EBC               0x0EBC  // EFI Byte Code
#define IMAGE_FILE_MACHINE_AMD64             0x8664  // AMD64 (K8)
#define IMAGE_FILE_MACHINE_M32R              0x9041  // M32R little-endian
#define IMAGE_FILE_MACHINE_CEE               0xC0EE

#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor

using namespace Balau;

igor_result c_PELoader::load(BFile reader, IgorLocalSession * session)
{
    reader->seek(0);

    bool success = false;
    // DOS .EXE header
    {
        u16    e_magic = reader->readBEU16().get(); // Magic number
        u16    e_cblp = reader->readU16().get(); // Bytes on last page of file
        u16    e_cp = reader->readU16().get(); // Pages in file
        u16    e_crlc = reader->readU16().get(); // Relocations
        u16    e_cparhdr = reader->readU16().get(); // Size of header in paragraphs
        u16    e_minalloc = reader->readU16().get(); // Minimum extra paragraphs needed
        u16    e_maxalloc = reader->readU16().get(); // Maximum extra paragraphs needed
        u16    e_ss = reader->readU16().get(); // Initial (relative) SS value
        u16    e_sp = reader->readU16().get(); // Initial SP value
        u16    e_csum = reader->readU16().get(); // Checksum
        u16    e_ip = reader->readU16().get(); // Initial IP value
        u16    e_cs = reader->readU16().get(); // Initial (relative) CS value
        u16    e_lfarlc = reader->readU16().get(); // File address of relocation table
        u16    e_ovno = reader->readU16().get(); // Overlay number
        u16    e_res[4]; // Reserved words
        for (int i = 0; i<4; i++)
        {
            e_res[i] = reader->readU16().get();
        }
        u16    e_oemid = reader->readU16().get(); // OEM identifier (for e_oeminfo)
        u16    e_oeminfo = reader->readU16().get(); // OEM information; e_oemid specific
        u16    e_res2[10]; // Reserved words
        for (int i = 0; i<10; i++)
        {
            e_res2[i] = reader->readU16().get();
        }
        u32    e_lfanew = reader->readU32().get(); // File address of new exe header

        IAssert(e_magic == 0x4D5A, "magic isn't MZ: %016x", e_magic); // MZ
        reader->seek(e_lfanew);
    }

    // NT HEADER
    u32 signature = reader->readBEU32().get();
    IAssert(signature == 0x50450000, "PE signature isn't PE00: %032x", signature); // PE00

    // IMAGE_FILE_HEADER
    m_Machine = reader->readU16().get();
    m_NumberOfSegments = reader->readU16().get();
    m_TimeDateStamp = reader->readU32().get();
    m_PointerToSymbolTable = reader->readU32().get();
    m_NumberOfSymbols = reader->readU32().get();
    m_SizeOfOptionalHeader = reader->readU16().get();
    m_Characteristics = reader->readU16().get();

    u64 optionalHeaderOffset = reader->tell();

    igor_cpu_handle cpuHandle;
    s_igorDatabase * db = session->getDB();

    c_cpu_x86_llvm* pCpu = NULL;

    switch (m_Machine)
    {
        case IMAGE_FILE_MACHINE_I386:
            pCpu = new c_cpu_x86_llvm(c_cpu_x86_llvm::X86);
            //pCpu->m_defaultState.m_executionMode = c_cpu_x86_state::_32bits;
            loadOptionalHeader386(reader);
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            pCpu = new c_cpu_x86_llvm(c_cpu_x86_llvm::X64);
            //pCpu->m_defaultState.m_executionMode = c_cpu_x86_state::_64bits;
            loadOptionalHeader64(reader);
            break;
        default:
            Failure("Unknown machine type");
    }

    db->igor_add_cpu(pCpu, cpuHandle);

    igorLinearAddress entryPoint = m_ImageBase + m_EntryPointVA;
    bool foundEntryPointSection = false;
    uint16_t entryPointSection;

    //IMAGE_SECTION_HEADER
    for (int i = 0; i<m_NumberOfSegments; i++)
    {
        reader->seek(optionalHeaderOffset + m_SizeOfOptionalHeader + i * 40);

        s_segmentData segmentData;
        ssize_t l = reader->read(segmentData.Name, 8);
        EAssert(l == 8, "Couldn't read segment name");
        segmentData.Misc = reader->readU32().get(); // u32   PhysicalAddress union with u32   VirtualSize; (depends if it's a DLL or a .EXE)
        segmentData.VirtualAddress = reader->readU32().get();
        segmentData.SizeOfRawData = reader->readU32().get();
        segmentData.PointerToRawData = reader->readU32().get();
        segmentData.PointerToRelocations = reader->readU32().get();
        segmentData.PointerToLinenumbers = reader->readU32().get();
        segmentData.NumberOfRelocations = reader->readU16().get();
        segmentData.NumberOfLinenumbers = reader->readU16().get();
        segmentData.Characteristics = reader->readU32().get();

        igor_segment_handle segmentHandle;
        db->create_segment(m_ImageBase + segmentData.VirtualAddress, segmentData.Misc, segmentHandle);
        Balau::String sectionName;
        sectionName.append("%s", (char*)segmentData.Name);
        db->setSegmentName(segmentHandle, sectionName);

        // IMAGE_SCN_CNT_CODE
        if (segmentData.Characteristics & 0x00000020)
        {
            db->set_segment_option(segmentHandle, IGOR_SECTION_OPTION_CODE);
        }

        //IMAGE_SCN_MEM_EXECUTE
        if (segmentData.Characteristics & 0x20000000)
        {
            db->set_segment_option(segmentHandle, IGOR_SECTION_OPTION_EXECUTE);
        }

        //IMAGE_SCN_MEM_READ
        if (segmentData.Characteristics & 0x40000000)
        {
            db->set_segment_option(segmentHandle, IGOR_SECTION_OPTION_READ);
        }

        reader->seek(segmentData.PointerToRawData);
        db->load_segment_data(segmentHandle, reader, segmentData.SizeOfRawData);

        segmentData.sectionId = segmentHandle;

        m_segments.push_back(segmentData);

        igorAddress start(session, m_ImageBase + segmentData.VirtualAddress, segmentHandle);
        igorAddress end = start + segmentData.Misc;
        igorAddress supposedEntry(session, entryPoint, segmentHandle);

        if ((start <= supposedEntry) && (supposedEntry < end))
        {
            entryPointSection = segmentHandle;
            foundEntryPointSection = true;
        }
    }

    loadDebug(db, reader, session);
    loadImports(db, reader);

    EAssert(foundEntryPointSection, "Couldn't find entry point's section");
    igorAddress entryPointAddress(session, entryPoint, entryPointSection);
    db->m_entryPoint = entryPointAddress;

    igorAddress base(session, m_ImageBase, -1);
    base += m_EntryPointVA;
    session->add_code_analysis_task(base);

    return IGOR_SUCCESS;
}

int c_PELoader::loadOptionalHeader386(BFile reader)
{
    u16                  Magic = reader->readU16().get();
    IAssert(Magic == 0x10b, "Magic isn't IMAGE_NT_OPTIONAL_HDR32_MAGIC: %016x", Magic); // IMAGE_NT_OPTIONAL_HDR32_MAGIC 


    u8                   MajorLinkerVersion = reader->readU8().get();
    u8                   MinorLinkerVersion = reader->readU8().get();
    u32                  SizeOfCode = reader->readU32().get();
    u32                  SizeOfInitializedData = reader->readU32().get();
    u32                  SizeOfUninitializedData = reader->readU32().get();
    m_EntryPointVA = reader->readU32().get();
    u32                  BaseOfCode = reader->readU32().get();
    u32                  BaseOfData = reader->readU32().get();
    m_ImageBase = reader->readU32().get();
    u32                  SectionAlignment = reader->readU32().get();
    u32                  FileAlignment = reader->readU32().get();
    u16                  MajorOperatingSystemVersion = reader->readU16().get();
    u16                  MinorOperatingSystemVersion = reader->readU16().get();
    u16                  MajorImageVersion = reader->readU16().get();
    u16                  MinorImageVersion = reader->readU16().get();
    u16                  MajorSubsystemVersion = reader->readU16().get();
    u16                  MinorSubsystemVersion = reader->readU16().get();
    u32                  Win32VersionValue = reader->readU32().get();
    u32                  SizeOfImage = reader->readU32().get();
    u32                  SizeOfHeaders = reader->readU32().get();
    u32                  CheckSum = reader->readU32().get();
    u16                  Subsystem = reader->readU16().get();
    u16                  DllCharacteristics = reader->readU16().get();
    u32                  SizeOfStackReserve = reader->readU32().get();
    u32                  SizeOfStackCommit = reader->readU32().get();
    u32                  SizeOfHeapReserve = reader->readU32().get();
    u32                  SizeOfHeapCommit = reader->readU32().get();
    u32                  LoaderFlags = reader->readU32().get();
    u32                  NumberOfRvaAndSizes = reader->readU32().get();

    // this should always be 16, read http://opcode0x90.wordpress.com/2007/04/22/windows-loader-does-it-differently/
    TAssert(NumberOfRvaAndSizes == 0x10);

    for (int i = 0; i<16; i++)
    {
        m_imageDirectory[i].VirtualAddress = reader->readU32().get();
        m_imageDirectory[i].Size = reader->readU32().get();
    }

    return 0;
}

int c_PELoader::loadOptionalHeader64(BFile reader)
{
    //IMAGE_OPTIONAL_HEADER64
    u16         Magic = reader->readU16().get();
    IAssert(Magic == 0x20b, "Magic isn't IMAGE_NT_OPTIONAL_HDR64_MAGIC: %016x", Magic); // IMAGE_NT_OPTIONAL_HDR64_MAGIC 

    u8          MajorLinkerVersion = reader->readU8().get();
    u8          MinorLinkerVersion = reader->readU8().get();
    u32         SizeOfCode = reader->readU32().get();
    u32         SizeOfInitializedData = reader->readU32().get();
    u32         SizeOfUninitializedData = reader->readU32().get();
    m_EntryPointVA = reader->readU32().get();
    u32         BaseOfCode = reader->readU32().get();
    m_ImageBase = reader->readU64().get();
    u32         SectionAlignment = reader->readU32().get();
    u32         FileAlignment = reader->readU32().get();
    u16         MajorOperatingSystemVersion = reader->readU16().get();
    u16         MinorOperatingSystemVersion = reader->readU16().get();
    u16         MajorImageVersion = reader->readU16().get();
    u16         MinorImageVersion = reader->readU16().get();
    u16         MajorSubsystemVersion = reader->readU16().get();
    u16         MinorSubsystemVersion = reader->readU16().get();
    u32         Win32VersionValue = reader->readU32().get();
    u32         SizeOfImage = reader->readU32().get();
    u32         SizeOfHeaders = reader->readU32().get();
    u32         CheckSum = reader->readU32().get();
    u16         Subsystem = reader->readU16().get();
    u16         DllCharacteristics = reader->readU16().get();
    u64         SizeOfStackReserve = reader->readU64().get();
    u64         SizeOfStackCommit = reader->readU64().get();
    u64         SizeOfHeapReserve = reader->readU64().get();
    u64         SizeOfHeapCommit = reader->readU64().get();
    u32         LoaderFlags = reader->readU32().get();
    u32         NumberOfRvaAndSizes = reader->readU32().get();

    // this should always be 16, read http://opcode0x90.wordpress.com/2007/04/22/windows-loader-does-it-differently/
    TAssert(NumberOfRvaAndSizes == 0x10);

    //IMAGE_DATA_DIRECTORY
    for (int i = 0; i<16; i++)
    {
        m_imageDirectory[i].VirtualAddress = reader->readU32().get();
        m_imageDirectory[i].Size = reader->readU32().get();
    }

    return 0;
}

void loadUDT(c_PELoader* pLoader, PSYM Sym, PSYMD Symd, s_igorDatabase * db, IgorLocalSession * session)
{
    char* typeName = TPILookupTypeName(Symd->TpiHdr, Sym->Udt.typind);
    IAssert(typeName, "Failed to fine type name");

    char* symbolDeclaration = TPIGetSymbolDeclaration(Symd->TpiHdr, typeName, (char*)Sym->Udt.name);
    /*
    TPIGetSymbolDeclaration(Symd->TpiHdr,
    TPILookupTypeName(Symd->TpiHdr, Sym->Udt.typind),
    (char*)Sym->Udt.name
    ));
    */
}

void loadPub32(c_PELoader* pLoader, PSYM Sym, PSYMD Symd, s_igorDatabase * db, IgorLocalSession * session)
{
    /*
    printf("S_PUB32| [%04x] public%s%s %p = %s (type %04x)",
    Sym->Pub32.seg, // 0x0c
    Sym->Pub32.pubsymflags.fCode ? " code" : "",
    Sym->Pub32.pubsymflags.fFunction ? " function" : "",
    Sym->Pub32.off, Sym->Pub32.name, // 0x08 0x0e
    Sym->Data32.typind); // 0x04
    printf("\n");*/

    //if (Sym->Pub32.pubsymflags.fFunction)
    int segmentIndex = Sym->Pub32.seg - 1;
    if (segmentIndex < pLoader->m_segments.size())
    {
        // TODO: which section ?
        igorAddress symbolAddress(db, pLoader->m_ImageBase + pLoader->m_segments[segmentIndex].VirtualAddress + Sym->Pub32.off, -1);
        db->declare_name(symbolAddress, (const char*)Sym->Pub32.name);

        if (Sym->Pub32.pubsymflags.fFunction)
            session->add_code_analysis_task(symbolAddress);
    }
    else
    {
        // this happens for symbols in segment index 0
    }
}

void loadData32(c_PELoader* pLoader, PSYM Sym, PSYMD Symd, s_igorDatabase * db, IgorLocalSession * session)
{
    /*
    char *type = TPILookupTypeName(Symd->TpiHdr, Sym->Data32.typind);
    char *decl = TPIGetSymbolDeclaration(Symd->TpiHdr, type, (char*)Sym->Data32.name);
    printf("S_%sDATA32| data [%s; type %04x] %p = %s",
    Sym->Sym.rectyp == S_LDATA32 ? "L" : "G",
    Sym->Sym.rectyp == S_LDATA32 ? "local" : "global",
    Sym->Data32.typind, Sym->Data32.off, decl);
    */
    if (Sym->Data32.seg == 0)
    {
        // TODO: which section ?
        igorAddress symbolAddress(db, pLoader->m_ImageBase + Sym->Data32.off, -1);
        db->declare_name(symbolAddress, (const char*)Sym->Data32.name);
    }
    else
    {
        int segmentIndex = Sym->Data32.seg - 1;
        if (segmentIndex < pLoader->m_segments.size())
        {
            // TODO: which section ?
            igorAddress symbolAddress(db, pLoader->m_ImageBase + pLoader->m_segments[Sym->Data32.seg - 1].VirtualAddress + Sym->Data32.off, -1);
            db->declare_name(symbolAddress, (const char*)Sym->Data32.name);
        }
    }
}

void ProcessTypeRecord(PHDR pHdr, PlfRecord plr);

void ProcessTypeRecordStructure(PHDR pHdr, PlfStructure pls)
{
    DWORD dBytes;
    PBYTE pbName = TPIRecordValue(pls->data, &dBytes);

    ULONG dBase = 0;
    ULONG dSize = 0;
    PlfRecord plr = TPILookupTypeRecord(pHdr, pls->field, &dBase, &dSize);
    if(plr)
    {
        IAssert(plr->leaf == LF_FIELDLIST, "plf record not a leaf");
    }
}

void ProcessTypeRecord(PHDR pHdr, PlfRecord plr)
{
    LEAF_ENUM_e leafType = (LEAF_ENUM_e)plr->leaf;

    switch (leafType)
    {
    case LF_STRUCTURE:
        ProcessTypeRecordStructure(pHdr, &plr->Structure);
        break;
    default:
        break;
    }
}

void c_PELoader::loadDebug(s_igorDatabase * db, BFile reader, IgorLocalSession * session)
{
    IMAGE_DATA_DIRECTORY* pDebugDirectory = &m_imageDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

    igorAddress startDebugTableVA = igorAddress(db, m_ImageBase + pDebugDirectory->VirtualAddress, -1);
    igorAddress debugTableVA = startDebugTableVA;

    while (debugTableVA < startDebugTableVA + pDebugDirectory->Size)
    {
        u32 characteristics = db->readU32(debugTableVA); debugTableVA += 4;
        u32 timeDateStamp = db->readU32(debugTableVA); debugTableVA += 4;
        u16 majorVersion = db->readU16(debugTableVA); debugTableVA += 2;
        u16 minorVersion = db->readU16(debugTableVA); debugTableVA += 2;
        u32 type = db->readU32(debugTableVA); debugTableVA += 4;
        u32 sizeOfData = db->readU32(debugTableVA); debugTableVA += 4;
        u32 addressOfRawData = db->readU32(debugTableVA); debugTableVA += 4;
        u32 pointerToRawData = db->readU32(debugTableVA); debugTableVA += 4;

        if (type == 2) // IMAGE_DEBUG_TYPE_CODEVIEW
        {
            igorLinearAddress codeViewData = m_ImageBase + addressOfRawData;
            igorAddress codeViewDataAddr(db, codeViewData, -1);

            u32 signature = db->readU32(codeViewDataAddr); codeViewDataAddr += 4;
            u8 guid[16];
            for (int i = 0; i < 16; i++)
            {
                guid[i] = db->readU8(codeViewDataAddr); codeViewDataAddr++;
            }
            u32 age = db->readU32(codeViewDataAddr); codeViewDataAddr += 4;

            String pdbName;
            db->readString(codeViewDataAddr, pdbName);

            if (signature == 'SDSR')
            {
                PPDB pPdb = PdbOpen(pdbName.to_charp());

                if (pPdb == NULL)
                {
                    // try in the current folder
                    pPdb = PdbOpen(pdbName.to_charp(pdbName.strrchr('\\')+1));
                }

                if (pPdb)
                {
                    //SYMDumpSymbols(pPdb->Symd, 0xFFFF);

                    // types
                    {
                        PHDR pHdr = pPdb->Symd->TpiHdr;
                        ULONG dTypes = pHdr->tiMac - pHdr->tiMin;
                        ULONG dBase = pHdr->cbHdr;
                        PVOID pData = ((PUCHAR)pHdr + dBase);
                        PlfRecord plr;

                        printf("TPI version: %lu\nIndex range: %lX..%lX\nType count: %lu\n",
                            pHdr->vers, pHdr->tiMin, pHdr->tiMac - 1, dTypes);

                        for (ULONG i = 0; i < dTypes; i++)
                        {
                            ULONG dSize = *(PWORD)pData;
                            dBase += sizeof(WORD);
                            plr = (PlfRecord)((PUCHAR)pHdr + dBase);

                            //printf("// %6lX: %04hX %08lX\n", pHdr->tiMin + i, plr->leaf, dBase - sizeof(WORD));

                            ProcessTypeRecord(pHdr, plr);

                            dBase += dSize;
                            pData = (char*)pHdr + dBase;
                        }
                    }

                    PSYM Sym = pPdb->Symd->SymRecs;
                    while (Sym < pPdb->Symd->SymMac)
                    {
                        if (Sym->Sym.rectyp)
                        {
                            switch (Sym->Sym.rectyp)
                            {                                
                                case S_UDT: loadUDT(this, Sym, pPdb->Symd, db, session); break;
                                case S_PUB32: loadPub32(this, Sym, pPdb->Symd, db, session); break;
                                case S_LDATA32: loadData32(this, Sym, pPdb->Symd, db, session); break;
                                case S_GDATA32: loadData32(this, Sym, pPdb->Symd, db, session); break;
                                default:
                                    break;
                            }
                        }

                        Sym = NextSym(Sym);
                    }

                    PdbClose(pPdb);
                }                
            }
        }
    }
}

void c_PELoader::loadImports(s_igorDatabase * db, BFile reader)
{
    IMAGE_DATA_DIRECTORY* pImportDirectory = &m_imageDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

    igorAddress startImportTableAddressVirtual(db, m_ImageBase + pImportDirectory->VirtualAddress, -1);
    igorAddress importTableAddressVirtual = startImportTableAddressVirtual;

    while (importTableAddressVirtual < startImportTableAddressVirtual + pImportDirectory->Size)
    {
        u32 originalFirstThunkRVA = db->readU32(importTableAddressVirtual); importTableAddressVirtual += 4;

        if (originalFirstThunkRVA == 0)
            break;

        u32 timestamp = db->readU32(importTableAddressVirtual); importTableAddressVirtual += 4;
        u32 forwardChain = db->readU32(importTableAddressVirtual); importTableAddressVirtual += 4;
        u32 nameRVA = db->readU32(importTableAddressVirtual); importTableAddressVirtual += 4;
        u32 firstThunkRVA = db->readU32(importTableAddressVirtual); importTableAddressVirtual += 4;

        Balau::String name;
        igorAddress imageBase(db->m_sessionId, m_ImageBase, -1);
        db->readString(imageBase + nameRVA, name);

        u32 importFunctionIndex = 0;
        igorAddress thunkAddress = imageBase + originalFirstThunkRVA;
        do
        {
            u64 functionNameRVA;

            switch (m_Machine)
            {
                case IMAGE_FILE_MACHINE_I386:
                    functionNameRVA = db->readU32(thunkAddress);
                    thunkAddress += 4;
                    break;
                case IMAGE_FILE_MACHINE_AMD64:
                    functionNameRVA = db->readU64(thunkAddress);
                    thunkAddress += 8;
                    break;
                default:
                    Failure("Unknown machine type");
            }

            if (functionNameRVA == 0)
            {
                break;
            }

            if ((m_Machine == IMAGE_FILE_MACHINE_I386) && (functionNameRVA & 0x80000000))
            {
                continue;
            }

            if ((m_Machine == IMAGE_FILE_MACHINE_AMD64) && (functionNameRVA & 0x8000000000000000))
            {
                continue;
            }

            {
                u16 functionId = db->readU16(imageBase + functionNameRVA);
                Balau::String functionName;
                db->readString(imageBase + functionNameRVA + 2, functionName);

                if (m_Machine == IMAGE_FILE_MACHINE_I386)
                {
                    db->declare_variable(imageBase + firstThunkRVA + 4 * importFunctionIndex, s_igorDatabase::TYPE_U32); // should we have a db type like "function pointer"?
                    db->declare_name(imageBase + firstThunkRVA + 4 * importFunctionIndex, functionName);
                }
                else
                {
                    db->declare_variable(imageBase + firstThunkRVA + 8 * importFunctionIndex, s_igorDatabase::TYPE_U64); // should we have a db type like "function pointer"?
                    db->declare_name(imageBase + firstThunkRVA + 8 * importFunctionIndex, functionName);
                }
            }

            importFunctionIndex++;
        } while (1);
    }
}
