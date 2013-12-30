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

c_wxAsmWidget::c_wxAsmWidget(IgorSession* pAnalysis, wxWindow *parent, wxWindowID id,
	const wxString& value,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& name) : wxTextCtrl(parent, id, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY)
{
    m_pAnalysis = pAnalysis;

	m_timer = new wxTimer(this, EVT_RefreshDatabase);
	m_timer->Start(300);

    m_currentPosition = m_pAnalysis->findSymbol("entryPoint");

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
        c_cpu_module* pCpu = m_pAnalysis->getCpuForAddress(currentPC);

		s_analyzeState analyzeState;
		analyzeState.m_PC = currentPC;
		analyzeState.pCpu = pCpu;
        analyzeState.pCpuState = m_pAnalysis->getCpuStateForAddress(currentPC);
        analyzeState.pAnalysis = m_pAnalysis;
		analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

		while (numDrawnLines < numLinesInWindow)
		{
			if (m_pAnalysis->is_address_flagged_as_code(analyzeState.m_PC) && (pCpu->analyze(&analyzeState) == IGOR_SUCCESS))
			{
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
                wxString displayDisassembledString = wxString::Format("0x%01X\n", m_pAnalysis->readU8(analyzeState.m_PC));

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
        m_currentPosition = m_pAnalysis->get_next_valid_address_after(m_currentPosition + amount);
		updateDatabaseView();
	}
	if (amount < 0)
	{
        m_currentPosition = m_pAnalysis->get_next_valid_address_before(m_currentPosition + amount);
		updateDatabaseView();
	}

	//case amount == 0 is intentionally left doing nothing
}

BEGIN_EVENT_TABLE(c_wxAsmWidget, wxTextCtrl)
EVT_TIMER(c_wxAsmWidget::EVT_RefreshDatabase, c_wxAsmWidget::OnTimer)
END_EVENT_TABLE()
