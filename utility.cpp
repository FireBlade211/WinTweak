///
///	utility.cpp:
///		Defines utility functions.
/// 

#include "utility.h"
#include <windows.h>
#include "macro.h"
#include <string>
#include <tchar.h>
#include <regex>
#include <aclapi.h>
#include <commctrl.h>
#include <tlhelp32.h>
#include "const.h"
#include <strsafe.h>

BOOL IsWindowsVersionOrGreaterEx(INT build, INT major, INT minor)
{
	_OSVERSIONINFOEX ver = {};
	ver.dwOSVersionInfoSize = sizeof(_OSVERSIONINFOEX);
	ver.dwBuildNumber = build;
	ver.dwMajorVersion = major;
	ver.dwMinorVersion = minor;

	ULONG comp = 0;

	VER_SET_CONDITION(comp, VER_BUILDNUMBER, VER_GREATER_EQUAL);
	VER_SET_CONDITION(comp, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(comp, VER_MINORVERSION, VER_GREATER_EQUAL);

	return VerifyVersionInfo(&ver, VER_BUILDNUMBER | VER_MAJORVERSION | VER_MINORVERSION, comp);
}


int _clamp(int i, int max, int min)
{
	if (i > max) return max;
	if (i < min) return min;

	return i;
}

long _clamp_l(long l, long max, long min)
{
	if (l > max) return max;
	if (l < min) return min;

	return l;
}

BOOL EnablePrivilege(LPCTSTR privName)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return FALSE;
    }

    if (!LookupPrivilegeValue(NULL, privName, &luid))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL success = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(hToken);

    return (success && GetLastError() == ERROR_SUCCESS);
}

int RestartExplorerDialog(HWND hwndOwner)
{
    TASKDIALOGCONFIG tdc = {};
    tdc.cbSize = sizeof(TASKDIALOGCONFIG);
    
    TASKDIALOG_BUTTON buttons[2] = {};
    buttons[0].pszButtonText = TEXT("Restart Explorer now");
    buttons[0].nButtonID = 1000;
    
    buttons[1].pszButtonText = TEXT("Restart Explorer later");
    buttons[1].nButtonID = 1001;

    tdc.cButtons = ARRAYSIZE(buttons);
    tdc.pButtons = buttons;

    tdc.dwFlags = TDF_SIZE_TO_CONTENT;
    tdc.pszWindowTitle = TEXT("Restart Explorer");
    tdc.pszMainInstruction = TEXT("You need to restart Explorer to apply these changes");
    tdc.pszContent = TEXT("These changes will not apply until you restart Explorer.");
    tdc.pszMainIcon = TD_WARNING_ICON;
    tdc.hwndParent = hwndOwner;
    
    int button = 0;
    if (SUCCEEDED(TaskDialogIndirect(&tdc, &button, NULL, NULL)))
    {
        if (button == 1000)
        {
            TCHAR bufSysDir[MAX_PATH] = TEXT("C:\\Windows\\System32");
            GetSystemDirectory(bufSysDir, MAX_PATH);

            ShellExecute(hwndOwner, NULL, TEXT("cmd.exe"), TEXT("/c \"taskkill /f /im explorer.exe && explorer\""), bufSysDir, SW_HIDE);
        }

        return button;
    }

    return 0;
}

DWORD GetPIDByName(const STD_TSTR process_name)
{
	HANDLE snapshot_handle;
	if ((snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE)
	{
		return -1;
	}

	DWORD pid = -1;
	PROCESSENTRY32 pe;
	ZeroMemory(&pe, sizeof(PROCESSENTRY32));
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(snapshot_handle, &pe))
	{
		while (Process32Next(snapshot_handle, &pe))
		{
			if (_tcscmp(pe.szExeFile, process_name.c_str()) == 0)
			{
				pid = pe.th32ProcessID;
				break;
			}
		}
	}
	else
	{
		CloseHandle(snapshot_handle);
		return -1;
	}

	CloseHandle(snapshot_handle);
	return pid;
}

BOOL ImpersonateSystem()
{
	const auto system_pid = GetPIDByName(TEXT("winlogon.exe"));
	if (system_pid == -1) return FALSE;

	HANDLE process_handle;
	if ((process_handle = OpenProcess(
		PROCESS_QUERY_LIMITED_INFORMATION,
		FALSE,
		system_pid)) == nullptr)
	{
		return FALSE;
	}

	HANDLE token_handle;
	if (!OpenProcessToken(
		process_handle,
		TOKEN_DUPLICATE,
		&token_handle))
	{
		CloseHandle(process_handle);
		return FALSE;
	}
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
		return FALSE;
	}
	CloseHandle(token_handle);

	if (!ImpersonateLoggedOnUser(dup_token_handle))
	{
		CloseHandle(dup_token_handle);
		return FALSE;
	}
	CloseHandle(dup_token_handle);

	return TRUE;
}

