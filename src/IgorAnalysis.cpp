#include "IgorAnalysis.h"
#include "IgorDatabase.h"

void igor_add_code_analysis_task(u64 PC)
{
	s_igorDatabase* pDatabase = getCurrentIgorDatabase();

	s_analysisRequest newAnalysisRequest;
	newAnalysisRequest.m_pc = PC;

	pDatabase->m_analysisRequests.push_back(newAnalysisRequest);
}

void igor_analysis_run()
{
	s_igorDatabase* pDatabase = getCurrentIgorDatabase();

	for(int i=0; i<pDatabase->m_analysisRequests.size(); i++)
	{
		s_analysisRequest* pRequest = &pDatabase->m_analysisRequests[i];

		u64 currentPC = pRequest->m_pc;

		c_cpu_module* pCpu = pDatabase->getCpuForAddress(currentPC);
		c_cpu_state* pCpuState = pDatabase->getCpuStateForAddress(currentPC);

		if(pCpu)
		{
			s_analyzeState analyzeState;
			analyzeState.m_PC = currentPC;
			analyzeState.pCpuState = pCpuState;
			analyzeState.pDataBase = pDatabase;

			analyzeState.m_mnemonic = -1;
			analyzeState.m_analyzeResult = e_analyzeResult::continue_analysis;

			do
			{
				if (pCpu->analyze(&analyzeState) != IGOR_SUCCESS)
				{
					analyzeState.m_analyzeResult = e_analyzeResult::stop_analysis;
				}
			} while (analyzeState.m_analyzeResult == e_analyzeResult::continue_analysis);
		}
	}
}