#include "stdafx.h"
#include "IgorUsers.h"
#include "wxManageUsers.h"

#include <vector>
#include "BString.h"

using namespace Balau;

wxManageUsersDialog::wxManageUsersDialog(wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style)
{
	Init();
	Create(parent, id, caption, pos, size, style);
}

void wxManageUsersDialog::Init()
{
	m_userList = NULL;
}

bool wxManageUsersDialog::Create(wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style)
{
	wxDialog::Create(parent, id, caption, pos, size, style);
	CreateControls();
	if (GetSizer())
	{
		GetSizer()->SetSizeHints(this);
	}
	Centre();
	return true;
}

void wxManageUsersDialog::CreateControls()
{
	m_userList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
	RefreshList();

	wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(itemBoxSizer2);
	itemBoxSizer2->Add(m_userList);

	wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
	itemBoxSizer2->Add(itemBoxSizer3);

	itemBoxSizer3->Add(new wxButton(this, ID_ADD_USER, "Add user"));
	itemBoxSizer3->Add(new wxButton(this, ID_REMOVE_USER, "Remove user"));
	itemBoxSizer3->AddSpacer(10);
	itemBoxSizer3->Add(new wxButton(this, ID_CHANGE_PASSWORD, "Change password"));
}

void wxManageUsersDialog::RefreshList()
{
	m_userList->ClearAll();

	wxListItem col0;
	col0.SetId(0);
	col0.SetText(_("Name"));
	col0.SetWidth(300);
	m_userList->InsertColumn(0, col0);

#if 0
	wxListItem col1;
	col1.SetId(0);
	col1.SetText(_("Permissions"));
	col1.SetWidth(300);
	m_userList->InsertColumn(1, col1);
#endif

    std::vector<String> users = IgorUsers::getUsers();

    int n = 0;

    for (auto & user : users)
    {
        wxListItem item;
        item.SetId(n);
        item.SetText(user.to_charp());
        m_userList->InsertItem(item);
        n++;
    }
}

void wxManageUsersDialog::OnAddUser(wxCommandEvent& event)
{
	wxTextEntryDialog* pTextEntryDialog = new wxTextEntryDialog(this, "Enter new user");

	if (pTextEntryDialog->ShowModal() == wxID_OK)
	{
		wxString wxNewUserName = pTextEntryDialog->GetValue();
        Balau::String newUsername = wxNewUserName.char_str();
        bool success = IgorUsers::addUser(newUsername, SRP::generateVerifier(newUsername, "default"));

        // TODO: display message

		RefreshList();
	}
}

void wxManageUsersDialog::OnRemoveUser(wxCommandEvent& event)
{
	std::vector<Balau::String> selectedUsernames;

	long itemIndex = -1;

	for (;;)
    {
		itemIndex = m_userList->GetNextItem(itemIndex,
			wxLIST_NEXT_ALL,
			wxLIST_STATE_SELECTED);

		if (itemIndex == -1) break;

		// Got the selected item index
		selectedUsernames.push_back(m_userList->GetItemText(itemIndex).c_str().AsChar());
	}

	if (selectedUsernames.size())
	{
        for (auto & user : selectedUsernames)
        {
            bool success = IgorUsers::delUser(user);

            // TODO: display message
        }
		//TODO: remove from list
		RefreshList();
	}
}

void wxManageUsersDialog::OnChangePassword(wxCommandEvent& event)
{
    if (m_userList->GetSelectedItemCount() != 1)
    {
        // TODO: display message
        return;
    }

    long itemIndex = m_userList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    String username = m_userList->GetItemText(itemIndex).char_str();

    wxPasswordEntryDialog* pNewPasswordEntryDialog = new wxPasswordEntryDialog(this, "Enter new password");

	if (pNewPasswordEntryDialog->ShowModal() == wxID_OK)
	{
		String newPassword = pNewPasswordEntryDialog->GetValue().char_str();
        bool success = IgorUsers::changePassword(username, SRP::generateVerifier(username, newPassword));

        // TODO: display message
	}
}

BEGIN_EVENT_TABLE(wxManageUsersDialog, wxDialog)
EVT_BUTTON(ID_ADD_USER, wxManageUsersDialog::OnAddUser)
EVT_BUTTON(ID_REMOVE_USER, wxManageUsersDialog::OnRemoveUser)
EVT_BUTTON(ID_CHANGE_PASSWORD, wxManageUsersDialog::OnChangePassword)
END_EVENT_TABLE()