/*************************************************************************
*
*	aboutdlg.c - Handles the About dialog.
*
*	TODO: Rotating rendered "Gem" icon!
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"
/*
BOOL CALLBACK AboutDlgProc(
	HWND hdlg,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	static TCHAR sz[256];

	switch (msg)
	{
		case WM_INITDIALOG:
			wsprintf(sz, _T("Version: %d.%d"), CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR);
			SetDlgItemText(hdlg, IDC_GEM_ABOUT_VERSION, sz); 
			wsprintf(sz, _T("Build: %s"), g_szAppBuild);
			SetDlgItemText(hdlg, IDC_GEM_ABOUT_BUILDINFO, sz); 
			WinUtil_CenterWindow(hdlg, GetWindow(hdlg, GW_OWNER));
			return 1;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					EndDialog(hdlg, 0);
					return 1;
			}
			break;
	}

	return 0;
}
*/
//
//  FUNCTION: About(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for "About" dialog box
//       This version allows greater flexibility over the contents of the 'About' box,
//       by pulling out values from the 'Version' resource.
//
//  MESSAGES:
//
// WM_INITDIALOG - initialize dialog box
// WM_COMMAND    - Input received
//
//
BOOL CALLBACK AboutDlgProc(
	HWND hDlg,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
/*
	static HFONT hfontDlg;    // Font for dialog text
	static HFONT hFinePrint;  // Font for 'fine print' in dialog
	DWORD	dwVerInfoSize;     // Size of version information block
	LPSTR	lpVersion;         // String pointer to 'version' text
	DWORD	dwVerHnd=0;        // An 'ignored' parameter, always '0'
	UINT	uVersionLen;
	int		nRootLen;
	BOOL	bRetCode;
	int		i;
	TCHAR	szFullPath[256];
	TCHAR	szResult[256];
	TCHAR	szGetName[256];
	DWORD	dwVersion;
	TCHAR	szVersion[256];
	DWORD	dwResult;
	OSVERSIONINFOEX	osvi;
	BOOL	bOsVersionInfoEx = TRUE;
	BOOL	bSuccess;
*/
	switch (message) {
		case WM_INITDIALOG:
#if 0
			ShowWindow (hDlg, SW_HIDE);

			if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE)
			{
				hfontDlg = CreateFont(16, 0, 0, 0, 0, 0, 0, 0,
					SHIFTJIS_CHARSET, 0, 0, 0,
					VARIABLE_PITCH | FF_DONTCARE,
					_T(""));
				hFinePrint = CreateFont(14, 0, 0, 0, 0, 0, 0, 0,
					SHIFTJIS_CHARSET, 0, 0, 0,
					VARIABLE_PITCH | FF_DONTCARE,
					_T(""));
			}
			else
			{
				hfontDlg = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					ANTIALIASED_QUALITY,
					VARIABLE_PITCH | FF_SWISS,
					_T(""));
				hFinePrint = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					ANTIALIASED_QUALITY,
					VARIABLE_PITCH | FF_SWISS,
					_T(""));
			}

			WinUtil_CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
			GetModuleFileName (hInst, szFullPath, sizeof(szFullPath));

			// Now lets dive in and pull out the version information:
			dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
			if (dwVerInfoSize) {
				LPTSTR   lpstrVffInfo;
				HANDLE  hMem;
				hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
				lpstrVffInfo  = GlobalLock(hMem);
				GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo);
				// The below 'hex' value looks a little confusing, but
				// essentially what it is, is the hexidecimal representation
				// of a couple different values that represent the language
				// and character set that we are wanting string values for.
				// 040904E4 is a very common one, because it means:
				//   US English, Windows MultiLingual characterset
				// Or to pull it all apart:
				// 04------        = SUBLANG_ENGLISH_USA
				// --09----        = LANG_ENGLISH
				// --11----        = LANG_JAPANESE
				// ----04E4 = 1252 = Codepage for Windows:Multilingual

				lstrcpy(szGetName, WinUtil_GetStringRes(IDS_VER_INFO_LANG));

				nRootLen = lstrlen(szGetName); // Save this position

				// Set the title of the dialog:
				lstrcat (szGetName, "ProductName");
				bRetCode = VerQueryValue((LPVOID)lpstrVffInfo,
					szGetName,
					(LPVOID)&lpVersion,
					(UINT *)&uVersionLen);

				// Notice order of version and string...
				if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE)
				{
					lstrcpy(szResult, lpVersion);
					lstrcat(szResult, _T(" ÇÃÉoÅ[ÉWÉáÉìèÓïÒ"));
				}
				else
				{
					lstrcpy(szResult, _T("About "));
					lstrcat(szResult, lpVersion);
				}

				// -----------------------------------------------------

				SetWindowText (hDlg, szResult);

				// Walk through the dialog items that we want to replace:
				for (i = IDC_COMPANY; i <= IDC_TRADEMARK; i++) {
					GetDlgItemText(hDlg, i, szResult, sizeof(szResult));
					szGetName[nRootLen] = '\0';
					lstrcat(szGetName, szResult);
					uVersionLen = 0;
					lpVersion = NULL;
					bRetCode = VerQueryValue((LPVOID)lpstrVffInfo,
						szGetName,
						(LPVOID)&lpVersion,
						(UINT *)&uVersionLen);

					if (bRetCode && uVersionLen && lpVersion) {
						// Replace dialog item text with version info
						lstrcpy(szResult, lpVersion);
						SetDlgItemText(hDlg, i, szResult);
					}
					else
					{
						dwResult = GetLastError();

						wsprintf(szResult, WinUtil_GetStringRes(IDS_VERSION_ERROR), dwResult);
						SetDlgItemText(hDlg, i, szResult);
					}
					SendMessage(GetDlgItem(hDlg, i), WM_SETFONT,
						(UINT)((i==IDC_TRADEMARK)?hFinePrint:hfontDlg),
						TRUE);
				} // for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++)


				GlobalUnlock(hMem);
				GlobalFree(hMem);

			} else {
				// No version information available.
			} // if (dwVerInfoSize)

			SendMessage (GetDlgItem (hDlg, IDC_LABEL), WM_SETFONT,
			(WPARAM)hfontDlg,(LPARAM)TRUE);

