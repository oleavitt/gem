/*************************************************************************
*
*  gemfile.c - Application-level file management functions.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"


static OPENFILENAME ofn;


void GemFileInitialize(HWND hwnd)
{
	static TCHAR szFilter[] =
		_T("Scene Files (*.SCN)\0*.scn\0")
		_T("Neutral File Format (*.NFF)\0*.nff\0")
		_T("All Files (*.*)\0*.*\0\0");

	ofn.lStructSize			= sizeof(OPENFILENAME);
	ofn.hwndOwner			= hwnd;
	ofn.hInstance			= NULL;
	ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 0;
	ofn.lpstrFile			= NULL;
	ofn.nMaxFile			= _MAX_PATH;
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= _MAX_FNAME + _MAX_EXT;
	ofn.lpstrInitialDir		= NULL;
	ofn.lpstrTitle			= NULL;
	ofn.Flags				= 0;
	ofn.nFileOffset			= 0;
	ofn.nFileExtension		= 0;
	ofn.lpstrDefExt			= _T("scn");
	ofn.lCustData			= 0L;
	ofn.lpfnHook			= NULL;
	ofn.lpTemplateName		= NULL;
#if (_WIN32_WINNT >= 0x0500)
	ofn.pvReserved			= NULL;
	ofn.dwReserved			= 0L;
	ofn.FlagsEx				= 0L;
#endif /* (_WIN32_WINNT >= 0x0500) */
}


int GemFileOpenDlg(HWND hwnd, LPCTSTR lpszFilename, LPCTSTR lpszTitle)
{
	static TCHAR szPath[_MAX_PATH];
	HKEY hkey;
	DWORD dwResult;

	if ((hkey = WinUtil_GetAppRegKey(g_szAppRegRoot, GEM_REG_FILEOPEN, &dwResult)) != NULL)
	{
		if (dwResult != REG_CREATED_NEW_KEY)
		{
			WinUtil_RegGetStringValue(hkey, GEM_REG_FILEOPEN_LAST_PATH, szPath, _MAX_PATH);
			ofn.lpstrInitialDir = szPath;
		}
		RegCloseKey(hkey);
	}

	ofn.hwndOwner			= hwnd;
	ofn.lpstrFile			= (LPTSTR)lpszFilename;
	ofn.lpstrFileTitle		= (LPTSTR)lpszTitle;
	ofn.Flags				= OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&ofn))
	{
		lstrcpy(szPath, ofn.lpstrFile);
		szPath[ofn.nFileOffset] = '\0';
		if ((hkey = WinUtil_GetAppRegKey(g_szAppRegRoot, GEM_REG_FILEOPEN, &dwResult)) != NULL)
		{
			WinUtil_RegSetStringValue(hkey, GEM_REG_FILEOPEN_LAST_PATH, szPath);
			RegCloseKey(hkey);
		}
#if (_WIN32_WINNT < 0x0500)
		/* TODO: Save last visited files. */
#endif /* (_WIN32_WINNT >= 0x0500) */
		switch (ofn.nFilterIndex)
		{
			case 1:    /* SCN file filter */
				return GEMFILE_SCN;
			case 2:    /* NFF file filter */
				return GEMFILE_NFF;
		}
		/*
		 * An explicit filter was not used - look at extension to determine
		 * type. If no extension, assume SCN.
		 */
		if (ofn.nFileExtension && ofn.lpstrFile[ofn.nFileExtension] != '\0')
		{
			TCHAR ext[_MAX_EXT], *ptr;
			int i;
			ptr = ofn.lpstrFile + ofn.nFileExtension;
			for (i = 0; i < _MAX_EXT; i++)
			{
				ext[i] = (TCHAR)_totupper(*ptr);
				if (*ptr == '\0')
					break;
				ptr++;
			}
			if (!_tcscmp(ext, _T("NFF")))
				return GEMFILE_NFF;
		}
		return GEMFILE_SCN;
	}
	return GEMFILE_FAIL;
}

