///
///	psystem.cpp:
///		Provides functions for the System property sheet page.
/// 

#include "psystem.h"
#include "macro.h"
#include "const.h"
#include <string>
#include <tchar.h>
#include "resource.h"
#include <versionhelpers.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <commctrl.h>
#include <vsstyle.h>
#include <uxtheme.h>
#include <winsatcominterfacei.h>
#include <htmlhelp.h>

EXTERN_C const CLSID CLSID_CInitiateWinSAT =
{ 0x489331DC, 0xF5E0, 0x4528, {0x9F, 0xDA, 0x45, 0x33, 0x1B, 0xF4, 0xA5, 0x71} };

#define WM_FREEWINSAT (WM_APP + 1)

void WinSATDlgReadScores(HWND hDlg)
{
	OutputDebugString(TEXT("Reading scores start!\n"));

	HRESULT hr = CoInitialize(0);
	if (SUCCEEDED(hr))
	{
		OutputDebugString(TEXT("COM inited!\n"));

		IWbemLocator* pLocator = nullptr;
		hr = CoCreateInstance(
			CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID*)&pLocator
		);

		if (SUCCEEDED(hr))
		{
			OutputDebugString(TEXT("Locator created!\n"));
			IWbemServices* pServices = nullptr;
			hr = pLocator->ConnectServer(
				_bstr_t(TEXT("ROOT\\CIMV2")),
				nullptr, nullptr, 0, 0, 0, 0, &pServices
			);

			if (SUCCEEDED(hr))
			{
				OutputDebugString(TEXT("Services created!\n"));
				hr = CoSetProxyBlanket(
					pServices,
					RPC_C_AUTHN_WINNT,
					RPC_C_AUTHZ_NONE,
					nullptr,
					RPC_C_AUTHN_LEVEL_CALL,
					RPC_C_IMP_LEVEL_IMPERSONATE,
					nullptr,
					EOAC_NONE
				);

				if (SUCCEEDED(hr))
				{
					OutputDebugString(TEXT("Proxy blanket set!\n"));
					IEnumWbemClassObject* pEnumerator = nullptr;
					hr = pServices->ExecQuery(
						bstr_t("WQL"),
						bstr_t("SELECT WinSATAssessmentState,CPUScore,D3DScore,DiskScore,GraphicsScore,MemoryScore,WinSPRLevel FROM Win32_WinSAT"),
						WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
						nullptr,
						&pEnumerator
					);

					if (SUCCEEDED(hr))
					{
						OutputDebugString(TEXT("Query executed!\n"));
						IWbemClassObject* pObj = nullptr;
						ULONG ret = 0;

						HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pObj, &ret);
						if (ret)
						{
							OutputDebugString(TEXT("Enumerated!\n"));
							VARIANT vtState{};
							pObj->Get(TEXT("WinSATAssessmentState"), 0, &vtState, nullptr, nullptr);

							if (vtState.uintVal == WINSAT_ASSESSMENT_STATE_INCOHERENT_WITH_HARDWARE || vtState.uintVal == WINSAT_ASSESSMENT_STATE_VALID)
							{
								HWND hStatic = GetDlgItem(hDlg, IDC_STATIC_COMPCONFCHANGED);
								LONG_PTR style = GetWindowLongPtr(hStatic, GWL_STYLE);

								if (vtState.uintVal == WINSAT_ASSESSMENT_STATE_INCOHERENT_WITH_HARDWARE)
								{
									style |= WS_VISIBLE;
								}
								else
								{
									style &= ~WS_VISIBLE;
								}

								SetWindowLongPtr(hStatic, GWL_STYLE, style);

								VARIANT vtCpu{};
								pObj->Get(TEXT("CPUScore"), 0, &vtCpu, nullptr, nullptr);

								VARIANT vtD3D{};
								pObj->Get(TEXT("D3DScore"), 0, &vtD3D, nullptr, nullptr);

								VARIANT vtDisk{};
								pObj->Get(TEXT("DiskScore"), 0, &vtDisk, nullptr, nullptr);

								VARIANT vtGraphics{};
								pObj->Get(TEXT("GraphicsScore"), 0, &vtGraphics, nullptr, nullptr);

								VARIANT vtMem{};
								pObj->Get(TEXT("MemoryScore"), 0, &vtMem, nullptr, nullptr);

								VARIANT vtBase{};
								pObj->Get(TEXT("WinSPRLevel"), 0, &vtBase, nullptr, nullptr);

								TCHAR tCpuBuf[16] = TEXT("-");
								_stprintf_s(tCpuBuf, ARRAYSIZE(tCpuBuf), TEXT("%.1f"), vtCpu.fltVal);
								SetDlgItemText(hDlg, IDC_STATIC_CPUSCORE, tCpuBuf);

								TCHAR tD3DBuf[16] = TEXT("-");
								_stprintf_s(tD3DBuf, ARRAYSIZE(tD3DBuf), TEXT("%.1f"), vtD3D.fltVal);
								SetDlgItemText(hDlg, IDC_STATIC_D3DSCORE, tD3DBuf);

								TCHAR tDiskBuf[16] = TEXT("-");
								_stprintf_s(tDiskBuf, ARRAYSIZE(tDiskBuf), TEXT("%.1f"), vtDisk.fltVal);
								SetDlgItemText(hDlg, IDC_STATIC_DISKSCORE, tDiskBuf);

								TCHAR tGraphicsBuf[16] = TEXT("-");
								_stprintf_s(tGraphicsBuf, ARRAYSIZE(tGraphicsBuf), TEXT("%.1f"), vtGraphics.fltVal);
								SetDlgItemText(hDlg, IDC_STATIC_GRAPHICSSCORE, tGraphicsBuf);

								TCHAR tMemBuf[16] = TEXT("-");
								_stprintf_s(tMemBuf, ARRAYSIZE(tMemBuf), TEXT("%.1f"), vtMem.fltVal);
								SetDlgItemText(hDlg, IDC_STATIC_MEMORYSCORE, tMemBuf);

								TCHAR tBaseBuf[16] = TEXT("-");
								_stprintf_s(tBaseBuf, ARRAYSIZE(tBaseBuf), TEXT("%.1f"), vtBase.fltVal);
								SetDlgItemText(hDlg, IDC_STATIC_MAINSCORE, tBaseBuf);

								VariantClear(&vtCpu);
								VariantClear(&vtD3D);
								VariantClear(&vtDisk);
								VariantClear(&vtGraphics);
								VariantClear(&vtMem);
								VariantClear(&vtBase);

								OutputDebugString(TEXT("Scores got!\n"));
							}

							VariantClear(&vtState);
							pObj->Release();
						}

						pEnumerator->Release();
					}
				}
			}
		}

		CoUninitialize();
	}
}

