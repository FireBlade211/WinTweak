#include "pexplorer.h"
#include "macro.h"
#include "resource.h"

INT_PTR CALLBACK ExplorerPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		ComboBox_AddString(GetDlgItem(hDlg, IDC_COMBO_DRIVELETTERS), TEXT("Always, after labels (default)"));
		ComboBox_AddString(GetDlgItem(hDlg, IDC_COMBO_DRIVELETTERS), TEXT("Always, after local and before network drives"));
		ComboBox_AddString(GetDlgItem(hDlg, IDC_COMBO_DRIVELETTERS), TEXT("Never"));
		ComboBox_AddString(GetDlgItem(hDlg, IDC_COMBO_DRIVELETTERS), TEXT("Always, before labels"));

		break;
	}
	}

	return FALSE;
}
