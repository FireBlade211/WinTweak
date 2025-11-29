#include "pkeyboard.h"
#include <commctrl.h>
#include "resource.h"
#include "macro.h"
#include <tchar.h>
#include <versionhelpers.h>
#include "const.h"
#include <strsafe.h>
#include <shlwapi.h>
#include <intshcut.h>

typedef struct KEYITEM
{
	DWORD dwKeyCode;
	BOOL bUseShellExec;
	LPCTSTR lpShellExec;
	BOOL bUseAssoc;
	LPCTSTR lpAssoc;
	BOOL bDoNothing;
};

typedef KEYITEM* LPKEYITEM;

LPCTSTR keys[54] = {
	TEXT("Web Browser Back"),
	TEXT("Web Browser Forward"),
	TEXT("Web Browser Refresh"),
	TEXT("Web Browser Stop"),
	TEXT("Search"),
	TEXT("Web Browser Favorites"),
	TEXT("Web Browser Home"),
	TEXT("Volume Mute"),
	TEXT("Volume Down"),
	TEXT("Volume Up"),
	TEXT("Media Next"),
	TEXT("Media Previous"),
	TEXT("Media Stop"),
	TEXT("Media Play/pause"),
	TEXT("Mail"),
	TEXT("Media Select"),
	// we could fetch the name through the PIDL but then we'd get the localized or customized name
	// we want a consistent key name with the only exception being the windows version
	// so we use this
	IsWindows8Point1OrGreater() ? TEXT("This PC") : IsWindowsVistaOrGreater() ? TEXT("Computer") : TEXT("My Computer"),
	TEXT("Calculator"),
	TEXT("Bass Down"),
	TEXT("Bass Boost"),
	TEXT("Bass Up"),
	TEXT("Treble Down"),
	TEXT("Treble Up"),
	TEXT("Microphone Mute"),
	TEXT("Microphone Volume Down"),
	TEXT("Microphone Volume Up"),
	TEXT("Help"),
	TEXT("Find"),
	TEXT("New"),
	TEXT("Open"),
	TEXT("Close"),
	TEXT("Save"),
	TEXT("Print"),
	TEXT("Undo"),
	TEXT("Redo"),
	TEXT("Copy"),
	TEXT("Cut"),
	TEXT("Paste"),
	TEXT("Mail Reply"),
	TEXT("Mail Forward"),
	TEXT("Mail Send"),
	TEXT("Spell Check"),
	TEXT("Dictation Toggle"),
	TEXT("Microphone Toggle"),
	TEXT("Correction List"),
	TEXT("Media Play"),
	TEXT("Media Pause"),
	TEXT("Media Record"),
	TEXT("Media Fast Forward"),
	TEXT("Media Rewind"),
	TEXT("Media Channel Up"),
	TEXT("Media Channel Down"),
	TEXT("Delete"),
	TEXT("Flip 3D")
};

