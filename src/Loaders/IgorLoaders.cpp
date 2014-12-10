#include "IgorLoaders.h"

#include "llvm/llvmLoader.h"
#include "PE/PELoader.h"
#include "Dmp/dmpLoader.h"
#include "Elf/elfLoader.h"

std::vector<c_IgorLoaders::s_registeredLoader> c_IgorLoaders::m_loaders;

void c_IgorLoaders::initialize()
{
    // llvm
    {
        s_registeredLoader llvmLoader;
        llvmLoader.m_isSupported = &c_LLVMLoader::isSupported;
        llvmLoader.m_createLoader = &c_LLVMLoader::createLoader;
        m_loaders.push_back(llvmLoader);
    }

    // PE
    {
        s_registeredLoader PELoader;
        PELoader.m_isSupported = &c_PELoader::isSupported;
        PELoader.m_createLoader = &c_PELoader::createLoader;
        m_loaders.push_back(PELoader);
    }

    /*
    m_loaders.push_back(new c_dmpLoader);
    m_loaders.push_back(new c_elfLoader);
    */
}

igor_result c_IgorLoaders::load(String fileName, BFile reader, IgorLocalSession *session)
{
    for (u32 i = 0; i < m_loaders.size(); i++)
    {
        if (m_loaders[i].m_isSupported(fileName))
        {
            c_IgorLoader* pLoader = m_loaders[i].m_createLoader();

            if (pLoader->load(reader, session))
            {
                delete pLoader;
                return IGOR_SUCCESS;
            }

            delete pLoader;
        }
    }

    return IGOR_FAILURE;
}
