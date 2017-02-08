#include "IgorAnalysis.h"
#include "IgorSession.h"
#include "IgorLocalSession.h"

#include <Printer.h>
#include <BString.h>
#include <TaskMan.h>

#include "IgorMemory.h"

using namespace Balau;

void IgorAnalysis::Do()
{
    u8 counter = 0;

    StacklessBegin();

    m_pCpu = m_pDatabase->getCpuForAddress(m_PC);
    m_pCpuState = m_pDatabase->getCpuStateForAddress(m_PC);

    if (!m_pCpu)
        return;
    
    m_analyzeState.m_PC = m_PC;
    m_analyzeState.pCpu = m_pCpu;
    m_analyzeState.pCpuState = m_pCpuState;
    m_analyzeState.pSession = m_session;
    m_analyzeState.m_cpu_analyse_result = m_pCpu->allocateCpuSpecificAnalyseResult();

    m_analyzeState.m_analyzeResult = e_analyzeResult::continue_analysis;
	

    do
    {
        if (m_pDatabase->is_address_flagged_as_code(m_analyzeState.m_PC))
        {
            break;
        }

        if (m_pCpu->analyze(&m_analyzeState, resultString) != IGOR_SUCCESS)
        {
            m_analyzeState.m_analyzeResult = e_analyzeResult::stop_analysis;
        }
        else
        {
            m_pDatabase->flag_address_as_instruction(m_analyzeState.m_cpu_analyse_result->m_startOfInstruction, m_analyzeState.m_cpu_analyse_result->m_instructionSize);
            m_session->add_instruction();

            m_pCpu->generateReferences(&m_analyzeState);
        }

        m_instructionsCounter++;
                
        if (++counter == 50)
        {
            //Printer::log(M_INFO, "Analysis Task %p yields after %" PRIu64 " instructions at %016" PRIx64, this, m_instructionsCounter, m_analyzeState.m_PC.offset);
            StacklessYield();
        }

    } while (m_analyzeState.m_analyzeResult == e_analyzeResult::continue_analysis);

    if (m_instructionsCounter >= 1024)
        Printer::log(M_INFO, "Analysis Task %p stops after %" PRIu64 " instructions at %016" PRIx64, this, m_instructionsCounter, m_analyzeState.m_PC.offset);

    delete m_analyzeState.m_cpu_analyse_result;
    m_analyzeState.m_cpu_analyse_result = NULL;

    m_session->m_nTasks--;
    m_analysisManager->m_gotOneStop.trigger();

    StacklessEnd();
}

IgorAnalysis::IgorAnalysis(s_igorDatabase * db, igorAddress PC, IgorLocalSession * parent, class IgorAnalysisManagerLocal * manager) : m_pDatabase(db), m_PC(PC), m_session(parent), m_analysisManager(manager)
{
    m_name.set("IgorAnalysis for %016" PRIx64, PC.offset);
    m_session->addRef();
}

IgorAnalysis::~IgorAnalysis()
{
    m_session->release();
}
