///
///	macro.h:
///		Defines common macros.
/// 

#pragma once
#include <windows.h>
#include <optional>

#ifdef _UNICODE
#define _OSVERSIONINFO _OSVERSIONINFOW
#define _OSVERSIONINFOEX _OSVERSIONINFOEXW
// newer code should use the typedefs defined in utility.h instead of the STD_TSTR* macros
#define STD_TSTR std::wstring
#define STD_TSTRSTREAM std::wstringstream
#define CreateProcessWithToken CreateProcessWithTokenW
#define StartService StartServiceW
#else
#define _OSVERSIONINFO _OSVERSIONINFOA
#define _OSVERSIONINFOEX _OSVERSIONINFOEXA
// newer code should use the typedefs defined in utility.h instead of the STD_TSTR* macros
#define STD_TSTR std::string
#define STD_TSTRSTREAM std::stringstream
#define CreateProcessWithToken CreateProcessWithTokenA
#define StartService StartServiceA
#endif

#define LOADCUSTOMICON(idi) LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(idi))
#define LOADSYSICON(idi) LoadIcon(nullptr, MAKEINTRESOURCE(idi))
#define LOADICON(idi) (((idi) >= 32512 && (idi) <= 32518) ? LOADSYSICON(idi) : LOADCUSTOMICON(idi))

#define GETSTOCKICON(siid, flags) []() -> std::optional<HICON> {\
SHSTOCKICONINFO sii = {};\
sii.cbSize = sizeof(SHSTOCKICONINFO);\
if (SUCCEEDED(SHGetStockIconInfo(siid, flags | SHGSI_ICON, &sii)))\
{\
	return sii.hIcon;\
}\
return std::nullopt;\
}()

#define ShowDlgItem(hDlg, idc, nCmdShow) ShowWindow(GetDlgItem(hDlg, idc), nCmdShow)

#define ComboBox_AddString(hwnd, lpStr) SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)lpStr)
#define ComboBox_SetItemData(hwnd, iItem, data) SendMessage(hwnd, CB_SETITEMDATA, iItem, (LPARAM)data)
#define ComboBox_GetCurSel(hwnd) SendMessage(hwnd, CB_GETCURSEL, 0, 0)
#define ComboBox_GetListBoxText(hwnd, iItem, buffer) SendMessage(hwnd, CB_GETLBTEXT, iItem, (LPARAM)buffer)
#define ComboBox_SetCurSel(hwnd, iItem) SendMessage(hwnd, CB_SETCURSEL, (WPARAM)(iItem), NULL)

#define Window_SetRedraw(hwnd, bRedraw) SendMessage(hwnd, WM_SETREDRAW, (WPARAM)bRedraw, 0)

#define Static_SetImage(hwnd, dwImgType, hImg) SendMessage(hwnd, STM_SETIMAGE, (WPARAM)dwImgType, (LPARAM)hImg);

#define Edit_LimitText(hwnd, dwMaxTChar) SendMessage(hwnd, EM_LIMITTEXT, (WPARAM)dwMaxTChar, 0)

#define TrackBar_GetPos(hwnd) (INT)SendMessage(hwnd, TBM_GETPOS, 0, 0)
#define TrackBar_SetRange(hwnd, bRedraw, dwMin, dwMax) SendMessage(hwnd, TBM_SETRANGE, (WPARAM)bRedraw, (LPARAM)MAKELPARAM(dwMin, dwMax))
#define TrackBar_SetPos(hwnd, bRedraw, dwPos) SendMessage(hwnd, TBM_SETPOS, (WPARAM)bRedraw, (LPARAM)dwPos)

#define UpDown_SetPos32(hwnd, dwPos) SendMessage(hwnd, UDM_SETPOS32, 0, (LPARAM)dwPos)
#define UpDown_SetRange32(hwnd, dwMin, dwMax) SendMessage(hwnd, UDM_SETRANGE32, (WPARAM)dwMin, (LPARAM)dwMax)
#define UpDown_GetPos32(hwnd, pbSuccess) (INT)SendMessage(hwnd, UDM_GETPOS32, 0, (LPARAM)pbSuccess)

#define ListBox_AddString(hwnd, lpstr) SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)lpstr)