#include "ptools.h"
#include <commctrl.h>
#include "resource.h"
#include <tchar.h>
#include "const.h"
#include <optional>
#include <memory>
#include "macro.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <string>
#include <tlhelp32.h>
#include <codecvt>
#include <map>
#include <strsafe.h>
#include <versionhelpers.h>
#include "utility.h"

#define ID_MENUOPTION_ADDLANG 9001

DWORD dwCurrentErrCheckerLang = 0;

typedef struct ERRORCHECKSPLITITEMINFO
{
	HMENU hMenu;
	std::map<DWORD, LANGID> mLangIds;
};

typedef ERRORCHECKSPLITITEMINFO* LPERRORCHECKSPLITITEMINFO;

using Microsoft::WRL::ComPtr;

HRESULT LoadImageWIC(LPCWSTR filePath, HBITMAP* pHBmp)
{
	HRESULT hr = CoInitialize(0);

	if (SUCCEEDED(hr))
	{
		ComPtr<IWICImagingFactory> factory;
		ComPtr<IWICBitmapDecoder> decoder;
		ComPtr<IWICBitmapFrameDecode> frame;
		ComPtr<IWICFormatConverter> converter;

		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&factory)
		);

		if (SUCCEEDED(hr))
		{
			hr = factory->CreateDecoderFromFilename(
				filePath,
				nullptr,
				GENERIC_READ,
				WICDecodeMetadataCacheOnLoad,
				&decoder
			);

			if (SUCCEEDED(hr))
			{
				hr = decoder->GetFrame(0, &frame);
				if (SUCCEEDED(hr))
				{
					hr = factory->CreateFormatConverter(&converter);
					if (SUCCEEDED(hr))
					{
						// Convert to 32bpp BGRA for GDI compatibility
						hr = converter->Initialize(
							frame.Get(),
							GUID_WICPixelFormat32bppBGRA,
							WICBitmapDitherTypeNone,
							nullptr,
							0.0,
							WICBitmapPaletteTypeCustom
						);

						if (SUCCEEDED(hr))
						{
							UINT width = 0, height = 0;
							frame->GetSize(&width, &height);

							BITMAPINFO bmi = {};
							bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
							bmi.bmiHeader.biWidth = width;
							bmi.bmiHeader.biHeight = -(LONG)height; // top-down
							bmi.bmiHeader.biPlanes = 1;
							bmi.bmiHeader.biBitCount = 32;
							bmi.bmiHeader.biCompression = BI_RGB;

							void* pvImageBits = nullptr;
							HDC hdc = GetDC(nullptr);
							HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvImageBits, nullptr, 0);
							ReleaseDC(nullptr, hdc);

							if (hBmp)
							{
								const UINT stride = width * 4;
								const UINT bufSize = stride * height;

								hr = converter->CopyPixels(nullptr, stride, bufSize, static_cast<BYTE*>(pvImageBits));
								if (SUCCEEDED(hr))
								{
									*pHBmp = hBmp;
								}
								else
								{
									DeleteObject(hBmp);
								}
							}
						}
					}
				}
			}
		}

		CoUninitialize();
	}

	return hr;
}

HBITMAP ScaleBitmap(HBITMAP hSrc, int newWidth, int newHeight)
{
	if (!hSrc) return nullptr;

	BITMAP bm;
	GetObject(hSrc, sizeof(BITMAP), &bm);

	HDC hSrcDC = CreateCompatibleDC(nullptr);
	HDC hDstDC = CreateCompatibleDC(nullptr);

	// Create a 32bpp top-down DIBSection
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = newWidth;
	bmi.bmiHeader.biHeight = -newHeight; // negative = top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void* pPixels = nullptr;
	HBITMAP hDstBmp = CreateDIBSection(hDstDC, &bmi, DIB_RGB_COLORS, &pPixels, nullptr, 0);
	if (!hDstBmp)
	{
		DeleteDC(hSrcDC);
		DeleteDC(hDstDC);
		return nullptr;
	}

	HGDIOBJ oldSrc = SelectObject(hSrcDC, hSrc);
	HGDIOBJ oldDst = SelectObject(hDstDC, hDstBmp);

	SetStretchBltMode(hDstDC, HALFTONE);
	StretchBlt(
		hDstDC,
		0, 0, newWidth, newHeight,
		hSrcDC,
		0, 0, bm.bmWidth, bm.bmHeight,
		SRCCOPY
	);

	SelectObject(hSrcDC, oldSrc);
	SelectObject(hDstDC, oldDst);
	DeleteDC(hSrcDC);
	DeleteDC(hDstDC);

	return hDstBmp;
}

