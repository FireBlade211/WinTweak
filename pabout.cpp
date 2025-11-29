#include "pabout.h"
#include <commctrl.h>
#include <tchar.h>
#include "macro.h"
#include "resource.h"
#include <uxtheme.h>
#include "const.h"
#include <strsafe.h>

LRESULT CALLBACK AboutPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		ComboBox_AddString(GetDlgItem(hDlg, IDC_COMBO_BACKMATERIAL), TEXT("None"));
		ComboBox_AddString(GetDlgItem(hDlg, IDC_COMBO_BACKMATERIAL), TEXT("Auto"));
		ComboBox_AddString(GetDlgItem(hDlg, IDC_COMBO_BACKMATERIAL), TEXT("Mica"));
		ComboBox_AddString(GetDlgItem(hDlg, IDC_COMBO_BACKMATERIAL), TEXT("Acrylic"));
		ComboBox_AddString(GetDlgItem(hDlg, IDC_COMBO_BACKMATERIAL), TEXT("Mica alt"));
		
		HICON hIcon;
		HRESULT hr = LoadIconWithScaleDown(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON), 64, 64, &hIcon);

		if (SUCCEEDED(hr))
		{
			Static_SetImage(GetDlgItem(hDlg, IDC_STATIC_APPLOGO), IMAGE_ICON, hIcon);
		}
		
		CheckDlgButton(hDlg, IDC_CHECK_USEVS, ((GetThemeAppProperties() & STAP_ALLOW_CONTROLS) != 0) ? BST_CHECKED : BST_UNCHECKED);

		TCHAR bufBD[16] = TEXT("");
		DWORD dwBDSize = sizeof(bufBD);
		LONG result = RegGetValue(HKCU, WINTWEAK_CONFIG_REG_ROOTKEY, TEXT("AppWin11BackDropType"), RRF_RT_REG_SZ, NULL, bufBD, &dwBDSize);

		if (result == ERROR_SUCCESS)
		{
			ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_COMBO_BACKMATERIAL), _tcscmp(bufBD, TEXT("BD_NONE")) == 0 ? 0
				: _tcscmp(bufBD, TEXT("BD_AUTO")) == 0 ? 1
				: _tcscmp(bufBD, TEXT("BD_MICA")) == 0 ? 2
				: _tcscmp(bufBD, TEXT("BD_ACRYLIC")) == 0 ? 3
				: _tcscmp(bufBD, TEXT("BD_MICA_ALT")) == 0 ? 4
				: 0);
		}
		else
		{
			ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_COMBO_BACKMATERIAL), 0);
		}

		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_COMBO_BACKMATERIAL:
		{
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
			{
				HMODULE hDwm = LoadLibrary(TEXT("dwmapi.dll"));
				if (hDwm)
				{
					PFN_DwmSetWindowAttribute DwmSetWindowAttribute =
						(PFN_DwmSetWindowAttribute)GetProcAddress(hDwm, "DwmSetWindowAttribute");

					if (DwmSetWindowAttribute)
					{
						int chosenItem = ComboBox_GetCurSel((HWND)lParam);
						int preference = chosenItem == 0 ? 1 /* DWMSBT_NONE */
							: chosenItem == 1 ? 0 /* DWMSBT_AUTO */
							: chosenItem;

						DwmSetWindowAttribute(GetParent(hDlg), 38 /* DWMWA_SYSTEMBACKDROP_TYPE */, &preference, sizeof(preference));

						HKEY hKey;
						LONG result = RegCreateKeyEx(HKCU, WINTWEAK_CONFIG_REG_ROOTKEY, 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);

						if (result == ERROR_SUCCESS)
						{
							TCHAR value[32] = TEXT("");
							StringCchCopy(value, 32, chosenItem == 0 ? TEXT("BD_NONE") :
								chosenItem == 1 ? TEXT("BD_AUTO") :
								chosenItem == 2 ? TEXT("BD_MICA") :
								chosenItem == 3 ? TEXT("BD_ACRYLIC") :
								chosenItem == 4 ? TEXT("BD_MICA_ALT") :
								TEXT("BD_UNKNOWN"));

							RegSetKeyValue(hKey, NULL, TEXT("AppWin11BackDropType"), REG_SZ, value, sizeof(value) / sizeof(TCHAR));

							RegCloseKey(hKey);
						}
					}

					FreeLibrary(hDwm);
				}

				break;
			}
			}

			break;
		}
		case IDC_CHECK_USEVS:
		{
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
			{
				OutputDebugString(TEXT("STAP"));
				SetThemeAppProperties(IsDlgButtonChecked(hDlg, IDC_CHECK_USEVS) == BST_CHECKED 
					? (STAP_ALLOW_CONTROLS | STAP_ALLOW_NONCLIENT | STAP_ALLOW_WEBCONTENT)
					: 0);

				SendMessage(GetParent(hDlg), WM_THEMECHANGED, 0, 0);
				SendMessage(hDlg, WM_THEMECHANGED, 0, 0);

				EnumChildWindows(hDlg, [](HWND hChild, LPARAM lParam) -> BOOL CALLBACK {
					SendMessage(hChild, WM_THEMECHANGED, 0, 0);
					return TRUE;
					}, NULL);

				EnumChildWindows(GetParent(hDlg), [](HWND hChild, LPARAM lParam) -> BOOL CALLBACK {
					SendMessage(hChild, WM_THEMECHANGED, 0, 0);
					return TRUE;
					}, NULL);

				//HWND hTab = PropSheet_GetTabControl(GetParent(hDlg));
				//SendMessage(hTab, WM_THEMECHANGED, 0, 0);

				//for (int i = 0; i < TabCtrl_GetItemCount(hTab); i++)
				//{
				//	HWND hPage = PropSheet_IndexToHwnd(GetParent(hDlg), i);

				//	if (hPage != 0)
				//	{
				//		SendMessage(hPage, WM_THEMECHANGED, 0, 0);

				//		EnumChildWindows(hPage, [](HWND hChild, LPARAM lParam) -> BOOL CALLBACK {
				//			SendMessage(hChild, WM_THEMECHANGED, 0, 0);
				//			return TRUE;
				//			}, NULL);
				//	}
				//}

				HKEY hKey;
				LONG result = RegCreateKeyEx(HKCU, WINTWEAK_CONFIG_REG_ROOTKEY, 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);

				if (result == ERROR_SUCCESS)
				{
					DWORD dwVS = IsDlgButtonChecked(hDlg, IDC_CHECK_USEVS) == BST_CHECKED;

					RegSetKeyValue(hKey, NULL, TEXT("AllowUseVisualStyle"), REG_DWORD, &dwVS, sizeof(dwVS));

					RegCloseKey(hKey);
				}

				break;
			}
			}

			break;
		}
		}

		break;
	}
	case WM_NOTIFY:
	{
		LPNMHDR nmhdr = (LPNMHDR)lParam;

		switch (nmhdr->code)
		{
		case NM_CLICK:
		{
			PNMLINK pNmLink = (PNMLINK)lParam;
			
			ShellExecute(hDlg, NULL, pNmLink->item.szUrl, NULL, NULL, SW_SHOW);

			break;
		}
		}
		break;
	}
	}

	return FALSE;
}