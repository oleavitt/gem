/*************************************************************************
*
*  gemreg.c - Registry operations.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"

#define STR_VALUE_MAX 256
static TCHAR GszValue[STR_VALUE_MAX];

void GemRegGetAppFrameState(RECT *pRect, int *pnShow)
{
	HKEY hkey;
	DWORD dwResult;

	/* Fetch window settings from the registry (or create them, if none). */
	hkey = WinUtil_GetAppRegKey(g_szAppRegRoot, GEM_REG_LAYOUT, &dwResult);

	/* Default app window position and state. */
	pRect->left = pRect->top = 2;
	pRect->right = pRect->left + GetSystemMetrics(SM_CXSCREEN) - 4;
	pRect->bottom = pRect->top + GetSystemMetrics(SM_CYSCREEN) * 3 / 4;
	*pnShow = 0;
	splitpos = 6667;
	if (dwResult == REG_CREATED_NEW_KEY)
	{
		/* Write app window position and state. */
		wsprintf(GszValue,
			_T("%d %d %d %d %d %d"),
			pRect->left,
			pRect->top,
			pRect->right,
			pRect->bottom,
			*pnShow, splitpos);
		WinUtil_RegSetStringValue(hkey, GEM_REG_LAYOUT_APP_FRAME, GszValue); 
	}
	else if (WinUtil_RegGetStringValue(hkey, GEM_REG_LAYOUT_APP_FRAME,
		GszValue, STR_VALUE_MAX))
	{
		/* Read app window position and state. */
		_stscanf(GszValue,
			_T("%d %d %d %d %d %d"),
			&pRect->left,
			&pRect->top,
			&pRect->right,
			&pRect->bottom,
			pnShow,
			&splitpos);
	}

	RegCloseKey(hkey);
}


void GemRegSaveAppFrameState(HWND hwnd)
{
	HKEY hkey;
	DWORD dwResult;
	RECT rect;
	int nShow;

	/* Fetch registry key for layout (or create it, if not there). */
	hkey = WinUtil_GetAppRegKey(g_szAppRegRoot, GEM_REG_LAYOUT, &dwResult);

	if (IsIconic(hwnd) || IsZoomed(hwnd))
	{
		/* Default app window position and state. */
		rect.left = rect.top = 2;
		rect.right = rect.left + GetSystemMetrics(SM_CXSCREEN) - 4;
		rect.bottom = rect.top + GetSystemMetrics(SM_CYSCREEN) * 3 / 4;
		nShow = 0;
		if (dwResult != REG_CREATED_NEW_KEY)
		{
			int dummyint;
			if (WinUtil_RegGetStringValue(hkey, GEM_REG_LAYOUT_APP_FRAME,
				GszValue, STR_VALUE_MAX))
			{
				/* Read last saved window position and state. */
				_stscanf(GszValue,
					_T("%d %d %d %d %d %d"),
					&rect.left,
					&rect.top,
					&rect.right,
					&rect.bottom,
					&nShow,
					&dummyint);
			}
		}
	}
	else
	{
		GetWindowRect(hwnd, &rect);
	}

	/* Write app window position and state. */
	wsprintf(GszValue, _T("%d %d %d %d %d %d"), rect.left, rect.top,
		rect.right, rect.bottom, IsZoomed(hwnd), splitpos);
	WinUtil_RegSetStringValue(hkey, GEM_REG_LAYOUT_APP_FRAME, GszValue);

	RegCloseKey(hkey);
}

void GemRegGetSearchPaths()
{
	HKEY hkey;
	DWORD dwResult;
	/* Fetch registry key for search paths (or create it, if not there). */
	hkey = WinUtil_GetAppRegKey(g_szAppRegRoot, GEM_REG_PATHS, &dwResult);
	if (dwResult != REG_CREATED_NEW_KEY)
	{
		WinUtil_RegGetStringValue(hkey, GEM_REG_PATHS_SEARCH_PATHS,
			pathsdlg_paths, PATHSDLG_PATHS_MAX);
	}
	RegCloseKey(hkey);
}

void GemRegSaveSearchPaths()
{
	HKEY hkey;
	DWORD dwResult;
	/* Fetch registry key for search paths (or create it, if not there). */
	hkey = WinUtil_GetAppRegKey(g_szAppRegRoot, GEM_REG_PATHS, &dwResult);
	/* Write paths to registry. */
	WinUtil_RegSetStringValue(hkey, GEM_REG_PATHS_SEARCH_PATHS, pathsdlg_paths);
	RegCloseKey(hkey);
}
