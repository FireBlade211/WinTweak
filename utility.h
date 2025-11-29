///
///	utility.h:
///		Defines signatures for utility functions.
/// 

#include <windows.h>
#include "macro.h"
#include <string>

BOOL IsWindowsVersionOrGreaterEx(INT build, INT major, INT minor);

int _clamp(int i, int max, int min);
long _clamp_l(long l, long max, long min);

BOOL EnablePrivilege(LPCTSTR privName);

int RestartExplorerDialog(HWND hwndOwner);

DWORD GetPIDByName(const STD_TSTR process_name);
BOOL ImpersonateSystem();
int StartTIService();
void ShowTIExecError(STD_TSTR msg, DWORD dwErr, HWND hDlg);

// reg error dialog mode flags
enum REDMF
{
	REDMF_READ,
	REDMF_WRITE
};

void ShowRegistryError(HWND hwndOwner, REDMF mode, LPCTSTR lpKeyName, LPCTSTR lpValName, DWORD dwResult);

namespace std
{
#ifdef UNICODE
	typedef wstring tstring;
	typedef wstringstream tstringstream;
#else
	typedef string tstring;
	typedef stringstream tstringstream;
#endif
}