#pragma once

#include <wx/generic/grid.h>

struct s_igorDatabase;

class wxAsmWidget : public wxGrid
{
public:
	wxAsmWidget(s_igorDatabase* pDatabase, wxWindow *parent, wxWindowID id,
		const wxString& value = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxWANTS_CHARS,
		const wxString& name = "");
private:

	void OnMouseEvent(wxMouseEvent& event);
	void OnScroll(wxScrollWinEvent &event);
	DECLARE_EVENT_TABLE()
};