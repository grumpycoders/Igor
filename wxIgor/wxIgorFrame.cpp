#include "stdafx.h"
#include "wxIgorFrame.h"
#include "wxAsmWidget.h"
#include "wxIgorApp.h"
#include "wxManageUsers.h"

#include "IgorAnalysis.h"
#include "IgorAsyncActions.h"
#include "IgorLocalSession.h"
#include "IgorUtils.h"
#include "Loaders/PE/PELoader.h"
#include "Loaders/Dmp/dmpLoader.h"
#include "Loaders/Elf/elfLoader.h"

#include "IgorUsers.h"

#ifndef _WIN32
#include "appicon.xpm"
static const char ** const appicon_xpm = appicon;
#endif

using namespace Balau;

#define new DEBUG_NEW

c_wxIgorSessionPanel::c_wxIgorSessionPanel(IgorSession* pSession, wxWindow *parent) : wxPanel(parent)
{
    m_session = pSession;
    pSession->addRef();

    wxNotebook* pNoteBook = new wxNotebook(this, -1);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);

    hbox->Add(pNoteBook, 1, wxEXPAND);
    vbox->Add(hbox, 1, wxEXPAND);
    SetSizerAndFit(hbox, true);

    {
        wxPanel* pAsmPagePanel = new wxPanel(pNoteBook);
        wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);

        m_pAsmWidget = new c_wxAsmWidget(m_session, pAsmPagePanel, -1, "ASM view");
        c_wxAsmWidgetScrollbar* pAsmWidgetScrollbar = new c_wxAsmWidgetScrollbar(m_pAsmWidget, pAsmPagePanel, -1);

        m_pAsmWidget->SetSize(GetSize());

        hbox->Add(m_pAsmWidget, 1, wxEXPAND);
        hbox->Add(pAsmWidgetScrollbar, 0, wxEXPAND);

        vbox->Add(hbox, 1, wxEXPAND);

        pAsmPagePanel->SetSizerAndFit(hbox, true);

        pNoteBook->AddPage(pAsmPagePanel, "ASM");
    }
}

c_wxIgorSessionPanel::~c_wxIgorSessionPanel()
{
    delete m_pAsmWidget;
    if (m_session)
        m_session->release();
    m_pAsmWidget = NULL;
    m_session = NULL;
}

BEGIN_EVENT_TABLE(c_wxIgorSessionPanel, wxPanel)
END_EVENT_TABLE()

c_wxIgorFrame::c_wxIgorFrame(const wxString& title, const wxPoint& pos, const wxSize& size) : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
    m_session = NULL;

    c_wxIgorApp* pApp = (c_wxIgorApp*)wxApp::GetInstance();

    wxMenu *menuFile = new wxMenu;
    menuFile->Append(new wxMenuItem(menuFile, wxID_OPEN, wxT("&Open\tCtrl+O")));
    menuFile->Append(new wxMenuItem(menuFile, wxID_CLOSE, wxT("&Close")));
    menuFile->Append(ID_EXPORT_DISASSEMBLY, "Export disassembly");
    menuFile->AppendSeparator();
    menuFile->Append(ID_MANAGE_USERS, "Manage users");
    menuFile->AppendSeparator();
    menuFile->Append(ID_SAVE_DATABASE, "Save database");
    menuFile->Append(ID_LOAD_DATABASE, "Load database");
    menuFile->AppendSeparator();
    menuFile->Append(new wxMenuItem(menuFile, wxID_EXIT, wxT("&Quit\tCtrl+W")));

    pApp->m_fileHistory->UseMenu(menuFile);
    pApp->m_fileHistory->AddFilesToMenu();

    wxMenu *menuNavigation = new wxMenu;
    menuNavigation->Append(ID_GO_TO_ADDRESS, "Go to &address");
    menuNavigation->Append(ID_GO_TO_SYMBOL, "Go to &symbol");

    m_sessionsMenu = new wxMenu;

    wxMenu *menuMisc = new wxMenu;
    menuMisc->Append(ID_RUN_SELF_TESTS, "Run self &tests");

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuNavigation, "&Navigation");
    menuBar->Append(m_sessionsMenu, "&Sessions");
    menuBar->Append(menuMisc, "&Misc");

    SetMenuBar(menuBar);

    SetIcon(wxICON(appicon)); // loads resource under win32, or appicon.xpm elsewhere

    CreateStatusBar();
}

