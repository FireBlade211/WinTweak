#include "psettingscpl.h"
#include "resource.h"
#include <commctrl.h>
#include "macro.h"
#include <tchar.h>
#include <string>
#include <strsafe.h>
#include <vector>
#include <ios>
#include <fstream>
#include <unordered_set>
#include <algorithm>
#include <htmlhelp.h>
#include "const.h"
#include <sstream>
#include <psapi.h>
#include <tlhelp32.h>

static std::string WideToAnsi(const std::wstring& w)
{
    if (w.empty()) return std::string();
    int needed = ::WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    if (needed == 0) return std::string();
    std::string out(needed, 0);
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), &out[0], needed, nullptr, nullptr);
    return out;
}

std::vector<std::wstring> GetMSSettingsFromSystemSettingsDLL(const std::wstring& dllPath)
{
	std::vector<std::wstring> results;

	std::ifstream file(dllPath, std::ios::binary);
	if (!file) return results;

	file.seekg(0, std::ios::end);
	std::streamsize size = file.tellg();
	if (size <= 0) return results;
	file.seekg(0, std::ios::beg);

	std::vector<char> data((size_t)size);
	file.read(data.data(), size);

	std::unordered_set<std::wstring> unique;

	const wchar_t* wdata = reinterpret_cast<const wchar_t*>(data.data());
	size_t wcharCount = size / sizeof(wchar_t);

	for (size_t i = 0; i < wcharCount; ++i)
	{
		// Look for substring starting with "ms-settings:"
		if (i + 12 < wcharCount && wcsncmp(&wdata[i], L"ms-settings:", 12) == 0)
		{
			std::wstring match = L"ms-settings:";
			size_t j = i + 12;
			while (j < wcharCount &&
				((wdata[j] >= L'a' && wdata[j] <= L'z') || wdata[j] == L'-'))
			{
				match.push_back(wdata[j]);
				++j;
			}
			unique.insert(match);
		}
	}

	results.assign(unique.begin(), unique.end());
	std::sort(results.begin(), results.end());
	return results;
}


_Success_(return) bool IsProcessRunningByPath(_In_ const STD_TSTR& targetPath, _Out_opt_ DWORD* pOutPid = nullptr)
{
	bool found = false;

	// Normalize target path (case-insensitive)
	STD_TSTR normalizedTarget = targetPath;
	std::transform(normalizedTarget.begin(), normalizedTarget.end(), normalizedTarget.begin(),
		[](TCHAR c) { return (TCHAR)_totlower(c); });

	// Take snapshot of all processes
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
		return false;

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);

	if (Process32First(hSnap, &pe))
	{
		do
		{
			HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
			if (hProc)
			{
				TCHAR path[MAX_PATH];
				DWORD size = MAX_PATH;

				if (QueryFullProcessImageName(hProc, 0, path, &size))
				{
					STD_TSTR currentPath(path);
					std::transform(currentPath.begin(), currentPath.end(), currentPath.begin(),
						[](TCHAR c) { return (TCHAR)_totlower(c); });

					if (currentPath == normalizedTarget)
					{
						found = true;
						if (pOutPid)
							*pOutPid = pe.th32ProcessID;
						CloseHandle(hProc);
						break;
					}
				}
				CloseHandle(hProc);
			}
		} while (Process32Next(hSnap, &pe));
	}

	CloseHandle(hSnap);
	return found;
}

