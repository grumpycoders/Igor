#include "wxIgorFrame.h"
#include "wxAsmWidget.h"
#include "wxIgorApp.h"

#include <Input.h>
#include <Buffer.h>
#include <TaskMan.h>

#include "IgorAnalysis.h"
#include "PELoader.h"

//using namespace Balau;

c_wxIgorFrame::c_wxIgorFrame(const wxString& title, const wxPoint& pos, const wxSize& size) : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	c_wxIgorApp* pApp = (c_wxIgorApp*)wxApp::GetInstance();

	wxMenu *menuFile = new wxMenu;
	menuFile->Append(wxID_OPEN, "&Open");
	menuFile->Append(ID_EXPORT_DISASSEMBLY, "Export disassembly");

	pApp->m_fileHistory->UseMenu(menuFile);
	pApp->m_fileHistory->AddFilesToMenu();

	wxMenu *menuNavigation = new wxMenu;
	menuNavigation->Append(ID_GO_TO_ADDRESS, "&Go to address");

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuNavigation, "&Navigation");

	SetMenuBar(menuBar);

	CreateStatusBar();
}

void c_wxIgorFrame::OpenFile(wxString& fileName)
{
	// add to history
	{
		c_wxIgorApp* pApp = (c_wxIgorApp*)wxApp::GetInstance();
		pApp->m_fileHistory->AddFileToHistory(fileName);
		pApp->m_fileHistory->Save(*pApp->m_config);
	}

	Balau::IO<Balau::Input> file(new Balau::Input(fileName.c_str()));
	file->open();

	size_t size = file->getSize();
	uint8_t * buffer = (uint8_t *)malloc(size);
	file->forceRead(buffer, size);
	Balau::IO<Balau::Buffer> reader(new Balau::Buffer(buffer, file->getSize()));

	m_session = new IgorLocalSession();

	s_igorDatabase * db = new s_igorDatabase;

    // TODO: make a reader selector UI
	c_PELoader PELoader;
    // Note: having the session here is actually useful not just for the entry point,
    // but for all the possible hints the file might have for us.
	igor_result r = PELoader.loadPE(db, reader, m_session);
	reader->close();
	free(buffer);

    // Add the task even in case of failure, so it can properly clean itself out.
	Balau::TaskMan::registerTask(m_session);

    if (r != IGOR_SUCCESS)
    {
        delete db;
        return;
    }

	wxPanel *panel = new wxPanel(this, -1);

	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);

	m_pAsmWidget = new c_wxAsmWidget(m_session, panel, -1, "ASM view");
	c_wxAsmWidgetScrollbar* pAsmWidgetScrollbar = new c_wxAsmWidgetScrollbar(m_pAsmWidget, panel, -1);

	m_pAsmWidget->SetSize(GetSize());

	hbox->Add(m_pAsmWidget, 1, wxALIGN_RIGHT | wxEXPAND);
	hbox->Add(pAsmWidgetScrollbar, 0, wxALIGN_RIGHT | wxEXPAND);

	vbox->Add(hbox, 1, wxALIGN_RIGHT | wxEXPAND);

	panel->SetSizerAndFit(hbox, true);
	SendSizeEvent();
}

void c_wxIgorFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog fileDialog(this, "Choose a file", "", "", "Executables (*.exe)|*.exe", wxFD_OPEN | wxFD_MULTIPLE);
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

		Balau::String enteredString(wxEnteredString.c_str());
		u64 address = enteredString.to_int(16);

		if (m_pAsmWidget && address)
		{
			m_pAsmWidget->m_currentPosition = address;
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

		FILE* fHandle = fopen(exportPath, "w+");
		if (fHandle)
		{
			igorAddress entryPointAddress = m_session->getEntryPoint();

			c_cpu_module* pCpu = m_session->getCpuForAddress(entryPointAddress);
			igor_section_handle sectionHandle = m_session->getSectionFromAddress(entryPointAddress);

			igorAddress sectionStart = m_session->getSectionStartVirtualAddress(sectionHandle);
			u64 sectionSize = m_session->getSectionSize(sectionHandle);

			s_analyzeState analyzeState;
			analyzeState.m_PC = sectionStart;
			analyzeState.pCpu = pCpu;
			analyzeState.pCpuState = m_session->getCpuStateForAddress(sectionStart);
			analyzeState.pSession = m_session;
			analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

			while (analyzeState.m_PC < sectionStart + sectionSize)
			{
				if (m_session->is_address_flagged_as_code(analyzeState.m_PC) && (pCpu->analyze(&analyzeState) == IGOR_SUCCESS))
				{
					Balau::String disassembledString;
					pCpu->printInstruction(&analyzeState, disassembledString);

					fprintf(fHandle, "%s\n", disassembledString.to_charp(0));

					analyzeState.m_PC = analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize;
				}
				else
				{
					u8 byte;
					m_session->readU8(analyzeState.m_PC, byte);

					fprintf(fHandle, "0x%01X\n", byte);

					analyzeState.m_PC++;
				}
			}

			delete analyzeState.m_cpu_analyse_result;
			fclose(fHandle);
		}
	}
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
EVT_MENU(ID_GO_TO_ADDRESS, c_wxIgorFrame::OnGoToAddress)
EVT_MENU(ID_EXPORT_DISASSEMBLY, c_wxIgorFrame::OnExportDisassembly)
EVT_IDLE(c_wxIgorFrame::OnIdle)

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