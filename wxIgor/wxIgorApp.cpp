#include "stdafx.h"
#include <atomic>

#include "wxIgorApp.h"
#include "wxIgorFrame.h"
#include "IgorDatabase.h"

#ifdef _DEBUG
//#pragma comment(lib, "wxexpatd.lib")
#pragma comment(lib, "wxmsw30ud.lib")
#pragma comment(lib, "wxzlibd.lib")
#pragma comment(lib, "wxjpegd.lib")
#pragma comment(lib, "wxpngd.lib")
#pragma comment(lib, "wxtiffd.lib")
#else
//#pragma comment(lib, "wxexpat.lib")
#pragma comment(lib, "wxmsw30u.lib")
#pragma comment(lib, "wxzlib.lib")
#pragma comment(lib, "wxjpeg.lib")
#pragma comment(lib, "wxpng.lib")
#pragma comment(lib, "wxtiff.lib")
#endif

wxIMPLEMENT_APP_NO_MAIN(c_wxIgorApp);

bool c_wxIgorApp::OnInit()
{
	m_config = new wxConfig("wxIgor");

	m_fileHistory = new wxFileHistory();
	m_fileHistory->Load(*m_config);

	m_mainFrame = new c_wxIgorFrame("Igor", wxPoint(50, 50), wxSize(800, 600));
	SetTopWindow(m_mainFrame);
	m_mainFrame->Show(true);

	return TRUE;
}

bool c_wxIgorApp::balauStart(int & argc, char ** argv) {
    bool success = wxEntryStart(argc, argv);

    if (!success)
        return false;

    return wxGetApp().balauStart();
}

static std::atomic<bool> s_exitting;
static std::atomic<bool> s_onExitCalled;

bool c_wxIgorApp::balauStart() {
    CallOnInit();

    m_exitOnFrameDelete = Yes;

    auto r = balauLoop();

    return !r.first;
}

std::pair<bool, int> c_wxIgorApp::balauLoop() {
    wxIgorEventLoop * loop = dynamic_cast<wxIgorEventLoop *>(m_mainLoop);
    if (!loop) {
        m_mainLoop = loop = new wxIgorEventLoop(CreateMainLoop());
        wxEventLoopBase::SetActive(loop);
    }
    int r = loop->run();
    return std::pair<bool, int>(s_exitting.load(), r);
}

void c_wxIgorApp::balauExit() {
    if (!s_onExitCalled.load())
        OnExit();
    wxIgorEventLoop * loop = dynamic_cast<wxIgorEventLoop *>(m_mainLoop);
    if (loop)
        loop->run();
    wxEventLoopBase::SetActive(NULL);
    wxEntryCleanup();
}

int wxIgorEventLoop::run() {
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

    if (m_shouldExit)
        s_exitting.exchange(true);

    return m_shouldExit ? m_exitcode : 0;
}

void wxIgorEventLoop::ScheduleExit(int rc) {
    m_exitcode = rc;
    m_shouldExit = true;
    s_onExitCalled.exchange(true);
    OnExit();
    WakeUp();
}

#include "wxIgorShared.h"

bool wxIgorStartup(int & argc, char ** argv) {
    return c_wxIgorApp::balauStart(argc, argv);
}

std::pair<bool, int> wxIgorLoop() {
    return wxGetApp().balauLoop();
}

void wxIgorExit() {
    wxGetApp().balauExit();
}
