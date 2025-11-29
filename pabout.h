#include <windows.h>

typedef HRESULT(WINAPI* PFN_DwmSetWindowAttribute)(
	HWND hwnd,
	DWORD dwAttribute,
	LPCVOID pvAttribute,
	DWORD cbAttribute
	);

LRESULT CALLBACK AboutPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);