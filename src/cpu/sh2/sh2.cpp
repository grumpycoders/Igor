#include "sh2.h"

#include <stdint.h>
#define UINT32 uint32_t
#define UINT16 uint16_t
#define INT32 int32_t

#include "IgorSession.h"

c_sh2::c_sh2()
{

}

unsigned DasmSH2(char *buffer, unsigned pc, UINT16 opcode);

igor_result c_sh2::analyze(s_analyzeState* pState, Balau::String& outputString, bool bUseColor)
{
    char printBuffer[1024];

    u16 opcode0 = pState->pSession->readU8(pState->m_PC);
    u16 opcode1 = pState->pSession->readU8(pState->m_PC + 1);

    unsigned int flags = DasmSH2(printBuffer, pState->m_PC.offset, (opcode0 << 8) | opcode1);

    pState->m_cpu_analyse_result->m_startOfInstruction = pState->m_PC;
    pState->m_analyzeResult = continue_analysis;
    pState->m_cpu_analyse_result->m_instructionSize = 2;
    pState->m_PC += 2;

	outputString = printBuffer;

    return IGOR_SUCCESS;
}

const char* g_sh2_supported_cpus[] =
{
    "sh2",
    NULL,
};

const s_cpuInfo g_sh2CpuInfo =
{
    "sh2",
    g_sh2_supported_cpus,
    c_sh2::create,
};

void c_sh2::registerCpuModule(std::vector<const s_cpuInfo*>& cpuList)
{
    cpuList.push_back(&g_sh2CpuInfo);
}

