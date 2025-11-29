///
///	main.cpp:
///		The main entry point of the application.
///	

#include <windows.h>
#include "const.h"
#include "resource.h"
#include <prsht.h>
#include <versionhelpers.h>
#include <tchar.h>
#include <shlobj.h>
#include "utility.h"
#include <uxtheme.h>

/* Page Includes Start */
#include "psystem.h"
#include "pwin11.h"
#include "papperance.h"
#include "padvappearance.h"
#include "pbehavior.h"
#include "pkeyboard.h"
#include "pprivacy.h"
#include "pbootlogon.h"
#include "psettingscpl.h"
#include "pexplorer.h"
#include "pnetwork.h"
#include "ppower.h"
#include "ptools.h"
#include "pabout.h"
/* Page Includes End */

static HWND hPropSheet;

LRESULT CALLBACK PropSheetProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case 8001:
			ShellAbout(hDlg, TEXT("Windows"), NULL, NULL);
			break;
		}
		break;
	}

	return DefSubclassProc(hDlg, uMsg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		PROPSHEETPAGE ppsp[14] = {};
		ppsp[0].pszTemplate = MAKEINTRESOURCE(IDD_SYSTEM);
		ppsp[0].pfnDlgProc = SystemPageProc;

		ppsp[1].pszTemplate = MAKEINTRESOURCE(IDD_WIN11);
		ppsp[1].pfnDlgProc = Win11PageProc;

		ppsp[2].pszTemplate = MAKEINTRESOURCE(IDD_APPEARANCE);
		ppsp[2].pfnDlgProc = AppearancePageProc;

		ppsp[3].pszTemplate = MAKEINTRESOURCE(IDD_ADVAPPEARANCE);
		ppsp[3].pfnDlgProc = AdvAppearancePageProc;

		ppsp[4].pszTemplate = MAKEINTRESOURCE(IDD_BEHAVIOR);
		ppsp[4].pfnDlgProc = BehaviorPageProc;

		ppsp[5].pszTemplate = MAKEINTRESOURCE(IDD_KEYBOARD);
		ppsp[5].pfnDlgProc = KeyboardPageProc;

		ppsp[6].pszTemplate = MAKEINTRESOURCE(IDD_PRIVACY);
		ppsp[6].pfnDlgProc = PrivacyPageProc;

		ppsp[7].pszTemplate = MAKEINTRESOURCE(IDD_BOOTLOGON);
		ppsp[7].pfnDlgProc = BootLogonPageProc;

		ppsp[8].pszTemplate = MAKEINTRESOURCE(IDD_SETTINGSCPL);
		ppsp[8].pfnDlgProc = SettingsCplPageProc;

		ppsp[9].pszTemplate = MAKEINTRESOURCE(IDD_EXPLORER);
		ppsp[9].pfnDlgProc = ExplorerPageProc;

		ppsp[10].pszTemplate = MAKEINTRESOURCE(IDD_NETWORK);
		ppsp[10].pfnDlgProc = NetworkPageProc;

		ppsp[11].pszTemplate = MAKEINTRESOURCE(IDD_POWER);
		ppsp[11].pfnDlgProc = PowerPageProc;

		ppsp[12].pszTemplate = MAKEINTRESOURCE(IDD_TOOLS);
		ppsp[12].pfnDlgProc = ToolsPageProc;

		ppsp[13].pszTemplate = MAKEINTRESOURCE(IDD_ABOUT);
		ppsp[13].pfnDlgProc = AboutPageProc;

		// Set values common to all pages
		for (int i = 0; i < ARRAYSIZE(ppsp); i++)
		{
			ppsp[i].dwSize = sizeof(PROPSHEETPAGE);
			ppsp[i].hInstance = GetModuleHandle(nullptr);
		}

		PROPSHEETHEADER psh = {};
		psh.dwSize = sizeof(PROPSHEETHEADER);
		psh.dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS | PSH_USEHICON;
		psh.pszCaption = TEXT("WinTweak");
		psh.nPages = ARRAYSIZE(ppsp);
		psh.ppsp = ppsp;
		psh.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON));

		hPropSheet = (HWND)PropertySheet(&psh);

		HMENU hSysMenu = GetSystemMenu(hPropSheet, FALSE);
		AppendMenu(hSysMenu, MF_SEPARATOR, 0, TEXT(""));
		AppendMenu(hSysMenu, MF_STRING, 8001, TEXT("&About Windows..."));
		
		SetWindowSubclass(hPropSheet, PropSheetProc, 100, NULL);

		WINDOWPLACEMENT wp{};
		DWORD dwWpSize = sizeof(wp);
		LONG result = RegGetValue(HKCU, WINTWEAK_CONFIG_REG_ROOTKEY, TEXT("LastWindowPlacement"), RRF_RT_REG_BINARY, NULL, &wp, &dwWpSize);

		if (result == ERROR_SUCCESS)
		{
			SetWindowPlacement(hPropSheet, &wp);
		}

		TCHAR bufBD[16] = TEXT("");
		DWORD dwBDSize = sizeof(bufBD);
		result = RegGetValue(HKCU, WINTWEAK_CONFIG_REG_ROOTKEY, TEXT("AppWin11BackDropType"), RRF_RT_REG_SZ, NULL, bufBD, &dwBDSize);

		if (result == ERROR_SUCCESS)
		{
			HMODULE hDwm = LoadLibrary(TEXT("dwmapi.dll"));
			if (hDwm)
			{
				PFN_DwmSetWindowAttribute DwmSetWindowAttribute =
					(PFN_DwmSetWindowAttribute)GetProcAddress(hDwm, "DwmSetWindowAttribute");

				if (DwmSetWindowAttribute)
				{
					int preference = _tcscmp(bufBD, TEXT("BD_NONE")) == 0 ? 1 /* DWMSBT_NONE */
						: _tcscmp(bufBD, TEXT("BD_AUTO")) == 0 ? 0 /* DWMSBT_AUTO */
						: _tcscmp(bufBD, TEXT("BD_MICA")) == 0 ? 2
						: _tcscmp(bufBD, TEXT("BD_ACRYLIC")) == 0 ? 3
						: _tcscmp(bufBD, TEXT("BD_MICA_ALT")) == 0 ? 4
						: -1;

					if (preference == -1)
					{
						ShowRegistryError(hPropSheet, REDMF_READ, TEXT("HKCU\\Software\\FireBlade\\WinTweak"), TEXT("AppWin11BackDropType"), ERROR_INVALID_DATA);
						PostQuitMessage(0);

						// unfinished:
						//TCHAR bufText[256] = TEXT("");
						//_stprintf_s(bufText, 256, TEXT("Invalid value for AppWin11BackDropType in key HKCU\\Software\\FireBlade\\WinTweak. Got %s, expected one of the following values: BD_NONE, BD_AUTO, BD_MICA, BD_ACRYLIC, BD_MICA_ALT."),
						//	)

						//	

						//TaskDialog(hwnd, NULL, TEXT("Configuration Error"), TEXT("Configuration error"),
						//	)
					}
					else
					{
						DwmSetWindowAttribute(hPropSheet, 38 /* DWMWA_SYSTEMBACKDROP_TYPE */, &preference, sizeof(preference));
					}
				}

				FreeLibrary(hDwm);
			}
		}
		else if (result != ERROR_FILE_NOT_FOUND)
		{
			ShowRegistryError(hPropSheet, REDMF_READ, TEXT("HKCU\\Software\\FireBlade\\WinTweak"), TEXT("AppWin11BackDropType"), result);
		}
		break;
	}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	DWORD dwVS = 0;
	DWORD dwVSSize = sizeof(dwVS);
	LONG result = RegGetValue(HKCU, WINTWEAK_CONFIG_REG_ROOTKEY, TEXT("AllowUseVisualStyle"), RRF_RT_REG_DWORD, NULL, &dwVS, &dwVSSize);

	if (result == ERROR_SUCCESS)
	{
		SetThemeAppProperties(dwVS ? (STAP_ALLOW_CONTROLS | STAP_ALLOW_NONCLIENT | STAP_ALLOW_WEBCONTENT) : 0);
	}

	EnablePrivilege(SE_BACKUP_NAME);
	EnablePrivilege(SE_DEBUG_NAME);
	EnablePrivilege(SE_IMPERSONATE_NAME);

	WNDCLASS wc = {};
	wc.lpszClassName = WC_MAINWND;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));

	if (!RegisterClass(&wc)) return 0;

	HWND hwnd = CreateWindowEx(
		0, WC_MAINWND, TEXT("WinTweak"), 0,
		0, 0, 0, 0,
		HWND_MESSAGE, NULL, hInstance, NULL
	);

	if (!hwnd) return 0;

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!IsDialogMessage(hPropSheet, &msg))
		{
			if (hPropSheet && (PropSheet_GetCurrentPageHwnd(hPropSheet) == NULL))
			{
				DWORD dwResult = PropSheet_GetResult(hPropSheet);

				// OLD CODE
				// We don't use this anymore because this makes it so that if you press Restart later or whatever the button is
				// it closes the prop sheet anyway, and if we only do the closing if the result is the proper one then we just
				// get stuck in a loop of the restart dialog. Therefore, we instead manually trigger the dialog in the actual pages
				// with macro constants defined in const.h for the messages.
				// 
				//if (dwResult == ID_PSREBOOTSYSTEM)
				//{
				//	RestartDialog(hPropSheet, TEXT("You need to log off to apply the changes. Do you want to log off now?"), EWX_LOGOFF);
				//	OutputDebugString(TEXT("Log off!\n"));
				//}
				//else if (dwResult == ID_PSRESTARTWINDOWS)
				//{
				//	RestartDialog(hPropSheet, TEXT("You need to rebbot the system to apply the changes. Do you want to reboot now?"), EWX_REBOOT);
				//}

				WINDOWPLACEMENT wp{};
				if (GetWindowPlacement(hPropSheet, &wp))
				{
					HKEY hKey;
					result = RegCreateKeyEx(HKCU, WINTWEAK_CONFIG_REG_ROOTKEY, 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
					
					if (result == ERROR_SUCCESS)
					{
						RegSetKeyValue(hKey, NULL, TEXT("LastWindowPlacement"), REG_BINARY, &wp, sizeof(wp));

						RegCloseKey(hKey);
					}
				}

				// enable the parent first to prevent another window from becoming the foreground window
				EnableWindow(hwnd, TRUE);
				DestroyWindow(hPropSheet);
				hPropSheet = NULL;

				PostQuitMessage(0);
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}