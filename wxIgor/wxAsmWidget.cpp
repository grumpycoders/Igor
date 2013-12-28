#include "wxAsmWidget.h"

#include "IgorDatabase.h"
#include "IgorAnalysis.h"

class wxAsmWidgetGridCellProvider : public wxGridCellAttrProvider
{
public:
	wxGridCellAttr *GetAttr(int row, int col,wxGridCellAttr::wxAttrKind  kind) const
	{
		return NULL;
	}
};

wxAsmWidget::wxAsmWidget(s_igorDatabase* pDatabase, wxWindow *parent, wxWindowID id,
	const wxString& value,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& name) : wxGrid(parent, id)
{
	CreateGrid(0, 3);

	HideRowLabels();
	HideColLabels();
	EnableGridLines(false);

	EnableEditing(false);

	GetTable()->SetAttrProvider(new wxAsmWidgetGridCellProvider());

	// dump the section that contains symbol "entryPoint"
	u64 entryPointPC = pDatabase->findSymbol("entryPoint");

	if (entryPointPC != -1)
	{
		s_igorSection* pSection = pDatabase->findSectionFromAddress(entryPointPC);
		c_cpu_module* pCpu = pDatabase->getCpuForAddress(entryPointPC);

		s_analyzeState analyzeState;
		analyzeState.m_PC = pSection->m_virtualAddress;
		analyzeState.pCpu = pCpu;
		analyzeState.pCpuState = pDatabase->getCpuStateForAddress(entryPointPC);
		analyzeState.pDataBase = pDatabase;
		analyzeState.pAnalysis = NULL;
		analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

		while (analyzeState.m_PC < pSection->m_virtualAddress + pSection->m_size)
		{
			if (igor_is_address_flagged_as_code(pDatabase, analyzeState.m_PC))
			{
				if (pCpu->analyze(&analyzeState) != IGOR_SUCCESS)
				{
					break;
				}

				Balau::String disassembledString;
				pCpu->printInstruction(&analyzeState, disassembledString);

				AppendRows();
				int newRow = GetNumberRows() - 1;

				wxString displayDisassembledString;
				displayDisassembledString = disassembledString.to_charp(0);

				GetTable()->SetValue(newRow, 0, displayDisassembledString);

				analyzeState.m_PC = analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize;
			}
			else
			{
				analyzeState.m_PC++;
			}
		}

		delete analyzeState.m_cpu_analyse_result;
	}
}

void wxAsmWidget::OnMouseEvent(wxMouseEvent& event)
{

}

void wxAsmWidget::OnScroll(wxScrollWinEvent &event)
{
	event.StopPropagation();
}

BEGIN_EVENT_TABLE(wxAsmWidget, wxGrid)
EVT_MOUSE_EVENTS(wxAsmWidget::OnMouseEvent)
EVT_SCROLLWIN(wxAsmWidget::OnScroll)
END_EVENT_TABLE()