/*************************************************************************
*
*  split.c - Split bar window functions.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"

typedef struct tagSplitWndData
{
	int pos, oldpos;
	int lo, hi;
	int snapdist;
} SPLITWNDDATA, *LPSPLITWNDDATA;

TCHAR szSplitWndClassName[] = _T("GemSplitWnd");

static int SplitWndOnCreate(HWND hwnd, LPCREATESTRUCT lpcs); /* WM_CREATE */
static void SplitWndOnLButtonDown(HWND hwnd); /* WM_LBUTTONDOWN */
static void SplitWndOnLButtonUp(HWND hwnd); /* WM_LBUTTONUP */
static void SplitWndOnDestroy(HWND hwnd); /* WM_DESTROY */

LRESULT CALLBACK SplitWndProc(HWND, UINT, WPARAM, LPARAM);


BOOL SplitWndRegisterClass(void)
{
	WNDCLASSEX wndclass;

	/* Define the app frame window class. */
	wndclass.cbSize              = sizeof(WNDCLASSEX);
	wndclass.style               = 0;
	wndclass.lpfnWndProc         = SplitWndProc;
	wndclass.cbClsExtra          = 0;
	wndclass.cbWndExtra          = sizeof(HANDLE); /* ptr to local window info. */
	wndclass.hInstance           = hInst;
	wndclass.hIcon               = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(hInst, MAKEINTRESOURCE(IDC_SPLITCURSOR));
	wndclass.hbrBackground       = (HBRUSH)(COLOR_ACTIVEBORDER + 1);
	wndclass.lpszMenuName        = NULL;
	wndclass.lpszClassName       = szSplitWndClassName;
	wndclass.hIconSm             = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wndclass))
		return FALSE;

	return TRUE;
}


HWND SplitWndCreate(HWND hwndParent, int id)
{
  return CreateWindowEx(
    0,                          /* extended window style */
    szSplitWndClassName,        /* window class name */
    _T(""),                     /* window caption */
    WS_CHILD,                   /* window style */
    0,                          /* initial x position */
    0,                          /* initial y position */
    0,                          /* initial x size */
    0,                          /* initial y size */
    hwndParent,                 /* parent window handle */
    (HMENU)id,                  /* window menu handle or child id */
    hInst,                      /* program instance handle */
    NULL);                      /* creation parameters */
}


LRESULT CALLBACK SplitWndProc(HWND hwnd, UINT imsg, WPARAM wParam, LPARAM lParam)
{
	switch (imsg)
	{
		case WM_CREATE:
			return SplitWndOnCreate(hwnd, (LPCREATESTRUCT)lParam);

		case WM_LBUTTONDOWN:
			SplitWndOnLButtonDown(hwnd);
			return 0;

		case WM_LBUTTONUP:
			SplitWndOnLButtonUp(hwnd);
			return 0;

		case WM_DESTROY:
			SplitWndOnDestroy(hwnd);
			return 0;
	}

	return DefWindowProc(hwnd, imsg, wParam, lParam);
}


int SplitWndOnCreate(HWND hwnd, LPCREATESTRUCT lpcs)
{
	LPSPLITWNDDATA pd = (LPSPLITWNDDATA)HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY, sizeof(SPLITWNDDATA));
	if (pd == NULL)
		return -1;
	pd->oldpos = -1;
	SetWindowLong(hwnd, 0, (long)pd);

	/* Not used */
	lpcs;

	return 0;
}


void SplitWndOnDestroy(HWND hwnd)
{
	LPSPLITWNDDATA pd = (LPSPLITWNDDATA)GetWindowLong(hwnd, 0);	
	if (pd != NULL)
	{
		HeapFree(GetProcessHeap(), 0, pd);
	}
}


void SplitWndOnLButtonDown(HWND hwnd)
{
	SetCapture(hwnd);
}

void SplitWndOnLButtonUp(HWND hwnd)
{
	if (GetCapture() == hwnd)
	{
		ReleaseCapture();
	}
}