c_wxIgorFrame::~c_wxIgorFrame()
{

}

void c_wxIgorFrame::OpenFile(const wxString& fileName)
{
    igor_result result;
    String errorMsg1, errorMsg2;

    // add to history
    {
        c_wxIgorApp* pApp = (c_wxIgorApp*)wxApp::GetInstance();
        pApp->m_fileHistory->AddFileToHistory(fileName);
        pApp->m_fileHistory->Save(*pApp->m_config);
    }

    IgorSession * session = NULL;

    std::tie(result, session, errorMsg1, errorMsg2) = IgorAsyncLoadBinary(fileName.c_str());

    if (result != IGOR_SUCCESS) {
        wxString errorMsg = "File error:\n" + wxString(errorMsg1.to_charp()) + "\n" + wxString(errorMsg2.to_charp());
        wxMessageDialog * dial = new wxMessageDialog(NULL, errorMsg, "Error", wxOK | wxICON_ERROR);
        dial->ShowModal();
    } else {
        if (m_session)
            m_session->release();
        delete m_sessionPanel;
        m_session = session;
        m_sessionPanel = new c_wxIgorSessionPanel(m_session, this);
        SendSizeEvent();
    }
}

void c_wxIgorFrame::OnMenuOpen(wxMenuEvent& event)
{
    if (event.GetMenu() != m_sessionsMenu)
        return;

    const wxMenuItemList list = m_sessionsMenu->GetMenuItems();

    for (auto & item : list)
        m_sessionsMenu->Destroy(item);

    IgorSession::enumerate([&](IgorSession * session)
    {
        wxWindowID id = NewControlId();
        String uuid = session->getUUID();
        wxMenuItem * menuItem = new wxMenuItem(m_sessionsMenu, id, session->getSessionName().to_charp());
        m_sessionsMenu->Append(menuItem);
        Bind(wxEVT_COMMAND_MENU_SELECTED, [this, uuid](wxCommandEvent& WXUNUSED(event))
        {
            delete m_sessionPanel;
            if (m_session)
                m_session->release();
            m_sessionPanel = NULL;

            m_session = IgorSession::find(uuid);

            if (m_session)
            {
                m_sessionPanel = new c_wxIgorSessionPanel(m_session, this);
                SendSizeEvent();
            }
        }, id);
    });
}

const char* supportedFormats =
"PE executables(*.exe)|*.exe"
"|Mini dump(*.dmp)|*.dmp"
"|Elf executalbes(*.elf)|*.elf"
"|Anything|*.*"
;

void c_wxIgorFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fileDialog(this, "Choose a file", "", "", supportedFormats, wxFD_OPEN | wxFD_MULTIPLE);
    if (fileDialog.ShowModal() == wxID_OK)
    {
        wxArrayString stringArray;

        fileDialog.GetPaths(stringArray);

        for (unsigned int i = 0; i < stringArray.size(); i++)
        {
            OpenFile(stringArray[i]);
        }
    }
}

void c_wxIgorFrame::closeFile()
{
    delete m_sessionPanel;
    if (m_session) {
        m_session->stop();
        m_session->deactivate();
        m_session->release();
    }
    m_sessionPanel = NULL;
    m_session = NULL;
}

void c_wxIgorFrame::OnCloseFile(wxCommandEvent& WXUNUSED(event))
{
    closeFile();
}

void c_wxIgorFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
    closeFile();
    Close(true);
}

void c_wxIgorFrame::OnHistory(wxCommandEvent& event)
{
    c_wxIgorApp* pApp = (c_wxIgorApp*)wxApp::GetInstance();

    int fileId = event.GetId() - wxID_FILE1;

    OpenFile(pApp->m_fileHistory->GetHistoryFile(fileId));
}