/*
			// We are  using GetVersion rather then GetVersionEx
			// because earlier versions of Windows NT and Win32s
			// didn't include GetVersionEx:
			dwVersion = GetVersion();

			if (dwVersion < 0x80000000) {
				// Windows NT
				wsprintf (szVersion, _T("Microsoft Windows NT %u.%u (Build: %u)"),
					(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))),
					(DWORD)(HIWORD(dwVersion)) );
			} else if (LOBYTE(LOWORD(dwVersion))<4) {
				// Win32s
				wsprintf (szVersion, _T("Microsoft Win32s %u.%u (Build: %u)"),
					(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))),
					(DWORD)(HIWORD(dwVersion) & ~0x8000) );
			} else {
				// Windows 95
				wsprintf (szVersion, _T("Microsoft Windows 95 %u.%u"),
					(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))) );
			}
*/
			szVersion[0] = '\0';
			szResult[0] = '\0';
			osvi.dwOSVersionInfoSize = sizeof(osvi);
			bSuccess = GetVersionEx(&osvi);
			if (!bSuccess)
			{
				// OSVERSIONINFOEX not supported.
				osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
				bSuccess = GetVersionEx(&osvi);
				bOsVersionInfoEx = FALSE;
			}

			if (bSuccess)
			{
				switch (osvi.dwPlatformId)
				{
					case VER_PLATFORM_WIN32_NT:

						// Test for the product.
						if (osvi.dwMajorVersion <= 4)
							lstrcpy(szResult, _T("Microsoft Windows NT "));
						else if (osvi.dwMajorVersion == 5)
						{
							if (osvi.dwMinorVersion < 1)
								lstrcpy(szResult, _T("Microsoft Windows 2000 "));
							else
								lstrcpy(szResult, _T("Microsoft Windows XP "));
						}
						// Test for workstation versus server.
						if( bOsVersionInfoEx )
						{
							if (osvi.wProductType == VER_NT_WORKSTATION)
								lstrcat(szResult, _T("Professional"));
							else if (osvi.wProductType == VER_NT_SERVER)
								lstrcat(szResult, _T("Server"));
						}
						else
						{
							// Older version of NT
							HKEY hKey;
							TCHAR szProductType[80];
							DWORD dwBufLen;

							RegOpenKeyEx(HKEY_LOCAL_MACHINE,
								_T("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
								0, KEY_QUERY_VALUE, &hKey);
							RegQueryValueEx(hKey, _T("ProductType"), NULL, NULL,
								(LPBYTE)szProductType, &dwBufLen);
							RegCloseKey(hKey);
							if (lstrcmpi(_T("WINNT"), szProductType) == 0)
								lstrcat(szResult, _T("Workstation"));
							if (lstrcmpi(_T("SERVERNT"), szProductType) == 0)
								lstrcat(szResult, _T("Server"));
						}

						// Display version, service pack (if any), and build number.
						wsprintf(szVersion, _T("%s\nversion %d.%d (Build %d)\n%s"),
							szResult,
							osvi.dwMajorVersion,
							osvi.dwMinorVersion,
							osvi.dwBuildNumber,
							osvi.szCSDVersion);
						break;

					case VER_PLATFORM_WIN32_WINDOWS:
						if ((osvi.dwMajorVersion > 4) || 
							((osvi.dwMajorVersion == 4) &&
							(osvi.dwMinorVersion > 0)))
							lstrcpy(szResult, _T("Microsoft Windows 98 "));
						else
							lstrcpy(szResult, _T("Microsoft Windows 95 "));
						// Display version, service pack (if any), and build number.
						wsprintf(szVersion, _T("%s\nversion %d.%d (Build %d)\n%s"),
							szResult,
							osvi.dwMajorVersion,
							osvi.dwMinorVersion,
							LOWORD(osvi.dwBuildNumber),
							osvi.szCSDVersion);
						break;

					case VER_PLATFORM_WIN32s:
						lstrcpy(szVersion, _T("Microsoft Win32s "));
						break;
				}
   			}
			SetWindowText(GetDlgItem(hDlg, IDC_OSVERSION), szVersion);

			wsprintf(szResult, _T("Build: %s"), g_szAppBuild);
			SetDlgItemText(hDlg, IDC_GEM_ABOUT_BUILDINFO, szResult); 

			ShowWindow (hDlg, SW_SHOW);
#endif
			return (TRUE);

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
				EndDialog(hDlg, TRUE);
//				DeleteObject (hfontDlg);
//				DeleteObject (hFinePrint);
				return (TRUE);
			}
			break;
	}

	/* Not used */
	lParam;

    return FALSE;
}
