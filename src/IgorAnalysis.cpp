#include "IgorAnalysis.h"

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

			analyzeState.m_analyzeResult = e_analyzeResult::continue_analysis;

			do
			{
				if (igor_is_address_flagged_as_code(analyzeState.m_PC))
				{
					break;
				}

				if (pCpu->analyze(&analyzeState) != IGOR_SUCCESS)
				{
					analyzeState.m_analyzeResult = e_analyzeResult::stop_analysis;
				}

				igor_flag_address_as_instruction(analyzeState.m_cpu_analyse_result->m_startOfInstruction, analyzeState.m_cpu_analyse_result->m_instructionSize);
			} while (analyzeState.m_analyzeResult == e_analyzeResult::continue_analysis);
		}

        m_status = IDLE;
		yieldNoWait();
	}
}

igor_result IgorAnalysis::igor_flag_address_as_u32(u64 virtualAddress)
{
	// TODO!

	return IGOR_SUCCESS;
}

igor_result IgorAnalysis::igor_flag_address_as_instruction(u64 virtualAddress, u8 instructionSize)
{
	s_igorSection* pSection = m_pDatabase->findSectionFromAddress(virtualAddress);

	if (pSection == NULL)
	{
		return IGOR_FAILURE;
	}

	if (virtualAddress + instructionSize > pSection->m_virtualAddress + pSection->m_size)
	{
		return IGOR_FAILURE;
	}

	if (pSection->m_instructionSize == nullptr)
	{
		pSection->createInstructionArray();
	}
	
	u8* pInstructionSize = &pSection->m_instructionSize[virtualAddress - pSection->m_virtualAddress];

	if (*pInstructionSize) // already an instruction there
	{
		return IGOR_FAILURE;
	}

	*pInstructionSize = instructionSize;
	memset(pInstructionSize + 1, 0xFF, instructionSize - 1);

	return IGOR_SUCCESS;
}

igor_result IgorAnalysis::igor_is_address_flagged_as_code(u64 virtualAddress)
{
	s_igorSection* pSection = m_pDatabase->findSectionFromAddress(virtualAddress);

	if (pSection == NULL)
	{
		return IGOR_FAILURE;
	}

	if (pSection->m_instructionSize == nullptr)
	{
		return IGOR_FAILURE;
	}

	u8* pInstructionSize = &pSection->m_instructionSize[virtualAddress - pSection->m_virtualAddress];

	if (*pInstructionSize)
	{
		return IGOR_SUCCESS;
	}
	else
	{
		return IGOR_FAILURE;
	}
}