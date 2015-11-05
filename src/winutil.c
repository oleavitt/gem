/*************************************************************************
*
*	winutil.c - Misc. utility functions for working with Windows.
*
*************************************************************************/

#include "gempch.h"
#include "winutil.h"


/* Centers a window within another window. */
void WinUtil_CenterWindow(HWND hwnd,
						  HWND hwndCenterIn)
{
	RECT rectWnd;
	int x, y, cxWnd, cyWnd, cxScreen, cyScreen;
	
	assert(IsWindow(hwnd));

	GetWindowRect(hwnd, &rectWnd);
	cxWnd = rectWnd.right - rectWnd.left;
	cyWnd = rectWnd.bottom - rectWnd.top;
	cxScreen = GetSystemMetrics(SM_CXSCREEN); 
	cyScreen = GetSystemMetrics(SM_CYSCREEN); 

	/*
	 * If window that we're centering in is minimized or invisible,
	 * center window to screen.
	 */
	if (hwndCenterIn != NULL)
	{
		DWORD dwStyle = GetWindowLong(hwndCenterIn, GWL_STYLE);

		assert(IsWindow(hwndCenterIn));
		if (!(dwStyle & WS_VISIBLE) || (dwStyle & WS_MINIMIZE))
			hwndCenterIn = NULL;
	}

	if (hwndCenterIn != NULL)
	{
		RECT rectCenterIn;
		GetWindowRect(hwndCenterIn, &rectCenterIn);
		x = rectCenterIn.left;
		y = rectCenterIn.top;
		ScreenToClient(hwndCenterIn, (LPPOINT)&rectCenterIn.left);
		ScreenToClient(hwndCenterIn, (LPPOINT)&rectCenterIn.right);
		x += ((rectCenterIn.right - rectCenterIn.left) - cxWnd) / 2;
		y += ((rectCenterIn.bottom - rectCenterIn.top) - cyWnd) / 2;

		/* Keep window within screen area. */
		if (x < 0) x = 0;
		if (x > (cxScreen - cxWnd)) x = cxScreen - cxWnd;
		if (y < 0) y = 0;
		if (y > (cyScreen - cyWnd)) y = cyScreen - cyWnd;
	}
	else	/* Center to screen. */
	{
		x = (cxScreen - cxWnd) / 2;
		y = (cyScreen - cyWnd) / 2;
		if (x < 0) x = 0;
		if (y < 0) y = 0;
	}

	SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}


/* Registry helpers. */
/*
 * Open a registry key from the standard HKEY_CURRENT_USER\Software section.
 * If key doesn't exist, a new key is created. Key status is returned in lpdwResult.
 * Caller is responsible for closing key with RegCloseKey().
 */
HKEY WinUtil_GetAppRegKey(LPCTSTR lpszAppRoot,
						  LPCTSTR lpszKeyName,
						  LPDWORD lpdwResult)
{
	HKEY hkeySoftware = NULL;
	HKEY hkeyAppRoot = NULL;
	HKEY hkey = NULL;

	assert(lpszAppRoot != NULL);
	assert(lpszKeyName != NULL);

	if (RegOpenKeyEx(
		HKEY_CURRENT_USER,
		_T("Software"),
		0,
		KEY_READ | KEY_WRITE,
		&hkeySoftware) == ERROR_SUCCESS)
	{
		if (RegCreateKeyEx(
				hkeySoftware,
				lpszAppRoot,
				0,
				REG_NONE,
				REG_OPTION_NON_VOLATILE,
				KEY_READ | KEY_WRITE,
				NULL,
				&hkeyAppRoot,
				lpdwResult) == ERROR_SUCCESS)
		{
			RegCreateKeyEx(
				hkeyAppRoot,
				lpszKeyName,
				0,
				REG_NONE,
				REG_OPTION_NON_VOLATILE,
				KEY_READ | KEY_WRITE,
				NULL,
				&hkey,
				lpdwResult);
			RegCloseKey(hkeyAppRoot);
		}
		RegCloseKey(hkeySoftware);
	}

	return hkey;
}

/* Save a string value to registry key. */
BOOL WinUtil_RegSetStringValue(HKEY hkey,
							   LPCTSTR lpszEntry,
							   LPCTSTR lpszValue)
{
	return (RegSetValueEx(
		hkey,
		lpszEntry,
		0,
		REG_SZ,
		(LPBYTE)lpszValue,
		(lstrlen(lpszValue) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS);
}

/* Get a string value from registry key. */
BOOL WinUtil_RegGetStringValue(HKEY hkey,
							   LPCTSTR lpszEntry,
							   LPTSTR lpszValue,
							   int nValueMaxSize)
{
	DWORD dwValueType, dwValueSize;

	if (hkey != NULL &&
		RegQueryValueEx(
			hkey,
			(LPTSTR)lpszEntry,
			NULL,
			&dwValueType,
			NULL,
			&dwValueSize) == ERROR_SUCCESS)
	{
		assert(dwValueType == REG_SZ);

		if (dwValueSize > nValueMaxSize * sizeof(TCHAR))
			dwValueSize = nValueMaxSize * sizeof(TCHAR);
		return (RegQueryValueEx(
			hkey,
			lpszEntry,
			NULL,
			&dwValueType,
			(LPBYTE)lpszValue,
			&dwValueSize) == ERROR_SUCCESS);
	}

	return FALSE;
}

/* Save an int value to registry key. */
BOOL WinUtil_RegSetIntValue(
	HKEY hkey,
	LPCTSTR lpszEntry,
	int nValue)
{
	DWORD dwValue = (DWORD)nValue;
	return (RegSetValueEx(hkey, lpszEntry, 0, REG_DWORD,
		(LPBYTE)&dwValue, sizeof(DWORD)) == ERROR_SUCCESS);
}

/* Get an int value from registry key. */
BOOL WinUtil_RegGetIntValue(
	HKEY hkey,
	LPCTSTR lpszEntry,
	int *pnValue)
{
	DWORD dwValue, dwValueType, dwValueSize;

	dwValueSize = sizeof(DWORD);
	if (hkey != NULL &&
		RegQueryValueEx(
			hkey,
			(LPTSTR)lpszEntry,
			NULL,
			&dwValueType,
			(LPBYTE)&dwValue,
			&dwValueSize) == ERROR_SUCCESS)
	{
		assert(dwValueType == REG_DWORD);
		assert(dwValueSize == sizeof(DWORD));
		*pnValue = (int)dwValue;
		return TRUE;
	}

	return FALSE;
}

/* 
 * Load the resource string with the ID given, and return a
 * pointer to it. Notice that the buffer is common memory so
 * the string must be used before this call is made a second time.
 */
LPTSTR WinUtil_GetStringRes(int id)
{
  static TCHAR buffer[MAX_PATH];

  buffer[0]=0;
  LoadString(GetModuleHandle(NULL), id, buffer, MAX_PATH);

  return buffer;
}
