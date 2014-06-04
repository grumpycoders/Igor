#include "IgorDatabase.h"
#include "IgorSession.h"
#include "cpuModule.h"

const char* c_cpu_module::startColor(e_colors color, bool bUseColor)
{
    if (!bUseColor)
    {
        return "";
    }

    switch (color)
    {
    case RESET_COLOR:
        return ";C=DEFAULT;";
    case KNOWN_SYMBOL:
        return ";C=KNOWN_SYMBOL;";
    case MNEMONIC_DEFAULT:
        return ";C=MNEMONIC_DEFAULT;";
    case MNEMONIC_FLOW_CONTROL:
        return ";C=MNEMONIC_FLOW_CONTROL;";
    case OPERAND_REGISTER:
        return ";C=OPERAND_REGISTER;";
    case OPERAND_IMMEDIATE:
        return ";C=OPERAND_IMMEDIATE;";
    default:
        Failure("Unimplemented color");
    }

    return "";
}

const char* c_cpu_module::finishColor(e_colors color, bool bUseColor)
{
    if (!bUseColor)
    {
        return "";
    }

    return ";C=DEFAULT;";
}

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
