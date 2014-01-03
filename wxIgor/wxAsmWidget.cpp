#include "stdafx.h"
#include "wxAsmWidget.h"

#include "IgorDatabase.h"
#include "IgorAnalysis.h"

#include <wx/scrolBar.h>
#include <wx/dcbuffer.h>

using namespace Balau;

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

	m_AsmWidget->Refresh();
}

BEGIN_EVENT_TABLE(c_wxAsmWidgetScrollbar, wxScrollBar)
EVT_SCROLL(c_wxAsmWidgetScrollbar::OnScroll)
END_EVENT_TABLE()

c_wxAsmWidget::c_wxAsmWidget(IgorSession* pSession, wxWindow *parent, wxWindowID id,
	const wxString& value,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& name) : wxScrolledWindow(parent, id, wxDefaultPosition, wxDefaultSize)
{
    m_pSession = pSession;

	m_timer = new wxTimer(this, EVT_RefreshDatabase);
	m_timer->Start(300);

    m_currentPosition = m_pSession->findSymbol("entryPoint");

	SetScrollbar(wxVERTICAL, 250, 16, 500, 15);
	//SetScrollbar(wxVERTICAL, 0, 0, 0); // hide the default scrollbar
	SetFont(wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));

	//m_scrollbar = new c_wxAsmWidgetScrollbar(this, this, id);

	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

c_wxAsmWidget::~c_wxAsmWidget()
{
	delete m_timer;
}

void c_wxAsmWidget::OnSize(wxSizeEvent& event)
{
	Refresh();
	//skip the event.
	event.Skip();
}

void c_wxAsmWidget::OnDraw(wxDC& dc)
{
	/*wxFont font(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
	wxFONTWEIGHT_NORMAL, false, wxT("Courier 10 Pitch")); */

	wxFont currentFont = wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT);

	wxColour BGColor = GetBackgroundColour();
	wxBrush MyBrush(BGColor, wxSOLID);
	dc.SetBackground(MyBrush);
	dc.Clear();

	dc.SetFont(currentFont);
	wxSize size = GetSize();
	int width = size.GetWidth();

	//dc.DrawText(wxString::Format(wxT("Test")), 0, 0);

	int numLinesInWindow = (GetSize().GetY() / currentFont.GetPixelSize().GetHeight()) - 1;
	int numDrawnLines = 0;

	int drawY = 0;

	u64 currentPC = m_currentPosition;

	{
		c_cpu_module* pCpu = m_pSession->getCpuForAddress(currentPC);

		s_analyzeState analyzeState;
		analyzeState.m_PC = currentPC;
		analyzeState.pCpu = pCpu;
		analyzeState.pCpuState = m_pSession->getCpuStateForAddress(currentPC);
		analyzeState.pSession = m_pSession;
		analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

		while (numDrawnLines < numLinesInWindow)
		{
			String fullDisassembledString;
			fullDisassembledString.set("%016llX: ", analyzeState.m_PC);

			if (m_pSession->is_address_flagged_as_code(analyzeState.m_PC) && (pCpu->analyze(&analyzeState) == IGOR_SUCCESS))
			{
				String disassembledString;
				pCpu->printInstruction(&analyzeState, disassembledString);

				fullDisassembledString += disassembledString;

				wxString displayDisassembledString;
				displayDisassembledString = fullDisassembledString.to_charp();

				//SetDefaultStyle(wxTextAttr(*wxBLUE));
				dc.SetTextForeground(*wxBLUE);
				dc.DrawText(displayDisassembledString, 0, drawY);
				//AppendText(displayDisassembledString);
				//AppendText("\n");
				numDrawnLines++;

				analyzeState.m_PC = analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize;
			}
			else
			{
				wxString displayDisassembledString = wxString::Format("%016llX: 0x%02X\n", analyzeState.m_PC, m_pSession->readU8(analyzeState.m_PC));

				//SetDefaultStyle(wxTextAttr(*wxRED));
				//AppendText(displayDisassembledString);
				dc.SetTextForeground(*wxRED);
				dc.DrawText(displayDisassembledString, 0, drawY);
				numDrawnLines++;

				analyzeState.m_PC++;
			}

			drawY += currentFont.GetPixelSize().GetHeight();
		}

		delete analyzeState.m_cpu_analyse_result;
	}
}

void c_wxAsmWidget::OnPaint(wxPaintEvent& event)
{
	wxScrolledWindow::OnPaint(event);

	wxAutoBufferedPaintDC dc(this);
	OnDraw(dc);
}

void c_wxAsmWidget::updateDatabaseView()
{
/*	Freeze();

	Clear();

	int numLinesInWindow = (GetClientSize().GetY() / GetCharHeight()) - 1;
	int numDrawnLines = 0;

	u64 currentPC = m_currentPosition;

	{
        c_cpu_module* pCpu = m_pSession->getCpuForAddress(currentPC);

		s_analyzeState analyzeState;
		analyzeState.m_PC = currentPC;
		analyzeState.pCpu = pCpu;
        analyzeState.pCpuState = m_pSession->getCpuStateForAddress(currentPC);
        analyzeState.pSession = m_pSession;
		analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

		while (numDrawnLines < numLinesInWindow)
		{
            String fullDisassembledString;
            fullDisassembledString.set("%016llX: ", analyzeState.m_PC);

            if (m_pSession->is_address_flagged_as_code(analyzeState.m_PC) && (pCpu->analyze(&analyzeState) == IGOR_SUCCESS))
			{
				String disassembledString;
				pCpu->printInstruction(&analyzeState, disassembledString);

                fullDisassembledString += disassembledString;

				wxString displayDisassembledString;
                displayDisassembledString = fullDisassembledString.to_charp();

				SetDefaultStyle(wxTextAttr(*wxBLUE));
				AppendText(displayDisassembledString);
				AppendText("\n");
				numDrawnLines++;

				analyzeState.m_PC = analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize;
			}
			else
			{
                wxString displayDisassembledString = wxString::Format("%016llX: 0x%02X\n", analyzeState.m_PC, m_pSession->readU8(analyzeState.m_PC));

				SetDefaultStyle(wxTextAttr(*wxRED));
				AppendText(displayDisassembledString);
				numDrawnLines++;

				analyzeState.m_PC++;
			}
		}

		delete analyzeState.m_cpu_analyse_result;
	}

	Thaw();*/
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
        m_currentPosition = m_pSession->get_next_valid_address_after(m_currentPosition + amount);
		updateDatabaseView();
	}
	if (amount < 0)
	{
        m_currentPosition = m_pSession->get_next_valid_address_before(m_currentPosition + amount);
		updateDatabaseView();
	}

	//case amount == 0 is intentionally left doing nothing
}

BEGIN_EVENT_TABLE(c_wxAsmWidget, wxScrolledWindow)
//EVT_TIMER(c_wxAsmWidget::EVT_RefreshDatabase, c_wxAsmWidget::OnTimer)
EVT_PAINT(c_wxAsmWidget::OnPaint)
EVT_SIZE(c_wxAsmWidget::OnSize)
END_EVENT_TABLE()
