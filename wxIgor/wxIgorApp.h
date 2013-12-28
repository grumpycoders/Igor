#pragma once

#include <wx/wx.h>
#include <wx/evtloop.h>
#include <wx/apptrait.h>

class wxIgorEventLoop : public wxEventLoopManual {
public:
    wxIgorEventLoop(wxEventLoopBase * proxy) : m_proxyLoop(proxy) { }
    ~wxIgorEventLoop() { delete m_proxyLoop; }
private:
    bool ProcessEvents();

    // These are the one we're interested in...
    int DoRun(void) override;
    void ScheduleExit(int) override;

    // everything else is proxyed...
    bool Pending(void) const override { return m_proxyLoop->Pending(); }
    bool Dispatch(void) override { return m_proxyLoop->Dispatch(); }
    int DispatchTimeout(unsigned long t) override { return m_proxyLoop->DispatchTimeout(t); }
    void WakeUp(void) override { m_proxyLoop->WakeUp(); }
    bool YieldFor(long t) override { return m_proxyLoop->YieldFor(t); }

    wxEventLoopBase * m_proxyLoop;
};

class wxIgorAppTraits : public wxAppTraits {
public:
    wxIgorAppTraits(wxAppTraits * proxy) : m_proxyTraits(proxy) { }
    ~wxIgorAppTraits() { delete m_proxyTraits; }
private:
    // the one we're interested in overriding
    wxEventLoopBase * CreateEventLoop() override { return new wxIgorEventLoop(m_proxyTraits->CreateEventLoop()); }

    // everything else is proxyed...
    wxLog * CreateLogTarget() override { return m_proxyTraits->CreateLogTarget(); }
    wxMessageOutput * CreateMessageOutput() override { return m_proxyTraits->CreateMessageOutput(); }
    wxFontMapper *CreateFontMapper() override { return m_proxyTraits->CreateFontMapper(); }
    wxRendererNative *CreateRenderer() override { return m_proxyTraits->CreateRenderer(); }
    bool ShowAssertDialog(const wxString &str) override { return m_proxyTraits->ShowAssertDialog(str); }
    bool HasStderr() override { return m_proxyTraits->HasStderr(); }
    wxTimerImpl *CreateTimerImpl(wxTimer * a) override { return m_proxyTraits->CreateTimerImpl(a); }
    wxPortId GetToolkitVersion(int * a, int * b) const override { return m_proxyTraits->GetToolkitVersion(a, b); }
    bool IsUsingUniversalWidgets() const override { return m_proxyTraits->IsUsingUniversalWidgets(); }
    wxString GetDesktopEnvironment() const override { return m_proxyTraits->GetDesktopEnvironment(); }
    void *BeforeChildWaitLoop() override { return m_proxyTraits->BeforeChildWaitLoop(); }
    void AfterChildWaitLoop(void * a) override { return m_proxyTraits->AfterChildWaitLoop(a); }
    bool DoMessageFromThreadWait() override { return m_proxyTraits->DoMessageFromThreadWait(); }
    WXDWORD WaitForThread(WXHANDLE a, int b) override { return m_proxyTraits->WaitForThread(a, b); }
    bool CanUseStderr() override { return m_proxyTraits->CanUseStderr(); }
    bool WriteToStderr(const wxString & str) override { return m_proxyTraits->WriteToStderr(str); }

    wxAppTraits * m_proxyTraits;
};

class c_wxIgorApp : public wxApp
{
public:
    // Balau really doesn't want to know about wxWidgets...
    static bool balauStart(int & argc, char ** argv);
    static std::pair<bool, int> balauLoop();

    virtual bool OnInit();
    void CallOnExit();

private:
    // trampoline from the static
    bool balauStart();

    virtual wxAppTraits * CreateTraits() override { return new wxIgorAppTraits(wxApp::CreateTraits()); }

	wxFrame* m_mainFrame;
};
