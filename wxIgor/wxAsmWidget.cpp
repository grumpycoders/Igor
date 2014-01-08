#include "stdafx.h"
#include "wxAsmWidget.h"

#include "IgorDatabase.h"
#include "IgorAnalysis.h"
#include "IgorSession.h"

#include <wx/scrolBar.h>
#include <wx/dcbuffer.h>
#include <wx/caret.h>

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
    // First let's set everything in our object...
    m_pSession = pSession;

	m_timer = new wxTimer(this, EVT_RefreshDatabase);
    m_currentPosition = m_pSession->getEntryPoint();

	SetBackgroundStyle(wxBG_STYLE_PAINT);

    m_currentFont = wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT);
    m_currentFont.MakeBold();

    m_fontSize = m_currentFont.GetPixelSize();

    m_caret = new wxCaret(this, m_fontSize);

    // ...then let's start calling stuff, as it will go back into our object.
    m_timer->Start(300);

    SetCaret(m_caret);
    m_caret->Show();

    SetScrollbar(wxVERTICAL, 250, 16, 500, 15);
    //SetScrollbar(wxVERTICAL, 0, 0, 0); // hide the default scrollbar
    SetFont(wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));

    //m_scrollbar = new c_wxAsmWidgetScrollbar(this, this, id);
}

c_wxAsmWidget::~c_wxAsmWidget()
{
	delete m_timer;
}

