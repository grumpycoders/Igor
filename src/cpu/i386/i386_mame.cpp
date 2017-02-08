#include "i386_mame.h"
#include "emu.h"
#include "IgorSession.h"

#include <assert.h>
#include <mutex>

flags_t i386_dasm_one(std::ostream &stream, offs_t eip, const uint8_t *oprom, int mode);

c_i386_mame::c_i386_mame()
{
}

int c_i386_mame::analyze(s_analyzeState* pState, Balau::String& outputString, bool bUseColor)
{
	std::stringstream test;

	unsigned char opcode[15];
	for (int i = 0; i < 15; i++)
	{
		opcode[i] = pState->pSession->readU8(pState->m_PC + i);
	}

	flags_t flags = i386_dasm_one(test, pState->m_PC.offset, opcode,64);

	pState->m_cpu_analyse_result->m_startOfInstruction = pState->m_PC;
	pState->m_cpu_analyse_result->m_instructionSize = flags & DASMFLAG_LENGTHMASK;

	pState->m_PC += flags & DASMFLAG_LENGTHMASK;

	if (flags & DASMFLAG_STOP)
	{
		pState->m_analyzeResult = stop_analysis;
	}

	Balau::String testString(test.str());
	outputString = testString;

	return IGOR_SUCCESS;
}

const char* g_i386_supported_cpus[] =
{
	"x86",
	"x64",
	NULL,
};

const s_cpuInfo g_i386CpuInfo =
{
	"mame",
	g_i386_supported_cpus,
	c_i386_mame::create,
};

void c_i386_mame::registerCpuModule(std::vector<const s_cpuInfo*>& cpuList)
{
	cpuList.push_back(&g_i386CpuInfo);
}
