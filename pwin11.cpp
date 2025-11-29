///
///	pwin11.cpp:
///		Provides functions for the Windows 11 property sheet page.
/// 

#include "pwin11.h"
#include <versionhelpers.h>
#include <tchar.h>
#include "macro.h"
#include "resource.h"
#include <htmlhelp.h>
#include "const.h"
#include <strsafe.h>
#include "utility.h"
#include <shlobj.h>

BOOL CALLBACK DisableWindowsProc(HWND hwnd, LPARAM lParam)
{
	if (hwnd != (HWND)lParam)
	{
		EnableWindow(hwnd, FALSE);
	}

	return TRUE;
}

LRESULT CALLBACK Win11PageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		HKEY hKey;
		LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ, &hKey);

		if (result == ERROR_SUCCESS)
		{
			TCHAR tBufBuild[16] = TEXT("0");
			DWORD dwBufferSize = sizeof(tBufBuild);
			DWORD dwType = 0;

			result = RegQueryValueEx(hKey, TEXT("CurrentBuild"), nullptr, &dwType, reinterpret_cast<LPBYTE>(tBufBuild), &dwBufferSize);

			int build = 0;

			if (result == ERROR_SUCCESS)
			{
				build = _ttoi(tBufBuild);
			}

			if (!(IsWindowsVersionOrGreater(10, 0, 0) && build >= 22000))
			{
				ShowDlgItem(hDlg, IDC_STATIC_WIN11NOTICE, SW_SHOW);
				EnumChildWindows(hDlg, DisableWindowsProc, (LPARAM)GetDlgItem(hDlg, IDC_STATIC_WIN11NOTICE));
			}
			else
			{
				ShowDlgItem(hDlg, IDC_STATIC_WIN11NOTICE, SW_HIDE);
			}

			RegCloseKey(hKey);
		}

		TCHAR bufLegacyMenu[256] = TEXT("");
		DWORD dwLegacyMenuSize = 256;

		result = RegGetValue(HKCR, TEXT("CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\\InProcServer32"), TEXT(""), RRF_RT_REG_SZ, NULL, bufLegacyMenu,
			&dwLegacyMenuSize);

		if (result == ERROR_SUCCESS)
		{
			if (_tcscmp(bufLegacyMenu, TEXT("")) == 0)
			{
				CheckDlgButton(hDlg, IDC_CLASSICCTXMENU_CHECK, BST_CHECKED);
			}
		}

		DWORD dwOldNoCopilot = 0;
		DWORD dwOldNoCopilotSize = sizeof(dwOldNoCopilot);
		result = RegGetValue(HKCU, TEXT("Software\\Policies\\Microsoft\\Windows\\WindowsCopilot"), TEXT("TurnOffWindowsCopilot"), RRF_RT_REG_DWORD, NULL, &dwOldNoCopilot,
			&dwOldNoCopilotSize);

		if (result == ERROR_SUCCESS)
		{
			if (dwOldNoCopilot == 1)
			{
				CheckDlgButton(hDlg, IDC_NOCOPILOT_CHECK, BST_CHECKED);
			}
		}

		DWORD dwCopilotTbButton = 0;
		DWORD dwCopilotTbButtonSize = sizeof(dwCopilotTbButton);
		result = RegGetValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), TEXT("ShowCopilotButton"), RRF_RT_REG_DWORD, NULL,
			&dwCopilotTbButton, &dwCopilotTbButtonSize);

		if (result == ERROR_SUCCESS)
		{
			if (dwCopilotTbButton == 0)
			{
				CheckDlgButton(hDlg, IDC_CP_TBBUTTON_CHECK, BST_CHECKED);
			}
		}
		else if (result != ERROR_FILE_NOT_FOUND)
		{
			ShowRegistryError(hDlg, REDMF_READ, TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), TEXT("ShowCopilotButton"), result);
		}

		DWORD dwEndTaskButton = 0;
		DWORD dwEndTaskButtonSize = sizeof(dwEndTaskButton);
		result = RegGetValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\TaskbarDeveloperSettings"), TEXT("TaskbarEndTask"),
			RRF_RT_REG_DWORD, NULL, &dwEndTaskButton, &dwEndTaskButtonSize);

		if (result == ERROR_SUCCESS)
		{
			if (dwEndTaskButton == 1)
			{
				CheckDlgButton(hDlg, IDC_CHECK_SHOWENDTASK, BST_CHECKED);
			}
		}

		DWORD dwStickers = 0;
		DWORD dwStickersSize = sizeof(dwStickers);
		result = RegGetValue(HKLM, TEXT("SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\Stickers"), TEXT("EnableStickers"), RRF_RT_REG_DWORD,
			NULL, &dwStickers, &dwStickersSize);

		if (result == ERROR_SUCCESS)
		{
			if (dwStickers == 1)
			{
				CheckDlgButton(hDlg, IDC_DESKSTICKERS_CHECK, BST_CHECKED);
			}
		}

		break;
	}
	case WM_COMMAND:
	{
		PropSheet_Changed(GetParent(hDlg), hDlg);
		break;
	}
	case WM_NOTIFY:
	{
		LPNMHDR nmhdr = (LPNMHDR)lParam;

		switch (nmhdr->code)
		{
		case PSN_APPLY:
		{
			BOOL restartExplorer = FALSE;
			BOOL logOff = FALSE;
			BOOL restartWin = FALSE;

#pragma region CLASSIC CONTEXT MENU
			TCHAR bufLegacyMenu[256] = TEXT("");
			DWORD dwLegacyMenuSize = 256;
			LONG result = RegGetValue(HKCR, TEXT("CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\\InProcServer32"), TEXT(""), RRF_RT_REG_SZ, NULL, bufLegacyMenu,
				&dwLegacyMenuSize);
			BOOL bIsUsingLegacyMenu = FALSE;

			if (result == ERROR_SUCCESS)
			{
				if (_tcscmp(bufLegacyMenu, TEXT("")) == 0)
				{
					bIsUsingLegacyMenu = TRUE;
				}
			}

			// regular users and admins don't have write perms
			// use trustedinstaller to dodge
			const auto pid = StartTIService();
			if (ImpersonateSystem())
			{
				HANDLE process_handle;
				if ((process_handle = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
					FALSE,
					pid)) != nullptr)
				{
					HANDLE token_handle = 0;
					if (OpenProcessToken(
						process_handle,
						TOKEN_DUPLICATE,
						&token_handle))
					{
						CloseHandle(process_handle);

						HANDLE dup_token_handle;
						SECURITY_ATTRIBUTES token_attributes;
						token_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
						token_attributes.lpSecurityDescriptor = nullptr;
						token_attributes.bInheritHandle = FALSE;
						if (!DuplicateTokenEx(
							token_handle,
							TOKEN_ALL_ACCESS,
							&token_attributes,
							SecurityImpersonation,
							TokenImpersonation,
							&dup_token_handle))
						{
							CloseHandle(token_handle);
						}
						CloseHandle(token_handle);

						TCHAR bufSysDir[MAX_PATH] = TEXT("C:\\Windows\\System32");
						GetSystemDirectory(bufSysDir, MAX_PATH);

						TCHAR bufWinUIFileExplorerDll[MAX_PATH] = TEXT("C:\\Windows\\System32\\Windows.UI.FileExplorer.dll");
						_stprintf_s(bufWinUIFileExplorerDll, MAX_PATH, TEXT("%s\\Windows.UI.FileExplorer.dll"), bufSysDir);

						TCHAR bufCmdLine[256] = TEXT("");
						_stprintf_s(bufCmdLine, 256, TEXT("reg.exe add HKCR\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\\InProcServer32 /ve /t REG_SZ /d \"%s\" /f"),
							IsDlgButtonChecked(hDlg, IDC_CLASSICCTXMENU_CHECK) == BST_CHECKED ? TEXT("") : bufWinUIFileExplorerDll);

						STARTUPINFO startup_info;
						GetStartupInfo(&startup_info);
						PROCESS_INFORMATION process_info;
						ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));
						if (CreateProcessWithToken(
							dup_token_handle,
							LOGON_WITH_PROFILE,
							nullptr,
							bufCmdLine,
							CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW,
							nullptr,
							nullptr,
							&startup_info,
							&process_info))
						{
							CloseHandle(dup_token_handle);
							RevertToSelf();

							if ((IsDlgButtonChecked(hDlg, IDC_CLASSICCTXMENU_CHECK) == BST_CHECKED) != bIsUsingLegacyMenu)
							{
								restartExplorer = TRUE;
							}
						}
						else
						{
							CloseHandle(dup_token_handle);
							RevertToSelf();
						}
					}
					else
					{
						CloseHandle(process_handle);
						ShowTIExecError(TEXT("OpenProcessToken failed (TrustedInstaller.exe)."), GetLastError(), hDlg);
						RevertToSelf();
					}
				}
				else
				{
					ShowTIExecError(TEXT("OpenProcess failed (TrustedInstaller.exe)."), GetLastError(), hDlg);
					RevertToSelf();
				}
			}
			else
			{
				ShowTIExecError(TEXT("An error occured impersonating the system account."), ERROR_CANNOT_IMPERSONATE, hDlg);
			}
