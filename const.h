///
///	const.h:
///		Defines common constants.
/// 

#pragma once
#include <windows.h>

#define WC_MAINWND TEXT("wt_mainWindowCls")
#define STATUS_SUCCESS 0x00000000

// button help text (old)
#define TEXT_BUTTONOK_HELP TEXT("Applies the changes and closes WinTweak.")
#define TEXT_BUTTONCANCEL_HELP TEXT("Discards the changes and closes WinTweak.")
#define TEXT_BUTTONAPPLY_HELP TEXT("Applies the changes without closing WinTweak.")

// registry hkey shortcuts
#define HKLM HKEY_LOCAL_MACHINE
#define HKCR HKEY_CLASSES_ROOT
#define HKCU HKEY_CURRENT_USER
#define HKU HKEY_USERS
#define HKCC HKEY_CURRENT_CONFIG
#define HKPD HKEY_PERFORMANCE_DATA
#define HKPN HKEY_PERFORMANCE_NLSTEXT
#define HKPT HKEY_PERFORMANCE_TEXT

// number bases
#define NUMBASE_DECIMAL 10
#define NUMBASE_HEXADECIMAL 16
#define NUMBASE_OCTAL 8

// td icons
#define TD_SHIELDWARNING_ICON 65530
#define TD_SHIELDERROR_ICON 65529
#define TD_SHIELDSUCCESS_ICON 65528
// gray/blue banners but regular shield
#define TD_SHIELDGRAY_ICON 65527
#define TD_SHIELDBLUE_ICON 65531

// td buttons
#define TDCBF_CONTINUE_BUTTON 0x80000

// misc constants
#define MUI_ALL_INSTALLED_LANGUAGES 0x20
#define WINTWEAK_CONFIG_REG_ROOTKEY TEXT("Software\\FireBlade\\WinTweak") // hkcu

// dialog messages
#define DLGMESSAGE_LOGOFF TEXT("You need to log off to apply the changes. Do you want to log off now?")
#define DLGMESSAGE_REBOOT TEXT("You need to reboot the system to apply the changes. Do you want to reboot now?")