///
///	papperance.cpp:
///		Provides functions for the Appearance property sheet page.
/// 

#include "papperance.h"
#include <commctrl.h>
#include "resource.h"
#include <htmlhelp.h>
#include <prsht.h>
#include "const.h"
#include <tchar.h>
#include <filesystem>
#include "utility.h"
#include <shlobj.h>

#define ID_CUSTACCENTCOLMENU_DELETE 2000

INT_PTR CALLBACK AppearancePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		Button_SetNote(GetDlgItem(hwnd, IDC_COMMAND_ENABLEAEROLITE), TEXT("Show the hidden Aero Lite theme in Personalization settings"));

		HKEY hKey;
		LONG lResult = RegOpenKeyEx(HKCU, TEXT("Software\\Microsoft\\Windows\\DWM"), 0, KEY_READ | KEY_SET_VALUE, &hKey);

		if (lResult == ERROR_SUCCESS)
		{
			DWORD dwType;
			DWORD dwVal;
			DWORD dwSVal = sizeof(dwVal);

			lResult = RegQueryValueEx(hKey, TEXT("AnimationsShiftKey"), nullptr, &dwType, (LPBYTE)&dwVal, &dwSVal);

			if (lResult == ERROR_SUCCESS)
			{
				CheckDlgButton(hwnd, IDC_CHECK_SLOWWNDANIMS_ONSHIFT, dwVal ? BST_CHECKED : BST_UNCHECKED);
			}

			RegCloseKey(hKey);
		}

		TCHAR bufWinDir[MAX_PATH] = TEXT("C:\\Windows");
		GetWindowsDirectory(bufWinDir, ARRAYSIZE(bufWinDir));

		TCHAR bufAeroLite[MAX_PATH] = TEXT("C:\\Windows\\Resources\\Themes\\aerolite.theme");
		_stprintf_s(bufAeroLite, ARRAYSIZE(bufAeroLite), TEXT("%s\\Resources\\Themes\\aerolite.theme"), bufWinDir);

		if (std::filesystem::exists(bufAeroLite))
		{
			EnableWindow(GetDlgItem(hwnd, IDC_COMMAND_ENABLEAEROLITE), FALSE);
		}

		DWORD dwThemeChangesMousePointers = 0;
		DWORD dwTCMPSize = sizeof(dwThemeChangesMousePointers);

		lResult = RegGetValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes"), TEXT("ThemeChangesMousePointers"), RRF_RT_REG_DWORD, NULL,
			&dwThemeChangesMousePointers, &dwTCMPSize);

		if (lResult == ERROR_SUCCESS)
		{
			if (dwThemeChangesMousePointers == 1)
			{
				CheckDlgButton(hwnd, IDC_CHECK_THEMEALLOWPOINTERCHANGE, BST_CHECKED);
			}
		}
		else if (lResult == ERROR_FILE_NOT_FOUND)
		{
			CheckDlgButton(hwnd, IDC_CHECK_THEMEALLOWPOINTERCHANGE, BST_CHECKED);
		}
		else
		{
			ShowRegistryError(hwnd, REDMF_READ, TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes"), TEXT("ThemeChangesMousePointers"), lResult);
		}

		DWORD dwThemeChangesDeskIcons = 0;
		DWORD dwTCDISize = sizeof(dwThemeChangesDeskIcons);

		lResult = RegGetValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes"), TEXT("ThemeChangesDesktopIcons"), RRF_RT_REG_DWORD, NULL,
			&dwThemeChangesDeskIcons, &dwTCDISize);

		if (lResult == ERROR_SUCCESS)
		{
			if (dwThemeChangesDeskIcons == 1)
			{
				CheckDlgButton(hwnd, IDC_CHECK_THEMEALLOWDESKICONCHANGE, BST_CHECKED);
			}
		}
		else if (lResult == ERROR_FILE_NOT_FOUND)
		{
			CheckDlgButton(hwnd, IDC_CHECK_THEMEALLOWDESKICONCHANGE, BST_CHECKED);
		}
		else
		{
			ShowRegistryError(hwnd, REDMF_READ, TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes"), TEXT("ThemeChangesDesktopIcons"), lResult);
		}

		break;
	}
	case WM_NOTIFY:
	{
		LPNMHDR nmhdr = (LPNMHDR)lParam;

		switch (nmhdr->code)
		{
		case NM_CLICK:
			switch (nmhdr->idFrom)
			{
			case IDC_SYSLINK_CPLAPPEARANCE:
			{
				TCHAR buffer[MAX_PATH] = TEXT("C:\\Windows");

				GetWindowsDirectory(buffer, MAX_PATH);

				ShellExecute(GetParent(hwnd), NULL, TEXT("explorer.exe"), TEXT("shell:::{ED834ED6-4B5A-4bfe-8F11-A626DCB6A921}"), buffer, SW_SHOW);

				break;
			}
			case IDC_SYSLINK_COLORSETTINGS:
			{
				ShellExecute(hwnd, NULL, TEXT("ms-settings:personalization-colors"), NULL, NULL, SW_SHOW);

				break;
			}
			}
			break;
		case PSN_KILLACTIVE:
			SetWindowLongPtr(hwnd, DWLP_MSGRESULT, FALSE);
			return TRUE;

		case PSN_APPLY:
		{
			SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);

			HKEY hKey;
			LONG lResult = RegOpenKeyEx(HKCU, TEXT("Software\\Microsoft\\Windows\\DWM"), 0, KEY_READ | KEY_SET_VALUE, &hKey);

			if (lResult == ERROR_SUCCESS)
			{
				DWORD dwType;
				DWORD dwVal;
				DWORD dwSVal = sizeof(dwVal);
				BOOL bAnimsShiftEnabled = FALSE;

				lResult = RegQueryValueEx(hKey, TEXT("AnimationsShiftKey"), nullptr, &dwType, (LPBYTE)&dwVal, &dwSVal);

				if (lResult == ERROR_SUCCESS)
				{
					bAnimsShiftEnabled = dwVal;
				}

				if (bAnimsShiftEnabled && IsDlgButtonChecked(hwnd, IDC_CHECK_SLOWWNDANIMS_ONSHIFT) != BST_CHECKED)
				{
					DWORD value = FALSE;

					RegSetValueEx(hKey, TEXT("AnimationsShiftKey"), NULL, REG_DWORD, (const BYTE*)&value, sizeof(value));
					RestartDialog(hwnd, DLGMESSAGE_LOGOFF, EWX_LOGOFF);
				}
				else if (!bAnimsShiftEnabled && IsDlgButtonChecked(hwnd, IDC_CHECK_SLOWWNDANIMS_ONSHIFT) == BST_CHECKED)
				{
					DWORD value = TRUE;

					RegSetValueEx(hKey, TEXT("AnimationsShiftKey"), NULL, REG_DWORD, (const BYTE*)&value, sizeof(value));
					RestartDialog(hwnd, DLGMESSAGE_LOGOFF, EWX_LOGOFF);
				}

				RegCloseKey(hKey);
			}

			BOOL restartExplorer = FALSE;

			DWORD dwThemeChangesMousePointers = 0;
			DWORD dwTCMPSize = sizeof(dwThemeChangesMousePointers);

			lResult = RegGetValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes"), TEXT("ThemeChangesMousePointers"), RRF_RT_REG_DWORD, NULL,
				&dwThemeChangesMousePointers, &dwTCMPSize);

			DWORD dwTCMPNew = IsDlgButtonChecked(hwnd, IDC_CHECK_THEMEALLOWPOINTERCHANGE) == BST_CHECKED ? 1 : 0;
			lResult = RegSetKeyValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes"), TEXT("ThemeChangesMousePointers"), REG_DWORD, &dwTCMPNew,
				sizeof(dwTCMPNew));

			DWORD dwThemeChangesDeskIcons = 0;
			DWORD dwTCDISize = sizeof(dwThemeChangesDeskIcons);

			lResult = RegGetValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes"), TEXT("ThemeChangesDesktopIcons"), RRF_RT_REG_DWORD, NULL,
				&dwThemeChangesDeskIcons, &dwTCDISize);

			DWORD dwTCDINew = IsDlgButtonChecked(hwnd, IDC_CHECK_THEMEALLOWDESKICONCHANGE) == BST_CHECKED ? 1 : 0;
			lResult = RegSetKeyValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes"), TEXT("ThemeChangesDesktopIcons"), REG_DWORD, &dwTCDINew,
				sizeof(dwTCDINew));

			if (dwThemeChangesDeskIcons != dwTCDINew)
			{
				restartExplorer = TRUE;
			}

			if (dwThemeChangesMousePointers != dwTCMPNew)
			{
				restartExplorer = TRUE;
			}

			if (restartExplorer) RestartExplorerDialog(hwnd);

			return TRUE;
		}
		}

		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_COMMAND_ENABLEAEROLITE:
		{
			TCHAR bufWinDir[MAX_PATH] = TEXT("C:\\Windows");
			GetWindowsDirectory(bufWinDir, ARRAYSIZE(bufWinDir));

			TCHAR bufThemes[MAX_PATH] = TEXT("C:\\Windows\\Resources\\Themes");
			_stprintf_s(bufThemes, ARRAYSIZE(bufThemes), TEXT("%s\\Resources\\Themes"), bufWinDir);

			TCHAR bufAero[MAX_PATH] = TEXT("C:\\Windows\\Resources\\Themes\\aero.theme");
			_stprintf_s(bufAero, ARRAYSIZE(bufAero), TEXT("%s\\aero.theme"), bufThemes);

			TCHAR bufAeroLite[MAX_PATH] = TEXT("C:\\Windows\\Resources\\Themes\\aerolite.theme");
			_stprintf_s(bufAeroLite, ARRAYSIZE(bufAeroLite), TEXT("%s\\aerolite.theme"), bufThemes);

			if (CopyFile(bufAero, bufAeroLite, FALSE))
			{
				WritePrivateProfileString(TEXT("Theme"), TEXT("DisplayName"), TEXT("Aero Lite"), bufAeroLite);

				TCHAR bufVs[MAX_PATH] = TEXT("C:\\Windows\\Resources\\Themes\\Aero\\AeroLite.msstyles");
				_stprintf_s(bufVs, ARRAYSIZE(bufVs), TEXT("%s\\Aero\\AeroLite.msstyles"), bufThemes);

				WritePrivateProfileString(TEXT("VisualStyles"), TEXT("Path"), bufVs, bufAeroLite);

				EnableWindow(GetDlgItem(hwnd, IDC_COMMAND_ENABLEAEROLITE), FALSE);
			}

			break;
		}
		case IDC_SYSLINK_CPLAPPEARANCE:
			break;
		case IDC_BUTTON_CUSTACC1:
		case IDC_BUTTON_CUSTACC2:
		case IDC_BUTTON_CUSTACC3:
		case IDC_BUTTON_CUSTACC4:
		case IDC_BUTTON_CUSTACC5:
		case IDC_BUTTON_CUSTACC6:
		case IDC_BUTTON_CUSTACC7:
		case IDC_BUTTON_CUSTACC8:
		{
			static COLORREF custColors[16] = {};

			// cant use the constant because we aren't reading from root
			DWORD dwCustColorsSize = sizeof(custColors);
			LONG result = RegGetValue(HKCU, TEXT("Software\\FireBlade\\WinTweak\\Misc"), TEXT("CAccentDlgCustCols"), RRF_RT_REG_BINARY, NULL, &custColors,
				&dwCustColorsSize);

			int ctrlId = LOWORD(wParam);
			int iAccent = ctrlId == IDC_BUTTON_CUSTACC1 ? 0
				: ctrlId == IDC_BUTTON_CUSTACC2 ? 0
				: ctrlId == IDC_BUTTON_CUSTACC3 ? 1
				: ctrlId == IDC_BUTTON_CUSTACC4 ? 1
				: ctrlId == IDC_BUTTON_CUSTACC5 ? 2
				: ctrlId == IDC_BUTTON_CUSTACC6 ? 2
				: ctrlId == IDC_BUTTON_CUSTACC7 ? 3
				: ctrlId == IDC_BUTTON_CUSTACC8 ? 3
				: 0;

			int iTheme = ctrlId == IDC_BUTTON_CUSTACC1 ? 0
				: ctrlId == IDC_BUTTON_CUSTACC2 ? 1
				: ctrlId == IDC_BUTTON_CUSTACC3 ? 0
				: ctrlId == IDC_BUTTON_CUSTACC4 ? 1
				: ctrlId == IDC_BUTTON_CUSTACC5 ? 0
				: ctrlId == IDC_BUTTON_CUSTACC6 ? 1
				: ctrlId == IDC_BUTTON_CUSTACC7 ? 0
				: ctrlId == IDC_BUTTON_CUSTACC8 ? 1
				: 0;

			TCHAR bufKey[512] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Accents\\0\\Theme0");
			_stprintf_s(bufKey, 512, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Accents\\%d\\Theme%d"), iAccent, iTheme);

			CHOOSECOLOR cc = {};
			cc.lStructSize = sizeof(CHOOSECOLOR);
			cc.Flags = CC_ANYCOLOR | CC_SOLIDCOLOR | CC_FULLOPEN;
			cc.hwndOwner = hwnd;
			cc.lpCustColors = custColors;

			DWORD dwColorVal = 0;
			DWORD dwColorValSize = sizeof(dwColorVal);
			result = RegGetValue(HKLM, bufKey, TEXT("Color"), RRF_RT_REG_DWORD, NULL, &dwColorVal, &dwColorValSize);

			if (result == ERROR_SUCCESS)
			{
				BYTE r = (dwColorVal & 0x000000FF);        // RR
				BYTE g = (dwColorVal & 0x0000FF00) >> 8;   // GG
				BYTE b = (dwColorVal & 0x00FF0000) >> 16;  // BB

				cc.Flags |= CC_RGBINIT;
				cc.rgbResult = RGB(r, g, b);
			}

			if (ChooseColor(&cc))
			{
				HKEY hKey;
				result = RegCreateKeyEx(HKLM, bufKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);

				OutputDebugString(TEXT("check"));

				if (result == ERROR_SUCCESS)
				{
					BYTE r = GetRValue(cc.rgbResult);
					BYTE g = GetGValue(cc.rgbResult);
					BYTE b = GetBValue(cc.rgbResult);

					DWORD abgr =
						(0xFFu << 24) |   // alpha
						(DWORD)b << 16 |  // blue
						(DWORD)g << 8 |  // green
						(DWORD)r;         // red

					result = RegSetValueEx(hKey, TEXT("Color"), 0, REG_DWORD, (BYTE*)&abgr, sizeof(abgr));

					if (result != ERROR_SUCCESS)
					{
						TCHAR buf[512] = TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Accents\\0\\Theme0");
						_stprintf_s(buf, 512, TEXT("HKLM\\%s"), bufKey);

						ShowRegistryError(hwnd, REDMF_WRITE, buf, TEXT("Color"), result);
					}

					RegCloseKey(hKey);
				}
			}

			HKEY hKey;
			result = RegCreateKeyEx(HKCU, TEXT("Software\\FireBlade\\WinTweak\\Misc"), NULL, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

			if (result == ERROR_SUCCESS)
			{
				result = RegSetValueEx(hKey, TEXT("CAccentDlgCustCols"), 0, REG_BINARY, (BYTE*)custColors, sizeof(custColors));

				RegCloseKey(hKey);
			}

			break;
		}
		default:
			PropSheet_Changed(GetParent(hwnd), hwnd);
			break;
		}
		break;
	case WM_CONTEXTMENU:
	{
		DWORD dwCtrlId = GetDlgCtrlID((HWND)wParam);

		switch (dwCtrlId)
		{
		case IDC_BUTTON_CUSTACC1:
		case IDC_BUTTON_CUSTACC2:
		case IDC_BUTTON_CUSTACC3:
		case IDC_BUTTON_CUSTACC4:
		case IDC_BUTTON_CUSTACC5:
		case IDC_BUTTON_CUSTACC6:
		case IDC_BUTTON_CUSTACC7:
		case IDC_BUTTON_CUSTACC8:
		{
			HMENU hMenu = CreatePopupMenu();

			AppendMenu(hMenu, MF_STRING, ID_CUSTACCENTCOLMENU_DELETE, TEXT("&Clear color"));

			DWORD dwMX = LOWORD(lParam);
			DWORD dwMY = HIWORD(lParam);

			DWORD dwX = dwMX;
			DWORD dwY = dwMY;

			RECT rcClient;
			if (GetWindowRect((HWND)wParam, &rcClient))
			{
				if (dwMX == -1 || dwMX == 65535) dwX = rcClient.left;
				if (dwMY == -1 || dwMY == 65535) dwY = rcClient.bottom;
			}

			int result = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, dwX, dwY, 0, hwnd, NULL);

			if (result == ID_CUSTACCENTCOLMENU_DELETE)
			{
				int iAccent = dwCtrlId == IDC_BUTTON_CUSTACC1 ? 0
					: dwCtrlId == IDC_BUTTON_CUSTACC2 ? 0
					: dwCtrlId == IDC_BUTTON_CUSTACC3 ? 1
					: dwCtrlId == IDC_BUTTON_CUSTACC4 ? 1
					: dwCtrlId == IDC_BUTTON_CUSTACC5 ? 2
					: dwCtrlId == IDC_BUTTON_CUSTACC6 ? 2
					: dwCtrlId == IDC_BUTTON_CUSTACC7 ? 3
					: dwCtrlId == IDC_BUTTON_CUSTACC8 ? 3
					: 0;

				int iTheme = dwCtrlId == IDC_BUTTON_CUSTACC1 ? 0
					: dwCtrlId == IDC_BUTTON_CUSTACC2 ? 1
					: dwCtrlId == IDC_BUTTON_CUSTACC3 ? 0
					: dwCtrlId == IDC_BUTTON_CUSTACC4 ? 1
					: dwCtrlId == IDC_BUTTON_CUSTACC5 ? 0
					: dwCtrlId == IDC_BUTTON_CUSTACC6 ? 1
					: dwCtrlId == IDC_BUTTON_CUSTACC7 ? 0
					: dwCtrlId == IDC_BUTTON_CUSTACC8 ? 1
					: 0;

				TCHAR bufKey[512] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Accents\\0\\Theme0");
				_stprintf_s(bufKey, 512, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Accents\\%d\\Theme%d"), iAccent, iTheme);

				TCHAR bufFullKey[512] = TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Accents\\0\\Theme0");
				_stprintf_s(bufFullKey, 512, TEXT("HKLM\\%s"), bufKey);

				result = RegDeleteTree(HKLM, bufKey);

				if (result == ERROR_SUCCESS)
				{
					// regdeletetree deletes the entire key if it doesn't have subkeys
					MessageBox(hwnd, TEXT("The color in this slot was cleared successfully."), TEXT("Success"), MB_ICONINFORMATION);
				}
				else if (result == ERROR_FILE_NOT_FOUND)
				{
					MessageBox(hwnd, TEXT("The color in this slot cannot be cleared because a color is not assigned to this slot."), NULL, MB_ICONERROR);
				}
				else
				{
					ShowRegistryError(hwnd, REDMF_WRITE, bufFullKey, TEXT("<none>"), result);
				}
			}

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
		case IDC_BUTTON_CUSTACC1:
		case IDC_BUTTON_CUSTACC2:
		case IDC_BUTTON_CUSTACC3:
		case IDC_BUTTON_CUSTACC4:
		case IDC_BUTTON_CUSTACC5:
		case IDC_BUTTON_CUSTACC6:
		case IDC_BUTTON_CUSTACC7:
		case IDC_BUTTON_CUSTACC8:
			hhp.pszText = TEXT("Define up to 8 custom colors which will be displayed in the bottom of Settings -> Personalization -> Color.");
			break;
		case IDC_CHECK_SLOWWNDANIMS_ONSHIFT:
			hhp.pszText = TEXT("When enabled, you can slow down window animations by pressing and holding the Shift key. This is the animation you see when minimizing, maximizing or closing a window.");
			break;
		case IDC_CP_TBBUTTON_CHECK:
			hhp.pszText = TEXT("Diables the Copilot taskbar button - see the Disable Copilot checkbox above for more information.");
			break;
		case IDC_CHECK_THEMEALLOWDESKICONCHANGE:
			hhp.pszText = TEXT("Allows or prevents Windows themes from changing desktop icons.");
			break;
		case IDC_CHECK_THEMEALLOWPOINTERCHANGE:
			hhp.pszText = TEXT("Allows or prevents Windows themes from changing mouse pointers.");
			break;
		}

		HtmlHelp((HWND)lphi->hItemHandle, NULL, HH_DISPLAY_TEXT_POPUP, (DWORD_PTR)&hhp);

		break;
	}
	}

	return FALSE;
}
