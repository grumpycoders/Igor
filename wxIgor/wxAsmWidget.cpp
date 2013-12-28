#include "wxAsmWidget.h"

#include "IgorDatabase.h"
#include "IgorAnalysis.h"

#include <wx/scrolBar.h>

c_wxAsmWidgetScrollbar::c_wxAsmWidgetScrollbar(c_wxAsmWidget* pAsmWidget, wxWindow *parent, wxWindowID id) : wxScrollBar(parent, id, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL)
{
	m_AsmWidget = pAsmWidget;

	SetScrollbar(250, 16, 500, 15);
	m_previousThumPosition = 250;
}

void c_wxAsmWidgetScrollbar::OnScroll(wxScrollEvent& event)
{
	//int position = event.GetPosition();

	if (event.GetEventType() == wxEVT_SCROLL_LINEUP)
	{
		m_AsmWidget->seekPC(-1);
	}

	if (event.GetEventType() == wxEVT_SCROLL_LINEDOWN)
	{
		m_AsmWidget->seekPC(+1);
	}

	if (event.GetEventType() == wxEVT_SCROLL_THUMBTRACK)
	{
		int diff = event.GetPosition() - m_previousThumPosition;

		m_AsmWidget->seekPC(diff);
		m_previousThumPosition = event.GetPosition();
	}

	if (event.GetEventType() == wxEVT_SCROLL_THUMBRELEASE)
	{
		SetScrollbar(250, 16, 500, 15);
		m_previousThumPosition = 250;
	}
}

BEGIN_EVENT_TABLE(c_wxAsmWidgetScrollbar, wxScrollBar)
EVT_SCROLL(c_wxAsmWidgetScrollbar::OnScroll)
END_EVENT_TABLE()

c_wxAsmWidget::c_wxAsmWidget(s_igorDatabase* pDatabase, wxWindow *parent, wxWindowID id,
	const wxString& value,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& name) : wxTextCtrl(parent, id, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY)
{
	m_pDatabase = pDatabase;

	m_timer = new wxTimer(this, EVT_RefreshDatabase);
	m_timer->Start(300);

	m_currentPosition = m_pDatabase->findSymbol("entryPoint");

	SetScrollbar(wxVERTICAL, 0, 0, 0); // hide the default scrollbar
	SetFont(wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));

	//m_scrollbar = new c_wxAsmWidgetScrollbar(this, this, id);
}

void c_wxAsmWidget::updateDatabaseView()
{
	Freeze();

	Clear();

	int numLinesInWindow = (GetClientSize().GetY() / GetCharHeight()) - 1;
	int numDrawnLines = 0;

	u64 currentPC = m_currentPosition;

	{
		s_igorSection* pSection = m_pDatabase->findSectionFromAddress(currentPC);
		c_cpu_module* pCpu = m_pDatabase->getCpuForAddress(currentPC);

		s_analyzeState analyzeState;
		analyzeState.m_PC = currentPC;
		analyzeState.pCpu = pCpu;
		analyzeState.pCpuState = m_pDatabase->getCpuStateForAddress(currentPC);
		analyzeState.pDataBase = m_pDatabase;
		analyzeState.pAnalysis = NULL;
		analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

		while (numDrawnLines < numLinesInWindow)
		{
            if (m_pDatabase->is_address_flagged_as_code(analyzeState.m_PC))
			{
				if (pCpu->analyze(&analyzeState) != IGOR_SUCCESS)
				{
					break;
				}

				Balau::String disassembledString;
				pCpu->printInstruction(&analyzeState, disassembledString);

				wxString displayDisassembledString;
				displayDisassembledString = disassembledString.to_charp(0);

				AppendText(displayDisassembledString);
				AppendText("\n");
				numDrawnLines++;

				analyzeState.m_PC = analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize;
			}
			else
			{
				wxString displayDisassembledString = wxString::Format("0x%01X\n", m_pDatabase->readU8(analyzeState.m_PC));

				AppendText(displayDisassembledString);
				numDrawnLines++;

				analyzeState.m_PC++;
			}
		}

		delete analyzeState.m_cpu_analyse_result;
	}

	Thaw();
}

void c_wxAsmWidget::OnTimer(wxTimerEvent &event)
{
	updateDatabaseView();
	Refresh();
}

void c_wxAsmWidget::seekPC(int amount)
{
	if (amount > 0)
	{
        m_currentPosition = m_pDatabase->get_next_valid_address_after(m_currentPosition + amount);
		updateDatabaseView();
	}
	if (amount < 0)
	{
        m_currentPosition = m_pDatabase->get_next_valid_address_before(m_currentPosition + amount);
		updateDatabaseView();
	}

	//case amount == 0 is intentially left doint nothing
}

BEGIN_EVENT_TABLE(c_wxAsmWidget, wxTextCtrl)
EVT_TIMER(c_wxAsmWidget::EVT_RefreshDatabase, c_wxAsmWidget::OnTimer)
END_EVENT_TABLE()

class wxAsmWidgetGridCellProvider : public wxGridCellAttrProvider
{
public:
	wxGridCellAttr *GetAttr(int row, int col,wxGridCellAttr::wxAttrKind  kind) const
	{
		return NULL;
	}
};

c_wxAsmWidget_old::c_wxAsmWidget_old(s_igorDatabase* pDatabase, wxWindow *parent, wxWindowID id,
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

void c_wxAsmWidget_old::OnTimer(wxTimerEvent &event)
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
            if (m_pDatabase->is_address_flagged_as_code(analyzeState.m_PC))
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

void c_wxAsmWidget_old::OnMouseEvent(wxMouseEvent& event)
{

}

void c_wxAsmWidget_old::OnScroll(wxScrollWinEvent &event)
{
	event.StopPropagation();
}

BEGIN_EVENT_TABLE(c_wxAsmWidget_old, wxGrid)
EVT_MOUSE_EVENTS(c_wxAsmWidget_old::OnMouseEvent)
EVT_SCROLLWIN(c_wxAsmWidget_old::OnScroll)
EVT_TIMER(c_wxAsmWidget_old::EVT_RefreshDatabase, c_wxAsmWidget_old::OnTimer)
END_EVENT_TABLE()