std::optional<std::unique_ptr<TCHAR[]>> ReadRegSzValue(HKEY hKey, LPCWSTR valueName)
{
	DWORD dwSize = 0;
	LONG result = RegGetValue(hKey, nullptr, valueName, RRF_RT_REG_SZ, nullptr, nullptr, &dwSize);
	if (result != ERROR_SUCCESS || dwSize == 0)
		return std::nullopt;

	// Allocate buffer (size is in bytes, not characters!)
	auto buf = std::make_unique<TCHAR[]>(dwSize / sizeof(TCHAR));

	result = RegGetValue(hKey, nullptr, valueName, RRF_RT_REG_SZ, nullptr, buf.get(), &dwSize);
	if (result == ERROR_SUCCESS)
		return std::move(buf);

	return std::nullopt;
}

BOOL CALLBACK EnumUILangProcForErrorCodeChecker(LPTSTR lpLangId, LONG_PTR lParam)
{
	LPERRORCHECKSPLITITEMINFO ecsii = (LPERRORCHECKSPLITITEMINFO)lParam;
	dwCurrentErrCheckerLang++;

	LANGID langid = (LANGID)_tcstol(lpLangId, nullptr, 16);

	WCHAR localeName[LOCALE_NAME_MAX_LENGTH];
	if (!LCIDToLocaleName(MAKELCID(langid, SORT_DEFAULT), localeName, LOCALE_NAME_MAX_LENGTH, 0))
		return TRUE;

	WCHAR displayName[128];
	if (!GetLocaleInfoEx(localeName, LOCALE_SLOCALIZEDDISPLAYNAME, displayName, ARRAYSIZE(displayName)))
		StringCchCopy(displayName, ARRAYSIZE(displayName), localeName);

	AppendMenu(ecsii->hMenu, MF_STRING, dwCurrentErrCheckerLang, displayName);
	ecsii->mLangIds[dwCurrentErrCheckerLang] = langid;

	return TRUE;
}

