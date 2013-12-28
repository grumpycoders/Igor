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
	
	pApp->m_fileHistory->UseMenu(menuFile);
	pApp->m_fileHistory->AddFilesToMenu();

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");

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

	IgorAnalysis * analysis = new IgorAnalysis();

	s_igorDatabase * db = new s_igorDatabase;

	c_PELoader PELoader;
	PELoader.loadPE(db, reader, analysis);

	reader->close();
	free(buffer);

	Balau::TaskMan::registerTask(analysis);

	new c_wxAsmWidget(db, this, -1, "ASM view");

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

BEGIN_EVENT_TABLE(c_wxIgorFrame, wxFrame)
EVT_MENU(wxID_OPEN, c_wxIgorFrame::OnOpen)

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