void CALLBACK WinSATLaunchOnFinish(LPVOID param, BOOLEAN b)
{
	EnableWindow(GetDlgItem((HWND)param, IDOK), TRUE);
	EnableWindow(GetDlgItem((HWND)param, IDC_BUTTON_REFRESH_SATRATING), TRUE);

	WinSATDlgReadScores((HWND)param);
}

void WinSatProgressDialogEvents::Init(HWND hwndOwner)
{
	hOwner = hwndOwner;

	HRESULT hr = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pProgressDialog));

	if (SUCCEEDED(hr))
	{
		pProgressDialog->SetTitle(L"System assesment");
		pProgressDialog->StartProgressDialog(hwndOwner, NULL, PROGDLG_MODAL | PROGDLG_AUTOTIME | PROGDLG_NOMINIMIZE | PROGDLG_NOCANCEL, NULL);
		pProgressDialog->Timer(PDTIMER_RESET, NULL);
	}
}

STDMETHODIMP WinSatProgressDialogEvents::WinSATComplete(HRESULT hresult, LPCWSTR strDescription)
{
	if (pProgressDialog)
	{
		pProgressDialog->StopProgressDialog();
		pProgressDialog->Release();
		pProgressDialog = nullptr;
	}

	WCHAR bufDesc[256] = L"";
	swprintf_s(bufDesc, 256, L"%s.", strDescription);

	MessageBoxW(hOwner, bufDesc, L"System assesment", MB_ICONINFORMATION);
	WinSATDlgReadScores(hOwner);

	PostMessage(hOwner, WM_FREEWINSAT, NULL, (LPARAM)this);

	return S_OK;
}

