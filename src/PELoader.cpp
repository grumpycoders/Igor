#include "PELoader.h"
#include "IgorAPI.h"
#include "IgorDatabase.h"
#include "cpu/x86/cpu_x86.h"

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

s_igorDatabase * c_PELoader::loadPE(BFile reader, IgorAnalysis * analysis)
{
    s_igorDatabase * db = new s_igorDatabase;
    bool success = false;
    ScopedLambda sl([&]() { if (!success) delete db; });
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
		for(int i=0; i<4; i++)
		{
			e_res[i] = reader->readU16().get();
		}
		u16    e_oemid = reader->readU16().get(); // OEM identifier (for e_oeminfo)
		u16    e_oeminfo = reader->readU16().get(); // OEM information; e_oemid specific
		u16    e_res2[10]; // Reserved words
		for(int i=0; i<10; i++)
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
	m_NumberOfSections = reader->readU16().get();
	m_TimeDateStamp = reader->readU32().get();
	m_PointerToSymbolTable = reader->readU32().get();
	m_NumberOfSymbols = reader->readU32().get();
	m_SizeOfOptionalHeader = reader->readU16().get();
	m_Characteristics = reader->readU16().get();

	u64 optionalHeaderOffset = reader->tell();

	igor_cpu_handle cpuHandle;
	c_cpu_x86* pCpu = new c_cpu_x86();
    db->igor_add_cpu(pCpu, cpuHandle);

	switch(m_Machine)
	{
	case IMAGE_FILE_MACHINE_I386:
		loadOptionalHeader386(reader);
		break;
	case IMAGE_FILE_MACHINE_AMD64:
		loadOptionalHeader64(reader);
		break;
	default:
		Failure("Unknown machine type");
	}

	//IMAGE_SECTION_HEADER
	for(int i=0; i<m_NumberOfSections; i++)
	{
		reader->seek(optionalHeaderOffset + m_SizeOfOptionalHeader + i*40);

		u8      Name[8];
		reader->read(Name, 8);
		u32     Misc = reader->readU32().get(); // u32   PhysicalAddress union with u32   VirtualSize; (depends if it's a DLL or a .EXE)
		u32     VirtualAddress = reader->readU32().get();
		u32     SizeOfRawData = reader->readU32().get();
		u32     PointerToRawData = reader->readU32().get();
		u32     PointerToRelocations = reader->readU32().get();
		u32     PointerToLinenumbers = reader->readU32().get();
		u16     NumberOfRelocations = reader->readU16().get();
		u16     NumberOfLinenumbers = reader->readU16().get();
		u32     Characteristics = reader->readU32().get();

		igor_section_handle sectionHandle;
		igor_create_section(db, m_ImageBase + VirtualAddress, Misc, sectionHandle);

		// IMAGE_SCN_CNT_CODE
		if(Characteristics & 0x00000020)
		{
			igor_set_section_option(db, sectionHandle, IGOR_SECTION_OPTION_CODE);
		}

		//IMAGE_SCN_MEM_EXECUTE
		if(Characteristics & 0x20000000)
		{
            igor_set_section_option(db, sectionHandle, IGOR_SECTION_OPTION_EXECUTE);
		}

		//IMAGE_SCN_MEM_READ
		if(Characteristics & 0x40000000)
		{
            igor_set_section_option(db, sectionHandle, IGOR_SECTION_OPTION_READ);
		}

		reader->seek(PointerToRawData);
        igor_load_section_data(db, sectionHandle, reader, SizeOfRawData);

	}

	analysis->igor_add_code_analysis_task(m_ImageBase + m_EntryPoint);
    analysis->setDB(db);

    success = true;

	return db;
}

int c_PELoader::loadOptionalHeader386(BFile reader)
{
	u16                  Magic = reader->readU16().get();
	IAssert(Magic == 0x10b, "Magic isn't IMAGE_NT_OPTIONAL_HDR32_MAGIC: %016x", Magic); // IMAGE_NT_OPTIONAL_HDR32_MAGIC 


	u8  		         MajorLinkerVersion = reader->readU8().get();
	u8                   MinorLinkerVersion = reader->readU8().get();
	u32                  SizeOfCode = reader->readU32().get();
	u32                  SizeOfInitializedData = reader->readU32().get();
	u32                  SizeOfUninitializedData = reader->readU32().get();
	m_EntryPoint = reader->readU32().get();
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

	//IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];

	struct IMAGE_DATA_DIRECTORY
	{
		u32     VirtualAddress;
		u32     Size;
	} imageDirectory[16];

	//IMAGE_DATA_DIRECTORY
	for(int i=0; i<16; i++)
	{
		imageDirectory[i].VirtualAddress = reader->readU32().get();
		imageDirectory[i].Size = reader->readU32().get();
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
	m_EntryPoint = reader->readU32().get();
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

	/*
	#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
	#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
	#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
	#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
	#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
	#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
	#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
	//      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
	#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
	#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
	#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
	#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
	#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
	#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
	#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
	#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor
	*/

	struct IMAGE_DATA_DIRECTORY
	{
		u32     VirtualAddress;
		u32     Size;
	} imageDirectory[16];

	//IMAGE_DATA_DIRECTORY
	for(int i=0; i<16; i++)
	{
		imageDirectory[i].VirtualAddress = reader->readU32().get();
		imageDirectory[i].Size = reader->readU32().get();
	}

	return 0;
}