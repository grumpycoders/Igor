#include "wxIgorApp.h"

#include <Printer.h>

using namespace Balau;

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
	ExitCode Entry()
	{
		main(0, NULL);
		
		return NULL;
	}
};

bool c_wxIgorApp::OnInit()
{
	BalauThread* pBalauThread = new BalauThread;
	pBalauThread->Run();
	
	Sleep(3000);

	Printer::log(M_STATUS, "Igor starting up");

	m_mainFrame = new wxFrame((wxFrame *)NULL, -1, "Igor");
	SetTopWindow(m_mainFrame);
	m_mainFrame->Show(true);

	wxTextCtrl* disassembledView = new wxTextCtrl(m_mainFrame, -1, "Disassembly");

	return TRUE;
}

IMPLEMENT_APP(c_wxIgorApp)