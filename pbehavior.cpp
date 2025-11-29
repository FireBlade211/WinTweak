#include "pbehavior.h"
#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include <htmlhelp.h>
#include "const.h"
#include "utility.h"
#include <tchar.h>
#include <shlobj.h>

#define ID_AEROSNAPCMD_NOVERTICALMAXIMIZE 1001
#define ID_AEROSNAPCMD_NOMAXIMIZEDRAG 1002
#define ID_AEROSNAPCMD_NOSNAPPINGONLY 1003

typedef struct INFOBEHAVIORADVANCEDSTRUCT
{
	BOOL bNoAutoMaintenance;
	BOOL bNoFileDownloadWarn;
	BOOL bNoWinUpdateDriverUpdate;
	BOOL bNoWinUpdateMrtInstall;
	BOOL bNoSmartScreenWin;
	BOOL bNoSmartScreenEdge;
	BOOL bNoSmartScreenStore;
	BOOL bNoWinUpdate;
	BOOL bCrashOnCtrlScroll;
	BOOL bNoWinErrorReport; // WER, handled by werfault.exe
	BOOL bThumbCacheKeepOnReboot;
	DWORD dwMenuDelay;
	BOOL bNoNewFileAppsNotify;
	BOOL bNoRestorePointLimit;
	DWORD dwScreenSaverGracePeriod;
	BOOL bUsbWriteProtect;
	BOOL bMsiExecAllowInSafeMode;
};

typedef INFOBEHAVIORADVANCEDSTRUCT* LPINFOBEHAVIORADVACEDSTRUCT;

INFOBEHAVIORADVANCEDSTRUCT advStruct;

LRESULT CALLBACK AdvDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		CheckDlgButton(hDlg, IDC_CHECK_NOAUTOMAINTENANCE, advStruct.bNoAutoMaintenance ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_CHECK_NODOWNLOADBLOCK, advStruct.bNoFileDownloadWarn ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_CHECK_NOWINUPDATE_DRIVERUPDATE, advStruct.bNoWinUpdateDriverUpdate ? BST_CHECKED : BST_UNCHECKED);

		break;
	}
	case WM_NOTIFY:
	{
		LPNMHDR nmhdr = (LPNMHDR)lParam;

		switch (nmhdr->code)
		{
		case NM_CLICK:
		{
			switch (nmhdr->idFrom)
			{
			case IDC_SYSLINK_SMARTSCREENHELP:
				ShellExecute(hDlg, NULL, TEXT("https://learn.microsoft.com/windows/security/operating-system-security/virus-and-threat-protection/microsoft-defender-smartscreen"),
					NULL, NULL, SW_SHOW);
				break;
			case IDC_SYSLINK_MRTRUN:
			{
				TCHAR buffer[MAX_PATH] = TEXT("C:\\Windows\\System32");

				GetSystemDirectory(buffer, MAX_PATH);

				ShellExecute(hDlg, NULL, TEXT("mrt.exe"), NULL, buffer, SW_SHOW);
				break;
			}
			}
			break;
		}
		}

		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			advStruct.bNoAutoMaintenance = IsDlgButtonChecked(hDlg, IDC_CHECK_NOAUTOMAINTENANCE) == BST_CHECKED ? TRUE : FALSE;
			advStruct.bNoFileDownloadWarn = IsDlgButtonChecked(hDlg, IDC_CHECK_NODOWNLOADBLOCK) == BST_CHECKED ? TRUE : FALSE;
			advStruct.bNoWinUpdateDriverUpdate = IsDlgButtonChecked(hDlg, IDC_CHECK_NOWINUPDATE_DRIVERUPDATE) == BST_CHECKED ? TRUE : FALSE;

			EndDialog(hDlg, 0);
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hDlg, 1);
		break;
	case WM_HELP:
	{
		LPHELPINFO lphi = (LPHELPINFO)lParam;
		HH_POPUP hhp = {};
		hhp.cbStruct = sizeof(HH_POPUP);
		hhp.pt = lphi->MousePos;
		hhp.rcMargins = { -1, -1, -1, -1 };

		switch (lphi->iCtrlId)
		{
		case IDC_CHECK_NOAUTOMAINTENANCE:
			hhp.pszText = TEXT("When you are not using your PC, Windows performs automatic maintenance. It is a daily scheduled task that runs out-of-the-box. When enabled, it performs various tasks like app updates, Windows updates, security scans and many other things.If this feature produces issues for you, e.g. your PC hangs during idle time, you might want to disable it for troubleshooting purposes.");
			break;
		}

		HtmlHelp((HWND)lphi->hItemHandle, NULL, HH_DISPLAY_TEXT_POPUP, (DWORD_PTR)&hhp);
		break;
	}
	}

	return FALSE;
}

