#include "stdafx.h"
#include "wxAsmWidget.h"

#include "IgorDatabase.h"
#include "IgorAnalysis.h"
#include "IgorSession.h"

#include <wx/scrolbar.h>
#include <wx/dcbuffer.h>
#include <wx/caret.h>

#define new DEBUG_NEW

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

        m_AsmWidget->SetFocus();
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
    m_pSession->addRef();

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

//    SetScrollbars(20, 20, 50, 50);
    //SetScrollbar(wxVERTICAL, 0, 0, 0); // hide the default scrollbar
    SetFont(wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));

    //m_scrollbar = new c_wxAsmWidgetScrollbar(this, this, id);
}

c_wxAsmWidget::~c_wxAsmWidget()
{
    m_pSession->release();
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
    int caretLine = m_caret->GetPosition().y / m_fontSize.GetHeight();
    return m_textCache[caretLine].m_address;
}

void c_wxAsmWidget::updateTextCache()
{
    if (m_textCacheIsDirty == false)
        return;

    m_textCacheIsDirty = false;

    m_textCache.clear();

    igorAddress currentPC = m_currentPosition;
    int numLinesToDraw = (GetSize().GetY() / m_fontSize.GetHeight());

    {
        c_cpu_module* pCpu = m_pSession->getCpuForAddress(currentPC);

        s_analyzeState analyzeState;
        analyzeState.m_PC = currentPC;
        analyzeState.pCpu = pCpu;
        analyzeState.pCpuState = m_pSession->getCpuStateForAddress(currentPC);
        analyzeState.pSession = m_pSession;
        analyzeState.m_cpu_analyse_result = NULL;
        if (pCpu)
        {
            analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();
        }

        while (m_textCache.size() < numLinesToDraw + 1)
        {
            s_textCacheEntry currentEntry;

            currentEntry.m_address = analyzeState.m_PC;

            currentEntry.m_text.append("%016llX: ", analyzeState.m_PC.offset);

            Balau::String symbolName;
            if (m_pSession->getSymbolName(analyzeState.m_PC, symbolName))
            {
                s_textCacheEntry symbolNameEntry;

                symbolNameEntry.m_address = analyzeState.m_PC;
                symbolNameEntry.m_text.append(currentEntry.m_text.to_charp());
                symbolNameEntry.m_text.append("%s%s%s:", c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL), symbolName.to_charp(), c_cpu_module::finishColor(c_cpu_module::KNOWN_SYMBOL));

                m_textCache.push_back(symbolNameEntry);
            }

            // display cross references
            {
                std::vector<igorAddress> crossReferences;
                m_pSession->getReferences(analyzeState.m_PC, crossReferences);

                for (int i = 0; i < crossReferences.size(); i++)
                {
                    s_textCacheEntry crossRefEntry;
                    crossRefEntry.m_address = analyzeState.m_PC;
                    crossRefEntry.m_text.append(currentEntry.m_text.to_charp());
                    crossRefEntry.m_text.append("                            xref: "); // alignment of xref

                    Balau::String symbolName;
                    if (m_pSession->getSymbolName(crossReferences[i], symbolName))
                    {
                        crossRefEntry.m_text.append("%s%s 0x%08llX%s", c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL), symbolName.to_charp(), crossReferences[i].offset, c_cpu_module::finishColor(c_cpu_module::KNOWN_SYMBOL));
                    }
                    else
                    {
                        crossRefEntry.m_text.append("%s0x%08llX%s", c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL), crossReferences[i].offset, c_cpu_module::finishColor(c_cpu_module::KNOWN_SYMBOL));
                    }

                    m_textCache.push_back(crossRefEntry);
                }
            }

            currentEntry.m_text.append("         "); // alignment of code/data

            if (m_pSession->is_address_flagged_as_code(analyzeState.m_PC) && (pCpu->analyze(&analyzeState) == IGOR_SUCCESS))
            {
                /*
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
                */

                pCpu->printInstruction(&analyzeState, currentEntry.m_text, true);

                m_textCache.push_back(currentEntry);

                analyzeState.m_PC = analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize;
            }
            else
            {
                currentEntry.m_text.append("db       0x%02X", m_pSession->readU8(analyzeState.m_PC));

                m_textCache.push_back(currentEntry);

                analyzeState.m_PC++;
            }
        }

        delete analyzeState.m_cpu_analyse_result;
    }
}