#pragma endregion

			BOOL isNoCopilotEnabled = FALSE;
			DWORD dwOldNoCopilot = 0;
			DWORD dwOldNoCopilotSize = sizeof(dwOldNoCopilot);
			result = RegGetValue(HKCU, TEXT("Software\\Policies\\Microsoft\\Windows\\WindowsCopilot"), TEXT("TurnOffWindowsCopilot"), RRF_RT_REG_DWORD, NULL, &dwOldNoCopilot,
				&dwOldNoCopilotSize);

			if (result == ERROR_SUCCESS)
			{
				if (dwOldNoCopilot == 1)
				{
					isNoCopilotEnabled = TRUE;
				}
			}

			HKEY hKey;
			result = RegCreateKeyEx(HKCU, TEXT("Software\\Policies\\Microsoft\\Windows\\WindowsCopilot"), NULL, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);

			if (result == ERROR_SUCCESS)
			{
				DWORD dwNewValue = IsDlgButtonChecked(hDlg, IDC_NOCOPILOT_CHECK) == BST_CHECKED ? 1 : 0;

				result = RegSetValueEx(hKey, TEXT("TurnOffWindowsCopilot"), NULL, REG_DWORD, (BYTE*)&dwNewValue, sizeof(dwNewValue));

				RegCloseKey(hKey);

				if (dwNewValue != dwOldNoCopilot)
				{
					if (dwNewValue == 1)
					{
						logOff = TRUE;
					}
					else
					{
						restartWin = TRUE;
					}
				}
			}

			result = RegOpenKeyEx(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), 0, KEY_WRITE, &hKey);

			if (result == ERROR_SUCCESS)
			{
				DWORD dwIsEnabled = FALSE;
				DWORD dwIsEnabledSize = sizeof(dwIsEnabled);
				result = RegGetValue(hKey, NULL, TEXT("ShowCopilotButton"), RRF_RT_REG_DWORD, NULL, &dwIsEnabled, &dwIsEnabledSize);

				// inverted (we want to hide it)
				DWORD dwNewValue = IsDlgButtonChecked(hDlg, IDC_CP_TBBUTTON_CHECK) == BST_CHECKED ? FALSE : TRUE;
				result = RegSetKeyValue(hKey, NULL, TEXT("ShowCopilotButton"), REG_DWORD, &dwNewValue, sizeof(dwNewValue));

				if (dwIsEnabled != dwNewValue)
				{
					restartExplorer = TRUE;
				}

				RegCloseKey(hKey);
			}

			result = RegCreateKeyEx(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\TaskbarDeveloperSettings"), NULL, NULL, 0, KEY_WRITE,
				NULL, &hKey, NULL);

			if (result == ERROR_SUCCESS)
			{
				DWORD dwValue = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOWENDTASK) == BST_CHECKED ? 1 : 0;
				DWORD dwValueSize = sizeof(dwValue);

				RegSetKeyValue(hKey, NULL, TEXT("TaskbarEndTask"), REG_DWORD, &dwValue, dwValueSize);

				RegCloseKey(hKey);
			}

			result = RegCreateKeyEx(HKLM, TEXT("SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\Stickers"), 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);

			if (result == ERROR_SUCCESS)
			{
				BOOL oldValue = FALSE;
				DWORD dwOldValueSize = sizeof(oldValue);

				result = RegGetValue(hKey, NULL, TEXT("EnableStickers"), RRF_RT_REG_DWORD, NULL, &oldValue, &dwOldValueSize);

				DWORD dwValue = IsDlgButtonChecked(hDlg, IDC_DESKSTICKERS_CHECK) == BST_CHECKED ? 1 : 0;

				result = RegSetKeyValue(hKey, NULL, TEXT("EnableStickers"), REG_DWORD, &dwValue, sizeof(dwValue));

				if (oldValue != dwValue)
				{
					restartExplorer = TRUE;
				}

				RegCloseKey(hKey);
			}

			if (restartWin) RestartDialog(hDlg, DLGMESSAGE_LOGOFF, EWX_LOGOFF);
			if (logOff) RestartDialog(hDlg, DLGMESSAGE_LOGOFF, EWX_LOGOFF);
			if (restartExplorer) RestartExplorerDialog(hDlg);

			break;
		}
		}

		break;
	}
	case WM_HELP:
	{
		LPHELPINFO lphi = (LPHELPINFO)lParam;
		HH_POPUP hhp = {};
		hhp.cbStruct = sizeof(HH_POPUP);
		hhp.pt = lphi->MousePos;
		hhp.rcMargins = { -1, -1, -1, -1 };

		switch (lphi->iCtrlId)
		{
		case IDC_CLASSICCTXMENU_CHECK:
			hhp.pszText = TEXT("Turn on this option to enable classic context menus in Windows 11. The OS will display full menus directly, without you having to click on the \"Show more options\" entry first.");
			break;
		case IDC_NOCOPILOT_CHECK:
			hhp.pszText = TEXT("Disables the Copilot feature if you find no use for the AI-powered assistant for your daily tasks and online activities. Copilot is now an integral part of Windows 11, starting with version 23H2, and Windows 10 build 19045.3754 or newer.\n\n Note: By disabling Copilot you will also remove its taskbar button, so disabling it individually is not required in that case.");
			break;
		case IDC_CP_TBBUTTON_CHECK:
			hhp.pszText = TEXT("Diables the Copilot taskbar button - see the Disable Copilot checkbox above for more information.");
			break;
		case IDC_DESKSTICKERS_CHECK:
			hhp.pszText = TEXT("Allows you to place stickers on your desktop wallpaper. Enable this option, then right-click the desktop and select \"Add or edit stickers\". Clicking it will open an image chooser dialog with several stickers and a search box.");
			break;
		case IDC_NOWINSPOTL_DESKICON_CHECK:
			hhp.pszText = TEXT("If you set Windows Spotlight as your Desktop background, it will place the \"Leam more about this picture\" icon. If you aren't happy to see it, you can remove it by enabling this option.");
			break;
		}

		HtmlHelp((HWND)lphi->hItemHandle, NULL, HH_DISPLAY_TEXT_POPUP, (DWORD_PTR)&hhp);

		break;
	}
	}

	return FALSE;
}