#pragma once

#include <wx/grid.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/scrolbar.h>

#include "Igor.h"

struct s_igorDatabase;

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

class c_wxAsmWidget : public wxTextCtrl
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

	void seekPC(int amount);
private:

	void updateDatabaseView();

	void OnMouseEvent(wxMouseEvent& event);
	void OnScroll(wxScrollWinEvent &event);
	//void OnIdle(wxIdleEvent &event);
	void OnTimer(wxTimerEvent &event);

	s_igorDatabase* m_pDatabase;

	wxTimer* m_timer;

	u64 m_currentPosition;

	DECLARE_EVENT_TABLE()
};

class c_wxAsmWidget_old : public wxGrid
{
public:
	enum
	{
		EVT_RefreshDatabase = wxID_HIGHEST,
	};

	c_wxAsmWidget_old(s_igorDatabase* pDatabase, wxWindow *parent, wxWindowID id,
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