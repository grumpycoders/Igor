#include "IgorDatabase.h"
#include "IgorSession.h"
#include "cpuModule.h"

std::vector<c_cpu_factory*> c_cpu_factory::m_list;

c_cpu_module * c_cpu_factory::createCpuFromString(const Balau::String & desc)
{
    c_cpu_module * cpu = NULL;

    for (auto & factory : m_list)
    {
        cpu = factory->maybeCreateCpu(desc);
        if (cpu)
            return cpu;
    }

    return cpu;
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