LRESULT CALLBACK BehaviorPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL verticalMaximizeEnabled = TRUE;
	static BOOL maximizeDragEnabled = TRUE;
	static BOOL snappingOnlyEnabled = TRUE;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		DWORD dwPeriodicBackup = 0;
		DWORD dwPeriodicBackupSize = sizeof(dwPeriodicBackup);
		LONG result = RegGetValue(HKLM, TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Configuration Manager"), TEXT("EnablePeriodicBackup"),
			RRF_RT_REG_DWORD, NULL, &dwPeriodicBackup, &dwPeriodicBackupSize);

		if (result == ERROR_SUCCESS)
		{
			if (dwPeriodicBackup) CheckDlgButton(hDlg, IDC_CHECK_REGBACK, BST_CHECKED);
		}
		else if (result != ERROR_FILE_NOT_FOUND)
		{
			ShowRegistryError(hDlg, REDMF_READ, TEXT("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Configuration Manager"),
				TEXT("EnablePeriodicBackup"), result);
		}

		UpDown_SetRange32(GetDlgItem(hDlg, IDC_SPIN_HUNGTIMEOUT), 50, 30000);

		DWORD dwHungTimeoutMs = 0;
		if (SystemParametersInfo(SPI_GETHUNGAPPTIMEOUT, NULL, &dwHungTimeoutMs, 0))
		{
			UpDown_SetPos32(GetDlgItem(hDlg, IDC_SPIN_HUNGTIMEOUT), dwHungTimeoutMs);
		}

		BOOL blockSendInputReq = FALSE;
		if (SystemParametersInfo(SPI_GETBLOCKSENDINPUTRESETS, NULL, &blockSendInputReq, 0))
		{
			if (!blockSendInputReq) CheckDlgButton(hDlg, IDC_CHECK_BLOCKSENDINPUTREQ, BST_CHECKED);
		}

		BOOL arrangement = FALSE;
		if (SystemParametersInfo(SPI_GETWINARRANGING, NULL, &arrangement, 0))
		{
			if (!arrangement) CheckDlgButton(hDlg, IDC_CHECK_NOAEROSNAP, BST_CHECKED);
		}

		SystemParametersInfo(SPI_GETSNAPSIZING, NULL, &verticalMaximizeEnabled, 0);
		SystemParametersInfo(SPI_GETDOCKMOVING, NULL, &snappingOnlyEnabled, 0);
		SystemParametersInfo(SPI_GETDRAGFROMMAXIMIZE, NULL, &maximizeDragEnabled, 0);

		DWORD dwDisallowShaking = 0;
		DWORD dwDisallowShakingSize = sizeof(dwDisallowShaking);
		result = RegGetValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), TEXT("DisallowShaking"),
			RRF_RT_REG_DWORD, NULL, &dwDisallowShaking, &dwDisallowShakingSize);

		if (result == ERROR_SUCCESS)
		{
			if (dwDisallowShaking) CheckDlgButton(hDlg, IDC_CHECK_NOAEROSHAKE, BST_CHECKED);
		}

		DWORD dwNoUseStoreOpenWith = 0;
		DWORD dwNoUseStoreOpenWithSize = sizeof(dwNoUseStoreOpenWith);
		result = RegGetValue(HKLM, TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), TEXT("NoUseStoreOpenWith"), RRF_RT_REG_DWORD, NULL,
			&dwNoUseStoreOpenWith, &dwNoUseStoreOpenWithSize);

		if (result == ERROR_SUCCESS)
		{
			if (dwNoUseStoreOpenWith) CheckDlgButton(hDlg, IDC_CHECK_NOAPPSTORELOOK, BST_CHECKED);
		}

		DWORD dwDisplayParams = 0;
		DWORD dwDisplayParamsSize = sizeof(dwDisplayParams);
		result = RegGetValue(HKLM, TEXT("SYSTEM\\CurrentControlSet\\Control\\CrashControl"), TEXT("DisplayParameters"), RRF_RT_REG_DWORD,
			NULL, &dwDisplayParams, &dwDisplayParamsSize);

		if (result == ERROR_SUCCESS)
		{
			if (dwDisplayParams) CheckDlgButton(hDlg, IDC_CHECK_BSODINFO, BST_CHECKED);
		}

		DWORD dwDisableEmoticon = 0;
		DWORD dwDisableEmoticonSize = sizeof(dwDisableEmoticon);
		result = RegGetValue(HKLM, TEXT("SYSTEM\\CurrentControlSet\\Control\\CrashControl"), TEXT("DisableEmoticon"), RRF_RT_REG_DWORD,
			NULL, &dwDisableEmoticon, &dwDisableEmoticonSize);

		if (result == ERROR_SUCCESS)
		{
			if (dwDisableEmoticon) CheckDlgButton(hDlg, IDC_CHECK_BSODNOSMILEY, BST_CHECKED);
		}
		
		DWORD dwKfmBlockOptIn = 0;
		DWORD dwKfmBlockOptInSize = sizeof(dwKfmBlockOptIn);
		result = RegGetValue(HKLM, TEXT("SOFTWARE\\Policies\\Microsoft\\OneDrive"), TEXT("KFMBlockOptIn"), RRF_RT_REG_DWORD, NULL, &dwKfmBlockOptIn, &dwKfmBlockOptInSize);

		if (result == ERROR_SUCCESS)
		{
			if (dwKfmBlockOptIn) CheckDlgButton(hDlg, IDC_CHECK_NOONEDRIVEBACKUPPROMPT, BST_CHECKED);
		}

		DWORD dwExpressiveInputShell = 0;
		DWORD dwExpressiveInputShellSize = sizeof(dwExpressiveInputShell);
		result = RegGetValue(HKLM, TEXT("SOFTWARE\\Microsoft\\Input\\Settings"), TEXT("EnableExpressiveInputShellHotkey"), RRF_RT_REG_DWORD, NULL,
			&dwExpressiveInputShell, &dwExpressiveInputShellSize);

		if (result == ERROR_SUCCESS)
		{
			if (dwExpressiveInputShell) CheckDlgButton(hDlg, IDC_CHECK_EMOJIPICKER_ALLLANG, BST_CHECKED);
		}

		advStruct = {};
		DWORD dwNoAutoMaintenanceSize = sizeof(advStruct.bNoAutoMaintenance);
		result = RegGetValue(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\Maintenance"), TEXT("MaintenanceDisabled"), RRF_RT_REG_DWORD, NULL,
			&advStruct.bNoAutoMaintenance, &dwNoAutoMaintenanceSize);

		DWORD dwNoFileDownloadWarnSize = sizeof(advStruct.bNoFileDownloadWarn);
		result = RegGetValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"), TEXT("SaveZoneInformation"), RRF_RT_REG_DWORD, NULL,
			&advStruct.bNoFileDownloadWarn, &dwNoFileDownloadWarnSize);

		{
			BOOL b64 = FALSE;
			BOOL b32 = FALSE;
			DWORD dwB64Size = sizeof(b64);
			DWORD dwB32Size = sizeof(b32);

			result = RegGetValue(HKLM, TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), TEXT("ExcludeWUDriversInQualityUpdate"), RRF_RT_REG_DWORD, NULL,
				&b32, &dwB32Size);

			result = RegGetValue(HKLM, TEXT("SOFTWARE\\WOW6432Node\\Policies\\Microsoft\\Windows\\WindowsUpdate"), TEXT("ExcludeWUDriversInQualityUpdate"),
				RRF_RT_REG_DWORD, NULL, &b64, &dwB64Size);

			advStruct.bNoWinUpdateDriverUpdate = (b32 || b64);
		}
		break;
	}
	case WM_UNINITMENUPOPUP:
		Button_SetDropDownState(GetDlgItem(hDlg, IDC_SPLIT_AEROSNAPMORE), FALSE);
		break;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
			case IDC_BUTTON_BHVADV:
			{
				int result = DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ADVBEHAVIOR), hDlg, AdvDlgProc);
				if (result == 0) PropSheet_Changed(GetParent(hDlg), hDlg);
				break;
			}
			case ID_AEROSNAPCMD_NOVERTICALMAXIMIZE:
			{
				verticalMaximizeEnabled = !verticalMaximizeEnabled;
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;
			}
			case ID_AEROSNAPCMD_NOMAXIMIZEDRAG:
			{
				maximizeDragEnabled = !maximizeDragEnabled;
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;
			}
			case ID_AEROSNAPCMD_NOSNAPPINGONLY:
			{
				snappingOnlyEnabled = !snappingOnlyEnabled;
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;
			}
			case IDC_SPLIT_AEROSNAPMORE:
			{
				Button_SetDropDownState(GetDlgItem(hDlg, IDC_SPLIT_AEROSNAPMORE), TRUE);
				NMBCDROPDOWN nmbc = { 0 };
				nmbc.hdr.hwndFrom = GetDlgItem(hDlg, IDC_SPLIT_AEROSNAPMORE);
				nmbc.hdr.idFrom = IDC_SPLIT_AEROSNAPMORE;
				nmbc.hdr.code = BCN_DROPDOWN;

				GetClientRect(nmbc.hdr.hwndFrom, &nmbc.rcButton);
				
				// Send the notification asynchronously
				PostMessage(hDlg, WM_NOTIFY, nmbc.hdr.idFrom, (LPARAM)&nmbc);

				break;
			}
			case IDC_EDIT_HUNGTIMEOUT:
			{
				TCHAR bufText[256] = TEXT("");
				GetDlgItemText(hDlg, IDC_EDIT_HUNGTIMEOUT, bufText, 256);

				DWORD dwTimeout = _tstoi(bufText);
				float fTimeoutSeconds = ((float)dwTimeout) / 1000;

				TCHAR bufSecondText[256] = TEXT("");
				_stprintf_s(bufSecondText, 256, TEXT("%.2f seconds"), fTimeoutSeconds);

				SetDlgItemText(hDlg, IDC_STATIC_HUNGTIMEOUTSECONDS, bufSecondText);

				if (HIWORD(wParam) != EN_SETFOCUS) PropSheet_Changed(GetParent(hDlg), hDlg);
				break;
			}
			case IDC_CHECK_BLOCKSENDINPUTREQ:
			case IDC_CHECK_NOAEROSNAP:
			case IDC_CHECK_REGBACK:
			case IDC_CHECK_NOAEROSHAKE:
			case IDC_CHECK_NOAPPSTORELOOK:
			case IDC_CHECK_BSODINFO:
			case IDC_CHECK_BSODNOSMILEY:
			case IDC_CHECK_NOONEDRIVEBACKUPPROMPT:
			case IDC_CHECK_EMOJIPICKER_ALLLANG:
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;
		}
		
		break;
	}
	case WM_NOTIFY:
	{
		LPNMHDR nmhdr = (LPNMHDR)lParam;

		switch (nmhdr->code)
		{
		case BCN_DROPDOWN:
		{
			NMBCDROPDOWN* pDropDown = (NMBCDROPDOWN*)lParam;

			switch (nmhdr->idFrom)
			{
			case IDC_SPLIT_AEROSNAPMORE:
			{
				POINT pt;
				pt.x = pDropDown->rcButton.left;
				pt.y = pDropDown->rcButton.bottom;
				ClientToScreen(pDropDown->hdr.hwndFrom, &pt);

				HMENU hSplitMenu = CreatePopupMenu();
				AppendMenu(hSplitMenu, MF_BYPOSITION | (verticalMaximizeEnabled ? MF_UNCHECKED : MF_CHECKED),
					ID_AEROSNAPCMD_NOVERTICALMAXIMIZE, TEXT("Disable vertical maximizing"));
				AppendMenu(hSplitMenu, MF_BYPOSITION | (maximizeDragEnabled ? MF_UNCHECKED : MF_CHECKED), ID_AEROSNAPCMD_NOMAXIMIZEDRAG,
					TEXT("Disable dragging of maximized windows"));
				AppendMenu(hSplitMenu, MF_BYPOSITION | (snappingOnlyEnabled ? MF_UNCHECKED : MF_CHECKED), ID_AEROSNAPCMD_NOSNAPPINGONLY, TEXT("Disable snapping only"));

				TrackPopupMenu(hSplitMenu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, hDlg, NULL);

				break;
			}
			}

			break;
		}
		case PSN_APPLY:
		{
			BOOL reboot = FALSE;

			DWORD dwPeriodicBackup = 0;
			DWORD dwPeriodicBackupSize = sizeof(dwPeriodicBackup);
			LONG result = RegGetValue(HKLM, TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Configuration Manager"), TEXT("EnablePeriodicBackup"),
				RRF_RT_REG_DWORD, NULL, &dwPeriodicBackup, &dwPeriodicBackupSize);

			DWORD dwNewPeriodicBackup = IsDlgButtonChecked(hDlg, IDC_CHECK_REGBACK) == BST_CHECKED ? 1 : 0;

			result = RegSetKeyValue(HKLM, TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Configuration Manager"), TEXT("EnablePeriodicBackup"),
				REG_DWORD, &dwNewPeriodicBackup, sizeof(dwNewPeriodicBackup));

			if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND)
			{
				ShowRegistryError(hDlg, REDMF_WRITE, TEXT("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Configuration Manager"),
					TEXT("EnablePeriodicBackup"), result);
			}

			if (dwPeriodicBackup != dwNewPeriodicBackup)
			{
				reboot = TRUE;
			}

			{
				TCHAR bufText[256] = TEXT("");
				GetDlgItemText(hDlg, IDC_EDIT_HUNGTIMEOUT, bufText, 256);

				DWORD dwTimeout = _tstoi(bufText);
				SystemParametersInfo(SPI_SETHUNGAPPTIMEOUT, dwTimeout, NULL, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}

			SystemParametersInfo(SPI_SETBLOCKSENDINPUTRESETS, IsDlgButtonChecked(hDlg, IDC_CHECK_BLOCKSENDINPUTREQ) != BST_CHECKED ? TRUE : FALSE,
				NULL, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

			SystemParametersInfo(SPI_SETWINARRANGING, IsDlgButtonChecked(hDlg, IDC_CHECK_NOAEROSNAP) != BST_CHECKED ? TRUE : FALSE,
				(PVOID)(IsDlgButtonChecked(hDlg, IDC_CHECK_NOAEROSNAP) != BST_CHECKED ? TRUE : FALSE), SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

			SystemParametersInfo(SPI_SETSNAPSIZING, verticalMaximizeEnabled, (PVOID)verticalMaximizeEnabled, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			SystemParametersInfo(SPI_SETDOCKMOVING, snappingOnlyEnabled, (PVOID)snappingOnlyEnabled, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			SystemParametersInfo(SPI_SETDRAGFROMMAXIMIZE, maximizeDragEnabled, (PVOID)maximizeDragEnabled, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			
			BOOL restartExplorer = FALSE;

			DWORD dwOldDisallowShaking = 0;
			DWORD dwOldDisallowShakingSize = sizeof(dwOldDisallowShaking);
			result = RegGetValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), TEXT("DisallowShaking"),
				RRF_RT_REG_DWORD, NULL, &dwOldDisallowShaking, &dwOldDisallowShakingSize);

			DWORD dwNewDisallowShaking = IsDlgButtonChecked(hDlg, IDC_CHECK_NOAEROSHAKE) == BST_CHECKED ? 1 : 0;

			result = RegSetKeyValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), TEXT("DisallowShaking"), REG_DWORD,
				&dwNewDisallowShaking, sizeof(dwNewDisallowShaking));

			if (dwOldDisallowShaking != dwNewDisallowShaking)
			{
				restartExplorer = TRUE;
			}

			HKEY hKey;
			result = RegCreateKeyEx(HKLM, TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);

			if (result == ERROR_SUCCESS)
			{
				DWORD dwNewNoUseStoreOpenWith = IsDlgButtonChecked(hDlg, IDC_CHECK_NOAPPSTORELOOK) == BST_CHECKED ? 1 : 0;
				result = RegSetKeyValue(hKey, NULL, TEXT("NoUseStoreOpenWith"), REG_DWORD, &dwNewNoUseStoreOpenWith, sizeof(dwNewNoUseStoreOpenWith));

				RegCloseKey(hKey);
			}

			DWORD dwDisplayParams = 0;
			DWORD dwDisplayParamsSize = sizeof(dwDisplayParams);
			result = RegGetValue(HKLM, TEXT("SYSTEM\\CurrentControlSet\\Control\\CrashControl"), TEXT("DisplayParameters"), RRF_RT_REG_DWORD,
				NULL, &dwDisplayParams, &dwDisplayParamsSize);

			DWORD dwDisableEmoticon = 0;
			DWORD dwDisableEmoticonSize = sizeof(dwDisableEmoticon);
			result = RegGetValue(HKLM, TEXT("SYSTEM\\CurrentControlSet\\Control\\CrashControl"), TEXT("DisableEmoticon"), RRF_RT_REG_DWORD,
				NULL, &dwDisableEmoticon, &dwDisableEmoticonSize);

			DWORD dwNewDisplayParams = IsDlgButtonChecked(hDlg, IDC_CHECK_BSODINFO) == BST_CHECKED ? 1 : 0;
			result = RegSetKeyValue(HKLM, TEXT("SYSTEM\\CurrentControlSet\\Control\\CrashControl"), TEXT("DisplayParameters"), REG_DWORD, &dwNewDisplayParams,
				sizeof(dwNewDisplayParams));
			
			if (dwNewDisplayParams != dwDisplayParams) reboot = TRUE;

			DWORD dwNewDisableEmoticon = IsDlgButtonChecked(hDlg, IDC_CHECK_BSODNOSMILEY) == BST_CHECKED ? 1 : 0;
			result = RegSetKeyValue(HKLM, TEXT("SYSTEM\\CurrentControlSet\\Control\\CrashControl"), TEXT("DisableEmoticon"), REG_DWORD, &dwNewDisableEmoticon,
				sizeof(dwNewDisableEmoticon));

			if (dwNewDisableEmoticon != dwDisableEmoticon) reboot = TRUE;

			DWORD dwKfmBlockOptIn = 0;
			DWORD dwKfmBlockOptInSize = sizeof(dwKfmBlockOptIn);
			result = RegGetValue(HKLM, TEXT("SOFTWARE\\Policies\\Microsoft\\OneDrive"), TEXT("KFMBlockOptIn"), RRF_RT_REG_DWORD, NULL, &dwKfmBlockOptIn, &dwKfmBlockOptInSize);

			DWORD dwNewKfmBlockOptIn = IsDlgButtonChecked(hDlg, IDC_CHECK_NOONEDRIVEBACKUPPROMPT) == BST_CHECKED ? 1 : 0;

			result = RegCreateKeyEx(HKLM, TEXT("SOFTWARE\\Policies\\Microsoft\\OneDrive"), 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);

			if (result == ERROR_SUCCESS)
			{
				result = RegSetKeyValue(hKey, NULL, TEXT("KFMBlockOptIn"), REG_DWORD, &dwNewKfmBlockOptIn, sizeof(dwNewKfmBlockOptIn));

				RegCloseKey(hKey);
			}
			
			if (dwKfmBlockOptIn != dwNewKfmBlockOptIn) reboot = TRUE;

			DWORD dwExpressiveInputShell = 0;
			DWORD dwExpressiveInputShellSize = sizeof(dwExpressiveInputShell);
			result = RegGetValue(HKLM, TEXT("SOFTWARE\\Microsoft\\Input\\Settings"), TEXT("EnableExpressiveInputShellHotkey"), RRF_RT_REG_DWORD, NULL,
				&dwExpressiveInputShell, &dwExpressiveInputShellSize);

			DWORD dwNewExpressiveInputShell = IsDlgButtonChecked(hDlg, IDC_CHECK_EMOJIPICKER_ALLLANG) == BST_CHECKED ? 1 : 0;
			result = RegSetKeyValue(HKLM, TEXT("SOFTWARE\\Microsoft\\Input\\Settings"), TEXT("EnableExpressiveInputShellHotkey"), REG_DWORD, &dwNewExpressiveInputShell,
				sizeof(dwNewExpressiveInputShell));

			if (dwExpressiveInputShell != dwNewExpressiveInputShell) reboot = TRUE;
			
			result = RegCreateKeyEx(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\Maintenance"), 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);

			if (result == ERROR_SUCCESS)
			{
				DWORD dwOldValue = 0;
				DWORD dwOldValueSize = sizeof(dwOldValue);
				RegGetValue(hKey, NULL, TEXT("MaintenanceDisabled"), RRF_RT_REG_DWORD, NULL, &dwOldValue, &dwOldValueSize);

				RegSetKeyValue(hKey, NULL, TEXT("MaintenanceDisabled"), REG_DWORD, &advStruct.bNoAutoMaintenance, sizeof(advStruct.bNoAutoMaintenance));

				if (advStruct.bNoAutoMaintenance != dwOldValue) reboot = TRUE;

				RegCloseKey(hKey);
			}

			result = RegCreateKeyEx(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"), 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
			
			if (result == ERROR_SUCCESS)
			{
				result = RegSetKeyValue(hKey, NULL, TEXT("SaveZoneInformation"), REG_DWORD, &advStruct.bNoFileDownloadWarn, sizeof(advStruct.bNoFileDownloadWarn));

				RegCloseKey(hKey);
			}

			{
				BOOL b64 = FALSE;
				BOOL b32 = FALSE;
				DWORD dwB64Size = sizeof(b64);
				DWORD dwB32Size = sizeof(b32);

				result = RegGetValue(HKLM, TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), TEXT("ExcludeWUDriversInQualityUpdate"), RRF_RT_REG_DWORD, NULL,
					&b32, &dwB32Size);

				result = RegGetValue(HKLM, TEXT("SOFTWARE\\WOW6432Node\\Policies\\Microsoft\\Windows\\WindowsUpdate"), TEXT("ExcludeWUDriversInQualityUpdate"),
					RRF_RT_REG_DWORD, NULL, &b64, &dwB64Size);

				BOOL oldNoDriverUpdate = (b32 || b64);

				result = RegCreateKeyEx(HKLM, TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);

				if (result == ERROR_SUCCESS)
				{
					RegSetKeyValue(hKey, NULL, TEXT("ExcludeWUDriversInQualityUpdate"), REG_DWORD, &advStruct.bNoWinUpdateDriverUpdate,
						sizeof(advStruct.bNoWinUpdateDriverUpdate));

					RegCloseKey(hKey);
				}

				result = RegCreateKeyEx(HKLM, TEXT("SOFTWARE\\WOW6432Node\\Policies\\Microsoft\\Windows\\WindowsUpdate"), 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);

				if (result == ERROR_SUCCESS)
				{
					RegSetKeyValue(hKey, NULL, TEXT("ExcludeWUDriversInQualityUpdate"), REG_DWORD, &advStruct.bNoWinUpdateDriverUpdate,
						sizeof(advStruct.bNoWinUpdateDriverUpdate));

					RegCloseKey(hKey);
				}

				if (oldNoDriverUpdate != advStruct.bNoWinUpdateDriverUpdate) reboot = TRUE;
			}

			if (reboot) RestartDialog(hDlg, DLGMESSAGE_REBOOT, EWX_REBOOT);
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
		case IDC_CHECK_REGBACK:
			hhp.pszText = TEXT("Starting in Windows 1803, Microsoft has turned off the automatic registry backup feature by default, so the operating system doesn't create automatic backup copies of registry hives any more.\n\nYou can re-enable this feature in order to have a working copy of the Windows registry.");
			break;
		case IDC_CHECK_NOAEROSHAKE:
			hhp.pszText = TEXT("The Aero Shake feature minimizes all other background windows when you shake the active window. This box allows you to disable or enable it.");
			break;
		case IDC_CHECK_NOAEROSNAP:
			hhp.pszText = TEXT("Disable the window snapping behavior - the resizing and repositioning of a window that happens when you drag it to the left, top or right edges of the screen.");
			break;
		case IDC_CHECK_NOAPPSTORELOOK:
			hhp.pszText = TEXT("Disable \"Look for an app in the Store\" option when an unknown file type is opened. When disabled, Windows will only show a dialog with apps installed on your PC.");
			break;
		case IDC_EDIT_HUNGTIMEOUT:
			hhp.pszText = TEXT("The number of milliseconds that an application thread can go without dispatching a message to its window procedure before the operating system considers it not responding.");
			break;
		}

		HtmlHelp((HWND)lphi->hItemHandle, NULL, HH_DISPLAY_TEXT_POPUP, (DWORD_PTR)&hhp);

		break;
	}
	}

	return FALSE;
}