///
///	padvappearance.cpp:
///		Provides functions for the Appearance (advanced) property sheet page.
/// 

#include "padvappearance.h"
#include <commctrl.h>
#include "resource.h"
#include <htmlhelp.h>
#include "macro.h"
#include "utility.h"
#include "const.h"
#include <tchar.h>
#include <strsafe.h>
#include <shlobj.h>

LRESULT CALLBACK AdvAppearancePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		UpDown_SetRange32(GetDlgItem(hwnd, IDC_SPIN_ICONSPACEH), GetSystemMetrics(SM_CXICON), 182);
		UpDown_SetRange32(GetDlgItem(hwnd, IDC_SPIN_ICONSPACEV), GetSystemMetrics(SM_CYICON), 182);
		TrackBar_SetRange(GetDlgItem(hwnd, IDC_SLIDER_ICONSPACEH), TRUE, GetSystemMetrics(SM_CXICON), 182);
		TrackBar_SetRange(GetDlgItem(hwnd, IDC_SLIDER_ICONSPACEV), TRUE, GetSystemMetrics(SM_CYICON), 182);

		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_COMBOANIMTEST), TEXT("Preview"));
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_COMBOANIMTEST), TEXT("Item 1"));
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_COMBOANIMTEST), TEXT("Item 2"));
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_COMBOANIMTEST), TEXT("Item 3"));
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_COMBOANIMTEST), TEXT("Item 4"));
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_COMBOANIMTEST), TEXT("Item 5"));

		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_LISTBOXSCROLLPREVIEW), TEXT("Preview"));
		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_LISTBOXSCROLLPREVIEW), TEXT("Item 1"));
		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_LISTBOXSCROLLPREVIEW), TEXT("Item 2"));
		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_LISTBOXSCROLLPREVIEW), TEXT("Item 3"));
		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_LISTBOXSCROLLPREVIEW), TEXT("Item 4"));
		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_LISTBOXSCROLLPREVIEW), TEXT("Item 5"));

		TrackBar_SetRange(GetDlgItem(hwnd, IDC_SLIDER_SBWIDTH), TRUE, 8, 100);
		TrackBar_SetRange(GetDlgItem(hwnd, IDC_SLIDER_SBBTNSIZE), TRUE, 8, 100);
	
		SetDlgItemText(hwnd, IDC_COMBO_COMBOANIMTEST, TEXT("Preview"));

		INT iconHSpacing;
		
		if (SystemParametersInfo(SPI_ICONHORIZONTALSPACING, NULL, &iconHSpacing, 0))
		{
			TrackBar_SetPos(GetDlgItem(hwnd, IDC_SLIDER_ICONSPACEH), TRUE, iconHSpacing);
			UpDown_SetPos32(GetDlgItem(hwnd, IDC_SPIN_ICONSPACEH), iconHSpacing);
		}

		INT iconVSpacing;
		
		if (SystemParametersInfo(SPI_ICONVERTICALSPACING, NULL, &iconVSpacing, 0))
		{
			TrackBar_SetPos(GetDlgItem(hwnd, IDC_SLIDER_ICONSPACEV), TRUE, iconVSpacing);
			UpDown_SetPos32(GetDlgItem(hwnd, IDC_SPIN_ICONSPACEV), iconVSpacing);
		}

		BOOL isComboAnimEnabled;

		if (SystemParametersInfo(SPI_GETCOMBOBOXANIMATION, NULL, &isComboAnimEnabled, 0))
		{
			CheckDlgButton(hwnd, IDC_CHECK_COMBOOPENANIM, isComboAnimEnabled ? BST_CHECKED : BST_UNCHECKED);
		}

		BOOL isLBSmoothScroll;

		if (SystemParametersInfo(SPI_GETLISTBOXSMOOTHSCROLLING, NULL, &isLBSmoothScroll, 0))
		{
			CheckDlgButton(hwnd, IDC_CHECK_LISTBOXSMOOTHSCROLL, isLBSmoothScroll ? BST_CHECKED : BST_UNCHECKED);
		}

		BOOL isSelFade;

		if (SystemParametersInfo(SPI_GETSELECTIONFADE, NULL, &isSelFade, 0))
		{
			CheckDlgButton(hwnd, IDC_CHECK_MENUSELFADE, isSelFade ? BST_CHECKED : BST_UNCHECKED);
		}

		BOOL tooltipFade;
		
		if (SystemParametersInfo(SPI_GETTOOLTIPANIMATION, NULL, &tooltipFade, 0))
		{
			if (tooltipFade)
			{
				BOOL isFade;

				if (SystemParametersInfo(SPI_GETTOOLTIPFADE, NULL, &isFade, 0))
				{
					if (isFade) CheckDlgButton(hwnd, IDC_RADIO_TOOLTIPANIM_FADE, BST_CHECKED);
					else CheckDlgButton(hwnd, IDC_RADIO_TOOLTIPANIM_SLIDE, BST_CHECKED);
				}
			}
			else
			{
				CheckDlgButton(hwnd, IDC_RADIO_TOOLTIPANIM_NONE, BST_CHECKED);
			}
		}

		BOOL hotTrack;

		if (SystemParametersInfo(SPI_GETHOTTRACKING, NULL, &hotTrack, 0))
		{
			CheckDlgButton(hwnd, IDC_CHECK_HOTTRACKEFFECTS, hotTrack ? BST_CHECKED : BST_UNCHECKED);
		}

		BOOL menuFade;

		if (SystemParametersInfo(SPI_GETMENUANIMATION, NULL, &menuFade, 0))
		{
			if (menuFade)
			{
				BOOL isFade;

				if (SystemParametersInfo(SPI_GETMENUFADE, NULL, &isFade, 0))
				{
					if (isFade) CheckDlgButton(hwnd, IDC_RADIO_MENUANIM_FADE, BST_CHECKED);
					else CheckDlgButton(hwnd, IDC_RADIO_MENUANIM_SLIDE, BST_CHECKED);
				}
			}
			else
			{
				CheckDlgButton(hwnd, IDC_RADIO_MENUANIM_NONE, BST_CHECKED);
			}
		}

		ANIMATIONINFO ai = {};
		ai.cbSize = sizeof(ANIMATIONINFO);

		if (SystemParametersInfo(SPI_GETANIMATION, sizeof(ANIMATIONINFO), &ai, 0))
		{
			CheckDlgButton(hwnd, IDC_CHECK_USERESTOREANIM, ai.iMinAnimate != 0 ? BST_CHECKED : BST_UNCHECKED);
		}

		TCHAR buffer[64];
		DWORD cb = sizeof(buffer);

		LONG result = RegGetValue(
			HKCU,
			TEXT("Control Panel\\Desktop\\WindowMetrics"),
			TEXT("ScrollWidth"),
			RRF_RT_REG_SZ,
			NULL,
			(PVOID)buffer,
			&cb);

		if (result == ERROR_SUCCESS)
		{
			// Ensure null termination manually
			size_t chars = cb / sizeof(TCHAR);
			if (chars == 0)
				chars = 1;
			buffer[chars - 1] = 0;

			long lScrollWidthInternal = _tcstol(buffer, NULL, 10);
			long lScrollWidth = lScrollWidthInternal / -15; // get the "real" value

			TrackBar_SetPos(GetDlgItem(hwnd, IDC_SLIDER_SBWIDTH), TRUE, lScrollWidth);
			UpDown_SetPos32(GetDlgItem(hwnd, IDC_SPIN_SBWIDTH), lScrollWidth);
		}
		else if (result != ERROR_FILE_NOT_FOUND)
		{
			ShowRegistryError(hwnd, REDMF_READ, TEXT("HKCU\\Control Panel\\Desktop\\WindowMetrics"), TEXT("ScrollWidth"), result);
		}

		cb = sizeof(buffer);

		result = RegGetValue(
			HKCU,
			TEXT("Control Panel\\Desktop\\WindowMetrics"),
			TEXT("ScrollHeight"),
			RRF_RT_REG_SZ,
			NULL,
			(PVOID)buffer,
			&cb);

		if (result == ERROR_SUCCESS)
		{
			// Ensure null termination manually
			size_t chars = cb / sizeof(TCHAR);
			if (chars == 0)
				chars = 1;
			buffer[chars - 1] = 0;

			long lScrollHeightInternal = _tcstol(buffer, NULL, 10);
			long lScrollHeight = lScrollHeightInternal / -15; // get the "real" value

			TrackBar_SetPos(GetDlgItem(hwnd, IDC_SLIDER_SBBTNSIZE), TRUE, lScrollHeight);
			UpDown_SetPos32(GetDlgItem(hwnd, IDC_SPIN_SBBTNSIZE), lScrollHeight);
		}
		else if (result != ERROR_FILE_NOT_FOUND)
		{
			ShowRegistryError(hwnd, REDMF_READ, TEXT("HKCU\\Control Panel\\Desktop\\WindowMetrics"), TEXT("ScrollWidth"), result);
		}

		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_LIST_LISTBOXSCROLLPREVIEW:
		case IDC_COMBO_COMBOANIMTEST:
			break;
		case IDC_EDIT_SBWIDTH:
		case IDC_EDIT_SBBTNSIZE:
		case IDC_EDIT_ICONSPACEH:
		case IDC_EDIT_ICONSPACEV:
		{
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				PropSheet_Changed(GetParent(hwnd), hwnd);
				break;
			case EN_KILLFOCUS:
			{
				TCHAR bufText[32] = TEXT("");
				GetDlgItemText(hwnd, LOWORD(wParam), bufText, 32);

				int value = _tstoi(bufText);
				TrackBar_SetPos(GetDlgItem(hwnd, LOWORD(wParam) == IDC_EDIT_SBWIDTH
					? IDC_SLIDER_SBWIDTH
					: LOWORD(wParam) == IDC_EDIT_SBBTNSIZE
					? IDC_SLIDER_SBBTNSIZE
					: LOWORD(wParam) == IDC_EDIT_ICONSPACEH
					? IDC_SLIDER_ICONSPACEH
					: LOWORD(wParam) == IDC_EDIT_ICONSPACEV
					? IDC_SLIDER_ICONSPACEV
					: 0), TRUE, value);

				UpDown_SetPos32(GetDlgItem(hwnd, LOWORD(wParam) == IDC_EDIT_SBWIDTH
					? IDC_SPIN_SBWIDTH
					: LOWORD(wParam) == IDC_EDIT_SBBTNSIZE
					? IDC_SPIN_SBBTNSIZE
					: LOWORD(wParam) == IDC_EDIT_ICONSPACEH
					? IDC_SPIN_ICONSPACEH
					: LOWORD(wParam) == IDC_EDIT_ICONSPACEV
					? IDC_SPIN_ICONSPACEV
					: 0), value);

				break;
			}
			}

			break;
		}

		case IDC_BUTTON_FONTCHANGE_ICONS:
		case IDC_BUTTON_FONTCHANGE_MENUS:
		case IDC_BUTTON_FONTCHANGE_MESSAGES:
		case IDC_BUTTON_FONTCHANGE_STATUSBARS:
		case IDC_BUTTON_FONTCHANGE_SMCAPTION:
		case IDC_BUTTON_FONTCHANGE_TITLEBARS:
		{

			LOGFONT lf = {};

			DWORD cb = sizeof(lf);
			LPCWSTR lpValName = LOWORD(wParam) == IDC_BUTTON_FONTCHANGE_ICONS
				? TEXT("IconFont")
				: LOWORD(wParam) == IDC_BUTTON_FONTCHANGE_MENUS
				? TEXT("MenuFont")
				: LOWORD(wParam) == IDC_BUTTON_FONTCHANGE_MESSAGES
				? TEXT("MessageFont")
				: LOWORD(wParam) == IDC_BUTTON_FONTCHANGE_STATUSBARS
				? TEXT("StatusFont")
				: LOWORD(wParam) == IDC_BUTTON_FONTCHANGE_SMCAPTION
				? TEXT("SmCaptionFont")
				: LOWORD(wParam) == IDC_BUTTON_FONTCHANGE_TITLEBARS
				? TEXT("CaptionFont")
				: TEXT("0");

			if (_tcscmp(lpValName, TEXT("0")) == 0) break;

			LONG result = RegGetValue(HKCU, TEXT("Control Panel\\Desktop\\WindowMetrics"), lpValName, RRF_RT_REG_BINARY, NULL, &lf, &cb);
			
			CHOOSEFONT cf = {};
			cf.Flags = CF_EFFECTS | CF_SCREENFONTS | CF_ENABLEHOOK | CF_INITTOLOGFONTSTRUCT;
			cf.hwndOwner = hwnd;
			cf.lpLogFont = &lf;
			cf.lStructSize = sizeof(CHOOSEFONT);
			cf.lpfnHook = [](HWND hDlg, UINT fuMsg, WPARAM fwParam, LPARAM flParam) -> UINT_PTR CALLBACK {
				switch (fuMsg)
				{
				case WM_INITDIALOG:
				{
					HWND hwndCtrl = GetDlgItem(hDlg, cmb4);
					ShowWindow(hwndCtrl, SW_HIDE);

					hwndCtrl = GetDlgItem(hDlg, stc4);
					ShowWindow(hwndCtrl, SW_HIDE);
					break;
				}
					}

				return FALSE;
				};

			if (ChooseFont(&cf))
			{
				result = RegSetKeyValue(HKCU, TEXT("Control Panel\\Desktop\\WindowMetrics"), lpValName, REG_BINARY, &lf, sizeof(lf));
				
				if (result == ERROR_SUCCESS)
					RestartDialog(hwnd, TEXT("You need to log off to apply the changes. Do you want to log off now?"), EWX_LOGOFF);
			}
			break;
		}
		default:
			PropSheet_Changed(GetParent(hwnd), hwnd);
			break;
		}

		break;
	}
	case WM_HSCROLL:
	{
		switch (GetDlgCtrlID((HWND)lParam))
		{
		case IDC_SLIDER_ICONSPACEH:
		{
			int value = TrackBar_GetPos((HWND)lParam);
			UpDown_SetPos32(GetDlgItem(hwnd, IDC_SPIN_ICONSPACEH), value);

			break;
		}
		case IDC_SLIDER_SBWIDTH:
		{
			int value = TrackBar_GetPos((HWND)lParam);
			UpDown_SetPos32(GetDlgItem(hwnd, IDC_SPIN_SBWIDTH), value);

			break;
		}
		case IDC_SLIDER_SBBTNSIZE:
		{
			int value = TrackBar_GetPos((HWND)lParam);
			UpDown_SetPos32(GetDlgItem(hwnd, IDC_SPIN_SBBTNSIZE), value);

			break;
		}
		}

		break;
	}
	case WM_VSCROLL:
	{
		switch (GetDlgCtrlID((HWND)lParam))
		{
		case IDC_SLIDER_ICONSPACEV:
		{
			int value = TrackBar_GetPos((HWND)lParam);
			UpDown_SetPos32(GetDlgItem(hwnd, IDC_SPIN_ICONSPACEV), value);

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
		case PSN_APPLY:
		{
			BOOL restartExplorer = FALSE;
			BOOL logoff = FALSE;
			int iconSpaceH = TrackBar_GetPos(GetDlgItem(hwnd, IDC_SLIDER_ICONSPACEH));
			int iconSpaceV = TrackBar_GetPos(GetDlgItem(hwnd, IDC_SLIDER_ICONSPACEV));

			ICONMETRICS im = {};
			im.cbSize = sizeof(ICONMETRICS);
			
			SystemParametersInfo(SPI_GETICONMETRICS, sizeof(ICONMETRICS), &im, 0);

			if (im.iHorzSpacing != iconSpaceH)
			{
				restartExplorer = TRUE;
			}

			if (im.iVertSpacing != iconSpaceV)
			{
				restartExplorer = TRUE;
			}

			im.iHorzSpacing = iconSpaceH;
			im.iVertSpacing = iconSpaceV;

			SystemParametersInfo(SPI_SETICONMETRICS, sizeof(ICONMETRICS), &im, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			SystemParametersInfo(SPI_SETICONS, 0, NULL, SPIF_SENDWININICHANGE); // reload icons

			SystemParametersInfo(SPI_SETCOMBOBOXANIMATION, NULL, (PVOID)(IsDlgButtonChecked(hwnd, IDC_CHECK_COMBOOPENANIM) == BST_CHECKED ? TRUE : FALSE),
				SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

			SystemParametersInfo(SPI_SETLISTBOXSMOOTHSCROLLING, NULL, (PVOID)(IsDlgButtonChecked(hwnd, IDC_CHECK_LISTBOXSMOOTHSCROLL) == BST_CHECKED ? TRUE : FALSE),
				SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

			SystemParametersInfo(SPI_SETSELECTIONFADE, NULL, (PVOID)(IsDlgButtonChecked(hwnd, IDC_CHECK_MENUSELFADE) == BST_CHECKED ? TRUE : FALSE),
				SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

			if (IsDlgButtonChecked(hwnd, IDC_RADIO_TOOLTIPANIM_NONE) == BST_CHECKED)
			{
				SystemParametersInfo(SPI_SETTOOLTIPANIMATION, NULL, FALSE,
					SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
			else if (IsDlgButtonChecked(hwnd, IDC_RADIO_TOOLTIPANIM_FADE) == BST_CHECKED)
			{
				SystemParametersInfo(SPI_SETTOOLTIPANIMATION, NULL, (PVOID)TRUE,
					SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

				SystemParametersInfo(SPI_SETTOOLTIPFADE, NULL, (PVOID)TRUE,
					SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
			else if (IsDlgButtonChecked(hwnd, IDC_RADIO_TOOLTIPANIM_SLIDE) == BST_CHECKED)
			{
				SystemParametersInfo(SPI_SETTOOLTIPANIMATION, NULL, (PVOID)TRUE,
					SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

				SystemParametersInfo(SPI_SETTOOLTIPFADE, NULL, FALSE,
					SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}

			SystemParametersInfo(SPI_SETHOTTRACKING, NULL,
				(PVOID)(IsDlgButtonChecked(hwnd, IDC_CHECK_HOTTRACKEFFECTS) == BST_CHECKED ? TRUE : FALSE), SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

			if (IsDlgButtonChecked(hwnd, IDC_RADIO_MENUANIM_NONE) == BST_CHECKED)
			{
				SystemParametersInfo(SPI_SETMENUANIMATION, NULL, FALSE,
					SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
			else if (IsDlgButtonChecked(hwnd, IDC_RADIO_MENUANIM_FADE) == BST_CHECKED)
			{
				SystemParametersInfo(SPI_SETMENUANIMATION, NULL, (PVOID)TRUE,
					SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

				SystemParametersInfo(SPI_SETMENUFADE, NULL, (PVOID)TRUE,
					SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
			else if (IsDlgButtonChecked(hwnd, IDC_RADIO_MENUANIM_SLIDE) == BST_CHECKED)
			{
				SystemParametersInfo(SPI_SETMENUANIMATION, NULL, (PVOID)TRUE,
					SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

				SystemParametersInfo(SPI_SETMENUFADE, NULL, FALSE,
					SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}

			ANIMATIONINFO ai = {};
			ai.cbSize = sizeof(ANIMATIONINFO);
			ai.iMinAnimate = IsDlgButtonChecked(hwnd, IDC_CHECK_USERESTOREANIM); // bst checked is 0x0001, unchecked is 0x0000

			SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &ai, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

			long lOldScrollWidth = 0;
			long lOldScrollHeight = 0;
			TCHAR buffer[64];
			DWORD cb = sizeof(buffer);

			LONG result = RegGetValue(
				HKCU,
				TEXT("Control Panel\\Desktop\\WindowMetrics"),
				TEXT("ScrollWidth"),
				RRF_RT_REG_SZ,
				NULL,
				(PVOID)buffer,
				&cb);

			if (result == ERROR_SUCCESS)
			{
				// Ensure null termination manually
				size_t chars = cb / sizeof(TCHAR);
				if (chars == 0)
					chars = 1;
				buffer[chars - 1] = 0;

				lOldScrollWidth = _tcstol(buffer, NULL, 10);
			}
			else if (result != ERROR_FILE_NOT_FOUND)
			{
				ShowRegistryError(hwnd, REDMF_READ, TEXT("HKCU\\Control Panel\\Desktop\\WindowMetrics"), TEXT("ScrollWidth"), result);
			}

			cb = sizeof(buffer);

			result = RegGetValue(
				HKCU,
				TEXT("Control Panel\\Desktop\\WindowMetrics"),
				TEXT("ScrollHeight"),
				RRF_RT_REG_SZ,
				NULL,
				(PVOID)buffer,
				&cb);

			if (result == ERROR_SUCCESS)
			{
				// Ensure null termination manually
				size_t chars = cb / sizeof(TCHAR);
				if (chars == 0)
					chars = 1;
				buffer[chars - 1] = 0;

				lOldScrollHeight = _tcstol(buffer, NULL, 10);
			}

			int newSbWidth = TrackBar_GetPos(GetDlgItem(hwnd, IDC_SLIDER_SBWIDTH));
			TCHAR newSBFullWidth[32] = TEXT("");
			_itot_s(-15 * newSbWidth, newSBFullWidth, 32, NUMBASE_DECIMAL);
			result = RegSetKeyValue(HKCU, TEXT("Control Panel\\Desktop\\WindowMetrics"), TEXT("ScrollWidth"), REG_SZ, newSBFullWidth, sizeof(newSBFullWidth));

			int newSbHeight = TrackBar_GetPos(GetDlgItem(hwnd, IDC_SLIDER_SBBTNSIZE));
			TCHAR newSBFullHeight[32] = TEXT("");
			_itot_s(-15 * newSbHeight, newSBFullHeight, 32, NUMBASE_DECIMAL);
			result = RegSetKeyValue(HKCU, TEXT("Control Panel\\Desktop\\WindowMetrics"), TEXT("ScrollHeight"), REG_SZ, newSBFullHeight, sizeof(newSBFullHeight));

			if (newSbWidth != lOldScrollWidth) logoff = TRUE;
			if (newSbHeight != lOldScrollHeight) logoff = TRUE;


			if (logoff) RestartDialog(hwnd, DLGMESSAGE_LOGOFF, EWX_LOGOFF);
			if (restartExplorer) RestartExplorerDialog(hwnd);

			break;
		}
		case UDN_DELTAPOS:
		{
			LPNMUPDOWN pNmUpDown = (LPNMUPDOWN)lParam;
			int value = pNmUpDown->iPos + pNmUpDown->iDelta;

			switch (nmhdr->idFrom)
			{
			case IDC_SPIN_ICONSPACEV:
			{
				TrackBar_SetPos(GetDlgItem(hwnd, IDC_SLIDER_ICONSPACEV), TRUE, value);
				break;
			}
			case IDC_SPIN_SBWIDTH:
			{
				TrackBar_SetPos(GetDlgItem(hwnd, IDC_SLIDER_SBWIDTH), TRUE, value);
				break;
			}
			case IDC_SPIN_SBBTNSIZE:
			{
				TrackBar_SetPos(GetDlgItem(hwnd, IDC_SLIDER_SBBTNSIZE), TRUE, value);
				break;
			}
			case IDC_SPIN_ICONSPACEH:
			{
				TrackBar_SetPos(GetDlgItem(hwnd, IDC_SLIDER_ICONSPACEH), TRUE, value);
				break;
			}
			}

			return FALSE;
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
		case IDC_BUTTON_FONTCHANGE_ICONS:
			hhp.pszText = TEXT("Customize the font of Desktop and Explorer icons.");
			break;
		case IDC_BUTTON_FONTCHANGE_MENUS:
			hhp.pszText = TEXT("Change the font of menus and menu bars.");
			break;
		case IDC_BUTTON_FONTCHANGE_MESSAGES:
			hhp.pszText = TEXT("Change the font of message boxes. Unfortunately, many modern apps do not support these settings, but some like OpenVPN and Firefox still support it.");
			break;
		case IDC_BUTTON_FONTCHANGE_STATUSBARS:
			hhp.pszText = TEXT("Change the font of statusbars, e.g. the Notepad statusbar font. It will be also applied to tooltips in many apps. Note that many apps just not support this option.For example, Explorer will ignore these font settings.");
			break;
		case IDC_BUTTON_FONTCHANGE_SMCAPTION:
			hhp.pszText = TEXT("Change the font of small title bars.");
			break;
		case IDC_BUTTON_FONTCHANGE_TITLEBARS:
			hhp.pszText = TEXT("Change the title bar font.");
			break;
		case IDC_CHECK_THEMEALLOWPOINTERCHANGE:
			hhp.pszText = TEXT("Allows or prevents Windows themes from changing mouse pointers.");
			break;
		case IDC_CHECK_COMBOOPENANIM:
			hhp.pszText = TEXT("Enables or disables the slide-open effect for drop-downs.");
			break;
		case IDC_CHECK_HOTTRACKEFFECTS:
			hhp.pszText = TEXT("Enables or disables hot tracking of user-interface elements such as menu names on menu bars. Hot-tracking means that when the cursor moves over an item, it is highlighted but not selected.");
			break;
		case IDC_CHECK_LISTBOXSMOOTHSCROLL:
			hhp.pszText = TEXT("Enables or disables the smooth-scrolling effect for list boxes.");
			break;
		case IDC_CHECK_MENUSELFADE:
			hhp.pszText = TEXT("Enables or disables the selection fade effect. The selection fade effect causes the menu item selected by the user to remain on the screen briefly while fading out after the menu is dismissed. The selection fade effect is possible only if the system has a color depth of more than 256 colors.");
			break;
		case IDC_CHECK_USERESTOREANIM:
			hhp.pszText = TEXT("Enables or disables the minimize and restore animations.");
			break;
		}

		HtmlHelp((HWND)lphi->hItemHandle, NULL, HH_DISPLAY_TEXT_POPUP, (DWORD_PTR)&hhp);

		break;
	}
	}

	return FALSE;
}