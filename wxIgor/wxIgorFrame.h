#include <wx/wx.h>

class c_wxAsmWidget;
class IgorLocalSession;

class c_wxIgorFrame : public wxFrame
{
public:

	enum
	{
		ID_GO_TO_ADDRESS = wxID_HIGHEST,
		ID_EXPORT_DISASSEMBLY,
	};

	c_wxIgorFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

	void OpenFile(wxString& fileName);

	void OnOpen(wxCommandEvent& event);
	void OnHistory(wxCommandEvent& event);
	void OnIdle(wxIdleEvent& event);

	void OnGoToAddress(wxCommandEvent& event);
	void OnExportDisassembly(wxCommandEvent& event);

	c_wxAsmWidget* m_pAsmWidget;

	IgorLocalSession * m_session;

	DECLARE_EVENT_TABLE()
};