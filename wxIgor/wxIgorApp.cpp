#include <atomic>

#include "wxIgorApp.h"
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

wxIMPLEMENT_APP_NO_MAIN(c_wxIgorApp);

bool c_wxIgorApp::OnInit()
{
	m_mainFrame = new wxFrame((wxFrame *)NULL, -1, "Igor");
	SetTopWindow(m_mainFrame);
	m_mainFrame->Show(true);

//    wxAsmWidget* disassembledView = new wxAsmWidget(s_igorDatabase::getDefaultDatabase(), m_mainFrame, -1, "Disassembly");

	return TRUE;
}

bool c_wxIgorApp::balauStart(int & argc, char ** argv) {
    bool success = wxEntryStart(argc, argv);

    if (!success)
        return false;

//    wxEntryCleanup(); // that's to be called on exit, unlike what the doc says...

    return wxGetApp().balauStart();
}

static std::atomic<bool> s_exitting;

bool c_wxIgorApp::balauStart() {
    CallOnInit();

    m_exitOnFrameDelete = Yes;

    return true;
}

std::pair<bool, int> c_wxIgorApp::balauLoop() {
    return std::pair<bool, int>(s_exitting.load(), wxGetApp().MainLoop());
}

int wxIgorEventLoop::DoRun() {
    OnNextIteration();
    ProcessIdle();

    for (;;) {
        bool hasMoreEvents = false;
        if (wxTheApp && wxTheApp->HasPendingEvents()) {
            wxTheApp->ProcessPendingEvents();
            hasMoreEvents = true;
        }

        if (Pending()) {
            Dispatch();
            hasMoreEvents = true;
        }

        if (!hasMoreEvents)
            break;
    }

    return m_shouldExit ? m_exitcode : 0;
}

void wxIgorEventLoop::ScheduleExit(int rc) {
    m_exitcode = rc;
    m_shouldExit = true;
    s_exitting.exchange(true);
    OnExit();
    WakeUp();
}

// why you no protected ?
bool wxIgorEventLoop::ProcessEvents() {
    if (wxTheApp) {
        wxTheApp->ProcessPendingEvents();

        if (m_shouldExit)
            return false;
    }

    return Dispatch();
}

#include "wxIgorShared.h"

bool wxIgorStartup(int & argc, char ** argv) {
    return c_wxIgorApp::balauStart(argc, argv);
}

std::pair<bool, int> wxIgorLoop() {
    return c_wxIgorApp::balauLoop();
}
