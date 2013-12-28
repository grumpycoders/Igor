#pragma once

#include <wx/wx.h>
#include <wx/evtloop.h>
#include <wx/apptrait.h>
#include <wx/config.h>
#include <wx/filehistory.h>

class wxIgorEventLoop : public wxEventLoopManual {
public:
    wxIgorEventLoop(wxEventLoopBase * proxy) : m_proxyLoop(proxy) { m_shouldExit = false; }
    ~wxIgorEventLoop() { delete m_proxyLoop; }

    int run();

private:
    bool ProcessEvents();

    int DoRun(void) override { return -1; }
    void ScheduleExit(int) override;
    bool Pending(void) const override { return m_proxyLoop->Pending(); }
    bool Dispatch(void) override { return m_proxyLoop->Dispatch(); }
    int DispatchTimeout(unsigned long t) override { return m_proxyLoop->DispatchTimeout(t); }
    void WakeUp(void) override { m_proxyLoop->WakeUp(); }
    bool YieldFor(long t) override { return m_proxyLoop->YieldFor(t); }

    wxEventLoopBase * m_proxyLoop;
};

class c_wxIgorFrame;

class c_wxIgorApp : public wxApp
{
public:
    // Balau really doesn't want to know about wxWidgets...
    static bool balauStart(int & argc, char ** argv);
    std::pair<bool, int> balauLoop();
    void balauExit();

    virtual bool OnInit();

private:
    // trampoline from the static
    bool balauStart();

public:
	wxConfig* m_config;
	wxFileHistory* m_fileHistory;
	c_wxIgorFrame* m_mainFrame;
};
