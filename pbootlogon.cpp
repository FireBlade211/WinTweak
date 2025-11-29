#include "pbootlogon.h"
#include <wbemidl.h>
#include <comdef.h>
#include "resource.h"
#include <string>
#include "macro.h"
#include <tchar.h>
#include <commctrl.h>
#include "const.h"
#include <shlobj.h>

STD_TSTR RunCommandHidden(LPCTSTR cmdLine)
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = NULL, hWrite = NULL;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        return TEXT("");
    
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi{};
    TCHAR cmd[256];
    _tcscpy_s(cmd, cmdLine);

    if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        CloseHandle(hWrite);
        CloseHandle(hRead);
        return L"";
    }

    CloseHandle(hWrite);

    std::string output;
    char buffer[256];
    DWORD bytesRead;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = 0;
        output += buffer;
    }

    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    // Convert to wide string
    int len = MultiByteToWideChar(CP_OEMCP, 0, output.c_str(), -1, NULL, 0);
    STD_TSTR wout(len, 0);
    MultiByteToWideChar(CP_OEMCP, 0, output.c_str(), -1, &wout[0], len);
    return wout;
}


LRESULT CALLBACK LogonMessageDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        Edit_SetCueBannerText(GetDlgItem(hDlg, IDC_EDIT_TITLE), TEXT("Caption..."));
        Edit_SetCueBannerText(GetDlgItem(hDlg, IDC_EDIT_TEXT), TEXT("Text..."));

        TCHAR data[512] = TEXT("");
        DWORD dwDataSize = 512;
        
        LONG result = RegGetValue(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), TEXT("legalnoticecaption"), RRF_RT_REG_SZ,
            NULL, data, &dwDataSize);

        if (result == ERROR_SUCCESS)
        {
            SetDlgItemText(hDlg, IDC_EDIT_TITLE, data);
        }
        else if (result == ERROR_MORE_DATA)
        {
            TCHAR* buffer = new TCHAR[dwDataSize];

            result = RegGetValue(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), TEXT("legalnoticecaption"), RRF_RT_REG_SZ,
                NULL, buffer, &dwDataSize);

            if (result == ERROR_SUCCESS)
            {
                SetDlgItemText(hDlg, IDC_EDIT_TITLE, buffer);
            }

            delete[] buffer;
        }

        TCHAR dataText[512] = TEXT("");
        DWORD dwDataTextSize = 512;

        result = RegGetValue(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), TEXT("legalnoticetext"), RRF_RT_REG_SZ,
            NULL, dataText, &dwDataTextSize);

        if (result == ERROR_SUCCESS)
        {
            SetDlgItemText(hDlg, IDC_EDIT_TEXT, dataText);
        }
        else if (result == ERROR_MORE_DATA)
        {
            TCHAR* buffer = new TCHAR[dwDataTextSize];

            LONG result = RegGetValue(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), TEXT("legalnoticetext"), RRF_RT_REG_SZ,
                NULL, buffer, &dwDataTextSize);

            if (result == ERROR_SUCCESS)
            {
                SetDlgItemText(hDlg, IDC_EDIT_TEXT, buffer);
            }

            delete[] buffer;
        }

        break;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        case IDOK:
            HKEY hKey;
            LONG result = RegOpenKeyEx(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), 0, KEY_SET_VALUE, &hKey);

            if (result == ERROR_SUCCESS)
            {
                INT length = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_TITLE)) + 1;

                TCHAR* buffer = new TCHAR[length];

                GetWindowText(GetDlgItem(hDlg, IDC_EDIT_TITLE), buffer, length);

                RegSetValueEx(hKey, TEXT("legalnoticecaption"), 0, REG_SZ, (BYTE*)buffer, (lstrlen(buffer) + 1) * sizeof(TCHAR));

                delete[] buffer;

                length = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_TEXT)) + 1;

                buffer = new TCHAR[length];

                GetWindowText(GetDlgItem(hDlg, IDC_EDIT_TEXT), buffer, length);

                RegSetValueEx(hKey, TEXT("legalnoticetext"), 0, REG_SZ, (BYTE*)buffer, (lstrlen(buffer) + 1) * sizeof(TCHAR));

                delete[] buffer;

                RegCloseKey(hKey);

                RestartDialog(hDlg, TEXT("You need to restart your computer to apply changes. Do you want to restart now?"), EWX_REBOOT);
            }

            EndDialog(hDlg, IDOK);
            break;
        }

        break;
    }
    case WM_CLOSE:
    {
        EndDialog(hDlg, 0);

        break;
    }
    }

    return FALSE;
}

LRESULT CALLBACK BootLogonPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
        // parse bcdedit output
        STD_TSTR out = RunCommandHidden(TEXT("bcdedit.exe"));

        // BootMenuPolicy (works as before)
        if (out.find(TEXT("bootmenupolicy")) != STD_TSTR::npos)
        {
            if (out.find(TEXT("Legacy")) != STD_TSTR::npos)
                CheckDlgButton(hDlg, IDC_CHECK_BOOTMENUPOLICY, BST_CHECKED);
            else
                CheckDlgButton(hDlg, IDC_CHECK_BOOTMENUPOLICY, BST_UNCHECKED);
        }

        // OptionsEdit
        size_t pos = out.find(TEXT("optionsedit"));
        if (pos != STD_TSTR::npos)
        {
            // Find end of line
            size_t endline = out.find(TEXT("\n"), pos);
            if (endline == STD_TSTR::npos) endline = out.length();

            // Extract just this line
            STD_TSTR line = out.substr(pos, endline - pos);

            if (line.find(TEXT("Yes")) != STD_TSTR::npos)
                CheckDlgButton(hDlg, IDC_CHECK_BCDOPTIONSEDIT, BST_CHECKED);
            else
                CheckDlgButton(hDlg, IDC_CHECK_BCDOPTIONSEDIT, BST_UNCHECKED);
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
            switch (nmhdr->idFrom)
            {
            case IDC_SYSLINK_CUSTOMSIGNINMSG:
                DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_SIGNINMSGDLG), hDlg, LogonMessageDlgProc);
                break;
            case IDC_SYSLINK_NETWIZPL:
            {
                TCHAR bufSysDir[MAX_PATH] = TEXT("");

                GetSystemDirectory(bufSysDir, MAX_PATH);

                ShellExecute(hDlg, NULL, TEXT("netplwiz.exe"), NULL, bufSysDir, SW_SHOW);
                break;
            }
            }
        }
        }
    }
	}

	return FALSE;
}