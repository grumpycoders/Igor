#include "stdafx.h"
#include "wxIgorFrame.h"
#include "wxAsmWidget.h"
#include "wxIgorApp.h"

#include <Input.h>
#include <Output.h>
#include <Buffer.h>
#include <TaskMan.h>

#include "IgorAnalysis.h"
#include "IgorLocalSession.h"
#include "IgorUtils.h"
#include "Loaders/PE/PELoader.h"
#include "Loaders/Elf/elfLoader.h"

#ifndef _WIN32
#include "appicon.xpm"
static const char ** const appicon_xpm = appicon;
#endif

using namespace Balau;

#define new DEBUG_NEW

static void fileOperationSafe(std::function<void()> lambda) {
    Task::SimpleContext simpleContext;

    try {
        lambda();
    }
    catch (GeneralException & e) {
        wxString errorMsg = "File error:\n" + wxString(e.getMsg());
        wxMessageDialog * dial = new wxMessageDialog(NULL, errorMsg, "Error", wxOK | wxICON_ERROR);
        dial->ShowModal();
    }
    catch (...) {
        wxMessageDialog * dial = new wxMessageDialog(NULL, "Unknown file error.", "Error", wxOK | wxICON_ERROR);
        dial->ShowModal();
    }
}

c_wxIgorSessionPanel::c_wxIgorSessionPanel(IgorSession* pSession, wxWindow *parent) : wxPanel(parent)
{
	m_session = pSession;

	wxNotebook* pNoteBook = new wxNotebook(this, -1);
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);

	hbox->Add(pNoteBook, 1, wxALIGN_RIGHT | wxEXPAND);
	vbox->Add(hbox, 1, wxALIGN_RIGHT | wxEXPAND);
	SetSizerAndFit(hbox, true);

	{
		wxPanel* pAsmPagePanel = new wxPanel(pNoteBook);
		wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
		wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);

		m_pAsmWidget = new c_wxAsmWidget(m_session, pAsmPagePanel, -1, "ASM view");
		c_wxAsmWidgetScrollbar* pAsmWidgetScrollbar = new c_wxAsmWidgetScrollbar(m_pAsmWidget, pAsmPagePanel, -1);

		m_pAsmWidget->SetSize(GetSize());

		hbox->Add(m_pAsmWidget, 1, wxALIGN_RIGHT | wxEXPAND);
		hbox->Add(pAsmWidgetScrollbar, 0, wxALIGN_RIGHT | wxEXPAND);

		vbox->Add(hbox, 1, wxALIGN_RIGHT | wxEXPAND);

		pAsmPagePanel->SetSizerAndFit(hbox, true);

		pNoteBook->AddPage(pAsmPagePanel, "ASM");
	}
}

BEGIN_EVENT_TABLE(c_wxIgorSessionPanel, wxPanel)
END_EVENT_TABLE()

c_wxIgorFrame::c_wxIgorFrame(const wxString& title, const wxPoint& pos, const wxSize& size) : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	m_session = NULL;

	c_wxIgorApp* pApp = (c_wxIgorApp*)wxApp::GetInstance();

	wxMenu *menuFile = new wxMenu;
    menuFile->Append(new wxMenuItem(menuFile, wxID_OPEN, wxT("&Open\tCtrl+O")));
	menuFile->Append(ID_EXPORT_DISASSEMBLY, "Export disassembly");
    menuFile->AppendSeparator();
    menuFile->Append(ID_SAVE_DATABASE, "Save database");
    menuFile->Append(ID_LOAD_DATABASE, "Load database");
    menuFile->AppendSeparator();
    menuFile->Append(new wxMenuItem(menuFile, wxID_EXIT, wxT("&Quit\tCtrl+W")));

	pApp->m_fileHistory->UseMenu(menuFile);
	pApp->m_fileHistory->AddFilesToMenu();

	wxMenu *menuNavigation = new wxMenu;
	menuNavigation->Append(ID_GO_TO_ADDRESS, "&Go to address");

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuNavigation, "&Navigation");

	SetMenuBar(menuBar);

    SetIcon(wxICON(appicon)); // loads resource under win32, or appicon.xpm elsewhere

	CreateStatusBar();
}

