#pragma once

#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/scrolbar.h>
#include <wx/sysopt.h>

#if defined(_MSC_VER ) && defined (_DEBUG)
#define DEBUG_MEM
#endif

#ifdef DEBUG_MEM
#define DEBUG_NEW new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#else
#define DEBUG_NEW new
#endif