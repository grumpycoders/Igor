#include "IgorAnalysis.h"

void IgorAnalysis::igor_add_code_analysis_task(u64 PC)
{
	s_igorDatabase* pDatabase = getCurrentIgorDatabase();

    s_analysisRequest * newAnalysisRequest = new s_analysisRequest(PC);

	pDatabase->m_analysisRequests.push(newAnalysisRequest);
}

void IgorAnalysis::Do()
{
	s_igorDatabase* pDatabase = getCurrentIgorDatabase();
    s_analysisRequest * pRequest;

    for (;;)
	{
        m_status = RUNNING;
        s_analysisRequest* pRequest = pDatabase->m_analysisRequests.pop();

		u64 currentPC = pRequest->m_pc;

        // if (currentPC == (u64) -1) m_status = STOPPING; // blah; something like that

		c_cpu_module* pCpu = pDatabase->getCpuForAddress(currentPC);
		c_cpu_state* pCpuState = pDatabase->getCpuStateForAddress(currentPC);

		if(pCpu)
		{
			s_analyzeState analyzeState;
			analyzeState.m_PC = currentPC;
			analyzeState.pCpu = pCpu;
			analyzeState.pCpuState = pCpuState;
			analyzeState.pDataBase = pDatabase;

			analyzeState.m_analyzeResult = e_analyzeResult::continue_analysis;

			do
			{
				if (pCpu->analyze(&analyzeState) != IGOR_SUCCESS)
				{
					analyzeState.m_analyzeResult = e_analyzeResult::stop_analysis;
				}
			} while (analyzeState.m_analyzeResult == e_analyzeResult::continue_analysis);
		}

        m_status = IDLE;
		yieldNoWait();
	}
}

igor_result igor_flag_address_as_u32(u64 virtualAddress)
{
	// TODO!

	return IGOR_SUCCESS;
}