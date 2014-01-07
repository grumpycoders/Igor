#include "stdafx.h"
#include "wxAsmWidget.h"

#include "IgorDatabase.h"
#include "IgorAnalysis.h"
#include "IgorSession.h"

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

    m_currentPosition = m_pSession->getEntryPoint();

	SetScrollbar(wxVERTICAL, 250, 16, 500, 15);
	//SetScrollbar(wxVERTICAL, 0, 0, 0); // hide the default scrollbar
	SetFont(wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));

	//m_scrollbar = new c_wxAsmWidgetScrollbar(this, this, id);

	SetBackgroundStyle(wxBG_STYLE_PAINT);

    m_currentFont = wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT);
    m_currentFont.MakeBold();

    m_fontSize = m_currentFont.GetPixelSize();
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

igorAddress c_wxAsmWidget::GetAddressOfCursor()
{
    return m_addressOfCursor;
}

void c_wxAsmWidget::OnDraw(wxDC& dc)
{
	wxColour BGColor = GetBackgroundColour();
	wxBrush MyBrush(BGColor, wxSOLID);
	dc.SetBackground(MyBrush);
	dc.Clear();

    dc.SetFont(m_currentFont);
	wxSize size = GetSize();
	int width = size.GetWidth();

    int numLinesInWindow = (GetSize().GetY() / m_fontSize.GetHeight()) + 1;
	int numDrawnLines = 0;
    
	int drawY = 0;

	igorAddress currentPC = m_currentPosition;

    int cursorLine = m_mousePosition.y / m_fontSize.GetHeight();
    {
        dc.DrawRectangle(0, cursorLine * m_fontSize.GetHeight(), size.GetWidth(), m_fontSize.GetHeight() + 1);
    }

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
            Balau::String symbolName;
            if (m_pSession->getSymbolName(analyzeState.m_PC, symbolName))
            {
                dc.SetTextForeground(*wxBLUE);
                dc.DrawText(symbolName.to_charp(), 0, drawY);

                numDrawnLines++;
                drawY += m_fontSize.GetHeight();
            }

			String fullDisassembledString;
			fullDisassembledString.set("%016llX: ", analyzeState.m_PC.offset);

            if ((drawY <= m_mousePosition.y) && (drawY + m_fontSize.GetHeight() > m_mousePosition.y))
            {
                m_addressOfCursor = analyzeState.m_PC;
            }

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
				wxString displayDisassembledString = wxString::Format("%016llX: 0x%02X\n", analyzeState.m_PC.offset, m_pSession->readU8(analyzeState.m_PC));

				//SetDefaultStyle(wxTextAttr(*wxRED));
				//AppendText(displayDisassembledString);
				dc.SetTextForeground(*wxRED);
				dc.DrawText(displayDisassembledString, 0, drawY);
				numDrawnLines++;

				analyzeState.m_PC++;
			}

            drawY += m_fontSize.GetHeight();
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

void c_wxAsmWidget::OnTimer(wxTimerEvent &event)
{
	Refresh();
}

void c_wxAsmWidget::OnMouseMotion(wxMouseEvent& event)
{
    m_mousePosition = event.GetPosition();

    Refresh();
}

void c_wxAsmWidget::OnKeyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode())
    {
    case 'C':
        m_pSession->add_code_analysis_task(GetAddressOfCursor());
        break;
    }
}

void c_wxAsmWidget::seekPC(int amount)
{
	if (amount > 0)
	{
        m_currentPosition = m_pSession->get_next_valid_address_after(m_currentPosition + amount);
	}
	if (amount < 0)
	{
        m_currentPosition = m_pSession->get_next_valid_address_before(m_currentPosition + amount);
	}

	//case amount == 0 is intentionally left doing nothing

    Refresh();
}

BEGIN_EVENT_TABLE(c_wxAsmWidget, wxScrolledWindow)
EVT_TIMER(c_wxAsmWidget::EVT_RefreshDatabase, c_wxAsmWidget::OnTimer)
EVT_PAINT(c_wxAsmWidget::OnPaint)
EVT_SIZE(c_wxAsmWidget::OnSize)
EVT_MOTION(c_wxAsmWidget::OnMouseMotion)
EVT_KEY_DOWN(c_wxAsmWidget::OnKeyDown)
END_EVENT_TABLE()
