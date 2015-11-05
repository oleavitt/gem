/*************************************************************************
*
*  renddlg.c - Handles the Render|Settings dialog.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"

int renddlg_use_scn_res;
int renddlg_preview_mode;
int renddlg_output_width;
int renddlg_output_height;
int renddlg_aa_none;
int renddlg_aa_adaptive;
int renddlg_aa_threshold;
int renddlg_aa_depth;
int renddlg_jitter_on;
int renddlg_jitter_percent;
int renddlg_animation_on;
int renddlg_start_frame;
int renddlg_end_frame;

static void RendDlgSaveState(void)
{
	HKEY hkey = NULL;
	DWORD dwResult;

	/* Fetch window settings from the registry (or create them, if none). */
	if ((hkey = WinUtil_GetAppRegKey(g_szAppRegRoot, GEM_REG_RENDERER, &dwResult)) != NULL)
	{
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_WIDTH, renddlg_output_width);
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_HEIGHT, renddlg_output_height);
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_PREVIEW, renddlg_preview_mode);
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_USESCNRES, renddlg_use_scn_res);
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_AA, renddlg_aa_adaptive);
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_AATHRESH, renddlg_aa_threshold);
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_AADEPTH, renddlg_aa_depth);
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_JITTER, renddlg_jitter_on);
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_JITTERAMT, renddlg_jitter_percent);
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_ANIM, renddlg_animation_on);
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_ANIMSTART, renddlg_start_frame);
		WinUtil_RegSetIntValue(hkey, GEM_REG_RENDERER_ANIMEND, renddlg_end_frame);
		RegCloseKey(hkey);
	}
}

BOOL RendDlgColdStart(void)
{
	HKEY hkey;
	DWORD dwResult;

	/*
	 * Load Render|Settings dialog settings.
	 */
	renddlg_use_scn_res = 0;
	renddlg_preview_mode = 1;
	renddlg_output_width = 160;
	renddlg_output_height = 120;
	renddlg_aa_none = 1;
	renddlg_aa_adaptive = 0;
	renddlg_aa_threshold = 5;
	renddlg_aa_depth = 3;
	renddlg_jitter_on = 0;
	renddlg_jitter_percent = 20;
	renddlg_animation_on = 0;
	renddlg_start_frame = 1;
	renddlg_end_frame = 1;

	/* Fetch window settings from the registry (or create them, if none). */
	if ((hkey = WinUtil_GetAppRegKey(g_szAppRegRoot, GEM_REG_RENDERER, &dwResult)) != NULL)
	{
		if (dwResult == REG_CREATED_NEW_KEY)
		{
			RegCloseKey(hkey);
			RendDlgSaveState();
			return TRUE;
		}
		else
		{
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_WIDTH, &renddlg_output_width);
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_HEIGHT, &renddlg_output_height);
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_PREVIEW, &renddlg_preview_mode);
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_USESCNRES, &renddlg_use_scn_res);
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_AA, &renddlg_aa_adaptive);
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_AATHRESH, &renddlg_aa_threshold);
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_AADEPTH, &renddlg_aa_depth);
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_JITTER, &renddlg_jitter_on);
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_JITTERAMT, &renddlg_jitter_percent);
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_ANIM, &renddlg_animation_on);
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_ANIMSTART, &renddlg_start_frame);
			WinUtil_RegGetIntValue(hkey, GEM_REG_RENDERER_ANIMEND, &renddlg_end_frame);
		}
		RegCloseKey(hkey);
	}

	return TRUE;
}

