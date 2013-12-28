#pragma once

#include <wx/generic/grid.h>
#include <wx/timer.h>

struct s_igorDatabase;

class c_wxAsmWidget : public wxGrid
{
public:
	enum
	{
		EVT_RefreshDatabase = wxID_HIGHEST,
	};

	c_wxAsmWidget(s_igorDatabase* pDatabase, wxWindow *parent, wxWindowID id,
		const wxString& value = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxWANTS_CHARS,
		const wxString& name = "");
private:

	void OnMouseEvent(wxMouseEvent& event);
	void OnScroll(wxScrollWinEvent &event);
	//void OnIdle(wxIdleEvent &event);
	void OnTimer(wxTimerEvent &event);

	s_igorDatabase* m_pDatabase;

	wxTimer* m_timer;

	DECLARE_EVENT_TABLE()
};