STDMETHODIMP WinSatProgressDialogEvents::WinSATUpdate(UINT uCurrentTick, UINT uTickTotal, LPCWSTR strCurrentState)
{
	if (pProgressDialog)
	{
		pProgressDialog->SetProgress64(uCurrentTick, uTickTotal);
		pProgressDialog->SetLine(1, strCurrentState, FALSE, NULL);
		pProgressDialog->SetLine(2, strCurrentState, FALSE, NULL); // in case it doesn't fit on the title
	}
	// we do not have anything to put on the other lines
	// thats why we enabled time remaining

	return S_OK;
}

INT_PTR CALLBACK WinSATDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HFONT hMainFont;
	static IInitiateWinSATAssessment* assesment;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{		
		NONCLIENTMETRICS ncm = {};
		ncm.cbSize = sizeof(NONCLIENTMETRICS);

		// Get the metrics, including the system font
		if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
		{
			LOGFONT lf = ncm.lfMessageFont;  // System message font
			HDC hdc = GetDC(hDlg);
			lf.lfHeight = -MulDiv(32, GetDeviceCaps(hdc, LOGPIXELSY), 72);  // 48 pt font
			ReleaseDC(hDlg, hdc);

			hMainFont = CreateFontIndirect(&lf);

			SendDlgItemMessage(hDlg, IDC_STATIC_MAINSCORE, WM_SETFONT, (WPARAM)hMainFont, TRUE);
		}

		WinSATDlgReadScores(hDlg);
		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hDlg, 0);
			break;
		case IDC_BUTTON_REFRESH_SATRATING:
		{
			//TCHAR exePath[128];
			//ExpandEnvironmentStrings(TEXT("%SYSTEMROOT%\\System32\\winsat.exe"), exePath, ARRAYSIZE(exePath));

			//TCHAR cmdLine[256];
			//_stprintf_s(cmdLine, ARRAYSIZE(cmdLine), TEXT("\"%s\" formal"), exePath); // quote in case of spaces

			//TCHAR workDir[128];
			//ExpandEnvironmentStrings(TEXT("%SYSTEMROOT%\\System32"), workDir, ARRAYSIZE(workDir));

			//STARTUPINFO si = {};
			//PROCESS_INFORMATION pi = {};
			//si.cb = sizeof(si);

			//if (CreateProcess(
			//	exePath,       // lpApplicationName
			//	cmdLine,       // lpCommandLine (writable)
			//	nullptr,
			//	nullptr,
			//	FALSE,
			//	0,
			//	nullptr,
			//	workDir,       // current directory
			//	&si,
			//	&pi) && pi.hProcess != INVALID_HANDLE_VALUE)
			//{
			//	CloseHandle(pi.hThread); // no longer needed

			//	EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
			//	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_REFRESH_SATRATING), FALSE);

			//	HANDLE hWait = nullptr;

			//	RegisterWaitForSingleObject(
			//		&hWait,
			//		pi.hProcess,
			//		WinSATLaunchOnFinish,
			//		hDlg,
			//		INFINITE,
			//		WT_EXECUTEONLYONCE
			//	);
			//}

			HRESULT hr = CoInitialize(0);
			if (SUCCEEDED(hr))
			{
				IInitiateWinSATAssessment* pInitWinSat;
				hr = CoCreateInstance(CLSID_CInitiateWinSAT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pInitWinSat));

				if (SUCCEEDED(hr))
				{
					WinSatProgressDialogEvents* events = new WinSatProgressDialogEvents();
					events->Init(hDlg);

					// the events release themselves
					pInitWinSat->InitiateFormalAssessment((IWinSATInitiateEvents*)events, hDlg);

					pInitWinSat->AddRef();
					assesment = pInitWinSat;
				}
				else
					OutputDebugString(TEXT("Error")); // this is breakpointed for the debugging

				CoUninitialize();
			}

			break;
		}
		}

		break;
	case WM_FREEWINSAT:
		((WinSatProgressDialogEvents*)lParam)->Release();
		assesment->Release();
		break;
	case WM_DRAWITEM:
	{
		if (wParam == IDC_STATIC_TITLE)
		{
			LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;

			if (IsAppThemed())
			{
				HTHEME hTheme = OpenThemeData(lpDrawItem->hwndItem, TEXT("TEXTSTYLE"));
				if (hTheme)
				{
					DrawThemeParentBackground(lpDrawItem->hwndItem, lpDrawItem->hDC, &lpDrawItem->rcItem);

					DrawThemeText(hTheme, lpDrawItem->hDC,
						TEXT_MAININSTRUCTION, 0,
						L"Windows Experience Index", -1,
						DT_SINGLELINE | DT_CENTER | DT_VCENTER,
						0, &lpDrawItem->rcItem);
					
					CloseThemeData(hTheme);
				}
			}
			else
			{
				SetBkMode(lpDrawItem->hDC, TRANSPARENT);
				DrawText(lpDrawItem->hDC, TEXT("Windows Experience Index"), -1,
					(LPRECT)&lpDrawItem->rcItem, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			}

			return TRUE;
		}
		break;
	}
	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;
	case WM_DESTROY:
		if (hMainFont)
		{
			DeleteObject(hMainFont);
		}
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK SystemPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		//STD_TSTR verName = TEXT("<Unknown release>");

		//HKEY hKey;
		//LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ, &hKey);
		//BOOL keyOpen = FALSE;

		//if (result == ERROR_SUCCESS)
		//{
		//	keyOpen = TRUE;

		//	TCHAR tBufBuild[16] = TEXT("0");
		//	DWORD dwBufferSize = sizeof(tBufBuild);
		//	DWORD dwType = 0;

		//	result = RegQueryValueEx(hKey, TEXT("CurrentBuild"), nullptr, &dwType, reinterpret_cast<LPBYTE>(tBufBuild), &dwBufferSize);

		//	int build = 0;

		//	if (result == ERROR_SUCCESS)
		//	{
		//		build = _ttoi(tBufBuild);
		//	}

		//	if (IsWindowsVersionOrGreater(10, 0, 0) && build >= 22000) verName = TEXT("Windows 11");
		//	else
		//	{
		//		// Read the ProductName value under HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion
		//		TCHAR tBufProdName[64] = TEXT("");
		//		dwBufferSize = sizeof(tBufProdName);
		//		dwType = 0;

		//		result = RegQueryValueEx(hKey, TEXT("ProductName"), nullptr, &dwType, reinterpret_cast<LPBYTE>(tBufProdName), &dwBufferSize);

		//		if (result == ERROR_SUCCESS && dwType == REG_SZ)
		//		{
		//			verName = STD_TSTR(tBufProdName, dwBufferSize / sizeof(TCHAR) - 1); // exclude null terminator
		//		}
		//	}
		//}

		//STD_TSTR editionName = TEXT("<Unknown edition>");
		//STD_TSTR dispVerName = TEXT("<Unknown version>"); // actual version name, e.g 23H2, not to be confused with verName that has something like Windows 7 or something

		//if (keyOpen)
		//{
		//	TCHAR tBufEdName[32] = TEXT("");
		//	DWORD dwBufferSize = sizeof(tBufEdName);
		//	DWORD dwType = 0;

		//	result = RegQueryValueEx(hKey, TEXT("EditionID"), nullptr, &dwType, reinterpret_cast<LPBYTE>(tBufEdName), &dwBufferSize);

		//	if (result == ERROR_SUCCESS && dwType == REG_SZ)
		//	{
		//		editionName = STD_TSTR(tBufEdName, dwBufferSize / sizeof(TCHAR) - 1); // exclude null terminator
		//	}

		//	TCHAR tBufVerName[16] = TEXT("");
		//	dwBufferSize = sizeof(tBufVerName);

		//	result = RegQueryValueEx(hKey, TEXT("DisplayVersion"), nullptr, &dwType, reinterpret_cast<LPBYTE>(tBufVerName), &dwBufferSize);

		//	if (result == ERROR_SUCCESS && dwType == REG_SZ)
		//	{
		//		dispVerName = STD_TSTR(tBufVerName, dwBufferSize / sizeof(TCHAR) - 1); // exclude null terminator
		//	}

		//	RegCloseKey(hKey);
		//}

		//TCHAR buffer[128] = TEXT("");
		//_stprintf_s(buffer, ARRAYSIZE(buffer), TEXT("%s %s %s"), verName.c_str(), editionName.c_str(), dispVerName.c_str());

		//SetDlgItemText(hDlg, IDC_STATIC_OS, buffer);

		if (auto hIcon = GETSTOCKICON(SIID_SOFTWARE, SHGSI_SHELLICONSIZE))
		{
			SendDlgItemMessage(hDlg, IDC_STATIC_OSIMG, STM_SETICON, (WPARAM)*hIcon, 0);
		}

		SendDlgItemMessage(hDlg, IDC_STATIC_CPUIMG, STM_SETICON, (WPARAM)LOADICON(IDI_CPU), 0);

		if (auto hIcon = GETSTOCKICON(SIID_DRIVERAM, SHGSI_SHELLICONSIZE))
		{
			SendDlgItemMessage(hDlg, IDC_STATIC_RAMIMG, STM_SETICON, (WPARAM)*hIcon, 0);
		}

		if (auto hIcon = GETSTOCKICON(SIID_DESKTOPPC, SHGSI_SHELLICONSIZE))
		{
			SendDlgItemMessage(hDlg, IDC_STATIC_DISPLAYIMG, STM_SETICON, (WPARAM)*hIcon, 0);
			SendDlgItemMessage(hDlg, IDC_STATIC_COMPIMG, STM_SETICON, (WPARAM)*hIcon, 0);
		}

		HRESULT hr = CoInitialize(0);
		if (SUCCEEDED(hr))
		{
			hr = CoInitializeSecurity(
				nullptr, -1, nullptr, nullptr,
				RPC_C_AUTHN_LEVEL_DEFAULT,
				RPC_C_IMP_LEVEL_IMPERSONATE,
				nullptr, EOAC_NONE, nullptr
			);

			if (SUCCEEDED(hr))
			{
				IWbemLocator* pLocator = nullptr;
				hr = CoCreateInstance(
					CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
					IID_IWbemLocator, (LPVOID*)&pLocator
				);

				if (SUCCEEDED(hr))
				{
					IWbemServices* pServices = nullptr;
					hr = pLocator->ConnectServer(
						_bstr_t(TEXT("ROOT\\CIMV2")),
						nullptr, nullptr, 0, 0, 0, 0, &pServices
					);

					if (SUCCEEDED(hr))
					{
						hr = CoSetProxyBlanket(
							pServices,
							RPC_C_AUTHN_WINNT,
							RPC_C_AUTHZ_NONE,
							nullptr,
							RPC_C_AUTHN_LEVEL_CALL,
							RPC_C_IMP_LEVEL_IMPERSONATE,
							nullptr,
							EOAC_NONE
						);

						if (SUCCEEDED(hr))
						{
							IEnumWbemClassObject* pEnumerator = nullptr;
							hr = pServices->ExecQuery(
								bstr_t("WQL"),
								bstr_t("SELECT Name FROM Win32_Processor"),
								WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
								nullptr,
								&pEnumerator
							);

							if (SUCCEEDED(hr))
							{
								IWbemClassObject* pObj = nullptr;
								ULONG ret = 0;

								HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pObj, &ret);
								if (ret)
								{
									VARIANT vtName{};
									pObj->Get(TEXT("Name"), 0, &vtName, nullptr, nullptr);

									SetDlgItemText(hDlg, IDC_STATIC_CPU, vtName.bstrVal);

									VariantClear(&vtName);
									pObj->Release();
								}

								pEnumerator->Release();
							}

							hr = pServices->ExecQuery(
								bstr_t("WQL"),
								bstr_t("SELECT Caption FROM Win32_OperatingSystem"),
								WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
								nullptr,
								&pEnumerator
							);

							if (SUCCEEDED(hr))
							{
								IWbemClassObject* pObj = nullptr;
								ULONG ret = 0;

								HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pObj, &ret);
								if (ret)
								{
									VARIANT vtCaption{};
									pObj->Get(TEXT("Caption"), 0, &vtCaption, nullptr, nullptr);

									SetDlgItemText(hDlg, IDC_STATIC_OS, vtCaption.bstrVal);

									VariantClear(&vtCaption);
									pObj->Release();
								}

								pEnumerator->Release();
							}

							hr = pServices->ExecQuery(
								bstr_t("WQL"),
								bstr_t("SELECT Caption FROM CIM_DesktopMonitor"),
								WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
								nullptr,
								&pEnumerator
							);

							if (SUCCEEDED(hr))
							{
								IWbemClassObject* pObj = nullptr;
								ULONG ret = 0;

								HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pObj, &ret);
								if (ret)
								{
									VARIANT vtCaption{};
									pObj->Get(TEXT("Caption"), 0, &vtCaption, nullptr, nullptr);

									SetDlgItemText(hDlg, IDC_STATIC_DISPLAY, vtCaption.bstrVal);

									VariantClear(&vtCaption);
									pObj->Release();
								}

								pEnumerator->Release();
							}

							hr = pServices->ExecQuery(
								bstr_t("WQL"),
								bstr_t("SELECT Caption FROM CIM_ComputerSystem"),
								WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
								nullptr,
								&pEnumerator
							);

							if (SUCCEEDED(hr))
							{
								IWbemClassObject* pObj = nullptr;
								ULONG ret = 0;

								HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pObj, &ret);
								if (ret)
								{
									VARIANT vtCaption{};
									pObj->Get(TEXT("Caption"), 0, &vtCaption, nullptr, nullptr);

									SetDlgItemText(hDlg, IDC_STATIC_COMPNAME, vtCaption.bstrVal);

									VariantClear(&vtCaption);
									pObj->Release();
								}

								pEnumerator->Release();
							}

							hr = pServices->ExecQuery(
								bstr_t("WQL"),
								bstr_t("SELECT Capacity FROM Win32_PhysicalMemory"),
								WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
								nullptr,
								&pEnumerator
							);

							if (SUCCEEDED(hr))
							{
								ULONGLONG totalBytes = 0;

								IWbemClassObject* pObj = nullptr;
								ULONG ret = 0;
								while (pEnumerator->Next(WBEM_INFINITE, 1, &pObj, &ret) == S_OK)
								{
									VARIANT vtCapacity{};
									if (SUCCEEDED(pObj->Get(TEXT("Capacity"), 0, &vtCapacity, nullptr, nullptr)) &&
										vtCapacity.vt == VT_BSTR)
									{
										totalBytes += _tcstoui64(vtCapacity.bstrVal, nullptr, 10);
									}
									VariantClear(&vtCapacity);
									pObj->Release();
								}

								TCHAR buffer[256];
								_stprintf_s(buffer, ARRAYSIZE(buffer), TEXT("%.2f GB (total)"), totalBytes / 1073741824.0);
								SetDlgItemText(hDlg, IDC_STATIC_RAM, buffer);

								pEnumerator->Release();
							}

							pServices->Release();
						}

						pLocator->Release();
					}
				}
			}

			CoUninitialize();
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
			if (nmhdr->idFrom == IDC_WINSAT_SYSLINK)
			{
				DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_WINSATDLG), GetParent(hDlg), WinSATDlgProc);
			}
			break;
		}
		}

		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_SYSPROPS:
		{
			TCHAR buffer[128] = TEXT("C:\\Windows\\System32");

			ExpandEnvironmentStrings(TEXT("%SYSTEMROOT%\\System32"), buffer, ARRAYSIZE(buffer));

			ShellExecute(GetParent(hDlg), NULL, TEXT("control.exe"), TEXT("sysdm.cpl"), buffer, SW_SHOW);
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
		case IDC_BUTTON_SYSPROPS:
			hhp.pszText = TEXT("Shows a dialog box allowing you to configure advanced system properties.");
			break;
		case IDC_WINSAT_SYSLINK:
			hhp.pszText = TEXT("Rates your computer's performance based on several tests.");
			break;
		}

		HtmlHelp((HWND)lphi->hItemHandle, NULL, HH_DISPLAY_TEXT_POPUP, (DWORD_PTR)&hhp);

		break;
	}
	}

	return FALSE;
}
