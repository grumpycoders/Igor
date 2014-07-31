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
        reader->read(m_buffer, size);

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

    if (o->isMachO())
        targetTriple.setEnvironment(Triple::MachO);

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
    error_code ec;
    for (section_iterator i = o->begin_sections(),
        e = o->end_sections();
        i != e; i.increment(ec))
    {
        if (ec)
            break;

        StringRef sectionName;
        i->getName(sectionName);

        uint64_t sectionAddr;
        i->getAddress(sectionAddr);

        uint64_t sectionSize;
        i->getSize(sectionSize);

        StringRef sectionContents;
        i->getContents(sectionContents);

        igor_section_handle sectionHandle;
        m_session->getDB()->create_section(sectionAddr, sectionSize, sectionHandle);
        m_session->getDB()->load_section_data(sectionHandle, sectionContents.data(), sectionContents.size());
    }

    for (symbol_iterator i = o->begin_symbols(),
        e = o->end_symbols();
        i != e; i.increment(ec))
    {
        if (ec)
            break;

        uint64_t symbolAddr;
        i->getAddress(symbolAddr);

        StringRef symbolName;
        i->getName(symbolName);

        igorAddress symbolAddress(m_session, symbolAddr, -1);
        Balau::String name(symbolName.begin());
        m_session->getDB()->declare_name(symbolAddress, name);
    }

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

        igorAddress symbolAddress(m_session, symbolAddr, -1);
        Balau::String name(symbolName.begin());
        m_session->getDB()->declare_name(symbolAddress, name);

        if (symbolType == SymbolRef::Type::ST_Function)
        {
            m_session->add_code_analysis_task(symbolAddress);
        }
    }

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
    LLVMMemoryBuffer* pllvmMemoryBuffer = new LLVMMemoryBuffer(reader);
    OwningPtr<Binary> binary;
    if (createBinary(pllvmMemoryBuffer, binary))
    {
        return IGOR_FAILURE;
    }

    if (Archive *a = dyn_cast<Archive>(binary.get()))
    {
        // Archive
        for (Archive::child_iterator i = a->begin_children(),
            e = a->end_children(); i != e; ++i) {
            OwningPtr<Binary> child;
            /*if (error_code ec = i->getAsBinary(child)) {
                // Ignore non-object files.
                if (ec != object_error::invalid_file_type)
                    errs() << ToolName << ": '" << a->getFileName() << "': " << ec.message()
                    << ".\n";
                continue;
            }
            if (ObjectFile *o = dyn_cast<ObjectFile>(child.get()))
                DumpObject(o);
            else
                errs() << ToolName << ": '" << a->getFileName() << "': "
                << "Unrecognized file type.\n";*/
        }
    }
    else if (ObjectFile *o = dyn_cast<ObjectFile>(binary.get()))
    {
        // Object
        loadObject(o);
    }

    return IGOR_SUCCESS;
}
