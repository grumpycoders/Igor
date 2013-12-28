#include "wxIgorFrame.h"
#include "wxAsmWidget.h"

#include <Input.h>
#include <Buffer.h>
#include <TaskMan.h>

#include "IgorAnalysis.h"
#include "PELoader.h"

//using namespace Balau;

c_wxIgorFrame::c_wxIgorFrame(const wxString& title, const wxPoint& pos, const wxSize& size) : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(wxID_OPEN, "&Open");

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");

	SetMenuBar(menuBar);

	CreateStatusBar();
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
			Balau::IO<Balau::Input> file(new Balau::Input(stringArray[i].c_str()));
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
		}
	}
}

BEGIN_EVENT_TABLE(c_wxIgorFrame, wxFrame)
EVT_MENU(wxID_OPEN, c_wxIgorFrame::OnOpen)
END_EVENT_TABLE()