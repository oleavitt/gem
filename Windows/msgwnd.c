/*************************************************************************
*
*  msgwnd.c - Message output window proc.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"

typedef struct tagMsgWndData
{
	LPCTSTR lpszMsg;
	HWND hlbwnd;
	int line_num;
} MSGWNDDATA, *LPMSGWNDDATA;

TCHAR szMsgWndClassName[] = _T("GemMsgWnd");

static int MsgWndOnCreate(HWND hwnd, LPCREATESTRUCT lpcs); /* WM_CREATE */
static void MsgWndOnSize(HWND hwnd, int cx, int cy);  /* WM_SIZE */
static void MsgWndOnDestroy(HWND hwnd); /* WM_DESTROY */

LRESULT CALLBACK MsgWndProc(HWND, UINT, WPARAM, LPARAM);


BOOL MsgWndRegisterClass(void)
{
	WNDCLASSEX wndclass;

	/* Define the app frame window class. */
	wndclass.cbSize				= sizeof(WNDCLASSEX);
	wndclass.style				= 0;
	wndclass.lpfnWndProc		= MsgWndProc;
	wndclass.cbClsExtra			= 0;
	wndclass.cbWndExtra			= sizeof(HANDLE); /* ptr to local window info. */
	wndclass.hInstance			= hInst;
	wndclass.hIcon				= LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground		= (HBRUSH)(COLOR_WINDOW + 1);
	wndclass.lpszMenuName		= NULL;
	wndclass.lpszClassName		= szMsgWndClassName;
	wndclass.hIconSm			= LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wndclass))
		return FALSE;

	return TRUE;
}


HWND MsgWndCreate(HWND hwndParent)
{
	return CreateWindowEx(
		WS_EX_CLIENTEDGE,		/* extended window style */
		szMsgWndClassName,		/* window class name */
		_T(""),					/* window caption */
		WS_CHILD,				/* window style */
		0,						/* initial x position */
		0,						/* initial y position */
		0,						/* initial x size */
		0,						/* initial y size */
		hwndParent,				/* parent window handle */
		(HMENU)CHILD_ID_MSGWND,	/* window menu handle */
		hInst,					/* program instance handle */
		NULL);					/* creation parameters */
}


LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT imsg, WPARAM wParam, LPARAM lParam)
{
	switch (imsg)
	{
		case WM_CREATE:
			return MsgWndOnCreate(hwnd, (LPCREATESTRUCT)lParam);

		case WM_SIZE:
			MsgWndOnSize(hwnd, LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_DESTROY:
			MsgWndOnDestroy(hwnd);
			return 0;
	}

	return DefWindowProc(hwnd, imsg, wParam, lParam);
}


int MsgWndOnCreate(HWND hwnd, LPCREATESTRUCT lpcs)
{
	LOGFONT lf;
	HFONT hfont;
	LPMSGWNDDATA pmwd = (LPMSGWNDDATA)HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY, sizeof(MSGWNDDATA));
	if (pmwd == NULL)
		return -1;
	
	SetWindowLong(hwnd, 0, (long)pmwd);

	if ((pmwd->hlbwnd = CreateWindow(
		_T("LISTBOX"),
		_T(""),
		WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
		0, 0, 0, 0,
		hwnd,
		(HMENU)CHILD_ID_MSGWNDLISTBOX,
		hInst,
		NULL)) == NULL)
		return -1; 
	
	/* Use a fixed-pitch font for the list box control. */
	lf.lfHeight         = 13;
	lf.lfWidth          = 0;
	lf.lfEscapement     = 0;
	lf.lfOrientation    = 0;
	lf.lfWeight         = FW_DONTCARE;
	lf.lfItalic         = FALSE;
	lf.lfUnderline      = FALSE;
	lf.lfStrikeOut      = FALSE;
	lf.lfCharSet        = ANSI_CHARSET;
	lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	lf.lfQuality        = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
	/* Let the system pick a font that best fits our request. */
	lf.lfFaceName[0]    = 0;

	hfont = CreateFontIndirect(&lf);
	if(hfont == NULL)
		return -1;

	SendMessage(pmwd->hlbwnd, WM_SETFONT, (WPARAM)hfont, 0L);

	/* Not used */
	lpcs;

	return 0;
}


void MsgWndOnDestroy(HWND hwnd)
{
	LPMSGWNDDATA pmwd = (LPMSGWNDDATA)GetWindowLong(hwnd, 0);	
	if(pmwd != NULL)
	{
		HFONT hfont = (HFONT)SendMessage(pmwd->hlbwnd, WM_GETFONT, 0, 0L);
		DeleteObject(hfont);
		HeapFree(GetProcessHeap(), 0, pmwd);
	}
}


void MsgWndOnSize(HWND hwnd, int cx, int cy)
{
	RECT rect;
	LPMSGWNDDATA pmwd = (LPMSGWNDDATA)GetWindowLong(hwnd, 0);
	assert(pmwd != NULL);
	GetClientRect(hwnd, &rect);
	MoveWindow(pmwd->hlbwnd, rect.left, rect.top,
		rect.right, rect.bottom, TRUE);

	/* Not used */
	cx; cy;
}


void MsgWndClear(HWND hwnd)
{
	LPMSGWNDDATA pmwd = (LPMSGWNDDATA)GetWindowLong(hwnd, 0);
	assert(pmwd != NULL);
	SendMessage(pmwd->hlbwnd, LB_RESETCONTENT, 0, 0);
}


void MsgWndMessage(LPCTSTR lpszMsg)
{
	int result;
	LPMSGWNDDATA pmwd = (LPMSGWNDDATA)GetWindowLong(hwndMsgWnd, 0);
	assert(pmwd != NULL);
	result = SendMessage(pmwd->hlbwnd, LB_ADDSTRING, 0, (LPARAM)lpszMsg);
	if (result == LB_ERRSPACE)   /* burp! */
	{
		SendMessage(pmwd->hlbwnd, LB_RESETCONTENT, 0, 0);
		SendMessage(pmwd->hlbwnd, LB_ADDSTRING, 0, (LPARAM)lpszMsg);
	}
}


void MsgWndVMessage(const LPCTSTR szFmt, ...)
{
	static TCHAR szMsg[256];
	va_list ap;
	va_start(ap, szFmt);
	wsprintf(szMsg, szFmt, ap);
	va_end(ap);
	MsgWndMessage(szMsg);
}


