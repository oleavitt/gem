/*************************************************************************
*
*	winutil.h - Misc. utility functions for working with Windows.
*
*************************************************************************/

#ifndef __WINUTIL_H__
#define __WINUTIL_H__

/* Centers a window within another window. */
extern void WinUtil_CenterWindow(
	HWND hwnd,
	HWND hwndCenterIn);

/* Dialog control helpers */

/* Returns true if a check box or radio button is checked. */
#define ISCHECKED(hwnd, id) \
	(SendDlgItemMessage(hwnd, id, BM_GETCHECK, 0, 0) == BST_CHECKED)

/* Sets a check box or radio button. */
#define	SETCHECK(hwnd, id, checked) \
	(SendDlgItemMessage(hwnd, id, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0))

/* Registry helpers. */
/*
 * Open a registry key from the standard HKEY_CURRENT_USER\Software section.
 * If key doesn't exist, a new key is created.
 */
extern HKEY WinUtil_GetAppRegKey(
	LPCTSTR lpszAppRoot,
	LPCTSTR lpszKeyName,
	LPDWORD lpdwResult);

/* Save a string value to registry key. */
extern BOOL WinUtil_RegSetStringValue(
	HKEY hkey,
	LPCTSTR lpszEntry,
	LPCTSTR lpszValue);

/* Get a string value from registry key. */
extern BOOL WinUtil_RegGetStringValue(
	HKEY hkey,
	LPCTSTR lpszEntry,
	LPTSTR lpszValue,
	int nValueMaxSize);

/* Save an int value to registry key. */
extern BOOL WinUtil_RegSetIntValue(
	HKEY hkey,
	LPCTSTR lpszEntry,
	int nValue);

/* Get an int value from registry key. */
extern BOOL WinUtil_RegGetIntValue(
	HKEY hkey,
	LPCTSTR lpszEntry,
	int *pnValue);

/*
 * Load a string resource and return a ptr to a static buffer
 * containing string.
 */
extern LPTSTR WinUtil_GetStringRes(
	int id);

#endif /* __WINUTIL_H__ */