void c_wxIgorFrame::OnGoToAddress(wxCommandEvent& event)
{
    if (!m_sessionPanel || !m_session)
        return;

    wxTextEntryDialog* pAddressEntryDialog = new wxTextEntryDialog(this, "Go to address");

    if (pAddressEntryDialog->ShowModal() == wxID_OK)
    {
        wxString wxEnteredString = pAddressEntryDialog->GetValue();

        String enteredString(wxEnteredString.c_str());
        u64 address = enteredString.to_uint64(16);

        if (address)
        {
            // TODO: needs to figure out a section here
            GoToAddress(igorAddress(m_session, address, -1));
        }
    }

    delete pAddressEntryDialog;
}

void c_wxIgorFrame::GoToAddress(igorAddress& address)
{
    if (m_sessionPanel && m_sessionPanel->m_pAsmWidget)
    {
        m_sessionPanel->m_pAsmWidget->m_currentPosition = address;
        m_sessionPanel->m_pAsmWidget->Refresh();
    }
}

void c_wxIgorFrame::OnGoToSymbol(wxCommandEvent& event)
{
    wxFrame* pSymbolListFrame = new c_wxIgorSymbolListFrame(this);

}

void c_wxIgorFrame::OnExportDisassembly(wxCommandEvent& event)
{
    wxFileDialog fileDialog(this, "Export disassembly", "", "", "Text file (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fileDialog.ShowModal() == wxID_OK)
    {
        wxString exportPath = fileDialog.GetPath();

        igor_export_to_file(exportPath, m_session);
    }
}

void c_wxIgorFrame::OnSaveDatabase(wxCommandEvent& event)
{
    wxFileDialog fileDialog(this, "Save database", "", "", "Igor database (*.igorDB)|*.igorDB", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fileDialog.ShowModal() == wxID_OK && m_session)
    {
        wxString fileName = fileDialog.GetPath();
        igor_result result;
        String errorMsg1, errorMsg2;
        std::tie(result, errorMsg1, errorMsg2) = m_session->serialize(fileName.c_str().AsChar());
        if (result == IGOR_FAILURE)
        {
            String errorMsg = errorMsg1 + " - " + errorMsg2;
            wxMessageBox(errorMsg.to_charp(), "Error", wxICON_ERROR | wxOK);
        }
    }
}

void c_wxIgorFrame::OnLoadDatabase(wxCommandEvent& event)
{
    wxFileDialog fileDialog(this, "Choose a file", "", "", "Igor database (*.igorDB)|*.igorDB", wxFD_OPEN);
    if (fileDialog.ShowModal() == wxID_OK)
    {
        wxString fileName = fileDialog.GetPath();
        igor_result result;
        IgorLocalSession * session;
        String errorMsg1, errorMsg2;
        std::tie(result, session, errorMsg1, errorMsg2) = IgorLocalSession::deserialize(fileName.c_str().AsChar());

        if (result == IGOR_FAILURE)
        {
            String errorMsg = errorMsg1 + " - " + errorMsg2;
            wxMessageBox(errorMsg.to_charp(), "Error", wxICON_ERROR | wxOK);
        }
    }
}

void c_wxIgorFrame::OnClose(wxCloseEvent& event)
{
    if (event.CanVeto())
    {
        if (wxMessageBox("Do you really want to exit ?", "Exit", wxICON_QUESTION | wxYES_NO) != wxYES)
        {
            event.Veto();
            return;
        }
    }

    closeFile();

    event.Skip();
}

void c_wxIgorFrame::OnIdle(wxIdleEvent& event)
{
    if (m_session)
    {
        SetStatusText(m_session->getStatusString());
    }
}

void c_wxIgorFrame::OnManageUsers(wxCommandEvent& event)
{
    wxManageUsersDialog* pManageUserDialog = new wxManageUsersDialog(this);

    pManageUserDialog->ShowModal();
}

void c_wxIgorFrame::OnRunSelfTests(wxCommandEvent& event)
{
    bool success = false;
    
    try {
        success = SRP::selfTest();
    }
    catch (...) {
        success = false;
    }

    if (success) {
        wxMessageBox("SRP self tests successful", "Success", wxICON_INFORMATION | wxOK);
    } else {
        wxMessageBox("SRP self tests failed", "Failure", wxICON_ERROR | wxOK);
    }
}

BEGIN_EVENT_TABLE(c_wxIgorFrame, wxFrame)
EVT_MENU_OPEN(c_wxIgorFrame::OnMenuOpen)

EVT_MENU(wxID_OPEN, c_wxIgorFrame::OnOpen)
EVT_MENU(wxID_CLOSE, c_wxIgorFrame::OnCloseFile)
EVT_MENU(wxID_EXIT, c_wxIgorFrame::OnExit)
EVT_MENU(ID_GO_TO_ADDRESS, c_wxIgorFrame::OnGoToAddress)
EVT_MENU(ID_GO_TO_SYMBOL, c_wxIgorFrame::OnGoToSymbol)
EVT_MENU(ID_EXPORT_DISASSEMBLY, c_wxIgorFrame::OnExportDisassembly)
EVT_MENU(ID_SAVE_DATABASE, c_wxIgorFrame::OnSaveDatabase)
EVT_MENU(ID_LOAD_DATABASE, c_wxIgorFrame::OnLoadDatabase)

EVT_MENU(ID_MANAGE_USERS, c_wxIgorFrame::OnManageUsers)
EVT_MENU(ID_RUN_SELF_TESTS, c_wxIgorFrame::OnRunSelfTests)

EVT_IDLE(c_wxIgorFrame::OnIdle)
EVT_CLOSE(c_wxIgorFrame::OnClose)

// history
EVT_MENU(wxID_FILE1, c_wxIgorFrame::OnHistory)
EVT_MENU(wxID_FILE2, c_wxIgorFrame::OnHistory)
EVT_MENU(wxID_FILE3, c_wxIgorFrame::OnHistory)
EVT_MENU(wxID_FILE4, c_wxIgorFrame::OnHistory)
EVT_MENU(wxID_FILE5, c_wxIgorFrame::OnHistory)
EVT_MENU(wxID_FILE6, c_wxIgorFrame::OnHistory)
EVT_MENU(wxID_FILE7, c_wxIgorFrame::OnHistory)
EVT_MENU(wxID_FILE8, c_wxIgorFrame::OnHistory)
EVT_MENU(wxID_FILE9, c_wxIgorFrame::OnHistory)
END_EVENT_TABLE()


c_wxIgorSymbolListFrame::c_wxIgorSymbolListFrame(c_wxIgorFrame* parent) : wxFrame(parent, 0, "Symbol list"),
m_parent(parent)
{
    m_symbolList = new wxListView(this, ID_SYMBOL_LIST);
    m_symbolList->AppendColumn("Address");
    m_symbolList->AppendColumn("Symbol type");
    m_symbolList->AppendColumn("Symbol name");

    s_igorDatabase::t_symbolMap::iterator start;
    s_igorDatabase::t_symbolMap::iterator end;
    parent->m_session->lock();
    parent->m_session->getSymbolsIterator(start, end);

    int itemId = 0;

    while (start != end)
    {
        Balau::String addressString;
        addressString.append("0x%08llX", start->first.offset);
        wxListItem newItem;
        newItem.SetId(itemId);
        newItem.SetText(addressString.to_charp());
        int itemIndex = m_symbolList->InsertItem(newItem);


        m_symbolList->SetItem(itemIndex, 2, start->second.m_name.to_charp());

        itemId++;
        start++;
    }

    parent->m_session->unlock();

    Layout();
    Show();
}

c_wxIgorSymbolListFrame::~c_wxIgorSymbolListFrame()
{

}

void c_wxIgorSymbolListFrame::OnSymbolActivated(wxListEvent& event)
{
    const wxListItem selectedItem = event.GetItem();

    String enteredString(selectedItem.m_text.c_str());
    u64 address = enteredString.to_uint64(16);

    if (address)
    {
        m_parent->GoToAddress(igorAddress(m_parent->m_session, address, -1));
    }
}

BEGIN_EVENT_TABLE(c_wxIgorSymbolListFrame, wxFrame)

EVT_LIST_ITEM_ACTIVATED(ID_SYMBOL_LIST, c_wxIgorSymbolListFrame::OnSymbolActivated)

END_EVENT_TABLE()