LRESULT CALLBACK SettingsCplPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL lastCheckState;
	static INT lastCheckIndex;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS), LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

		RECT rc;

		if (GetClientRect(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS), &rc))
		{
			LVCOLUMN lvc = {};
			lvc.mask = LVCF_WIDTH | LVCF_TEXT;
			lvc.cx = (rc.right - rc.left) - 18;

			TCHAR buffer[32] = TEXT("Name");
			lvc.pszText = buffer;

			ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS), 0, &lvc);
		}

		TCHAR bufWinDir[MAX_PATH] = TEXT("C:\\Windows");

		GetWindowsDirectory(bufWinDir, MAX_PATH);

		TCHAR bufSysSettingsDll[MAX_PATH];

		_stprintf_s(bufSysSettingsDll, MAX_PATH, TEXT("%s\\ImmersiveControlPanel\\SystemSettings.dll"), bufWinDir);

		std::vector<std::pair<STD_TSTR, STD_TSTR>> settingsLinkMap = {
			{ TEXT("system"), TEXT("System") },
			{ TEXT("systemcomponents"), TEXT("System Components") },
			{ TEXT("tabletmode"), TEXT("Tablet Mode") },
			{ TEXT("taskbar"), TEXT("Taskbar") },
			{ TEXT("troubleshoot"), TEXT("Troubleshoot") },
			{ TEXT("typing"), TEXT("Typing") },
			{ TEXT("usb"), TEXT("USB") },
			{ TEXT("videoplayback"), TEXT("Video Playback") },
			{ TEXT("wifi-provisioning"), TEXT("Wi-Fi Provisioning") },
			{ TEXT("windowsdefender"), TEXT("Windows Defender") },
			{ TEXT("windowsinsider"), TEXT("Windows Insider") },
			{ TEXT("windowsinsider-optin"), TEXT("Windows Insider - Opt-In") },
			{ TEXT("windowsupdate"), TEXT("Windows Update") },
			{ TEXT("windowsupdate-activehours"), TEXT("Windows Update - Active Hours") },
			{ TEXT("windowsupdate-anotheruser"), TEXT("Windows Update - Another User") },
			{ TEXT("windowsupdate-history"), TEXT("Windows Update - Update History") },
			{ TEXT("windowsupdate-optionalupdates"), TEXT("Windows Update - Optional Updates") },
			{ TEXT("windowsupdate-options"), TEXT("Windows Update - Advanced Options") },
			{ TEXT("windowsupdate-restartoptions"), TEXT("Windows Update - Restart Options") },
			{ TEXT("windowsupdate-uninstallupdates"), TEXT("Windows Update - Uninstall Updates") },
			{ TEXT("workplace"), TEXT("Workplace") },
			{ TEXT("workplace-repairtoken"), TEXT("Workplace Repair Token") },
			{ TEXT("yourinfo"), TEXT("Your Info") },
			{ TEXT("multitasking"), TEXT("Multitasking") },
			{ TEXT("multitasking-sgupdate"), TEXT("Multitasking - Snap Groups") },
			{ TEXT("network"), TEXT("Network") },
			{ TEXT("network-advancedsettings"), TEXT("Network - Advanced settings") },
			{ TEXT("network-advancedsharing"), TEXT("Network - Advanced sharing options") },
			{ TEXT("network-airplanemode"), TEXT("Network - Airplane mode") },
			{ TEXT("network-dialup"), TEXT("Network - Dial-up") },
			{ TEXT("network-mobilehotspot"), TEXT("Network - Mobile hotspot") },
			{ TEXT("network-proxy"), TEXT("Network - Proxy") },
			{ TEXT("network-status"), TEXT("Network - Status") },
			{ TEXT("network-vpn"), TEXT("Network - VPN") },
			{ TEXT("network-wifi"), TEXT("Network - Wi-Fi") },
			{ TEXT("network-wifisettings"), TEXT("Network - Known Wi-Fi networks") },
			{ TEXT("nightlight"), TEXT("Night Light") },
			{ TEXT("notifications"), TEXT("Notifications") },
			{ TEXT("optionalfeatures"), TEXT("Optional Features") },
			{ TEXT("otherusers"), TEXT("Other Users") },
			{ TEXT("pen"), TEXT("Pen and Windows Ink") },
			{ TEXT("personalization"), TEXT("Personalization") },
			{ TEXT("personalization-background"), TEXT("Personalization - Desktop Background") },
			{ TEXT("personalization-colors"), TEXT("Personalization - Colors") },
			{ TEXT("personalization-copilot"), TEXT("Personalization - Copilot") },
			{ TEXT("personalization-lighting"), TEXT("Personalization - Dynamic Lighting") },
			{ TEXT("personalization-lockscreen"), TEXT("Personalization - Lock screen") },
			{ TEXT("personalization-start"), TEXT("Personalization - Start menu") },
			{ TEXT("personalization-start-places"), TEXT("Personalization - Start menu folders") },
			{ TEXT("personalization-textinput"), TEXT("Personalization - Text input") },
			{ TEXT("personalization-textinput-copilot-hardwarekey"), TEXT("Personalization - Customize Copilot hardware key") },
			{ TEXT("powersleep"), TEXT("Power and Sleep") },
			{ TEXT("printers"), TEXT("Printers") },
			{ TEXT("privacy"), TEXT("Privacy and Security") },
			{ TEXT("privacy-accountinfo"), TEXT("Privacy - Account Info") },
			{ TEXT("privacy-activityhistory"), TEXT("Privacy - Activity History") },
			{ TEXT("privacy-appdiagnostics"), TEXT("Privacy - App Diagnostics") },
			{ TEXT("privacy-automaticfiledownloads"), TEXT("Privacy - Automatic File Downloads") },
			{ TEXT("privacy-broadfilesystemaccess"), TEXT("Privacy - Broad File System Access") },
			{ TEXT("privacy-calendar"), TEXT("Privacy - Calendar") },
			{ TEXT("privacy-callhistory"), TEXT("Privacy - Call History") },
			{ TEXT("privacy-cloudpc"), TEXT("Privacy - Cloud PC") },
			{ TEXT("privacy-contacts"), TEXT("Privacy - Contacts") },
			{ TEXT("privacy-customdevices"), TEXT("Privacy - Other Devices") },
			{ TEXT("privacy-documents"), TEXT("Privacy - Documents") },
			{ TEXT("privacy-downloadsfolder"), TEXT("Privacy - Downloads Folder") },
			{ TEXT("privacy-email"), TEXT("Privacy - E-mail") },
			{ TEXT("privacy-feedback"), TEXT("Privacy - Feedback") },
			{ TEXT("privacy-feedback-optinlevelgroup"), TEXT("Privacy - Feedback (Opt-In Level Group)") },
			{ TEXT("privacy-feedback-telemetryviewergroup"), TEXT("Privacy - Feedback (Telemetry Viewer Group)") },
			{ TEXT("privacy-general"), TEXT("Privacy - General") },
			{ TEXT("privacy-graphicscaptureprogrammatic"), TEXT("Privacy - Programmatic Graphics Capture") },
			{ TEXT("privacy-graphicscapturewithoutborder"), TEXT("Privacy - Graphics Capture Without Border") },
			{ TEXT("privacy-location"), TEXT("Privacy - Location") },
			{ TEXT("privacy-messaging"), TEXT("Privacy - Messages") },
			{ TEXT("privacy-microphone"), TEXT("Privacy - Microphone") },
			{ TEXT("privacy-musiclibrary"), TEXT("Privacy - Music Library") },
			{ TEXT("privacy-notifications"), TEXT("Privacy - Notifications") },
			{ TEXT("privacy-phonecalls"), TEXT("Privacy - Phone Calls") },
			{ TEXT("privacy-pictures"), TEXT("Privacy - Pictures") },
			{ TEXT("privacy-radios"), TEXT("Privacy - Radio Devices") },
			{ TEXT("privacy-speech"), TEXT("Privacy - Speech") },
			{ TEXT("privacy-speechtyping"), TEXT("Privacy - Speech and Typing") },
			{ TEXT("privacy-tasks"), TEXT("Privacy - Tasks") },
			{ TEXT("privacy-videos"), TEXT("Privacy - Videos") },
			{ TEXT("privacy-voiceactivation"), TEXT("Privacy - Voice Activation") },
			{ TEXT("privacy-webcam"), TEXT("Privacy - Camera") },
			{ TEXT("project"), TEXT("Project Screen") },
			{ TEXT("proximity"), TEXT("Proximity") },
			{ TEXT("quiethours"), TEXT("Quiet Hours") },
			{ TEXT("recovery"), TEXT("Recovery") },
			{ TEXT("regionformatting"), TEXT("Region and Formatting") },
			{ TEXT("regionlanguage"), TEXT("Region and Language") },
			{ TEXT("regionlanguage-adddisplaylanguage"), TEXT("Region and Language - Add a display language") },
			{ TEXT("regionlanguage-languageoptions"), TEXT("Region and Language - Language Options") },
			{ TEXT("regionlanguage-setdisplaylanguage"), TEXT("Region and Language - Set Display Language") },
			{ TEXT("remotedesktop"), TEXT("Remote Desktop (RDP)") },
			{ TEXT("savedpasskeys"), TEXT("Saved Passkeys") },
			{ TEXT("savelocations"), TEXT("Save Locations") },
			{ TEXT("screenrotation"), TEXT("Screen Rotation") },
			{ TEXT("search"), TEXT("Search") },
			{ TEXT("search-permissions"), TEXT("Search - Permissions") },
			{ TEXT("signinoptions"), TEXT("Sign-in Options") },
			{ TEXT("signinoptions-dynamiclock"), TEXT("Sign-in Options - Dynamic Lock") },
			{ TEXT("signinoptions-launchfaceenrollment"), TEXT("Sign-in Options - Windows Hello setup (face)") },
			{ TEXT("signinoptions-launchfingerprintenrollment"), TEXT("Sign-in Options - Windows Hello setup (fingerprint)") },
			{ TEXT("signinoptions-launchpinenrollment"), TEXT("Sign-in Options - Change PIN") },
			{ TEXT("signinoptions-launchsecuritykeyenrollment"), TEXT("Sign-in Options - Windows Hello setup (security key)") },
			{ TEXT("sound"), TEXT("Sound") },
			{ TEXT("sound-defaultinputproperties"), TEXT("Sound - Default Input Properties") },
			{ TEXT("sound-defaultoutputproperties"), TEXT("Sound - Default Output Properties") },
			{ TEXT("sound-devices"), TEXT("Sound - Devices") },
			{ TEXT("speech"), TEXT("Speech") },
			{ TEXT("startupapps"), TEXT("Startup") },
			{ TEXT("storagepolicies"), TEXT("Storage Policies") },
			{ TEXT("storagerecommendations"), TEXT("Storage") },
			{ TEXT("storagesense"), TEXT("Storage Sense") },
			{ TEXT("sync"), TEXT("Sync Microsoft Account") },
			{ TEXT("about"), TEXT("About") },
			{ TEXT("accounts"), TEXT("Accounts") },
			{ TEXT("activation"), TEXT("Activation") },
			{ TEXT("advanced-apps"), TEXT("Advanced App Settings") },
			{ TEXT("appsfeatures"), TEXT("Installed Apps") },
			{ TEXT("appsforwebsites"), TEXT("Apps for Websites") },
			{ TEXT("apps-volume"), TEXT("Volume Mixer") },
			{ TEXT("assignedaccess"), TEXT("Assigned Access (Kiosk Mode)") },
			{ TEXT("autoplay"), TEXT("Auto-play") },
			{ TEXT("backup"), TEXT("Backup") },
			{ TEXT("batterysaver"), TEXT("Battery Saver") },
			{ TEXT("batterysaver-settings"), TEXT("Battery Saver Settings") },
			{ TEXT("bluetooth"), TEXT("Bluetooth") },
			{ TEXT("camera"), TEXT("Devices - Camera") },
			{ TEXT("clipboard"), TEXT("Clipboard History") },
			{ TEXT("colors"), TEXT("Colors") },
			{ TEXT("connecteddevices"), TEXT("Connected Devices") },
			{ TEXT("cortana-windowssearch"), TEXT("Cortana - Windows Search") },
			{ TEXT("crossdevice"), TEXT("Nearby Sharing") },
			{ TEXT("datausage"), TEXT("Network - Data Usage") },
			{ TEXT("dateandtime"), TEXT("Date and Time") },
			{ TEXT("defaultapps"), TEXT("Default Apps") },
			{ TEXT("defaultbrowsersettings"), TEXT("Default Browser Settings") },
			{ TEXT("delivery-optimization"), TEXT("Delivery Optimization") },
			{ TEXT("delivery-optimization-activity"), TEXT("Delivery Optimization - Activity") },
			{ TEXT("delivery-optimization-advanced"), TEXT("Delivery Optimization - Advanced") },
			{ TEXT("delivery-optimization-advanced"), TEXT("Delivery Optimization - Advanced") },
			{ TEXT("developers"), TEXT("For Developers") },
			{ TEXT("devices"), TEXT("Bluetooth and Devices") },
			{ TEXT("devicestyping-hwkbtextsuggestions"), TEXT("Typing Devices - Text Suggestions") },
			{ TEXT("deviceusage"), TEXT("Device Usage") },
			{ TEXT("disksandvolumes"), TEXT("Disks and Volumes") },
			{ TEXT("display"), TEXT("Display") },
			{ TEXT("display-advanced"), TEXT("Display - Advanced") },
			{ TEXT("display-advancedgraphics"), TEXT("Display - Graphics") },
			{ TEXT("display-advancedgraphics-default"), TEXT("Display - Graphics (Default)") },
			{ TEXT("easeofaccess"), TEXT("Ease of Access") },
			{ TEXT("easeofaccess-audio"), TEXT("Ease of Access - Audio") },
			{ TEXT("easeofaccess-closedcaptioning"), TEXT("Ease of Access - Closed Captions") },
			{ TEXT("easeofaccess-colorfilter"), TEXT("Ease of Access - Color Filter") },
			{ TEXT("easeofaccess-colorfilter-adaptivecolorlink"), TEXT("Ease of Access - Adaptive Color") },
			{ TEXT("easeofaccess-colorfilter-bluelightlink"), TEXT("Ease of Access - Blue Light") },
			{ TEXT("easeofaccess-cursor"), TEXT("Ease of Access - Cursor") },
			{ TEXT("easeofaccess-display"), TEXT("Ease of Access - Display") },
			{ TEXT("easeofaccess-eyecontrol"), TEXT("Ease of Access - Eye Control") },
			{ TEXT("easeofaccess-highcontrast"), TEXT("Ease of Access - High Contrast") },
			{ TEXT("easeofaccess-keyboard"), TEXT("Ease of Access - Keyboard") },
			{ TEXT("easeofaccess-magnifier"), TEXT("Ease of Access - Magnifier") },
			{ TEXT("easeofaccess-mouse"), TEXT("Ease of Access - Mouse") },
			{ TEXT("easeofaccess-mousepointer"), TEXT("Ease of Access - Mouse Pointer") },
			{ TEXT("easeofaccess-speechrecognition"), TEXT("Ease of Access - Speech Recognition") },
			{ TEXT("easeofaccess-visualeffects"), TEXT("Ease of Access - Visual Effects") },
			{ TEXT("emailandaccounts"), TEXT("Email and Accounts") },
			{ TEXT("energyrecommendations"), TEXT("Energy Recommendations") },
			{ TEXT("family-group"), TEXT("Family Group") }
		};

		auto settingsLinks = GetMSSettingsFromSystemSettingsDLL(bufSysSettingsDll);

		std::vector<STD_TSTR> checkedPages = std::vector<STD_TSTR>();

		DWORD dwCheckedPagesSize = 0;

		if (RegGetValue(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), TEXT("SettingsPageVisibility"), RRF_RT_REG_SZ,
			NULL, NULL, &dwCheckedPagesSize) == ERROR_SUCCESS)
		{
			TCHAR* bufCheckedPages = new TCHAR[dwCheckedPagesSize + 1];

			if (RegGetValue(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), TEXT("SettingsPageVisibility"), RRF_RT_REG_SZ,
				NULL, bufCheckedPages, &dwCheckedPagesSize) == ERROR_SUCCESS)
			{
				STD_TSTR checkedPageStr = STD_TSTR(bufCheckedPages);
				STD_TSTR prefix = TEXT("hide:");

				if (checkedPageStr.rfind(prefix, 0) == 0)
				{
					checkedPageStr = checkedPageStr.substr(prefix.size());

					CheckDlgButton(hDlg, IDC_RADIO_BLOCKLIST, BST_CHECKED);
				}
				else
				{
					prefix = TEXT("showonly:");

					if (checkedPageStr.rfind(prefix, 0) == 0)
					{
						checkedPageStr = checkedPageStr.substr(prefix.size());

						CheckDlgButton(hDlg, IDC_RADIO_ALLOWLIST, BST_CHECKED);
					}
					else
					{
						CheckDlgButton(hDlg, IDC_RADIO_BLOCKLIST, BST_CHECKED);
					}
				}

				STD_TSTRSTREAM ss(checkedPageStr);
				STD_TSTR item;
				while (std::getline(ss, item, TEXT(';')))
				{
					checkedPages.push_back(item);
				}
			}
		}
		else
		{
			CheckDlgButton(hDlg, IDC_RADIO_BLOCKLIST, BST_CHECKED);
		}

		for (int i = 0; i < settingsLinks.size(); i++)
		{
			LVITEM lvi = {};
			lvi.mask = LVIF_TEXT | LVIF_PARAM;

			TCHAR bufText[256];
			STD_TSTR str = settingsLinks[i];

			STD_TSTR prefix = TEXT("ms-settings:");

			STD_TSTR trimmed = str;

			// Check if it starts with the prefix
			if (str.rfind(prefix, 0) == 0)  // rfind returns 0 if prefix matches at start
			{
				// Remove the prefix
				trimmed = str.substr(prefix.size());
			}

			BOOL found = FALSE;
			for (int im = 0; im < settingsLinkMap.size(); im++)
			{
				if (_tcscmp(settingsLinkMap[im].first.c_str(), trimmed.c_str()) == 0)
				{
					StringCchCopy(bufText, 256, settingsLinkMap[im].second.c_str());
					found = TRUE;
					break;
				}
			}

			if (!found)
			{
				StringCchCopy(bufText, 256, trimmed.c_str());
			}

			lvi.pszText = bufText;
			lvi.lParam = (LPARAM)_tcsdup(trimmed.c_str());

			INT iItem = ListView_InsertItem(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS), &lvi);

			for (int ic = 0; ic < checkedPages.size(); ic++)
			{
				if (_tcscmp(checkedPages[ic].c_str(), trimmed.c_str()) == 0)
				{
					ListView_SetCheckState(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS), iItem, TRUE);
					break;
				}
			}
		}
		break;
	}
	case WM_NOTIFY:
	{
		LPNMHDR nmhdr = (LPNMHDR)lParam;

		switch (nmhdr->code)
		{
		case LVN_ITEMCHANGING:
		{
			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

			lastCheckState = ListView_GetCheckState(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS), pnmv->iItem);
			lastCheckIndex = pnmv->iItem;

			break;
		}
		case LVN_ITEMCHANGED:
		{
			int count = ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS));

			ShowDlgItem(hDlg, IDC_SYSLINK_OPENSELECTED_SETTINGSPAGE, count > 0 ? SW_SHOW : SW_HIDE);

			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

			if (lastCheckIndex == pnmv->iItem)
			{
				if (lastCheckState != ListView_GetCheckState(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS), pnmv->iItem))
				{
					PropSheet_Changed(GetParent(hDlg), hDlg);
				}
			}

			break;
		}
		case NM_CLICK:
		{
			switch (nmhdr->idFrom)
			{
			case IDC_SYSLINK_OPEN_CLASSICUSRACCOUNTS:
			{
				TCHAR bufSysDir[MAX_PATH] = TEXT("");

				GetSystemDirectory(bufSysDir, MAX_PATH);

				ShellExecute(hDlg, NULL, TEXT("netplwiz.exe"), NULL, bufSysDir, SW_SHOW);
				break;
			}
			case IDC_SYSLINK_OPEN_CLASSICPERSONALIZE:
			{
				TCHAR bufWinDir[MAX_PATH] = TEXT("");

				GetWindowsDirectory(bufWinDir, MAX_PATH);

				ShellExecute(hDlg, NULL, TEXT("explorer.exe"), TEXT("shell:::{ED834ED6-4B5A-4bfe-8F11-A626DCB6A921}"), bufWinDir, SW_SHOW);
				break;
			}
			case IDC_SYSLINK_OPEN_CLASSICCOLORAPPEAR:
			{
				TCHAR bufWinDir[MAX_PATH] = TEXT("");

				GetWindowsDirectory(bufWinDir, MAX_PATH);

				ShellExecute(hDlg, NULL, TEXT("explorer.exe"), TEXT("shell:::{ED834ED6-4B5A-4bfe-8F11-A626DCB6A921} -Microsoft.Personalization\\pageColorization"), bufWinDir, SW_SHOW);
				break;
			}
			case IDC_SYSLINK_OPEN_CLASSICDESKBG:
			{
				TCHAR bufWinDir[MAX_PATH] = TEXT("");

				GetWindowsDirectory(bufWinDir, MAX_PATH);

				ShellExecute(hDlg, NULL, TEXT("explorer.exe"), TEXT("shell:::{ED834ED6-4B5A-4bfe-8F11-A626DCB6A921} -Microsoft.Personalization\\pageWallpaper"), bufWinDir, SW_SHOW);
				break;
			}
			case IDC_SYSLINK_OPENSELECTED_SETTINGSPAGE:
			{
				int count = ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS));

				for (int i = count - 1; i >= 0; i--)
				{
					if (ListView_GetItemState(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS), i, LVIS_SELECTED) & LVIS_SELECTED)
					{
						LVITEM lvi = {};
						lvi.iItem = i;
						lvi.mask = LVIF_PARAM;

						if (ListView_GetItem(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS), &lvi))
						{
							TCHAR buffer[256] = TEXT("");

							_stprintf_s(buffer, 256, TEXT("ms-settings:%s"), (LPWSTR)lvi.lParam);

							ShellExecute(hDlg, NULL, buffer, NULL, NULL, SW_SHOW);
						}
					}
				}

				break;
			}
			}

			break;
		}
		case PSN_KILLACTIVE:
		{
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);

			break;
		}
		case PSN_APPLY:
		{
			TCHAR sHideSettings[256] = TEXT("");

			std::vector<STD_TSTR> hideSettingsStrings = std::vector<STD_TSTR>();

			for (int i = 0; i < ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS)); i++)
			{
				if (ListView_GetCheckState(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS), i))
				{
					LVITEM lvi = {};
					lvi.iItem = i;
					lvi.mask = LVIF_PARAM;

					ListView_GetItem(GetDlgItem(hDlg, IDC_LIST_HIDESETTINGS), &lvi);

					hideSettingsStrings.push_back(STD_TSTR((LPWSTR)lvi.lParam));
				}
			}

			STD_TSTR itemStr = TEXT("");

			for (size_t i = 0; i < hideSettingsStrings.size(); ++i)
			{
				itemStr += hideSettingsStrings[i];
				if (i + 1 < hideSettingsStrings.size())
					itemStr += TEXT(";");
			}

			STD_TSTR finalStr = TEXT("");

			if (hideSettingsStrings.size() < 1)
			{
				finalStr = TEXT("");
			}
			else
			{
				if (IsDlgButtonChecked(hDlg, IDC_RADIO_ALLOWLIST) == BST_CHECKED)
				{
					finalStr = TEXT("showonly:") + itemStr;
				}
				else
				{
					finalStr = TEXT("hide:") + itemStr;
				}
			}

			HKEY hKey;
			LONG result = RegOpenKeyEx(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), 0, KEY_SET_VALUE, &hKey);

			if (result == ERROR_SUCCESS)
			{
				RegSetValueEx(
					hKey,
					TEXT("SettingsPageVisibility"),
					0,
					REG_SZ,
					reinterpret_cast<const BYTE*>(finalStr.c_str()),
					static_cast<DWORD>((finalStr.size() + 1) * sizeof(TCHAR))
				);

				RegCloseKey(hKey);

				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

				TCHAR tBufWinDir[MAX_PATH] = TEXT("C:\\Windows");

				GetWindowsDirectory(tBufWinDir, MAX_PATH);

				TCHAR tBufTargetPath[MAX_PATH] = TEXT("C:\\Windows\\ImmersiveControlPanel\\SystemSettings.exe");

				_stprintf_s(tBufTargetPath, MAX_PATH, TEXT("%s\\ImmersiveControlPanel\\SystemSettings.exe"), tBufWinDir);

				DWORD dwPid;

				if (IsProcessRunningByPath(tBufTargetPath, &dwPid))
				{
					TASKDIALOGCONFIG tdc = {};
					tdc.cbSize = sizeof(TASKDIALOGCONFIG);
					tdc.hwndParent = hDlg;
					tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_SIZE_TO_CONTENT;
					tdc.pszMainInstruction = TEXT("Restart Settings to apply changes");
					tdc.pszContent = TEXT("The Settings app needs to be restarted for the changes to apply. Change any settings now so you don't have to navigate to the page again and press Restart Now to restart the app.");
					tdc.pszMainIcon = TD_WARNING_ICON;
					tdc.pszWindowTitle = TEXT("Apply changes");
					
					TASKDIALOG_BUTTON pButtons[2] = {};

					pButtons[0].nButtonID = 101;
					pButtons[0].pszButtonText = TEXT("Restart now");

					pButtons[1].nButtonID = 102;
					pButtons[1].pszButtonText = TEXT("Restart later");

					tdc.pButtons = pButtons;
					tdc.cButtons = 2;
					
					int button;
					
					if (SUCCEEDED(TaskDialogIndirect(&tdc, &button, NULL, NULL)))
					{
						if (button == 101)
						{
							// Terminate SystemSettings.exe
							HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, dwPid);

							if (hProcess != NULL)
							{
								TerminateProcess(hProcess, 0);

								WaitForSingleObject(hProcess, 10000); // up to 10 seconds

								// Now restart it
								ShellExecute(hDlg, NULL, TEXT("ms-settings:///"), NULL, NULL, SW_SHOW);

								CloseHandle(hProcess);
							}
						}
					}
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
		case IDC_RADIO_ALLOWLIST:
		case IDC_RADIO_BLOCKLIST:
			PropSheet_Changed(GetParent(hDlg), hDlg);
			break;
		}
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
		case IDC_RADIO_ALLOWLIST:
		{
			hhp.pszText = TEXT("Only show the pages that are checked in the list, in the Settings app.");
			break;
		}
		case IDC_RADIO_BLOCKLIST:
		{
			hhp.pszText = TEXT("Hide everything except for the pages that are checked in the list, in the Settings app.");
			break;
		}
		}

		HtmlHelp((HWND)lphi->hItemHandle, NULL, HH_DISPLAY_TEXT_POPUP, (DWORD_PTR)&hhp);

		break;
	}
	}

	return FALSE;
}