static void CheckState(HWND hdlg, UINT idCtrl)
{
	BOOL bResult;
	switch (idCtrl)
	{
		case 0: /* Check all */
			CheckState(hdlg, IDC_RENDERER_USE_SCN_RES);
			CheckState(hdlg, IDC_RENDERER_ANIMATION);
			CheckState(hdlg, IDC_RENDERER_AA_JITTER);
			CheckState(hdlg, IDC_RENDERER_AA_NONE);
			break;
		case IDC_RENDERER_USE_SCN_RES:
			bResult = ISCHECKED(hdlg, IDC_RENDERER_USE_SCN_RES);
			EnableWindow(GetDlgItem(hdlg, IDC_RENDERER_WIDTH), !bResult);
			EnableWindow(GetDlgItem(hdlg, IDC_RENDERER_HEIGHT), !bResult);
			break;
		case IDC_RENDERER_ANIMATION:
			bResult = ISCHECKED(hdlg, IDC_RENDERER_ANIMATION);
			EnableWindow(GetDlgItem(hdlg, IDC_RENDERER_START_FRAME), bResult);
			EnableWindow(GetDlgItem(hdlg, IDC_RENDERER_END_FRAME), bResult);
			break;
		case IDC_RENDERER_AA_JITTER:
			EnableWindow(GetDlgItem(hdlg, IDC_RENDERER_AA_JIT_AMT), 
				ISCHECKED(hdlg, IDC_RENDERER_AA_JITTER));
			break;
		case IDC_RENDERER_AA_NONE:
		case IDC_RENDERER_AA_ADAPT:
			bResult = ISCHECKED(hdlg, IDC_RENDERER_AA_ADAPT);
			EnableWindow(GetDlgItem(hdlg, IDC_RENDERER_AA_THRESHOLD), bResult);
			EnableWindow(GetDlgItem(hdlg, IDC_RENDERER_AA_DEPTH), bResult);
			break;
	}
}

static void ValidateChanges(HWND hdlg, UINT idCtrl, UINT idEvent)
{
	int value;
	BOOL bResult;

	if (idEvent != EN_CHANGE)
		return;

	value = (int)GetDlgItemInt(hdlg, idCtrl, &bResult, 0);
	switch (idCtrl)
	{
		case IDC_RENDERER_WIDTH:
		case IDC_RENDERER_HEIGHT:
		case IDC_RENDERER_START_FRAME:
		case IDC_RENDERER_END_FRAME:
			EnableWindow(GetDlgItem(hdlg, IDOK), bResult && value > 0);
			break;
		case IDC_RENDERER_AA_DEPTH:
			EnableWindow(GetDlgItem(hdlg, IDOK), bResult && value > 0 && value <= MAX_AA_DEPTH);
			break;
		case IDC_RENDERER_AA_THRESHOLD:
		case IDC_RENDERER_AA_JIT_AMT:
			EnableWindow(GetDlgItem(hdlg, IDOK), bResult && value >= 0);
			break;
	}
}

