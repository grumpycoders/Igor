#include "sh2.h"

#include "emu.h"

#include "IgorSession.h"

c_sh2::c_sh2()
{

}

flags_t DasmSH2(char *buffer, unsigned pc, UINT16 opcode);

igor_result c_sh2::analyze(s_analyzeState* pState)
{
    char printBuffer[1024];

    u16 opcode0 = pState->pSession->readU8(pState->m_PC);
    u16 opcode1 = pState->pSession->readU8(pState->m_PC + 1);

    flags_t flags = DasmSH2(printBuffer, pState->m_PC.offset, (opcode0 << 8) | opcode1);

    pState->m_cpu_analyse_result->m_startOfInstruction = pState->m_PC;
    pState->m_analyzeResult = continue_analysis;
    pState->m_cpu_analyse_result->m_instructionSize = 2;
    pState->m_PC += 2;

	pState->m_disassembly = printBuffer;

	if (flags & DASMFLAG_STOP)
	{
		pState->m_analyzeResult = stop_analysis;
	}

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

