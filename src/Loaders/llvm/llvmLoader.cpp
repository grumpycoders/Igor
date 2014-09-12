#include <stdint.h>

#include "llvmLoader.h"
#include "IgorLocalSession.h"
#include "cpu/x86_llvm/cpu_x86_llvm.h"

#include "llvm/Object/Archive.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/MachO.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/TargetRegistry.h"

#include <system_error>

using namespace llvm;
using namespace object;

class LLVMMemoryBuffer : public MemoryBuffer
{
public:
    LLVMMemoryBuffer(BFile& reader)
    {
        off64_t size = reader->getSize();

        m_buffer = new char[size];
        ssize_t r = reader->read(m_buffer, size);
        AAssert(r == size, "Asked for %" PRIu64 " bytes, but got %zi instead", size, r);

        init(m_buffer, m_buffer + size, false);
    }

    ~LLVMMemoryBuffer()
    {
        delete[] m_buffer;
    }

    virtual BufferKind getBufferKind() const
    {
        return MemoryBuffer_Malloc;
    }

private:
    char* m_buffer;
};

igor_result c_LLVMLoader::loadObject(ObjectFile* o)
{
    // get target
    llvm::Triple targetTriple("unknown-unknown-unknown");
    targetTriple.setArch(Triple::ArchType(o->getArch()));

	if (o->isCOFF())
		return IGOR_FAILURE;

    if (o->isMachO())
        targetTriple.setObjectFormat(Triple::MachO);

    std::string outputError;
    const Target* objectTarget = TargetRegistry::lookupTarget("", targetTriple, outputError);
    if (objectTarget == NULL)
        return IGOR_FAILURE;

    // Figure out the matching CPU module for Igor
    c_cpu_x86_llvm* pCpu = NULL;
    if (!strcmp(objectTarget->getName(), "x86-64"))
    {
        pCpu = new c_cpu_x86_llvm(c_cpu_x86_llvm::X64);
    }
    else if (!strcmp(objectTarget->getName(), "x86"))
    {
        pCpu = new c_cpu_x86_llvm(c_cpu_x86_llvm::X86);
    }

    igor_cpu_handle cpuHandle;
    if (pCpu)
    {
        m_session->getDB()->igor_add_cpu(pCpu, cpuHandle);
    }

    // iterate over sections
    for (const SectionRef &section : o->sections())
    {
        StringRef sectionName;
        section.getName(sectionName);

        uint64_t sectionAddr;
        section.getAddress(sectionAddr);

        uint64_t sectionSize;
        section.getSize(sectionSize);

        StringRef sectionContents;
        section.getContents(sectionContents);

        igor_section_handle sectionHandle;
        m_session->getDB()->create_section(sectionAddr, sectionSize, sectionHandle);
        m_session->getDB()->load_section_data(sectionHandle, sectionContents.data(), sectionContents.size());

        // relocation in this section
#if 0
        for (relocation_iterator relocation = i->begin_relocations(),
            e = i->end_relocations();
            relocation != e; relocation.increment(ec))
        {
            if (ec)
                break;

            SmallString<256> relocationTypeName;
            relocation->getTypeName(relocationTypeName);

            if (strstr(relocationTypeName.c_str(), "_JUMP_SLOT"))
            {
                symbol_iterator symbol = relocation->getSymbol();

                StringRef symbolName;
                i->getName(symbolName);
            }
        }
#endif
    }

    for (const SymbolRef &symbol : o->symbols())
    {
        uint64_t symbolAddr;
        symbol.getAddress(symbolAddr);

        StringRef symbolName;
        symbol.getName(symbolName);

        igorAddress symbolAddress(m_session, symbolAddr, -1);
        Balau::String name(symbolName.begin());
        m_session->getDB()->declare_name(symbolAddress, name);
    }
    /*
    for (symbol_iterator i = o->begin_dynamic_symbols(),
        e = o->end_dynamic_symbols();
        i != e; i.increment(ec))
    {
        if (ec)
            break;

        uint64_t symbolAddr;
        i->getAddress(symbolAddr);

        StringRef symbolName;
        i->getName(symbolName);

        SymbolRef::Type symbolType;
        i->getType(symbolType);

        uint32_t symbolFlags;
        i->getFlags(symbolFlags);

        igorAddress symbolAddress(m_session, symbolAddr, -1);
        Balau::String name(symbolName.begin());
        m_session->getDB()->declare_name(symbolAddress, name);

        if (symbolType == SymbolRef::Type::ST_Function)
        {
            m_session->add_code_analysis_task(symbolAddress);
        }
    }
    */
    /*
    for (library_iterator i = o->begin_libraries_needed(),
        e = o->end_libraries_needed();
        i != e; i.increment(ec))
    {
        if (ec)
            break;
    }
    */

    if (o->isELF())
    {
        igorAddress entryPointAddress;

        if (const ELF64LEObjectFile *elf = dyn_cast<ELF64LEObjectFile>(o))
        {
            entryPointAddress = igorAddress(m_session, elf->getELFFile()->getHeader()->e_entry, -1);
        }
        if (const ELF64BEObjectFile *elf = dyn_cast<ELF64BEObjectFile>(o))
        {
            entryPointAddress = igorAddress(m_session, elf->getELFFile()->getHeader()->e_entry, -1);
        }
        if (const ELF32LEObjectFile *elf = dyn_cast<ELF32LEObjectFile>(o))
        {
            entryPointAddress = igorAddress(m_session, elf->getELFFile()->getHeader()->e_entry, -1);
        }
        if (const ELF32BEObjectFile *elf = dyn_cast<ELF32BEObjectFile>(o))
        {
            entryPointAddress = igorAddress(m_session, elf->getELFFile()->getHeader()->e_entry, -1);
        }

        if (entryPointAddress.isValid())
        {
            m_session->getDB()->m_entryPoint = entryPointAddress;
            m_session->add_code_analysis_task(entryPointAddress);
        }
    }

    return IGOR_SUCCESS;
}

igor_result c_LLVMLoader::load(BFile reader, IgorLocalSession *session)
{
    m_session = session;

    // Attempt to open the binary.
    std::unique_ptr<MemoryBuffer> pllvmMemoryBuffer(new LLVMMemoryBuffer(reader));
    ErrorOr<Binary *> BinaryOrErr = createBinary(std::move(pllvmMemoryBuffer), NULL);
    if (BinaryOrErr.getError())
    {
        return IGOR_FAILURE;
    }

    if (Archive *a = dyn_cast<Archive>(BinaryOrErr.get()))
    {
    }
    else if (ObjectFile *o = dyn_cast<ObjectFile>(BinaryOrErr.get()))
    {
        // Object
        return  loadObject(o);
    }

    return IGOR_FAILURE;
}
