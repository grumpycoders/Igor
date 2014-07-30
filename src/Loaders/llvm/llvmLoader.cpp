#include "llvmLoader.h"

#include "llvm/Object/Archive.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/MachO.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/ErrorOr.h"

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

    virtual BufferKind getBufferKind() const
    {
        return MemoryBuffer_Malloc;
    }

private:
    char* m_buffer;
};

igor_result c_LLVMLoader::loadObject(ObjectFile* o)
{
    return IGOR_FAILURE;
}

igor_result c_LLVMLoader::load(BFile reader, IgorLocalSession *session)
{
    // Attempt to open the binary.
    LLVMMemoryBuffer llvmMemoryBuffer(reader);
    OwningPtr<Binary> binary;
    if (createBinary(&llvmMemoryBuffer, binary))
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

    return IGOR_FAILURE;
}
