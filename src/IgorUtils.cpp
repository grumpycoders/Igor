#include <stdio.h>

#include <Exceptions.h>

#include "Igor.h"
#include "IgorAnalysis.h"
#include "IgorUtils.h"

using namespace Balau;

bool igor_export_to_text(const char * exportPath, IgorSession * session) {
	FILE * fHandle = fopen(exportPath, "w+");
	if (!fHandle)
		return false;

	igorAddress entryPointAddress = session->getEntryPoint();

	c_cpu_module* pCpu = session->getCpuForAddress(entryPointAddress);
	igor_section_handle sectionHandle = session->getSectionFromAddress(entryPointAddress);

	igorAddress sectionStart = session->getSectionStartVirtualAddress(sectionHandle);
	u64 sectionSize = session->getSectionSize(sectionHandle);

	s_analyzeState analyzeState;
	analyzeState.m_PC = sectionStart;
	analyzeState.pCpu = pCpu;
	analyzeState.pCpuState = session->getCpuStateForAddress(sectionStart);
	analyzeState.pSession = session;
	analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

	while (analyzeState.m_PC < sectionStart + sectionSize)
	{
		if (session->is_address_flagged_as_code(analyzeState.m_PC) && (pCpu->analyze(&analyzeState) == IGOR_SUCCESS))
		{
			String disassembledString;
			pCpu->printInstruction(&analyzeState, disassembledString);

			fprintf(fHandle, "%s\n", disassembledString.to_charp(0));

			analyzeState.m_PC = analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize;
		}
		else
		{
			u8 byte;
			session->readU8(analyzeState.m_PC, byte);

			fprintf(fHandle, "0x%02X\n", byte);

			analyzeState.m_PC++;
		}
	}

	delete analyzeState.m_cpu_analyse_result;
	fclose(fHandle);

	return true;
}
