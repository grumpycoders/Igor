#pragma once

#include <wx/wx.h>
#include <wx/evtloop.h>
#include <wx/apptrait.h>
#include <wx/config.h>
#include <wx/filehistory.h>

class c_wxIgorFrame;

class c_wxIgorApp : public wxApp
{
public:
    virtual bool OnInit() override;
    virtual int OnExit() override;

public:
    wxConfig* m_config;
    wxFileHistory* m_fileHistory;
    c_wxIgorFrame* m_mainFrame;
};