c_wxIgorFrame::~c_wxIgorFrame()
{

}

void c_wxIgorFrame::OpenFile(const wxString& fileName)
{
    igor_result r = IGOR_FAILURE;

    // add to history
    {
        c_wxIgorApp* pApp = (c_wxIgorApp*)wxApp::GetInstance();
        pApp->m_fileHistory->AddFileToHistory(fileName);
        pApp->m_fileHistory->Save(*pApp->m_config);
    }

    m_session = new IgorLocalSession();

    fileOperationSafe([&]() {
        IO<Input> reader(new Input(fileName.c_str()));
        reader->open();

		if (fileName.find(".exe") != -1)
		{
			c_PELoader PELoader;
			// Note: having the session here is actually useful not just for the entry point,
			// but for all the possible hints the file might have for us.
			r = PELoader.loadPE(reader, m_session);
			// Note: destroying the object from the stack would do the same, but
			// as this might trigger a context switch, it's better to do it explicitely
			// than from a destructor, as a general good practice.
		}
		else if (fileName.find(".elf") != -1)
		{
			c_elfLoader elfLoader;
			r = elfLoader.load(reader, m_session);
		}
        reader->close();

        m_session->loaded(fileName.c_str());
    });

    // Add the task even in case of failure, so it can properly clean itself out.
    TaskMan::registerTask(m_session);

    m_sessionPanel = new c_wxIgorSessionPanel(m_session, this);

    SendSizeEvent();
}

const char* supportedFormats =
"PE executables(*.exe)|*.exe"
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

void c_wxIgorFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
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
	wxTextEntryDialog* pAddressEntryDialog = new wxTextEntryDialog(this, "Go to address");

	if (pAddressEntryDialog->ShowModal() == wxID_OK)
	{
		wxString wxEnteredString = pAddressEntryDialog->GetValue();

		String enteredString(wxEnteredString.c_str());
		u64 address = enteredString.to_int(16);

		if (m_sessionPanel->m_pAsmWidget && address)
		{
			m_sessionPanel->m_pAsmWidget->m_currentPosition = igorAddress(address);
            m_sessionPanel->m_pAsmWidget->Refresh();
		}
	}

	delete pAddressEntryDialog;
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
        auto result = m_session->serialize(fileName.c_str().AsChar());
        if (result.first)
        {
            wxMessageBox(result.second.to_charp(), "Error", wxICON_ERROR | wxOK);
        }
    }
}

void c_wxIgorFrame::OnLoadDatabase(wxCommandEvent& event)
{
    wxFileDialog fileDialog(this, "Choose a file", "", "", "Igor database (*.igorDB)|*.igorDB", wxFD_OPEN);
    if (fileDialog.ShowModal() == wxID_OK)
    {
        wxString fileName = fileDialog.GetPath();
        m_session = IgorLocalSession::deserialize(fileName.c_str().AsChar());
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

	event.Skip();
}

void c_wxIgorFrame::OnIdle(wxIdleEvent& event)
{
	if (m_session)
	{
		SetStatusText(m_session->getStatusString());
	}
}

BEGIN_EVENT_TABLE(c_wxIgorFrame, wxFrame)
EVT_MENU(wxID_OPEN, c_wxIgorFrame::OnOpen)
EVT_MENU(wxID_EXIT, c_wxIgorFrame::OnExit)
EVT_MENU(ID_GO_TO_ADDRESS, c_wxIgorFrame::OnGoToAddress)
EVT_MENU(ID_EXPORT_DISASSEMBLY, c_wxIgorFrame::OnExportDisassembly)
EVT_MENU(ID_SAVE_DATABASE, c_wxIgorFrame::OnSaveDatabase)
EVT_MENU(ID_LOAD_DATABASE, c_wxIgorFrame::OnLoadDatabase)
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
