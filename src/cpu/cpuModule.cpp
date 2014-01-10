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
        return ";C=BLUE;";
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