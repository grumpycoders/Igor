#include <stdio.h>

#include <Exceptions.h>

#include "Igor.h"
#include "IgorAnalysis.h"
#include "IgorUtils.h"

using namespace Balau;

static bool vararg_lambda(std::function<bool(const char * fmt, va_list ap)> output, const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool r = output(fmt, ap);
    va_end(ap);
    return r;
}

bool igor_export_to_text(std::function<bool(const char * fmt, va_list ap)> output, IgorSession * session) {
    bool success = true;

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

            success = vararg_lambda(output, "%016llx  %s\n", analyzeState.m_cpu_analyse_result->m_startOfInstruction, disassembledString.to_charp(0));
            if (!success)
                break;

            analyzeState.m_PC = analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize;
        }
        else
        {
            u8 byte;
            session->readU8(analyzeState.m_PC, byte);

            success = vararg_lambda(output, "%016llx  0x%02X\n", analyzeState.m_PC, byte);
            if (!success)
                break;

            analyzeState.m_PC++;
        }
    }

    delete analyzeState.m_cpu_analyse_result;

    return success;
}

bool igor_export_to_file(const char * exportPath, IgorSession * session) {
    FILE * f = fopen(exportPath, "w+");
    if (!f)
        return false;

    ScopedLambda sl([&](){ fclose(f); });

    return igor_export_to_text([&](const char * fmt, va_list ap) -> bool { vfprintf(f, fmt, ap); return true; }, session);
}
