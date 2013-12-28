#include <wx/wx.h>

class c_wxIgorFrame : public wxFrame
{
public:
	c_wxIgorFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

	void OnOpen(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};