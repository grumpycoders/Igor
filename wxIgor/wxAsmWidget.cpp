#include "wxAsmWidget.h"

#include "IgorDatabase.h"
#include "IgorAnalysis.h"

#include <wx/scrolBar.h>

class wxAsmWidgetGridCellProvider : public wxGridCellAttrProvider
{
public:
	wxGridCellAttr *GetAttr(int row, int col,wxGridCellAttr::wxAttrKind  kind) const
	{
		return NULL;
	}
};

c_wxAsmWidget::c_wxAsmWidget(s_igorDatabase* pDatabase, wxWindow *parent, wxWindowID id,
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

	//GetTable()->SetAttrProvider(new wxAsmWidgetGridCellProvider());

	m_pDatabase = pDatabase;

	m_timer = new wxTimer(this, EVT_RefreshDatabase);
	m_timer->Start(1000);
}

void c_wxAsmWidget::OnTimer(wxTimerEvent &event)
{
	Freeze();

	wxPoint viewStart = GetViewStart();

	int Rows = GetNumberRows();
	if (Rows)
	{
		DeleteRows(0, Rows, true);
	}

	u64 entryPointPC = m_pDatabase->findSymbol("entryPoint");

	if (entryPointPC != -1)
	{
		s_igorSection* pSection = m_pDatabase->findSectionFromAddress(entryPointPC);
		c_cpu_module* pCpu = m_pDatabase->getCpuForAddress(entryPointPC);

		s_analyzeState analyzeState;
		analyzeState.m_PC = pSection->m_virtualAddress;
		analyzeState.pCpu = pCpu;
		analyzeState.pCpuState = m_pDatabase->getCpuStateForAddress(entryPointPC);
		analyzeState.pDataBase = m_pDatabase;
		analyzeState.pAnalysis = NULL;
		analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

		while (analyzeState.m_PC < pSection->m_virtualAddress + pSection->m_size)
		{
			if (igor_is_address_flagged_as_code(m_pDatabase, analyzeState.m_PC))
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

	Scroll(viewStart);

	Thaw();
}

void c_wxAsmWidget::OnMouseEvent(wxMouseEvent& event)
{

}

void c_wxAsmWidget::OnScroll(wxScrollWinEvent &event)
{
	event.StopPropagation();
}

BEGIN_EVENT_TABLE(c_wxAsmWidget, wxGrid)
EVT_MOUSE_EVENTS(c_wxAsmWidget::OnMouseEvent)
EVT_SCROLLWIN(c_wxAsmWidget::OnScroll)
EVT_TIMER(c_wxAsmWidget::EVT_RefreshDatabase, c_wxAsmWidget::OnTimer)
END_EVENT_TABLE()