void c_wxAsmWidget::OnSize(wxSizeEvent& event)
{
    m_numLinesInWindow = (GetSize().GetY() / m_fontSize.GetHeight());

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

	int numDrawnLines = 0;
    
	int drawY = 0;

	igorAddress currentPC = m_currentPosition;

	{
		c_cpu_module* pCpu = m_pSession->getCpuForAddress(currentPC);

		s_analyzeState analyzeState;
		analyzeState.m_PC = currentPC;
		analyzeState.pCpu = pCpu;
		analyzeState.pCpuState = m_pSession->getCpuStateForAddress(currentPC);
		analyzeState.pSession = m_pSession;
		analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

		while (numDrawnLines < m_numLinesInWindow + 1)
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
                
				/*String disassembledString;
				pCpu->printInstruction(&analyzeState, disassembledString);

				fullDisassembledString += disassembledString;

				wxString displayDisassembledString;
				displayDisassembledString = fullDisassembledString.to_charp();

				dc.SetTextForeground(*wxBLUE);
				dc.DrawText(displayDisassembledString, 0, drawY);
*/
                int currentX = 0;

                Balau::String mnemonic;
                pCpu->getMnemonic(&analyzeState, mnemonic);

                const int numMaxOperand = 4;

                Balau::String operandStrings[numMaxOperand];

                int numOperandsInInstruction = pCpu->getNumOperands(&analyzeState);
                EAssert(numMaxOperand >= numOperandsInInstruction);

                for (int i = 0; i < numOperandsInInstruction; i++)
                {
                    pCpu->getOperand(&analyzeState, i, operandStrings[i]);
                }

                dc.SetTextForeground(*wxBLUE);

                // draw address
                dc.DrawText(fullDisassembledString.to_charp(), currentX, drawY);
                currentX += m_fontSize.GetWidth() * fullDisassembledString.strlen();

                // draw mnemonic
                dc.DrawText(mnemonic.to_charp(), currentX, drawY);
                currentX += m_fontSize.GetWidth() * mnemonic.strlen();
                currentX += m_fontSize.GetWidth();

                // draw operandes
                for (int i = 0; i < numOperandsInInstruction; i++)
                {
                    int xBeforeOperand = currentX;
                    int xAfterOperand = currentX + m_fontSize.GetWidth() * operandStrings[i].strlen();

                    // are we on the caret line?
                    if (numDrawnLines == m_caret->GetPosition().y / m_fontSize.GetHeight())
                    {
                        // is the caret over the current operand?
                        if ((xBeforeOperand <= m_caret->GetPosition().x) && (xAfterOperand > m_caret->GetPosition().x))
                        {
                            wxBrush oldBrush = dc.GetBrush();
                            wxPen oldPen = dc.GetPen();

                            dc.SetBrush(*wxYELLOW_BRUSH);
                            dc.SetPen(*wxYELLOW_PEN);

                            dc.DrawRectangle(xBeforeOperand, drawY, xAfterOperand - xBeforeOperand, m_fontSize.GetHeight());

                            dc.SetBrush(oldBrush);
                            dc.SetPen(oldPen);
                        }
                    }
                    dc.DrawText(operandStrings[i].to_charp(), currentX, drawY);
                    currentX += m_fontSize.GetWidth() * operandStrings[i].strlen();
                    currentX += m_fontSize.GetWidth();
                }
                /*
                for (int i = 0; i < x86_analyse_result->m_numOperands; i++)
                {
                    Balau::String operandString;
                    getOperand(pState, i, operandString);
                    if (i == 0)
                    {
                        instructionString += " ";
                    }
                    else
                    {
                        instructionString += ", ";
                    }

                    instructionString += operandString;
                }
                */

				numDrawnLines++;

				analyzeState.m_PC = analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize;
			}
			else
			{
				wxString displayDisassembledString = wxString::Format("%016llX: 0x%02X\n", analyzeState.m_PC.offset, m_pSession->readU8(analyzeState.m_PC));

				dc.SetTextForeground(*wxRED);
				dc.DrawText(displayDisassembledString, 0, drawY);
				numDrawnLines++;

				analyzeState.m_PC++;
			}

            drawY += m_fontSize.GetHeight();
		}

		delete analyzeState.m_cpu_analyse_result;
	}

    int cursorLine = m_mousePosition.y / m_fontSize.GetHeight();
    {
        wxBrush oldBrush = dc.GetBrush();
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(0, cursorLine * m_fontSize.GetHeight(), size.GetWidth(), m_fontSize.GetHeight() + 1);
        dc.SetBrush(oldBrush);
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

void c_wxAsmWidget::OnMouseLeftDown(wxMouseEvent& event)
{
    int cursorCollumn = m_mousePosition.x / m_fontSize.GetWidth();
    int cursorLine = m_mousePosition.y / m_fontSize.GetHeight();
    
    m_caret->Move(cursorCollumn * m_fontSize.GetWidth(), cursorLine * m_fontSize.GetHeight());

    Refresh();
}

void c_wxAsmWidget::moveCaret(int x, int y)
{
    m_caret->Move(m_caret->GetPosition() + wxPoint(x * m_fontSize.GetWidth(), y * m_fontSize.GetHeight()));

    if (m_caret->GetPosition().y < 0)
    {
        seekPC(-1);
        m_caret->Move(m_caret->GetPosition().x, 0);
        Refresh();
    }

    if (m_caret->GetPosition().y > m_numLinesInWindow * m_fontSize.GetHeight())
    {
        seekPC(1);
        m_caret->Move(m_caret->GetPosition().x, (m_numLinesInWindow) * m_fontSize.GetHeight());
        Refresh();
    }
}

void c_wxAsmWidget::OnKeyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode())
    {
    case 'C':
        m_pSession->add_code_analysis_task(GetAddressOfCursor());
        break;
    case WXK_LEFT:
        moveCaret(-1, 0);
        break;
    case WXK_RIGHT:
        moveCaret(1, 0);
        break;
    case WXK_UP:
        moveCaret(0, -1);
        break;
    case WXK_DOWN:
        moveCaret(0, 1);
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

// Mouse stuff
EVT_MOTION(c_wxAsmWidget::OnMouseMotion)
EVT_LEFT_DOWN(c_wxAsmWidget::OnMouseLeftDown)

EVT_KEY_DOWN(c_wxAsmWidget::OnKeyDown)

END_EVENT_TABLE()
