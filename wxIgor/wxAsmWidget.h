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

	void updateDatabaseView();

	void OnMouseEvent(wxMouseEvent& event);
	void OnScroll(wxScrollWinEvent &event);
	//void OnIdle(wxIdleEvent &event);
	void OnTimer(wxTimerEvent &event);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);

	void OnDraw(wxDC& dc);

    IgorSession* m_pSession;

	wxTimer* m_timer;

	igorAddress m_currentPosition;

	DECLARE_EVENT_TABLE()
};
