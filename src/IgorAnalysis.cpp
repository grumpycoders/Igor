#include "IgorAnalysis.h"

#include <Printer.h>
#include <BString.h>
#include <TaskMan.h>

using namespace Balau;

const char * IgorAnalysisManager::getStatusString() {
    switch (m_status) {
        case IDLE: return "IDLE"; break;
        case RUNNING: return "RUNNING"; break;
        case STOPPING: return "STOPPING"; break;    
    }
    return "ERROR";
}

void IgorAnalysisManager::add_code_analysis_task(u64 PC)
{
    s_analysisRequest * newAnalysisRequest = new s_analysisRequest(PC);

	m_pDatabase->m_analysisRequests.push(newAnalysisRequest);
}

void IgorAnalysisManager::Do()
{
    if (!m_pDatabase)
        return;

    waitFor(m_pDatabase->m_analysisRequests.getEvent());

    Printer::log(M_INFO, "AnalysisManager starting...");

    for (;;)
    {
        for (auto i = m_evts.begin(); i != m_evts.end(); i++)
        {
            Events::TaskEvent * evt = i->first;
            if (evt->gotSignal())
            {
                evt->ack();
                delete evt;
                m_evts.erase(i);
                i = m_evts.begin();
                if (m_evts.empty())
                    break;
            }
        }
        if (m_evts.size() == 0)
        {
            if (m_status == STOPPING)
                return;
            else if (m_pDatabase->m_analysisRequests.isEmpty())
                m_status = IDLE;
        }

        if (m_status == IDLE)
            Printer::log(M_INFO, "AnalysisManager going idle...");

        m_pDatabase->m_analysisRequests.getEvent()->resetMaybe();
        if (m_pDatabase->m_analysisRequests.isEmpty()) {
            yield();
            continue;
        }
        s_analysisRequest* pRequest = m_pDatabase->m_analysisRequests.pop();
        u64 currentPC = pRequest->m_pc;

        if (m_status == STOPPING || currentPC == -1)
        {
            m_status = STOPPING;
            continue;
        }

        m_status = RUNNING;

        Events::TaskEvent * evt = new Events::TaskEvent;
        m_evts.push_back(std::pair<Events::TaskEvent *, u64>(evt, currentPC));
        TaskMan::registerTask(new IgorAnalysis(m_pDatabase, currentPC, this), evt);
        waitFor(evt);
        yield();
    }
}

void IgorAnalysis::Do()
{
    u64 currentPC = m_PC;
	c_cpu_module* pCpu = m_pDatabase->getCpuForAddress(currentPC);
    c_cpu_state* pCpuState = m_pDatabase->getCpuStateForAddress(currentPC);

    if (!pCpu)
        return;
	s_analyzeState analyzeState;
	analyzeState.m_PC = currentPC;
	analyzeState.pCpu = pCpu;
	analyzeState.pCpuState = pCpuState;
    analyzeState.pDataBase = m_pDatabase;
    analyzeState.pAnalysis = m_parent;
	analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

	analyzeState.m_analyzeResult = e_analyzeResult::continue_analysis;

    u8 counter = 0;

	do
	{
        if (m_pDatabase->is_address_flagged_as_code(analyzeState.m_PC))
		{
			break;
		}

		if (pCpu->analyze(&analyzeState) != IGOR_SUCCESS)
		{
			analyzeState.m_analyzeResult = e_analyzeResult::stop_analysis;
		}

        m_pDatabase->flag_address_as_instruction(analyzeState.m_cpu_analyse_result->m_startOfInstruction, analyzeState.m_cpu_analyse_result->m_instructionSize);

		s_igorDatabase::s_symbolDefinition* pSymbol = m_pDatabase->get_Symbol(analyzeState.m_cpu_analyse_result->m_startOfInstruction);
		if (pSymbol)
		{
			if (pSymbol->m_name.strlen())
			{
				Printer::log(M_INFO, pSymbol->m_name);
			}
		}
				
        if (++counter == 0)
            yieldNoWait();

    } while (analyzeState.m_analyzeResult == e_analyzeResult::continue_analysis);

	delete analyzeState.m_cpu_analyse_result;
	analyzeState.m_cpu_analyse_result = NULL;
}
