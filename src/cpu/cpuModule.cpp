#include "IgorDatabase.h"
#include "IgorSession.h"
#include "cpuModule.h"

#include "x86_llvm/cpu_x86_llvm.h"
#include "sh2/sh2.h"
#include "i386/i386_mame.h"

std::vector<const s_cpuInfo*> c_cpu_factory::m_list;

void c_cpu_factory::initialize()
{
    //c_cpu_x86_llvm::registerCpuModule(m_list);
	c_i386_mame::registerCpuModule(m_list);
    c_sh2::registerCpuModule(m_list);
}

c_cpu_module * c_cpu_factory::createCpuFromString(const Balau::String & desc)
{
    for (int i = 0; i < m_list.size(); i++)
    {
        const char** thisCpuList = m_list[i]->m_supportedCpuList;
        while (*thisCpuList)
        {
            if (Balau::String(*thisCpuList) == desc)
            {
                return m_list[i]->m_cpuContructionFunc(NULL);
            }
            
            thisCpuList++;
        }
    }

    return NULL;
}

void c_cpu_factory::getCpuList(std::vector<Balau::String>& cpuList)
{
    c_cpu_module * cpu = NULL;

    for(int i=0; i<m_list.size(); i++)
    {
        const char** thisCpuList = m_list[i]->m_supportedCpuList;
        while (*thisCpuList)
        {
            cpuList.push_back(Balau::String(*thisCpuList));
            thisCpuList++;
        }
    }
}

uint32_t c_cpu_module::getColorForType(c_cpu_module::e_colors blockType)
{
    switch (blockType)
    {
    case c_cpu_module::DEFAULT:
    default:
        return 0xFFFFFFFF;
    case c_cpu_module::KNOWN_SYMBOL:
        return 0xFFE08030;
    case c_cpu_module::MNEMONIC_DEFAULT:
        return 0xFF775577;
    case c_cpu_module::MNEMONIC_FLOW_CONTROL:
        return 0xFF0000FF;
    case c_cpu_module::OPERAND_REGISTER:
        return 0xFF00FF00;
    case c_cpu_module::OPERAND_IMMEDIATE:
        return 0xFFFFFF00;
    case c_cpu_module::MEMORY_ADDRESS:
        return 0xFF770055;
    }
}

