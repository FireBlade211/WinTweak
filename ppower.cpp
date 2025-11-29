#include "ppower.h"
#include "resource.h"
#include <shlobj.h>
#include <tchar.h>

INT_PTR CALLBACK PowerPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_SHUTDOWNDLGTEST:
		{
            // Initialize COM
            HRESULT hr = CoInitialize(NULL);
            if (FAILED(hr))
                break;

            IShellDispatch2* pShell = nullptr;
            CLSID clsidShell;
            hr = CLSIDFromProgID(TEXT("Shell.Application"), &clsidShell);
            if (SUCCEEDED(hr))
            {
                hr = CoCreateInstance(clsidShell, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                    IID_PPV_ARGS(&pShell));
                if (SUCCEEDED(hr))
                {
                    hr = pShell->ShutdownWindows();
                    pShell->Release();
                }
            }

            CoUninitialize();

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
            switch (nmhdr->idFrom)
            {
            case IDC_SYSLINK_ADVPOWERCFG:
            {
                TCHAR bufSysDir[MAX_PATH] = TEXT("C:\\Windows\\System32");
                GetSystemDirectory(bufSysDir, MAX_PATH);

                TCHAR bufRunDll32[MAX_PATH] = TEXT("C:\\Windows\\System32\\rundll32.exe");
                _stprintf_s(bufRunDll32, MAX_PATH, TEXT("%s\\rundll32.exe"), bufSysDir);
                    
                ShellExecute(hDlg, NULL, bufRunDll32, TEXT("Shell32.dll,Control_RunDLL powercfg.cpl,,3"), bufSysDir, SW_SHOW);
                break;
            }
            }

            break;
        }
        }

        break;
    }
	}

	return FALSE;
}