LRESULT CALLBACK KeyboardPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		HWND hKeyCombo = GetDlgItem(hwnd, IDC_COMBO_MOREKEYS_KEY);

		int numKeys = sizeof(keys) / sizeof(keys[0]);

		for (int i = 0; i < numKeys; i++)
		{
			int index = ComboBox_AddString(hKeyCombo, keys[i]);
		}

		SendMessage(hKeyCombo, CB_SETCURSEL, SendMessage(hKeyCombo, CB_GETCOUNT, 0, 0) - 1, 0);

		Edit_SetCueBannerText(GetDlgItem(hwnd, IDC_EDIT_FILEEXT), TEXT(".txt"));
		Edit_SetCueBannerText(GetDlgItem(hwnd, IDC_EDIT_APPPATH), TEXT("EXE file path or URL"));

		CheckDlgButton(hwnd, IDC_RADIO_DONOTHING, BST_CHECKED);

		ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

		LVCOLUMN lvc = {};
		lvc.mask = LVCF_TEXT | LVCF_WIDTH;
		lvc.iOrder = 0;
		
		TCHAR tColumnText1[32] = TEXT("Keys");
		lvc.pszText = tColumnText1;
		lvc.cchTextMax = ARRAYSIZE(tColumnText1);
		lvc.cx = 98;

		ListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), 0, &lvc);

		TCHAR tColumnText2[32] = TEXT("Action");
		lvc.pszText = tColumnText2;
		lvc.iOrder = 1;

		ListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), 1, &lvc);

		TCHAR tColumnText3[32] = TEXT("Data");
		lvc.pszText = tColumnText3;
		lvc.iOrder = 2;

		ListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), 2, &lvc);

		HKEY hKey;
		LONG result = RegOpenKeyEx(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AppKey"), 0, KEY_ENUMERATE_SUB_KEYS, &hKey);

		if (result == ERROR_SUCCESS)
		{
			TCHAR lpSubkeyName[MAX_PATH] = TEXT("");
			DWORD dwSubkeyNameSize = MAX_PATH;
			// we could use RegQueryInfoKey to get the largest size and properly alloc the buffer but these keys should not exceed 256 chars anyway
			result = RegEnumKeyEx(hKey, 0, lpSubkeyName, &dwSubkeyNameSize, NULL, NULL, NULL, NULL);

			INT i = 0;
			do
			{
				i++;

				TCHAR shellExec[MAX_PATH] = TEXT("");
				BOOL bHasSX = FALSE;
				BOOL bHasAssoc = FALSE;
				DWORD dwShellExecSize = MAX_PATH;
				TCHAR assoc[MAX_PATH] = TEXT("");
				DWORD dwAssocSize = MAX_PATH;

				result = RegGetValue(hKey, lpSubkeyName, TEXT("ShellExecute"), RRF_RT_REG_SZ, NULL, &shellExec, &dwShellExecSize);

				if (result == ERROR_SUCCESS)
				{
					bHasSX = TRUE;
				}

				result = RegGetValue(hKey, lpSubkeyName, TEXT("Association"), RRF_RT_REG_SZ, NULL, &assoc, &dwAssocSize);

				if (result == ERROR_SUCCESS)
				{
					bHasAssoc = TRUE;
				}

				OutputDebugString(TEXT("ENUMERATED\n"));

				// doesn't use default action - item is rebound to a different action!
				if (bHasSX || bHasAssoc)
				{
					INT code = _tstoi(lpSubkeyName);

					LVITEM lvi = {};
					lvi.mask = LVIF_TEXT | LVIF_PARAM;
					lvi.iItem = i;

					TCHAR text[48] = TEXT("");
					StringCchCopy(text, 48, keys[code - 1]);

					lvi.pszText = text;
					lvi.cchTextMax = 48;
					lvi.iSubItem = 0;

					LPKEYITEM pKi = new KEYITEM();
					pKi->dwKeyCode = code;
					pKi->bUseAssoc = bHasAssoc;
					pKi->bUseShellExec = bHasSX;
					pKi->lpAssoc = _tcsdup(assoc);
					pKi->lpShellExec = _tcsdup(shellExec);

					lvi.lParam = (LPARAM)pKi;

					INT iItem = ListView_InsertItem(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), &lvi);

					if (bHasSX)
					{
						if (_tcscmp(shellExec, TEXT("")) == 0)
						{
							TCHAR tAction[48] = TEXT("Do nothing");
							ListView_SetItemText(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), iItem, 1, tAction);
						}
						else
						{
							TCHAR tAction[48] = TEXT("Launch an app");
							ListView_SetItemText(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), iItem, 1, tAction);
							ListView_SetItemText(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), iItem, 2, shellExec);
						}
					}
					else if (bHasAssoc)
					{
						if (_tcscmp(assoc, TEXT("mailto")) == 0)
						{
							TCHAR tAction[48] = TEXT("Launch default e-mail app");
							ListView_SetItemText(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), iItem, 1, tAction);
						}
						else if (_tcscmp(assoc, TEXT("http")) == 0)
						{
							OutputDebugString(TEXT("HTTP path!\n"));
							TCHAR tAction[48] = TEXT("Launch default web browser app");
							ListView_SetItemText(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), iItem, 1, tAction);
						}
						else
						{
							TCHAR lpClassName[256];
							DWORD dwClassNameSize = 256;
							BOOL isProtocol = FALSE;

							TCHAR keyPath[MAX_PATH];
							_sntprintf_s(keyPath, ARRAYSIZE(keyPath), _T("%s"), assoc);

							HKEY hKey;
							LONG result = RegOpenKeyEx(HKEY_CLASSES_ROOT, keyPath, 0, KEY_READ, &hKey);
							if (result == ERROR_SUCCESS)
							{
								DWORD type = 0;
								result = RegGetValue(hKey, nullptr, TEXT("URL Protocol"), RRF_RT_REG_SZ, &type, nullptr, nullptr);
								if (result == ERROR_SUCCESS)
								{
									// It's a URL protocol
									isProtocol = TRUE;
								}

								RegCloseKey(hKey);
							}

							if (isProtocol)
							{
								TCHAR tAction[48] = TEXT("Launch a URL protocol");
								ListView_SetItemText(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), iItem, 1, tAction);
							}
							else
							{
								TCHAR tAction[48] = TEXT("Launch the app associated with a file extension");
								ListView_SetItemText(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), iItem, 1, tAction);
							}
							ListView_SetItemText(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), iItem, 2, assoc);
						}
					}
				}

				dwShellExecSize = MAX_PATH;
				dwAssocSize = MAX_PATH;
				dwSubkeyNameSize = MAX_PATH;

				result = RegEnumKeyEx(hKey, i, lpSubkeyName, &dwSubkeyNameSize, NULL, NULL, NULL, NULL);
			} while (result == ERROR_SUCCESS);

			RegCloseKey(hKey);
		}

		break;
	}
	case WM_NOTIFY:
	{
		LPNMHDR nmhdr = (LPNMHDR)lParam;

		switch (nmhdr->code)
		{
		case LVN_ITEMCHANGED:
		{
			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
			
			INT selCount = ListView_GetSelectedCount(GetDlgItem(hwnd, IDC_LIST_MOREKEYS));

			EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_REMOVE), selCount > 0);
			break;
		}
		case PSN_APPLY:
		{
			// uncomment this for debugging
			 
			//for (int i = 0; i < ListView_GetItemCount(GetDlgItem(hwnd, IDC_LIST_MOREKEYS)); i++)
			//{
			//	LVITEM lvi = {};
			//	lvi.iItem = i;
			//	lvi.mask = LVIF_PARAM | LVIF_TEXT;

			//	TCHAR tBufText[256] = TEXT("");
			//	lvi.pszText = tBufText;
			//	lvi.cchTextMax = 256;

			//	if (ListView_GetItem(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), &lvi))
			//	{
			//		TCHAR buffer[512] = TEXT("<unknown>");
			//		
			//		LPKEYITEM ki = (LPKEYITEM)lvi.lParam;

			//		_stprintf_s(buffer, 512, TEXT("%s: %f\n"), tBufText, (float)ki->dwKeyCode);

			//		OutputDebugString(buffer);
			//	}
			//}

			HKEY hKey;
			LONG result = RegOpenKeyEx(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AppKey"), 0, KEY_READ | KEY_WRITE, &hKey);

			if (result == ERROR_SUCCESS)
			{
				TCHAR tKeyName[256];
				DWORD dwKeyNameSize = 256;

				RegDeleteTree(hKey, NULL);

				for (int i = 0; i < ListView_GetItemCount(GetDlgItem(hwnd, IDC_LIST_MOREKEYS)); i++)
				{
					LVITEM lvi = {};
					lvi.iItem = i;
					lvi.mask = LVIF_PARAM;

					if (ListView_GetItem(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), &lvi))
					{
						LPKEYITEM pKi = (LPKEYITEM)lvi.lParam;

						HKEY hSubkey;

						TCHAR buffer[128] = TEXT("");

						if (_itot_s(pKi->dwKeyCode, buffer, 128, 10) == 0)
						{
							result = RegCreateKeyEx(hKey, buffer, NULL, NULL, 0, KEY_SET_VALUE, NULL, &hSubkey, NULL);

							if (result == ERROR_SUCCESS)
							{
								if (pKi->bDoNothing)
								{
									LPCWSTR text = TEXT(""); // shellexec empty - makes the key do nothing

									RegSetValueEx(hSubkey, TEXT("ShellExecute"), NULL, REG_SZ, (const BYTE*)text, (DWORD)((_tcslen(text) + 1) * sizeof(TCHAR)));
								}
								else if (pKi->bUseShellExec)
								{
									RegSetValueEx(hSubkey, TEXT("ShellExecute"), NULL, REG_SZ, (const BYTE*)pKi->lpShellExec, (DWORD)((_tcslen(pKi->lpShellExec) + 1)
										* sizeof(TCHAR)));
								}
								else if (pKi->bUseAssoc)
								{
									RegSetValueEx(hSubkey, TEXT("Association"), NULL, REG_SZ, (const BYTE*)pKi->lpAssoc, (DWORD)((_tcslen(pKi->lpAssoc) + 1)
										* sizeof(TCHAR)));
								}

								RegCloseKey(hSubkey);
							}
						}
					}
				}

				RegCloseKey(hKey);
			}

			break;
		}
		}

		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RADIO_APP:
		case IDC_RADIO_ASSOCIATION:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
			{
				EnableWindow(LOWORD(wParam) != IDC_RADIO_APP ? GetDlgItem(hwnd, IDC_EDIT_FILEEXT) : GetDlgItem(hwnd, IDC_EDIT_APPPATH),
					IsDlgButtonChecked(hwnd, LOWORD(wParam)) == BST_CHECKED);

				EnableWindow(LOWORD(wParam) == IDC_RADIO_APP ? GetDlgItem(hwnd, IDC_EDIT_FILEEXT) : GetDlgItem(hwnd, IDC_EDIT_APPPATH),
					IsDlgButtonChecked(hwnd, LOWORD(wParam)) != BST_CHECKED);

				EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_BROWSE), IsDlgButtonChecked(hwnd, IDC_RADIO_APP) == BST_CHECKED);
				break;
			}
			}
			break;
		case IDC_RADIO_DONOTHING:
		case IDC_RADIO_LAUNCHBROWSER:
		case IDC_RADIO_LAUNCHMAIL:
			EnableWindow(GetDlgItem(hwnd, IDC_EDIT_APPPATH), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_EDIT_FILEEXT), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_BROWSE), FALSE);
			break;

		case IDC_BUTTON_REMOVE:
		{
			HWND hList = GetDlgItem(hwnd, IDC_LIST_MOREKEYS);
			int count = ListView_GetItemCount(hList);

			if (count > 0) PropSheet_Changed(GetParent(hwnd), hwnd);

			for (int i = count - 1; i >= 0; i--)
			{
				if (ListView_GetItemState(hList, i, LVIS_SELECTED) & LVIS_SELECTED)
				{
					ListView_DeleteItem(hList, i);
				}
			}
			break;
		}
		case IDC_BUTTON_ADD:
		{
			if (IsDlgButtonChecked(hwnd, IDC_RADIO_APP) == BST_CHECKED)
			{
				TCHAR path[MAX_PATH] = TEXT("");

				GetDlgItemText(hwnd, IDC_EDIT_APPPATH, path, ARRAYSIZE(path));

				if (_tcscmp(TEXT(""), path) == 0)
				{
					MessageBox(hwnd, TEXT("The application file path or URL cannot be empty. Please enter a valid file path or URL."), NULL, MB_ICONERROR);
					break;
				}
				else
				{
					PARSEDURL pu = {};
					pu.cbSize = sizeof(PARSEDURL);

					if (ParseURL(path, &pu) == URL_E_INVALID_SYNTAX)
					{
						// Invalid URL - check file path
						if (!PathFileExists(path))
						{
							MessageBox(hwnd, TEXT("The application file path is invalid. Please enter a valid URL or a file path."), NULL, MB_ICONERROR);
							break;
						}
					}
				}
			}
			else if (IsDlgButtonChecked(hwnd, IDC_RADIO_ASSOCIATION) == BST_CHECKED)
			{
				TCHAR tBufText[256] = TEXT("");

				GetDlgItemText(hwnd, IDC_EDIT_FILEEXT, tBufText, 256);

				if (_tcscmp(TEXT(""), tBufText) == 0)
				{
					MessageBox(hwnd, TEXT("The file extension cannot be empty. Please enter a valid file extension."), NULL, MB_ICONERROR);
					break;
				}
			}

			LVITEM lvi = {};
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			
			INT selected = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_MOREKEYS_KEY));
			TCHAR text[256];
			ComboBox_GetListBoxText(GetDlgItem(hwnd, IDC_COMBO_MOREKEYS_KEY), selected, text);

			INT code = -1;

			for (int i = 0; i < ARRAYSIZE(keys); i++)
			{
				if (_tcscmp(keys[i], text) == 0)
				{
					code = i + 1;
					break;
				}
			}

			if (code < 1)
			{
				MessageBox(hwnd, TEXT("An error occured fetching the key code of the selected key. Please try again later."), NULL, MB_ICONERROR);
				break;
			}

			for (int i = 0; i < ListView_GetItemCount(GetDlgItem(hwnd, IDC_LIST_MOREKEYS)); i++)
			{
				LVITEM item = {};
				item.mask = LVIF_PARAM;
				item.iItem = i;

				if (ListView_GetItem(GetDlgItem(hwnd, IDC_LIST_MOREKEYS), &item))
				{
					LPKEYITEM pKi = (LPKEYITEM)item.lParam;
					

					if (pKi->dwKeyCode == code)
					{
						MessageBox(hwnd, TEXT("Cannot add multiple bindings to the same key. Delete the existing binding or choose a different key instead."),
							NULL, MB_ICONERROR);

						return FALSE;
					}
				}
			}

			LPKEYITEM pKi = new KEYITEM();
			pKi->bUseShellExec = IsDlgButtonChecked(hwnd, IDC_RADIO_APP) == BST_CHECKED;
			pKi->dwKeyCode = code;

			TCHAR tBufAppPath[256] = TEXT("");

			GetDlgItemText(hwnd, IDC_EDIT_APPPATH, tBufAppPath, 256);

			TCHAR tBufAssoc[256] = TEXT("");

			GetDlgItemText(hwnd, IDC_EDIT_FILEEXT, tBufAssoc, 256);
			pKi->lpShellExec = _tcsdup(tBufAppPath);
			pKi->bUseAssoc = (IsDlgButtonChecked(hwnd, IDC_RADIO_ASSOCIATION) == BST_CHECKED) || (IsDlgButtonChecked(hwnd, IDC_RADIO_LAUNCHBROWSER) == BST_CHECKED) ||
				(IsDlgButtonChecked(hwnd, IDC_RADIO_LAUNCHMAIL) == BST_CHECKED);

			pKi->bDoNothing = IsDlgButtonChecked(hwnd, IDC_RADIO_DONOTHING) == BST_CHECKED;
			pKi->lpAssoc = (IsDlgButtonChecked(hwnd, IDC_RADIO_LAUNCHBROWSER) == BST_CHECKED) ? TEXT("http")
				: (IsDlgButtonChecked(hwnd, IDC_RADIO_LAUNCHMAIL) == BST_CHECKED) ? TEXT("mailto") : _tcsdup(tBufAssoc);

			pKi->bUseShellExec = IsDlgButtonChecked(hwnd, IDC_RADIO_APP) == BST_CHECKED;

			lvi.pszText = text;
			lvi.lParam = (LPARAM)pKi;

			HWND hLv = GetDlgItem(hwnd, IDC_LIST_MOREKEYS);

			INT iItem = ListView_InsertItem(hLv, &lvi);

			if (IsDlgButtonChecked(hwnd, IDC_RADIO_APP) == BST_CHECKED)
			{
				TCHAR tAction[32] = TEXT("Launch an app");

				ListView_SetItemText(hLv, iItem, 1, tAction);

				ListView_SetItemText(hLv, iItem, 2, tBufAppPath);
			}
			else if (IsDlgButtonChecked(hwnd, IDC_RADIO_ASSOCIATION) == BST_CHECKED)
			{
				TCHAR keyPath[MAX_PATH];

				_sntprintf_s(keyPath, ARRAYSIZE(keyPath), _T("%s"), tBufAssoc);

				BOOL isProtocol = FALSE;
				HKEY hKey;
				LONG result = RegOpenKeyEx(HKEY_CLASSES_ROOT, keyPath, 0, KEY_READ, &hKey);
				if (result == ERROR_SUCCESS)
				{
					DWORD type = 0;
					result = RegGetValue(hKey, nullptr, TEXT("URL Protocol"), RRF_RT_REG_SZ, &type, nullptr, nullptr);
					if (result == ERROR_SUCCESS)
					{
						// It's a URL protocol
						isProtocol = TRUE;
					}

					RegCloseKey(hKey);
				}

				if (isProtocol)
				{
					TCHAR tAction[64] = TEXT("Launch a URL protocol");

					ListView_SetItemText(hLv, iItem, 1, tAction);
					ListView_SetItemText(hLv, iItem, 2, tBufAssoc);
				}
				else
				{
					TCHAR tAction[64] = TEXT("Launch the app associated with a file extension");

					ListView_SetItemText(hLv, iItem, 1, tAction);

					TCHAR tBufExt[256] = TEXT("");

					_stprintf_s(tBufExt, ARRAYSIZE(tBufExt), TEXT(".%s"), tBufAssoc);

					ListView_SetItemText(hLv, iItem, 2, tBufExt);

				}
			}
			else if (IsDlgButtonChecked(hwnd, IDC_RADIO_DONOTHING) == BST_CHECKED)
			{
				TCHAR tAction[32] = TEXT("Do nothing");

				ListView_SetItemText(hLv, iItem, 1, tAction);
			}
			else if (IsDlgButtonChecked(hwnd, IDC_RADIO_LAUNCHBROWSER) == BST_CHECKED)
			{
				TCHAR tAction[32] = TEXT("Launch default web browser app");

				ListView_SetItemText(hLv, iItem, 1, tAction);
			}
			else if (IsDlgButtonChecked(hwnd, IDC_RADIO_LAUNCHMAIL) == BST_CHECKED)
			{
				TCHAR tAction[32] = TEXT("Launch default e-mail app");

				ListView_SetItemText(hLv, iItem, 1, tAction);
			}

			PropSheet_Changed(GetParent(hwnd), hwnd);

			break;
		}
		case IDC_BUTTON_BROWSE:
		{
			TCHAR tBufFileName[MAX_PATH] = TEXT("");

			OPENFILENAME ofn = {};
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hwnd;
			ofn.Flags = OFN_EXPLORER | OFN_NOLONGNAMES | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
			ofn.lpstrTitle = TEXT("Browse");
			ofn.lpstrFile = tBufFileName;
			ofn.nMaxFile = ARRAYSIZE(tBufFileName);

			if (GetOpenFileName(&ofn))
			{
				SetDlgItemText(hwnd, IDC_EDIT_APPPATH, tBufFileName);
			}

			break;
		}
		}
		break;
	}

	return FALSE;
}