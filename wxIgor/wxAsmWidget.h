#pragma once

#include <wx/grid.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/scrolbar.h>

#include "Igor.h"

class IgorSession;

class c_wxAsmWidget;

class c_wxAsmWidgetScrollbar : public wxScrollBar
{
public:
	c_wxAsmWidgetScrollbar(c_wxAsmWidget* pAsmWidget, wxWindow *parent, wxWindowID id);


private:
	c_wxAsmWidget* m_AsmWidget;

	void OnScroll(wxScrollEvent& event);

	int m_previousThumPosition;

	DECLARE_EVENT_TABLE()
};

class c_wxAsmWidget : public wxScrolledWindow
{
public:
	enum
	{
		EVT_RefreshDatabase = wxID_HIGHEST,
	};

    c_wxAsmWidget(IgorSession* pAnalysis, wxWindow *parent, wxWindowID id,
		const wxString& value = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxWANTS_CHARS,
		const wxString& name = "");
	~c_wxAsmWidget();

	void seekPC(int amount);
//private:

	void OnMouseEvent(wxMouseEvent& event);
	void OnScroll(wxScrollWinEvent &event);
	//void OnIdle(wxIdleEvent &event);
	void OnTimer(wxTimerEvent &event);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);

    void OnMouseMotion(wxMouseEvent& event);
    void OnMouseLeftDown(wxMouseEvent& event);
    
    void OnKeyDown(wxKeyEvent& event);
	void OnDraw(wxDC& dc);

    void updateTextCache();

    struct s_textCacheEntry
    {
        igorAddress m_address;
        Balau::String m_text;
    };
    std::vector<s_textCacheEntry> m_textCache;
    bool m_textCacheIsDirty;

    void moveCaret(int x, int y);

    igorAddress GetAddressOfCursor();

    int m_numLinesInWindow;

    wxCaret* m_caret;

    IgorSession* m_pSession;

	wxTimer* m_timer;

	igorAddress m_currentPosition;

    wxPoint m_mousePosition;

    wxFont m_currentFont;
    wxSize m_fontSize;

    igorAddress m_addressOfCursor;

	DECLARE_EVENT_TABLE()
};
