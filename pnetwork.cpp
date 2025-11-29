#include "pnetwork.h"
#include "resource.h"
#include <tchar.h>
#include "utility.h"
#include <commctrl.h>
#include <shlobj.h>
#include "const.h"
#include "macro.h"
#include <sstream>
#include <map>
#include <strsafe.h>
#include <vector>
#include <netfw.h>
#include <shobjidl.h>
#include <comdef.h>
#include <atlbase.h>

typedef struct FIREWALLRULEINFO
{
	STD_TSTR name;
	STD_TSTR desc;
	STD_TSTR appName;
	NET_FW_RULE_DIRECTION dir;
	NET_FW_ACTION action;
};

typedef FIREWALLRULEINFO* LPFIREWALLRULEINFO;

INT_PTR CALLBACK NetworkPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		SendDlgItemMessage(hDlg, IDC_EDIT_RDPPORT, EM_SETLIMITTEXT, 5, 0);

		TCHAR bufSysDir[MAX_PATH] = TEXT("C:\\Windows\\System32");

		GetSystemDirectory(bufSysDir, MAX_PATH);

		TCHAR bufRdpApp[MAX_PATH] = TEXT("C:\\Windows\\System32\\mstsc.exe");
		_stprintf_s(bufRdpApp, MAX_PATH, TEXT("%s\\mstsc.exe"), bufSysDir);

		// i just realized msvc requires win7+ 😭
		// all my earlier compatibility efforts went to waste
		HICON hRdpIcon;

		if (SUCCEEDED(SHDefExtractIcon(bufRdpApp, 0, 0, &hRdpIcon, NULL, MAKELONG(40, 0))))
		{
			SendDlgItemMessage(hDlg, IDC_STATIC_IMGRDP, STM_SETICON, (WPARAM)hRdpIcon, 0);
		}

		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_EDIT_RDPPORT:
		{
			switch (HIWORD(wParam))
			{
			case EN_KILLFOCUS:
			{
				TCHAR tempBuffer[16] = TEXT("");

				GetDlgItemText(hDlg, IDC_EDIT_RDPPORT, tempBuffer, 16);

				int num = _tstoi(tempBuffer);

				TCHAR textBuf[16] = TEXT("0");
				_itot_s(_clamp(num, 65535, 0), textBuf, 16, 10);

				SetDlgItemText(hDlg, IDC_EDIT_RDPPORT, textBuf);

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
			switch (nmhdr->idFrom)
			{
			case IDC_SYSLINK_LAUNCHRDP:
			{
				TCHAR bufSysDir[MAX_PATH] = TEXT("C:\\Windows\\System32");

				GetSystemDirectory(bufSysDir, MAX_PATH);

				TCHAR bufRdpApp[MAX_PATH] = TEXT("C:\\Windows\\System32\\mstsc.exe");
				_stprintf_s(bufRdpApp, MAX_PATH, TEXT("%s\\mstsc.exe"), bufSysDir);

				ShellExecute(hDlg, NULL, bufRdpApp, NULL, bufSysDir, SW_SHOW);

				break;
			}
			case IDC_SYSLINK_EXPORTFIREWALLRULES:
			{
				// used to work until I made it do more specific filtering (action and dir)
				// i have no fucking idea what is happening, im going insane, kill me

				//OPENFILENAME ofn = {};
				//TCHAR bufFile[MAX_PATH] = TEXT("");

				//ofn.lpstrFile = bufFile;
				//ofn.lStructSize = sizeof(OPENFILENAME);
				//ofn.hwndOwner = hDlg;
				//ofn.lpstrFilter = TEXT("Registry files (*.reg)\0*.reg\0Legacy Windows 9x/NT 4 registry file (*.reg)\0*.reg\0\0");
				//ofn.Flags = OFN_READONLY | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN | OFN_EXPLORER
				//	| OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_ENABLETEMPLATE;

				//ofn.hInstance = GetModuleHandle(nullptr);
				//ofn.lpstrTitle = TEXT("Export firewall rules");
				//ofn.nMaxFile = MAX_PATH;
				//ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FILEDLG_LISTVIEWEXT);

				//std::vector<FIREWALLRULEINFO> rules = {};
				//ofn.lCustData = (LPARAM)&rules;

				//ofn.lpfnHook = [](HWND fHDlg, UINT fUMsg, WPARAM fWParam, LPARAM fLParam) -> UINT_PTR CALLBACK {
				//	switch (fUMsg)
				//	{
				//	case WM_INITDIALOG:
				//	{
				//		SetDlgItemText(fHDlg, IDC_STATIC_FILEDLG_LVPROMPT, TEXT("Choose the rules to export:"));

				//		ListView_SetExtendedListViewStyle(GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV), LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

				//		LVCOLUMN lvc = {};
				//		lvc.mask = LVCF_TEXT | LVCF_WIDTH;
				//		
				//		TCHAR bufMainColText[32] = TEXT("Name");
				//		lvc.pszText = bufMainColText;
				//		lvc.cchTextMax = 32;
				//		lvc.cx = 120;
				//		
				//		ListView_InsertColumn(GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV), 0, &lvc);

				//		TCHAR bufProtoColText[32] = TEXT("Protocol");
				//		lvc.pszText = bufProtoColText;
				//		
				//		ListView_InsertColumn(GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV), 1, &lvc);

				//		TCHAR bufStateColText[32] = TEXT("State");
				//		lvc.pszText = bufStateColText;
				//		
				//		ListView_InsertColumn(GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV), 2, &lvc);

				//		TCHAR bufDescColText[32] = TEXT("Description");
				//		lvc.pszText = bufDescColText;
				//		
				//		ListView_InsertColumn(GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV), 3, &lvc);

				//		TCHAR bufAppColText[32] = TEXT("App");
				//		lvc.pszText = bufAppColText;
				//		
				//		ListView_InsertColumn(GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV), 4, &lvc);

				//		CComPtr<INetFwPolicy2> pPolicy;
				//		HRESULT hr = CoInitialize(0);
				//		if (SUCCEEDED(hr))
				//		{
				//			hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
				//				__uuidof(INetFwPolicy2), (void**)&pPolicy);
				//			if (FAILED(hr))
				//			{
				//				MessageBox(fHDlg, TEXT("Failed to create INetFwPolicy2 instance."), NULL, MB_ICONERROR);
				//				CoUninitialize();
				//				break;
				//			}

				//			CComPtr<INetFwRules> pRules;
				//			hr = pPolicy->get_Rules(&pRules);
				//			if (FAILED(hr))
				//			{
				//				MessageBox(fHDlg, TEXT("Failed to get Rules collection."), NULL, MB_ICONERROR);
				//				pPolicy.Release();
				//				CoUninitialize();
				//				break;
				//			}

				//			CComPtr<IUnknown> pEnum;
				//			hr = pRules->get__NewEnum(&pEnum);
				//			if (SUCCEEDED(hr))
				//			{
				//				CComPtr<IEnumVARIANT> pVariantEnum;
				//				pEnum->QueryInterface(__uuidof(IEnumVARIANT), (void**)&pVariantEnum);

				//				VARIANT var;
				//				VariantInit(&var);

				//				while (pVariantEnum->Next(1, &var, nullptr) == S_OK)
				//				{
				//					if (var.vt == VT_DISPATCH && var.pdispVal)
				//					{
				//						CComPtr<INetFwRule> pRule;
				//						var.pdispVal->QueryInterface(__uuidof(INetFwRule), (void**)&pRule);

				//						if (pRule)
				//						{
				//							BSTR bstrName = nullptr;
				//							BSTR bstrDesc = nullptr;
				//							BSTR bstrApp = nullptr;
				//							NET_FW_ACTION action = {};
				//							NET_FW_RULE_DIRECTION dir = {};
				//							long protocol = 0;
				//							VARIANT_BOOL enabled;

				//							pRule->get_Name(&bstrName);
				//							pRule->get_Description(&bstrDesc);
				//							pRule->get_ApplicationName(&bstrApp);
				//							pRule->get_Protocol(&protocol);
				//							pRule->get_Enabled(&enabled);
				//							pRule->get_Action(&action);
				//							pRule->get_Direction(&dir);

				//							LPFIREWALLRULEINFO pFwri = new FIREWALLRULEINFO();
				//							if (bstrName)
				//								pFwri->name = STD_TSTR(bstrName);

				//							if (bstrDesc)
				//								pFwri->desc = STD_TSTR(bstrDesc);

				//							if (bstrApp)
				//								pFwri->appName = STD_TSTR(bstrApp);

				//							pFwri->action = action;
				//							pFwri->dir = dir;

				//							// The name may be like "@FirewallAPI.dll,-28522" — resolve it with SHLoadIndirectString
				//							std::wstring nameResolved;
				//							if (bstrName)
				//							{
				//								nameResolved = bstrName;
				//								wchar_t buf[512];
				//								if (SUCCEEDED(SHLoadIndirectString(bstrName, buf, ARRAYSIZE(buf), nullptr)))
				//									nameResolved = buf;
				//							}

				//							// Add to ListView
				//							LVITEM lvi = {};
				//							lvi.mask = LVIF_TEXT | LVIF_PARAM;
				//							lvi.pszText = (LPTSTR)nameResolved.c_str();
				//							lvi.lParam = (LPARAM)pFwri;
				//							int iItem = ListView_InsertItem(GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV), &lvi);
				//							
				//							// Add other columns
				//							TCHAR protoBuf[16];
				//							_stprintf_s(protoBuf, 16, TEXT("%ld"), protocol);
				//							ListView_SetItemText(GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV), iItem, 1, protoBuf);

				//							TCHAR bufEnabled[16];
				//							StringCchCopy(bufEnabled, 16, (enabled == VARIANT_TRUE) ? TEXT("Enabled") : TEXT("Disabled"));
				//							 
				//							ListView_SetItemText(GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV), iItem, 2, bufEnabled);

				//							if (bstrDesc)
				//								ListView_SetItemText(GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV), iItem, 3, bstrDesc);

				//							if (bstrApp)
				//								ListView_SetItemText(GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV), iItem, 4, bstrApp);

				//							SysFreeString(bstrName);
				//							SysFreeString(bstrDesc);
				//							SysFreeString(bstrApp);
				//						}
				//					}
				//					VariantClear(&var);
				//				}
				//			}

				//			if (pPolicy)
				//				pPolicy.Release();

				//			CoUninitialize();
				//		}


				//		break;
				//	}
				//	case WM_SIZE:
				//	{
				//		HWND hLv = GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV);

				//		RECT rc;
				//		GetWindowRect(hLv, &rc);

				//		SetWindowPos(
				//			hLv,
				//			NULL,
				//			0, 0,                // ignored because of SWP_NOMOVE
				//			rc.right - rc.left,  // same width
				//			HIWORD(fLParam) - 32,
				//			SWP_NOZORDER | SWP_NOMOVE
				//		);

				//		break;
				//	}
				//	case WM_NOTIFY:
				//	{
				//		LPNMHDR nmhdr = (LPNMHDR)fLParam;

				//		switch (nmhdr->code)
				//		{
				//		case CDN_FILEOK:
				//		{
				//			LPOFNOTIFY ofnotif = (LPOFNOTIFY)fLParam;
				//			
				//			DWORD dwCheckedCount = 0;
				//			HWND hLv = GetDlgItem(fHDlg, IDC_LIST_FILEDLGLV);

				//			std::vector<FIREWALLRULEINFO>* pRules = (std::vector<FIREWALLRULEINFO>*)ofnotif->lpOFN->lCustData;

				//			for (int i = 0; i < ListView_GetItemCount(hLv); i++)
				//			{
				//				if (ListView_GetCheckState(hLv, i))
				//				{
				//					dwCheckedCount++;
				//					
				//					LVITEM lvi = {};
				//					lvi.mask = LVIF_PARAM;
				//					lvi.iItem = i;

				//					ListView_GetItem(hLv, &lvi);

				//					LPFIREWALLRULEINFO pInfo = (LPFIREWALLRULEINFO)lvi.lParam;
				//					pRules->push_back(*pInfo);

				//					delete pInfo;
				//				}
				//			}

				//			if (dwCheckedCount == 0)
				//			{
				//				MessageBox(fHDlg, TEXT("Please check at least 1 rule to export."), NULL, MB_ICONWARNING);
				//				SetWindowLongPtr(fHDlg, DWLP_MSGRESULT, TRUE);
				//				return TRUE;
				//			}

				//			SetWindowLongPtr(fHDlg, DWLP_MSGRESULT, FALSE);
				//			return FALSE;
				//		}
				//		}

				//		break;
				//	}
				//	}

				//	return FALSE;
				//};
				//
				//if (GetSaveFileName(&ofn))
				//{
				//	HKEY hKey;
				//	LONG result = RegOpenKeyEx(HKLM, TEXT("SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\FirewallRules"), 0, KEY_READ, &hKey);

				//	if (result == ERROR_SUCCESS)
				//	{
				//		// we need to use regedit.exe to export to get a regedit-compatible format that we can also modify because it's text not binary
				//		TCHAR bufWinDir[MAX_PATH];
				//		GetWindowsDirectory(bufWinDir, MAX_PATH);

				//		std::tstring regEditExeParams = ofn.nFilterIndex == 1 ? TEXT("/e \"") : TEXT("/a \"");
				//		regEditExeParams += bufFile;
				//		regEditExeParams += TEXT("\" HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\FirewallRules");

				//		SHELLEXECUTEINFO sei = {};
				//		sei.cbSize = sizeof(SHELLEXECUTEINFO);
				//		std::tstring cmd = TEXT("regedit.exe ");
				//		cmd += regEditExeParams;

				//		STARTUPINFO si = { sizeof(si) };
				//		PROCESS_INFORMATION pi = {};

				//		if (CreateProcess(
				//			NULL,               // application name (use cmdline instead)
				//			cmd.data(),         // full command line
				//			NULL, NULL,
				//			FALSE,
				//			0,   // no console window
				//			NULL,
				//			bufWinDir,          // working directory
				//			&si,
				//			&pi))
				//		{
				//			WaitForSingleObject(pi.hProcess, 5000);
				//			CloseHandle(pi.hProcess);
				//			CloseHandle(pi.hThread);

				//			for (int i = 0; i < rules.size(); i++)
				//			{
				//				FIREWALLRULEINFO fwri = rules[i];

				//				DWORD dwMaxNameLen;
				//				DWORD dwMaxValueLen;
				//				result = RegQueryInfoKey(hKey, NULL, NULL, 0, NULL, NULL, NULL, NULL, &dwMaxNameLen, &dwMaxValueLen, NULL, NULL);

				//				if (result == ERROR_SUCCESS)
				//				{
				//					TCHAR* bufValName = new TCHAR[dwMaxNameLen + 1];
				//					DWORD dwValNameSize = dwMaxNameLen + 1;

				//					result = RegEnumValue(hKey, 0, bufValName, &dwValNameSize, 0, NULL, NULL, NULL);
				//					INT idx = 0;

				//					do {
				//						TCHAR* bufValue = new TCHAR[dwMaxValueLen + 1];
				//						DWORD dwValueSize = dwMaxValueLen + 1;

				//						result = RegGetValue(hKey, NULL, bufValName, RRF_RT_REG_SZ, NULL, bufValue, &dwValueSize);

				//						if (result == ERROR_SUCCESS)
				//						{
				//							std::tstring name = TEXT("");
				//							std::tstring desc = TEXT("");
				//							std::tstring app = TEXT("");
				//							std::tstring dir = TEXT("");
				//							std::tstring action = TEXT("");

				//							std::tstringstream ss(bufValue);
				//							std::tstring content;
				//							while (std::getline(ss, content, TEXT('|')))
				//							{
				//								std::tstringstream css(content);
				//								std::tstring data;
				//								INT iData = 0;

				//								std::tstring name2;

				//								while (std::getline(css, data, TEXT('=')))
				//								{
				//									if (iData == 0)
				//									{
				//										name2 = data;
				//									}
				//									else
				//									{
				//										if (_tcscmp(name2.c_str(), TEXT("Name")) == 0)
				//										{
				//											TCHAR buffer[512] = TEXT("");

				//											if (SUCCEEDED(SHLoadIndirectString(data.c_str(), buffer, 512, NULL)))
				//											{
				//												name = std::tstring(buffer);
				//											}
				//										}
				//										else if (_tcscmp(name2.c_str(), TEXT("Desc")) == 0)
				//										{
				//											TCHAR buffer[512] = TEXT("");

				//											if (SUCCEEDED(SHLoadIndirectString(data.c_str(), buffer, 512, NULL)))
				//											{
				//												desc = std::tstring(buffer);
				//											}
				//										}
				//										else if (_tcscmp(name2.c_str(), TEXT("App")) == 0)
				//										{
				//											TCHAR buffer[512] = TEXT("");

				//											if (SUCCEEDED(SHLoadIndirectString(data.c_str(), buffer, 512, NULL)))
				//											{
				//												app = std::tstring(buffer);
				//											}
				//										}
				//										else if (_tcscmp(name2.c_str(), TEXT("Dir")) == 0)
				//										{
				//											dir = data;
				//										}
				//										else if (_tcscmp(name2.c_str(), TEXT("Action")) == 0)
				//										{
				//											action = data;
				//										}
				//									}

				//									iData++;
				//								}
				//							}

				//							if (!(_tcscmp(name.c_str(), fwri.name.c_str()) == 0
				//								&& _tcscmp(desc.c_str(), fwri.desc.c_str()) == 0
				//								&& _tcscmp(app.c_str(), fwri.appName.c_str()) == 0)
				//								&& ((fwri.dir == NET_FW_RULE_DIR_IN && (_tcscmp(dir.c_str(), TEXT("In")) == 0))
				//									|| (fwri.dir == NET_FW_RULE_DIR_OUT && (_tcscmp(dir.c_str(), TEXT("Out")) == 0)))
				//								&& ((fwri.action == NET_FW_ACTION_ALLOW && (_tcscmp(action.c_str(), TEXT("Allow")) == 0))
				//									|| (fwri.action == NET_FW_ACTION_BLOCK && (_tcscmp(action.c_str(), TEXT("Block")) == 0))))
				//							{
				//								TCHAR* buffer = new TCHAR[dwMaxNameLen + 3];

				//								_stprintf_s(buffer, dwMaxNameLen + 3, TEXT("\"%s\""), bufValName);

				//								WritePrivateProfileString(TEXT("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\FirewallRules"),
				//									buffer, NULL, bufFile);

				//								delete[] buffer;
				//							}
				//						}

				//						delete[] bufValue;

				//						idx++;
				//						dwValNameSize = dwMaxNameLen + 1;
				//						result = RegEnumValue(hKey, idx, bufValName, &dwValNameSize, 0, NULL, NULL, NULL);
				//					} while (result == ERROR_SUCCESS);

				//					delete[] bufValName;
				//				}
				//			}
				//		}
				//		else
				//		{
				//			DWORD err = GetLastError();
				//			TCHAR bufError[256];

				//			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, bufError, 256, NULL);

				//			TCHAR bufText[256];
				//			_stprintf_s(bufText, 256, TEXT("An error occured.\n%lu: %s"), err, bufError);

				//			MessageBox(hDlg, bufText, NULL, MB_ICONERROR);
				//		}

				//		RegCloseKey(hKey);
				//	}
				//}

				break;
			}
			case IDC_SYSLINK_NETDATAUSAGE:
			{
				PNMLINK pNmLink = (PNMLINK)lParam;

				if (_tcscmp(pNmLink->item.szID, TEXT("reset")) == 0)
				{
					TCHAR bufSysDir[MAX_PATH] = TEXT("C:\\Windows\\System32");
					GetSystemDirectory(bufSysDir, MAX_PATH);

					TCHAR bufArgs[256] = TEXT("/c \"net stop dps && del /F /S /Q /A %SYSTEMROOT%\\System32\\sru\\* && net start dps\"");
					_stprintf_s(bufArgs, 256, TEXT("/c \"net stop dps && del /F /S /Q /A %s\\sru\\* && net start dps\""), bufSysDir);

					ShellExecute(hDlg, NULL, TEXT("cmd.exe"), bufArgs, bufSysDir, SW_SHOW);
				}
				else if (_tcscmp(pNmLink->item.szID, TEXT("mssettings")) == 0)
				{
					ShellExecute(hDlg, NULL, TEXT("ms-settings:datausage"), NULL, NULL, SW_SHOW);
				}
				else if (_tcscmp(pNmLink->item.szID, TEXT("explorer")) == 0)
				{
					TCHAR bufSysDir[MAX_PATH] = TEXT("C:\\Windows\\System32");
					GetSystemDirectory(bufSysDir, MAX_PATH);

					TCHAR bufSruDir[MAX_PATH] = TEXT("C:\\Windows\\System32\\sru");
					_stprintf_s(bufSruDir, MAX_PATH, TEXT("%s\\sru"), bufSysDir);

					ShellExecute(hDlg, TEXT("explore"), bufSruDir, NULL, bufSysDir, SW_SHOW);
				}
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