int StartTIService()
{
	SC_HANDLE sc_manager_handle;
	if ((sc_manager_handle = OpenSCManager(
		nullptr,
		SERVICES_ACTIVE_DATABASE,
		SC_MANAGER_CONNECT)) == nullptr)
	{
		return -1;
	}

	SC_HANDLE service_handle;
	if ((service_handle = OpenService(
		sc_manager_handle,
		TEXT("TrustedInstaller"),
		SERVICE_QUERY_STATUS | SERVICE_START)) == nullptr)
	{
		CloseServiceHandle(sc_manager_handle);
		return -1;
	}
	CloseServiceHandle(sc_manager_handle);

	SERVICE_STATUS_PROCESS status_buffer;
	DWORD bytes_needed;
	while (QueryServiceStatusEx(
		service_handle,
		SC_STATUS_PROCESS_INFO,
		reinterpret_cast<LPBYTE>(&status_buffer),
		sizeof(SERVICE_STATUS_PROCESS),
		&bytes_needed))
	{
		if (status_buffer.dwCurrentState == SERVICE_STOPPED)
		{
			if (!StartService(service_handle, 0, nullptr))
			{
				CloseServiceHandle(service_handle);
				return -1;
			}
		}
		if (status_buffer.dwCurrentState == SERVICE_START_PENDING ||
			status_buffer.dwCurrentState == SERVICE_STOP_PENDING)
		{
			Sleep(status_buffer.dwWaitHint);
			continue;
		}
		if (status_buffer.dwCurrentState == SERVICE_RUNNING)
		{
			CloseServiceHandle(service_handle);
			return status_buffer.dwProcessId;
		}
	}
	CloseServiceHandle(service_handle);
	return -1;
}

void ShowTIExecError(STD_TSTR msg, DWORD dwErr, HWND hDlg)
{
	TCHAR bufMsg[256] = TEXT("");
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, 0, bufMsg, 256, NULL);

	TASKDIALOGCONFIG tdc = {};
	tdc.hwndParent = hDlg;
	tdc.cbSize = sizeof(TASKDIALOGCONFIG);
	tdc.dwFlags = TDF_SIZE_TO_CONTENT | TDF_ALLOW_DIALOG_CANCELLATION;
	tdc.pszWindowTitle = TEXT("TrustedInstaller execution error");
	tdc.pszMainInstruction = TEXT("An error occured");
	tdc.pszContent = msg.c_str();

	TCHAR bufExpandedInfo[256] = TEXT("");
	_stprintf_s(bufExpandedInfo, 256, TEXT("%d: %s"), dwErr, bufMsg);

	tdc.pszExpandedInformation = bufExpandedInfo;
	tdc.pszMainIcon = MAKEINTRESOURCE(TD_SHIELDERROR_ICON);

	TaskDialogIndirect(&tdc, NULL, NULL, NULL);
}

void ShowRegistryError(HWND hwndOwner, REDMF mode, LPCTSTR lpKeyName, LPCTSTR lpValName, DWORD dwResult)
{
	TASKDIALOGCONFIG tdc = {};
	tdc.pszWindowTitle = TEXT("Registry error");
	tdc.cbSize = sizeof(TASKDIALOGCONFIG);
	tdc.hwndParent = hwndOwner;
	tdc.pszMainInstruction = mode == REDMF_READ ? TEXT("Registry read error") : TEXT("Registry write error");
	tdc.pszContent = mode == REDMF_READ ? TEXT("An error occured reading the registry.") : TEXT("An error occured writing to the registry.");

	TCHAR bufMsg[256] = TEXT("Unable to find a detailed error message.");
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwResult, 0, bufMsg, 256, NULL);

	TCHAR bufExpanded[256] = TEXT("");
	_stprintf_s(bufExpanded, 256, TEXT("Code: 0x%X (%d)\nMessage: %s"), dwResult, dwResult, bufMsg);

	tdc.pszExpandedInformation = bufExpanded;

	TCHAR bufFootnote[512] = TEXT("");
	// href is required, task dialogs don't let us use ids
	_stprintf_s(bufFootnote, 512, TEXT("Key: <a href=\"openkey\">%s</a>, value: %s"), lpKeyName, lpValName);

	tdc.pszFooter = bufFootnote;
	tdc.pszMainIcon = TD_ERROR_ICON;
	tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_ENABLE_HYPERLINKS | TDF_SIZE_TO_CONTENT;
	tdc.lpCallbackData = (LONG_PTR)_tcsdup(lpKeyName);
	tdc.pfCallback = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData) -> HRESULT CALLBACK {
		switch (uMsg)
		{
		case TDN_HYPERLINK_CLICKED:
		{
			LPWSTR lpHref = (LPWSTR)lParam;

			if (_tcscmp(lpHref, TEXT("openkey")) == 0)
			{
				TCHAR buffer[512] = TEXT("");
				StringCchCopy(buffer, 512, (LPCTSTR)lpRefData);

				RegSetKeyValue(HKCU, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit"), TEXT("LastKey"), REG_SZ, buffer,
					(lstrlen(buffer) + 1) * sizeof(TCHAR));

				TCHAR bufSysDir[MAX_PATH] = TEXT("C:\\Windows\\System32");
				GetSystemDirectory(bufSysDir, MAX_PATH);

				// we could launch regedit directly but launch regedt32 for backward compatibility
				// since it redirects to regedit anyway
				TCHAR bufRegeditExe[MAX_PATH] = TEXT("C:\\Windows\\System32\\regedt32.exe");
				_stprintf_s(bufRegeditExe, MAX_PATH, TEXT("%s\\regedt32.exe"), bufSysDir);

				ShellExecute(hwnd, NULL, bufRegeditExe, TEXT("/m"), bufSysDir, SW_SHOW);
				
				return S_FALSE;
			}

			break;
		}
		}

		return S_OK;
	};

	TaskDialogIndirect(&tdc, NULL, NULL, NULL);
}