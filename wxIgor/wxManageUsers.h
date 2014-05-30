#pragma once

#include <wx/wx.h>
#include <wx/listctrl.h>

class  wxManageUsersDialog : public wxDialog
{
	DECLARE_EVENT_TABLE()
public:
	wxManageUsersDialog(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxString& caption = _("Manage users"),
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE);

	bool Create(wxWindow* parent,
		wxWindowID id,
		const wxString& caption,
		const wxPoint& pos,
		const wxSize& size,
		long style);

	void Init();
	void CreateControls();
	void RefreshList();

	void OnAddUser(wxCommandEvent& event);
	void OnRemoveUser(wxCommandEvent& event);
	void OnChangePassword(wxCommandEvent& event);

	wxListCtrl* m_userList;

	/// Control identifiers
	enum {
		ID_ADD_USER = 10000,
		ID_REMOVE_USER = 10001,
		ID_CHANGE_PASSWORD= 10002,
	};
};