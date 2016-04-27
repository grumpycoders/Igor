#include "stdafx.h"
#include <atomic>

#include <Exceptions.h>

#include "wxIgorApp.h"
#include "wxIgorFrame.h"
#include "wxIgorShared.h"
#include "IgorAsyncActions.h"
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

int c_wxIgorApp::OnExit() {
    stopIgorAsyncWorker();
    return 0;
}

class wxRunnerThread : public Balau::Thread {
public:
    wxRunnerThread(int & argc, char ** argv) : m_argc(argc), m_argv(argv) { }
    bool waitUntilStarted() {
        m_queue.pop();
        return m_success;
    }
private:
    void * proc() {
        m_success = wxEntryStart(m_argc, m_argv);
        if (!m_success) {
            m_queue.push(NULL);
            return NULL;
        }
        m_success = wxApp::GetInstance()->CallOnInit();
        if (!m_success) {
            m_queue.push(NULL);
            return NULL;
        }
        ScopedLambda sl([&]() { wxApp::GetInstance()->OnExit(); });
        startIgorAsyncWorker();
        m_queue.push(NULL);
        wxApp::GetInstance()->OnRun();

        return NULL;
    }
    int & m_argc;
    char ** m_argv;
    Balau::Queue<void> m_queue;
    bool m_success;
};

static wxRunnerThread * g_wxThread = NULL;

bool wxIgorStartup(int & argc, char ** argv) {
    g_wxThread = new wxRunnerThread(argc, argv);
    g_wxThread->threadStart();
    bool success = g_wxThread->waitUntilStarted();
    if (!success) {
        g_wxThread->join();
        delete g_wxThread;
        g_wxThread = NULL;
    }
    return success;
}

void wxIgorExit() {
    wxApp::GetInstance()->ExitMainLoop();
    g_wxThread->join();
    delete g_wxThread;
    g_wxThread = NULL;
}
