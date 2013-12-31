#include <wx/wx.h>
#include <wx/notebook.h>

class c_wxAsmWidget;
class IgorLocalSession;
class IgorSession;

class c_wxIgorSessionPanel : public wxPanel
{
public:
	c_wxIgorSessionPanel(IgorSession* pSession, wxWindow *parent);

	IgorSession * m_session;
	c_wxAsmWidget* m_pAsmWidget;

	DECLARE_EVENT_TABLE()
};

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

	c_wxIgorSessionPanel* m_sessionPanel;
	IgorLocalSession * m_session;

	DECLARE_EVENT_TABLE()
};