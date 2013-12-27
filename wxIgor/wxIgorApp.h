#pragma once

#include <wx/wx.h>

class c_wxIgorApp : public wxApp
{
	virtual bool OnInit();
	//virtual int OnExit();

private:
	wxFrame* m_mainFrame;
};
