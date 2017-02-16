#include "i386_mame.h"
#include "emu.h"
#include "IgorSession.h"

#include <assert.h>
#include <mutex>

flags_t i386_dasm_one(std::ostream &stream, offs_t eip, const uint8_t *oprom, int mode);

c_i386_mame::c_i386_mame()
{
}

int c_i386_mame::analyze(s_analyzeState* pState)
{
	std::stringstream disassemblyString;

	unsigned char opcode[15];
	for (int i = 0; i < 15; i++)
	{
		opcode[i] = pState->pSession->readU8(pState->m_PC + i);
	}

	flags_t flags = i386_dasm_one(disassemblyString, pState->m_PC.offset, opcode,64);

	pState->m_cpu_analyse_result->m_startOfInstruction = pState->m_PC;
	pState->m_cpu_analyse_result->m_instructionSize = flags & DASMFLAG_LENGTHMASK;

	pState->m_PC += flags & DASMFLAG_LENGTHMASK;

	if (flags & DASMFLAG_STOP)
	{
		pState->m_analyzeResult = stop_analysis;
	}

	pState->m_disassembly = disassemblyString.str();

	splitOperands(Balau::String(disassemblyString.str()), pState->m_operands);

	if ( flags & DASMFLAG_OPERAND_IS_CODE_ADDRESS )
	{
		igorLinearAddress linearAddress = getAsAddress(pState->m_operands[1]);

		if (linearAddress != -1)
		{
			igorAddress address(pState->m_PC.sessionId, linearAddress, pState->m_PC.segmentId);
			pState->pSession->add_code_analysis_task(address);
		}
	}

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
