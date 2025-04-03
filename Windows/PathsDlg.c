/*************************************************************************
*
*  pathsdlg.c - Handles the Search Paths dialog.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"

/* TODO: Should use an array of path strings and a Combo Box for the UI. */
/* TODO: Browse button. */
/* Maximum length of search paths string. */
#define PATHSDLG_PATHS_MAX 1024
/* String containing search paths. */
TCHAR pathsdlg_paths[PATHSDLG_PATHS_MAX] = _T("");

BOOL PathsDlgColdStart(void)
{
	pathsdlg_paths[0] = '\0';
	return TRUE;
}


BOOL CALLBACK PathsDlgProc(HWND hdlg, UINT msg,
	WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			/* Read in current settings. */
			SetDlgItemText(hdlg, IDC_EDIT_SEARCH_PATHS, pathsdlg_paths);
			return 1;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					/* Save dialog settings. */
					GetDlgItemText(hdlg, IDC_EDIT_SEARCH_PATHS, pathsdlg_paths,
						PATHSDLG_PATHS_MAX);
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