BOOL CALLBACK RendDlgProc(HWND hdlg, UINT msg,
	WPARAM wParam, LPARAM lParam)
{
	BOOL bResult;
	int itmp;

	switch (msg)
	{
		case WM_INITDIALOG:
			WinUtil_CenterWindow(hdlg, GetWindow(hdlg, GW_OWNER));
			/* Read in current settings. */
			SETCHECK(hdlg, IDC_RENDERER_ANIMATION, renddlg_animation_on);
			SetDlgItemInt(hdlg, IDC_RENDERER_START_FRAME,
				(UINT)renddlg_start_frame, 0);
			SetDlgItemInt(hdlg, IDC_RENDERER_END_FRAME,
				(UINT)renddlg_end_frame, 0);
			SETCHECK(hdlg, IDC_RENDERER_USE_SCN_RES, renddlg_use_scn_res);
			SETCHECK(hdlg, IDC_RENDERER_PREVIEW, renddlg_preview_mode);
			SetDlgItemInt(hdlg, IDC_RENDERER_WIDTH, (UINT)renddlg_output_width, 0);
			SetDlgItemInt(hdlg, IDC_RENDERER_HEIGHT, (UINT)renddlg_output_height, 0);
			if(renddlg_aa_adaptive)
				SETCHECK(hdlg, IDC_RENDERER_AA_ADAPT, TRUE);
			else
				SETCHECK(hdlg, IDC_RENDERER_AA_NONE, TRUE);
			SetDlgItemInt(hdlg, IDC_RENDERER_AA_THRESHOLD, (UINT)renddlg_aa_threshold, 0);
			SetDlgItemInt(hdlg, IDC_RENDERER_AA_DEPTH, (UINT)renddlg_aa_depth, 0);
			SETCHECK(hdlg, IDC_RENDERER_AA_JITTER, renddlg_jitter_on);
			SetDlgItemInt(hdlg, IDC_RENDERER_AA_JIT_AMT, (UINT)renddlg_jitter_percent, 0);
			CheckState(hdlg, 0);
			return 1;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_RENDERER_USE_SCN_RES:
				case IDC_RENDERER_AA_NONE:
				case IDC_RENDERER_AA_ADAPT:
				case IDC_RENDERER_AA_JITTER:
				case IDC_RENDERER_ANIMATION:
					CheckState(hdlg, LOWORD(wParam));
					break;

				case IDC_RENDERER_WIDTH:
				case IDC_RENDERER_HEIGHT:
				case IDC_RENDERER_AA_THRESHOLD:
				case IDC_RENDERER_AA_DEPTH:
				case IDC_RENDERER_AA_JIT_AMT:
				case IDC_RENDERER_START_FRAME:
				case IDC_RENDERER_END_FRAME:
					ValidateChanges(hdlg, LOWORD(wParam), HIWORD(wParam));
					break;

				case IDOK:
					/* Save dialog settings. */
					renddlg_animation_on = ISCHECKED(hdlg, IDC_RENDERER_ANIMATION);
					itmp = (int)GetDlgItemInt(hdlg, IDC_RENDERER_START_FRAME,
						&bResult, 0);
					if (bResult)
						renddlg_start_frame = max(itmp, 1);
					itmp = (int)GetDlgItemInt(hdlg, IDC_RENDERER_END_FRAME,
						&bResult, 0);
					if (bResult)
						renddlg_end_frame = max(itmp, 1);
					renddlg_use_scn_res = ISCHECKED(hdlg, IDC_RENDERER_USE_SCN_RES);
					renddlg_preview_mode = ISCHECKED(hdlg, IDC_RENDERER_PREVIEW);
					itmp = (int)GetDlgItemInt(hdlg, IDC_RENDERER_WIDTH, &bResult, 0);
					if (bResult)
						renddlg_output_width = itmp;
					itmp = (int)GetDlgItemInt(hdlg, IDC_RENDERER_HEIGHT, &bResult, 0);
					if (bResult)
						renddlg_output_height = itmp;
					if (ISCHECKED(hdlg, IDC_RENDERER_AA_NONE))
					{
						renddlg_aa_none = 1;
						renddlg_aa_adaptive = 0;
					}
					else
					{
						renddlg_aa_none = 0;
						renddlg_aa_adaptive = 1;
					}
					itmp = (int)GetDlgItemInt(hdlg, IDC_RENDERER_AA_THRESHOLD, &bResult, 0);
					if (bResult)
						renddlg_aa_threshold = min(max(itmp, 0), 255);
					itmp = (int)GetDlgItemInt(hdlg, IDC_RENDERER_AA_DEPTH, &bResult, 0);
					if (bResult)
						renddlg_aa_depth = min(max(itmp, 1), MAX_AA_DEPTH);
					renddlg_jitter_on = ISCHECKED(hdlg, IDC_RENDERER_AA_JITTER);
					itmp = (int)GetDlgItemInt(hdlg, IDC_RENDERER_AA_JIT_AMT, &bResult, 0);
					if (bResult)
						renddlg_jitter_percent = itmp;
					if (renddlg_end_frame < renddlg_start_frame)
					{
						itmp = renddlg_end_frame;
						renddlg_end_frame = renddlg_start_frame;
						renddlg_start_frame = itmp;
					}
					RendDlgSaveState();
					/* fall through */
				case IDCANCEL: /* Ignore dialog settings. */
					EndDialog(hdlg, 0);
					return 1;
			}
			break;
	}

	/* Not used */
	lParam;

	return 0;
}
