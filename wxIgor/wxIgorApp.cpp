#include "wxIgorApp.h"

#include <Printer.h>

#include "wxAsmWidget.h"
#include "IgorDatabase.h"

#ifdef _DEBUG
//#pragma comment(lib, "wxexpatd.lib")
#pragma comment(lib, "wxmsw29ud.lib")
#pragma comment(lib, "wxzlibd.lib")
#pragma comment(lib, "wxjpegd.lib")
#pragma comment(lib, "wxpngd.lib")
#pragma comment(lib, "wxtiffd.lib")
#else
//#pragma comment(lib, "wxexpat.lib")
#pragma comment(lib, "wxmsw29u.lib")
#pragma comment(lib, "wxzlib.lib")
#pragma comment(lib, "wxjpeg.lib")
#pragma comment(lib, "wxpng.lib")
#pragma comment(lib, "wxtiff.lib")
#endif

extern "C" {

	int main(int argc, char ** argv);
}

class BalauThread : public wxThread
{
public:
	BalauThread(int argc, char ** argv)
	{
		m_argc = argc;
		m_argv = argv;
	}
	ExitCode Entry()
	{
		main(m_argc, m_argv);
		
		return NULL;
	}

	int m_argc;
	char ** m_argv;
};

bool c_wxIgorApp::OnInit()
{
	BalauThread* pBalauThread = new BalauThread(argc, argv);
	pBalauThread->Run();
	
	Sleep(5000);

	Balau::Printer::log(Balau::M_STATUS, "Igor starting up");

	m_mainFrame = new wxFrame((wxFrame *)NULL, -1, "Igor");
	SetTopWindow(m_mainFrame);
	m_mainFrame->Show(true);

	wxAsmWidget* disassembledView = new wxAsmWidget(s_igorDatabase::getDefaultDatabase(), m_mainFrame, -1, "Disassembly");

	return TRUE;
}

IMPLEMENT_APP(c_wxIgorApp)