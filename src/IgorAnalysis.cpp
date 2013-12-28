#include "IgorAnalysis.h"

#include <Printer.h>
#include "BString.h"
using namespace Balau;

void IgorAnalysis::igor_add_code_analysis_task(u64 PC)
{
    s_analysisRequest * newAnalysisRequest = new s_analysisRequest(PC);

	m_pDatabase->m_analysisRequests.push(newAnalysisRequest);
}

void IgorAnalysis::Do()
{
    if (!m_pDatabase)
        return;

    for (;;)
	{
        m_status = RUNNING;
        s_analysisRequest* pRequest = m_pDatabase->m_analysisRequests.pop();

		u64 currentPC = pRequest->m_pc;

        // if (currentPC == (u64) -1) m_status = STOPPING; // blah; something like that

		c_cpu_module* pCpu = m_pDatabase->getCpuForAddress(currentPC);
        c_cpu_state* pCpuState = m_pDatabase->getCpuStateForAddress(currentPC);

		if(pCpu)
		{
			s_analyzeState analyzeState;
			analyzeState.m_PC = currentPC;
			analyzeState.pCpu = pCpu;
			analyzeState.pCpuState = pCpuState;
            analyzeState.pDataBase = m_pDatabase;
            analyzeState.pAnalysis = this;
			analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

			analyzeState.m_analyzeResult = e_analyzeResult::continue_analysis;

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
				
			} while (analyzeState.m_analyzeResult == e_analyzeResult::continue_analysis);

			delete analyzeState.m_cpu_analyse_result;
			analyzeState.m_cpu_analyse_result = NULL;
		}

        m_status = IDLE;
		yieldNoWait();
	}
}
