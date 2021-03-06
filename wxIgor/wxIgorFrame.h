#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>

class wxListView;
class c_wxAsmWidget;
class IgorLocalSession;
class IgorSession;
struct igorAddress;

class c_wxIgorSessionPanel : public wxPanel
{
public:
    c_wxIgorSessionPanel(IgorSession* pSession, wxWindow *parent);
    ~c_wxIgorSessionPanel();

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
        ID_GO_TO_SYMBOL,
        ID_EXPORT_DISASSEMBLY,
        ID_SAVE_DATABASE,
        ID_LOAD_DATABASE,
        ID_MANAGE_USERS,
        ID_RUN_SELF_TESTS,
    };

    c_wxIgorFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
    virtual ~c_wxIgorFrame();

    void GoToAddress(igorAddress& address);

    void closeFile();

    void OpenFile(const wxString& fileName);

    void OnMenuOpen(wxMenuEvent& event);

    void OnOpen(wxCommandEvent& event);
    void OnCloseFile(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnHistory(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnIdle(wxIdleEvent& event);

    void OnGoToAddress(wxCommandEvent& event);
    void OnGoToSymbol(wxCommandEvent& event);
    void OnExportDisassembly(wxCommandEvent& event);

    void OnSaveDatabase(wxCommandEvent& event);
    void OnLoadDatabase(wxCommandEvent& event);

    void OnManageUsers(wxCommandEvent& event);
    void OnRunSelfTests(wxCommandEvent& event);

    c_wxIgorSessionPanel* m_sessionPanel = NULL;
    IgorSession * m_session = NULL;

    wxMenu * m_sessionsMenu = NULL;

    DECLARE_EVENT_TABLE()
};

class c_wxIgorSymbolListFrame : public wxFrame
{
public:
    enum
    {
        ID_SYMBOL_LIST = wxID_HIGHEST,
    };

    c_wxIgorSymbolListFrame(c_wxIgorFrame* parent);
    virtual ~c_wxIgorSymbolListFrame();

    void OnSymbolActivated(wxListEvent& event);

    c_wxIgorFrame* m_parent;
    wxListView* m_symbolList;

    DECLARE_EVENT_TABLE()
};
