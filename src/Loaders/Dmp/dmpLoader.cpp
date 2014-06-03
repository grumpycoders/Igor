#include "dmpLoader.h"
#include <Exceptions.h>
#include "IgorLocalSession.h"

using namespace Balau;

struct s_locationDescriptor
{
    void read(BFile reader)
    {
        m_size = reader->readLEU32().get();
        m_rva = reader->readLEU32().get();
    }

    u32 m_size;
    u32 m_rva;
};

struct s_memoryDescriptor
{
    void read(BFile reader)
    {
        m_startOfMemoryRange = reader->readLEU64().get();
        m_memory.read(reader);
    }

    u64 m_startOfMemoryRange;
    s_locationDescriptor m_memory;
};

igor_result c_dmpLoader::load(BFile reader, IgorLocalSession * pSession)
{
    // read the header
    u32 dmpMagic = reader->readBEU32().get();

    if (dmpMagic != 'MDMP')
    {
        return IGOR_FAILURE;
    }

    u16 versionMajor = reader->readLEU16().get();
    u16 versionMinor = reader->readLEU16().get();
    u32 numStreams = reader->readLEU32().get();
    u32 directoryRva = reader->readLEU32().get();
    u32 checksum = reader->readLEU32().get();
    u32 timestamp = reader->readLEU32().get();
    u64 flags = reader->readLEU64().get();

    for (u32 currentStreamIndex = 0; currentStreamIndex < numStreams; currentStreamIndex++)
    {
        reader->seek(directoryRva + currentStreamIndex * 0xC);

        u32 streamType = reader->readLEU32().get();
        s_locationDescriptor streamDescriptor;
        streamDescriptor.read(reader);

        reader->seek(streamDescriptor.m_rva);

        switch (streamType)
        {
            case 3: // ThreadListStream
            {
                u32 numThreads = reader->readLEU32().get();

                for (u32 threadIndex = 0; threadIndex < numThreads; threadIndex++)
                {
                    u32 threadId = reader->readLEU32().get();
                    u32 suspendCount = reader->readLEU32().get();
                    u32 priorityClass = reader->readLEU32().get();
                    u32 priority = reader->readLEU32().get();
                    u64 teb = reader->readLEU64().get();

                    s_memoryDescriptor memoryDescriptor;
                    memoryDescriptor.read(reader);

                    s_locationDescriptor locationDescriptor;
                    locationDescriptor.read(reader);
                }
                break;
            }

            case 4: // ModuleListStream
            {
                u32 numModules = reader->readLEU32().get();

                for (u32 moduleIndex = 0; moduleIndex < numModules; moduleIndex++)
                {
                    reader->seek(streamDescriptor.m_rva + 4 + 0x6C * moduleIndex);

                    u64 imageBase = reader->readLEU64().get();
                    u32 imageSize = reader->readLEU32().get();
                    u32 checksum = reader->readLEU32().get();
                    u32 timestamp = reader->readLEU32().get();
                    u32 nameRVA = reader->readLEU32().get();

                    // skip over FIXEDFILEINFO for now
                    reader->seek(0x34, SEEK_CUR);

                    s_locationDescriptor cvRecord;
                    cvRecord.read(reader);
                    s_locationDescriptor miscRecord;
                    miscRecord.read(reader);

                    u64 reserved0 = reader->readLEU64().get();
                    u64 reserved1 = reader->readLEU64().get();
                }
                break;
            }

            case 9: // Memory64ListStream
            {
                u64 numMemoryRange = reader->readLEU64().get();
                u64 baseRva = reader->readLEU64().get();

                u64 currentRva = baseRva;

                for (u64 memoryRangeIndex = 0; memoryRangeIndex < numMemoryRange; memoryRangeIndex++)
                {
                    reader->seek(streamDescriptor.m_rva + 8 + 8 + memoryRangeIndex * (8+8));

                    u64 startOfMemory = reader->readLEU64().get();
                    u64 dataSize = reader->readLEU64().get();

                    // read the data
                    reader->seek(currentRva);
                    igor_section_handle sectionHandle;
                    pSession->getDB()->create_section(startOfMemory, dataSize, sectionHandle);
                    pSession->getDB()->load_section_data(sectionHandle, reader, dataSize);

                    currentRva += dataSize;
                }
                break;
            }
            default:
                break;
        }
    }

    return IGOR_SUCCESS;
}
