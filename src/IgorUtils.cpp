#include <stdio.h>

#include <Exceptions.h>

#include "Igor.h"
#include "IgorSession.h"
#include "IgorUtils.h"

#include "IgorMemory.h"

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
    igor_segment_handle segmentHandle = session->getSegmentFromAddress(entryPointAddress);

    igorAddress sectionStart = session->getSegmentStartVirtualAddress(segmentHandle);
    u64 sectionSize = session->getSegmentSize(segmentHandle);

    s_analyzeState analyzeState;
    analyzeState.m_PC = sectionStart;
    analyzeState.pCpu = pCpu;
    analyzeState.pCpuState = session->getCpuStateForAddress(sectionStart);
    analyzeState.pSession = session;
    analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

    while (analyzeState.m_PC < sectionStart + sectionSize)
    {
        String symbolName;
        if (session->getSymbolName(analyzeState.m_PC, symbolName))
        {
            success = vararg_lambda(output, "%s:\n", symbolName.to_charp());
            if (!success)
                break;
        }

        if (session->is_address_flagged_as_code(analyzeState.m_PC) && (pCpu->analyze(&analyzeState) == IGOR_SUCCESS))
        {
            success = vararg_lambda(output, "%016llx  %s\n", analyzeState.m_cpu_analyse_result->m_startOfInstruction.offset, analyzeState.m_disassembly.to_charp(0));
            if (!success)
                break;

            analyzeState.m_PC = analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize;
        }
        else
        {
            u8 byte;
            session->readU8(analyzeState.m_PC, byte);

            success = vararg_lambda(output, "%016llx  0x%02X\n", analyzeState.m_PC.offset, byte);
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