LRESULT CALLBACK ToolsPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static LPERRORCHECKSPLITITEMINFO _errorCheckSplitInfo;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		_errorCheckSplitInfo = {};

		Edit_SetCueBannerText(GetDlgItem(hDlg, IDC_EDIT_OEM), TEXT("Manufacturer"));
		Edit_SetCueBannerText(GetDlgItem(hDlg, IDC_EDIT_MODEL), TEXT("Model"));
		Edit_SetCueBannerText(GetDlgItem(hDlg, IDC_EDIT_SUPPORTHOURS), TEXT("Support Hours"));
		Edit_SetCueBannerText(GetDlgItem(hDlg, IDC_EDIT_SUPPORTPHONE), TEXT("Support Phone"));
		Edit_SetCueBannerText(GetDlgItem(hDlg, IDC_EDIT_SUPPORTURL), TEXT("Support URL"));

		Edit_SetCueBannerText(GetDlgItem(hDlg, IDC_EDIT_REGOWNER), TEXT("Registered owner"));
		Edit_SetCueBannerText(GetDlgItem(hDlg, IDC_EDIT_REGORGANIZATION), TEXT("Registered organization"));

		Edit_SetCueBannerText(GetDlgItem(hDlg, IDC_EDIT_EXECTI), TEXT("Path + arguments..."));
		Edit_SetCueBannerText(GetDlgItem(hDlg, IDC_EDIT_ERRORCODE), TEXT("Error code..."));

		Edit_LimitText(GetDlgItem(hDlg, IDC_EDIT_SUPPORTHOURS), 256);
		Edit_LimitText(GetDlgItem(hDlg, IDC_EDIT_SUPPORTPHONE), 256);

		SetDlgItemText(hDlg, IDC_EDIT_ERRORCODEVIEW, TEXT("Enter an error code and press the Check button to check the error message!"));

		HKEY hKey;
		LONG result = RegOpenKeyEx(HKLM, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OEMInformation"), 0, KEY_READ, &hKey);

		if (result == ERROR_SUCCESS)
		{
			auto optOem = ReadRegSzValue(hKey, TEXT("Manufacturer"));

			if (optOem.has_value())
			{
				SetDlgItemText(hDlg, IDC_EDIT_OEM, optOem.value().get());
			}

			auto optModel = ReadRegSzValue(hKey, TEXT("Model"));

			if (optModel.has_value())
			{
				SetDlgItemText(hDlg, IDC_EDIT_MODEL, optModel.value().get());
			}

			auto optHours = ReadRegSzValue(hKey, TEXT("SupportHours"));

			if (optHours.has_value())
			{
				SetDlgItemText(hDlg, IDC_EDIT_SUPPORTHOURS, optHours.value().get());
			}

			auto optPhone = ReadRegSzValue(hKey, TEXT("SupportPhone"));

			if (optPhone.has_value())
			{
				SetDlgItemText(hDlg, IDC_EDIT_SUPPORTPHONE, optPhone.value().get());
			}

			auto optUrl = ReadRegSzValue(hKey, TEXT("SupportURL"));

			if (optUrl.has_value())
			{
				SetDlgItemText(hDlg, IDC_EDIT_SUPPORTURL, optUrl.value().get());
			}

			auto optLogo = ReadRegSzValue(hKey, TEXT("Logo"));

			if (optLogo.has_value())
			{
				HBITMAP hBmp;
				HRESULT hr = LoadImageWIC(optLogo.value().get(), &hBmp);

				if (SUCCEEDED(hr))
				{
					HBITMAP hScaled = ScaleBitmap(hBmp, 60, 60);
					
					if (hScaled != nullptr)
						Static_SetImage(GetDlgItem(hDlg, IDC_STATIC_OEMLOGO), IMAGE_BITMAP, hScaled);

					// delete original
					DeleteObject(hBmp);
				}
				else
				{
					TCHAR bufMsg[256] = TEXT("");

					FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, bufMsg, 256, NULL);

					TCHAR bufText[256] = TEXT("");
					_stprintf_s(bufText, 256, TEXT("An error occured loading the OEM logo.\n\n%s"), bufMsg);

					MessageBox(hDlg, bufText, NULL, MB_ICONERROR);
				}
			}

			RegCloseKey(hKey);
		}

		auto hErrorIcon = GETSTOCKICON(SIID_ERROR, SHGSI_LARGEICON);

		if (hErrorIcon.has_value())
		{
			Static_SetImage(GetDlgItem(hDlg, IDC_STATIC_ICONERROR), IMAGE_ICON, hErrorIcon.value());
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
			case IDC_SYSLINK_OEMINFO:
			{
				PNMLINK pNmLink = (PNMLINK)lParam;
				
				if (_tcscmp(pNmLink->item.szID, TEXT("cpl")) == 0)
				{
					TCHAR bufSysDir[MAX_PATH] = TEXT("C:\\Windows\\System32");

					GetSystemDirectory(bufSysDir, MAX_PATH);

					TCHAR bufControlExe[MAX_PATH] = TEXT("C:\\Windows\\System32\\control.exe");
					_stprintf_s(bufControlExe, MAX_PATH, TEXT("%s\\control.exe"), bufSysDir);

					// note: this will redirect to settings if redirecting is not disabled through registry (you can fix this through ExplorerPatcher)
					ShellExecute(hDlg, NULL, bufControlExe, TEXT("/name Microsoft.System"), bufSysDir, SW_SHOW);
				}
				else
				{
					ShellExecute(hDlg, NULL, TEXT("ms-settings:about"), NULL, NULL, SW_SHOW);
				}

				break;
			}
			case IDC_SYSLINK_COMMANDTOOLSAD:
			{
				PNMLINK pNmLink = (PNMLINK)lParam;
				ShellExecuteW(hDlg, NULL, pNmLink->item.szUrl, NULL, NULL, SW_SHOW);

				break;
			}
			}

			break;
		}
		case BCN_DROPDOWN:
		{
			switch (nmhdr->idFrom)
			{
			case IDC_BUTTON_CHECKERROR:
			{
				NMBCDROPDOWN* pDropDown = (NMBCDROPDOWN*)lParam;

				POINT pt;
				pt.x = pDropDown->rcButton.left;
				pt.y = pDropDown->rcButton.bottom;
				ClientToScreen(pDropDown->hdr.hwndFrom, &pt);

				HMENU hSplitMenu = CreatePopupMenu();
				LPERRORCHECKSPLITITEMINFO pEcsii = new ERRORCHECKSPLITITEMINFO();
				pEcsii->hMenu = hSplitMenu;
				pEcsii->mLangIds = {};

				dwCurrentErrCheckerLang = 8000;
				_errorCheckSplitInfo = pEcsii;

				EnumUILanguages(EnumUILangProcForErrorCodeChecker, MUI_LANGUAGE_ID | MUI_ALL_INSTALLED_LANGUAGES, (LONG_PTR)pEcsii);

				AppendMenu(hSplitMenu, MF_SEPARATOR, 0, NULL);
				AppendMenu(hSplitMenu, MF_STRING, ID_MENUOPTION_ADDLANG, TEXT("Add a language..."));

				TrackPopupMenu(hSplitMenu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, hDlg, NULL);

				break;
			}
			}

			break;
		}
		}

		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) > 8000 && LOWORD(wParam) < 9000)
		{
			TCHAR bufText[128] = TEXT("0");
			GetDlgItemText(hDlg, IDC_EDIT_ERRORCODE, bufText, 128);

			unsigned long code = _tcstoul(bufText, NULL, 0); // auto-determine base

			TCHAR bufMsg[256] = TEXT("Unable to find an error message.");
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, _errorCheckSplitInfo->mLangIds[LOWORD(wParam)], bufMsg, 256, NULL);

			SetDlgItemText(hDlg, IDC_EDIT_ERRORCODEVIEW, bufMsg);

			delete _errorCheckSplitInfo;
			_errorCheckSplitInfo = nullptr;

		}
		else
		{
			switch (LOWORD(wParam))
			{
			case IDC_BUTTON_RUN:
			{
				DWORD dwLen = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_EXECTI));
				TCHAR* bufText = new TCHAR[dwLen + 1]; // include null terminator
				GetDlgItemText(hDlg, IDC_EDIT_EXECTI, bufText, dwLen + 1);

				if (_tcscmp(bufText, TEXT("")) == 0)
				{
					delete[] bufText;
					MessageBox(hDlg, TEXT("The file path box cannot be empty."), NULL, MB_ICONERROR); // caption defaults to Error but localized
					break;
				}

				int button = -1;
				if (SUCCEEDED(TaskDialog(hDlg, NULL, TEXT("Security Warning"), TEXT("Security Warning"),
					TEXT("Warning! Please read this carefully before continuing.\n\nRunning programs under TrustedInstaller privileges, especially a file manager or Registry Editor (regedit.exe) can be VERY risky and harmful for your OS. It's like a God mode in a first-person shooter game, where nothing can stop your actions, so if you execute a malware infected file as TrustedInstaller, it can cause damage to the operating system.\n\nContinue only if you perfectly understand what you are doing and if there is no simpler way than running it as TrustedInstaller."),
					TDCBF_CONTINUE_BUTTON | TDCBF_CANCEL_BUTTON, MAKEINTRESOURCE(TD_SHIELDWARNING_ICON), &button)))
				{
					if (button == IDCONTINUE)
					{
						const auto pid = StartTIService();
						if (!ImpersonateSystem())
						{
							ShowTIExecError(TEXT("An error occured impersonating the system account."), ERROR_CANNOT_IMPERSONATE, hDlg);
							delete[] bufText;
							break;
						}

						HANDLE process_handle;
						if ((process_handle = OpenProcess(
							PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
							FALSE,
							pid)) == nullptr)
						{
							ShowTIExecError(TEXT("OpenProcess failed (TrustedInstaller.exe)."), GetLastError(), hDlg);
							RevertToSelf();
							break;
						}

						HANDLE token_handle = 0;
						if (!OpenProcessToken(
							process_handle,
							TOKEN_DUPLICATE,
							&token_handle))
						{
							CloseHandle(process_handle);
							ShowTIExecError(TEXT("OpenProcessToken failed (TrustedInstaller.exe)."), GetLastError(), hDlg);
							RevertToSelf();
							break;
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
						}
						CloseHandle(token_handle);

						STARTUPINFO startup_info;
						GetStartupInfo(&startup_info);
						PROCESS_INFORMATION process_info;
						ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));
						if (!CreateProcessWithToken(
							dup_token_handle,
							LOGON_WITH_PROFILE,
							nullptr,
							bufText,
							CREATE_UNICODE_ENVIRONMENT,
							nullptr,
							nullptr,
							&startup_info,
							&process_info))
						{
							CloseHandle(dup_token_handle);
							delete[] bufText;
							RevertToSelf();

							break;
						}
						CloseHandle(dup_token_handle);
						delete[] bufText;
						RevertToSelf();
					}
				}
				break;
			}
			case IDC_BUTTON_CHECKERROR:
			{
				TCHAR bufText[128] = TEXT("0");
				GetDlgItemText(hDlg, IDC_EDIT_ERRORCODE, bufText, 128);

				unsigned long code = _tcstoul(bufText, NULL, 0); // auto-determine base

				TCHAR bufMsg[256] = TEXT("Unable to find an error message.");
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, bufMsg, 256, NULL);

				SetDlgItemText(hDlg, IDC_EDIT_ERRORCODEVIEW, bufMsg);
				break;
			}
			case ID_MENUOPTION_ADDLANG:
			{
				TCHAR bufSysDir[MAX_PATH] = TEXT("C:\\Windows\\System32");
				GetSystemDirectory(bufSysDir, MAX_PATH);

				TCHAR bufCpl[MAX_PATH] = TEXT("C:\\Windows\\System32\\control.exe");
				_stprintf_s(bufCpl, MAX_PATH, TEXT("%s\\control.exe"), bufSysDir);

				if (IsWindows8OrGreater()) ShellExecute(hDlg, NULL, bufCpl, TEXT("/name Microsoft.Language"), bufSysDir, SW_SHOW);
				else					   ShellExecute(hDlg, NULL, bufCpl, TEXT("/name Microsoft.RegionAndLanguage"), bufSysDir, SW_SHOW);

				break;
			}
			}
		}

		break;
	}
	}

	return FALSE;
}