void c_wxAsmWidget::OnDraw(wxDC& dc)
{
    m_textCacheIsDirty = true;
    updateTextCache();

    wxColour BGColor = GetBackgroundColour();
    wxBrush MyBrush(BGColor, wxBRUSHSTYLE_SOLID);
    dc.SetBackground(MyBrush);
    dc.Clear();

    dc.SetFont(m_currentFont);
    wxSize size = GetSize();
    int width = size.GetWidth();

    int numDrawnLines = 0;
    
    int drawY = 0;

    igorAddress currentPC = m_currentPosition;

    int currentLine = 0;

    while(currentLine < m_textCache.size())
    {
        // draw the text while honoring color changes
        int currentX = 0;
        Balau::String& currentText = m_textCache[currentLine].m_text;
        Balau::String::List stringList= currentText.split(';');

        dc.SetTextForeground(*wxBLACK);

        for (int i = 0; i < stringList.size(); i++)
        {
            if (strstr(stringList[i].to_charp(), "C="))
            {
                if (strstr(c_cpu_module::startColor(c_cpu_module::RESET_COLOR), stringList[i].to_charp()))
                {
                    dc.SetTextForeground(*wxBLACK);
                }
                else if (strstr(c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL), stringList[i].to_charp()))
                {
                    dc.SetTextForeground(*wxBLUE);
                }
                else if (strstr(c_cpu_module::startColor(c_cpu_module::MNEMONIC_DEFAULT), stringList[i].to_charp()))
                {
                    dc.SetTextForeground(wxColour(0xFF775577));
                }
                else if (strstr(c_cpu_module::startColor(c_cpu_module::MNEMONIC_FLOW_CONTROL), stringList[i].to_charp()))
                {
                    dc.SetTextForeground(*wxRED);
                }
                else if (strstr(c_cpu_module::startColor(c_cpu_module::OPERAND_REGISTER), stringList[i].to_charp()))
                {
                    dc.SetTextForeground(*wxGREEN);
                }
                else if (strstr(c_cpu_module::startColor(c_cpu_module::OPERAND_IMMEDIATE), stringList[i].to_charp()))
                {
                    dc.SetTextForeground(*wxCYAN);
                }
                else
                {
                    Failure("Unimplemented color");
                }

            }
            else
            {
                dc.DrawText(stringList[i].to_charp(), currentX, drawY);

                currentX += stringList[i].strlen() * m_fontSize.GetX();
            }
        }

        drawY += m_fontSize.GetHeight();

        currentLine++;
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
    SetFocus();

    int cursorCollumn = m_mousePosition.x / m_fontSize.GetWidth();
    int cursorLine = m_mousePosition.y / m_fontSize.GetHeight();
    
    m_caret->Move(cursorCollumn * m_fontSize.GetWidth(), cursorLine * m_fontSize.GetHeight());
    updateSelectedText();

    Refresh();
}

void c_wxAsmWidget::goToAddress(igorAddress address)
{
	m_history.push(m_currentPosition);
    m_currentPosition = address;

    Refresh();
}

void c_wxAsmWidget::popAddress()
{
	if (!m_history.empty())
	{
		m_currentPosition = m_history.top();
		m_history.pop();
		Refresh();
	}
}

void c_wxAsmWidget::goToSelectedSymbol()
{
    igorAddress address = m_pSession->findSymbol(m_selectedText.to_charp());

    if (address.isValid())
    {
        goToAddress(address);
    }
    else
    {
        u64 offset;
        if (m_selectedText.scanf("0x%016llX", &offset))
        {
            // TODO: need to find a section here
            goToAddress(igorAddress(m_pSession, offset, -1));
        }
    }
}

void c_wxAsmWidget::OnMouseLeftDClick(wxMouseEvent& event)
{
    goToSelectedSymbol();
}

void c_wxAsmWidget::moveCaret(int x, int y)
{
    m_caret->Move(m_caret->GetPosition() + wxPoint(x * m_fontSize.GetWidth(), y * m_fontSize.GetHeight()));

    if (m_caret->GetPosition().y < 0)
    {
        seekPC(-1);
        m_caret->Move(m_caret->GetPosition().x, 0);
    }

    if (m_caret->GetPosition().y > m_numLinesInWindow * m_fontSize.GetHeight())
    {
        seekPC(1);
        m_caret->Move(m_caret->GetPosition().x, (m_numLinesInWindow) * m_fontSize.GetHeight());
    }

    updateSelectedText();

    Refresh();
}

bool IsValidCharForSymbol(char character)
{
    if (character >= 'a' && character <= 'z')
        return true;

    if (character >= 'A' && character <= 'Z')
        return true;

    if (character >= '0' && character <= '9')
        return true;

    if (character == '_')
        return true;

    if (character == '?')
        return true;

    if (character == '@')
        return true;

    if (character == ':')
        return true;

    return false;
}

void c_wxAsmWidget::updateSelectedText()
{
    int caretCollumn = m_caret->GetPosition().x / m_fontSize.GetWidth();
    int caretLine = m_caret->GetPosition().y / m_fontSize.GetHeight();

    Balau::String& currentText = m_textCache[caretLine].m_text;
    Balau::String::List stringList = currentText.split(';');

    int currentX = 0;
    for (int i = 0; i < stringList.size(); i++)
    {
        // skip any color change
        if (strstr(stringList[i].to_charp(), "C=") == NULL)
        {
            int finishX = stringList[i].strlen();

            if (finishX > caretCollumn)
            {
                int startOfString = caretCollumn;
                while (startOfString && (startOfString >= 1 && IsValidCharForSymbol(stringList[i][startOfString-1])))
                {
                    startOfString--;
                }
                int endOfString = caretCollumn;
                while (IsValidCharForSymbol(stringList[i][endOfString]) && (endOfString + 1 < stringList[i].strlen()))
                {
                    endOfString++;
                }
                m_selectedText = stringList[i].to_charp(startOfString);
                return;
            }

            caretCollumn -= finishX;
            currentX = finishX;
        }
    }
}

void c_wxAsmWidget::OnKeyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode())
    {
    case 'C':
        m_pSession->add_code_analysis_task(GetAddressOfCursor());
        break;
	case WXK_BACK:
		popAddress();
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

    event.Skip();
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
EVT_LEFT_DCLICK(c_wxAsmWidget::OnMouseLeftDClick)

EVT_KEY_DOWN(c_wxAsmWidget::OnKeyDown)

END_EVENT_TABLE()
