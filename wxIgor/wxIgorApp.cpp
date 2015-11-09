#include "stdafx.h"
#include <atomic>

#include <Exceptions.h>

#include "wxIgorApp.h"
#include "wxIgorFrame.h"
#include "IgorDatabase.h"

#ifdef _DEBUG
//#pragma comment(lib, "wxexpatd.lib")
#pragma comment(lib, "wxmsw31ud.lib")
#pragma comment(lib, "wxzlibd.lib")
#pragma comment(lib, "wxjpegd.lib")
#pragma comment(lib, "wxpngd.lib")
#pragma comment(lib, "wxtiffd.lib")
#else
//#pragma comment(lib, "wxexpat.lib")
#pragma comment(lib, "wxmsw31u.lib")
#pragma comment(lib, "wxzlib.lib")
#pragma comment(lib, "wxjpeg.lib")
#pragma comment(lib, "wxpng.lib")
#pragma comment(lib, "wxtiff.lib")
#endif

using namespace Balau;

#define new DEBUG_NEW

wxIMPLEMENT_APP_NO_MAIN(c_wxIgorApp);

bool c_wxIgorApp::OnInit()
{
#ifdef _WIN32
    // this test for windows XP and 32bit color depth
    if (wxTheApp->GetComCtl32Version() >= 600 && ::wxDisplayDepth() >= 32)
    {
        // this disable color remapping for icons and use full 32 bit colors
        wxSystemOptions::SetOption("msw.remap", 2);
    }
#endif

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
    int r = 0;
    bool exception = false;
    try {
        r = loop->run();
    }
    catch (GeneralException & e) {
        exception = true;
        Printer::log(M_WARNING, "wxWidgets caused an exception: %s", e.getMsg());
        const char * details = e.getDetails();
        if (details)
            Printer::log(M_WARNING, "  %s", details);
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_DEBUG, "%s", str.to_charp());
    }
    catch (...) {
        exception = true;
        Printer::log(M_WARNING, "wxWidgets caused an unknown exception - stopping.");
    }
    if (exception)
        return std::pair<bool, int>(true, 0);
    return std::pair<bool, int>(s_exitting.load(), r);
}

void c_wxIgorApp::balauExit() {
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
            wxEventLoopBase::SetActive(m_proxyLoop);
            Dispatch();
            wxEventLoopBase::SetActive(this);
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
