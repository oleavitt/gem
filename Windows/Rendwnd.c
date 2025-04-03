/*************************************************************************
*
*  rendwnd.c - Renderer view window proc.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"

typedef struct tagRendViewData
{
	HBITMAP hRendBitmap; /* Bitmap to render image to. */
	HDC hRendDC;         /* Memory resident DC to draw pixels to. */
	HDC hRendViewDC;     /* DC for renderer view. */
	BOOL bPreviewMode;   /* TRUE if preview mode is selected. */
	int Width, Height;   /* Image size in pixels. */
} RENDVIEWDATA, *LPRENDVIEWDATA;

TCHAR szRendWndClassName[] = _T("GemRndWnd");

static int RendWndOnCreate(HWND hwnd, LPCREATESTRUCT lpcs); /* WM_CREATE */
static void RendWndOnPaint(HWND hwnd, HDC hdc, RECT *prcPaint);  /* WM_PAINT */
static BOOL RendWndOnErasebkgnd(HWND hwnd, HDC hdc);  /* WM_ERASEBKGND */
static void RendWndOnDestroy(HWND hwnd); /* WM_DESTROY */
static void RendWndOnSize(HWND hwnd, int cx, int cy, UINT uSizeType);
static POINT RendWndGetScrollPos(HWND hwnd);
static void RendWndOnHScroll(HWND hwnd, WORD wScroll, WORD wPos);
static void RendWndOnVScroll(HWND hwnd, WORD wScroll, WORD wPos);

static void RendWndSetScrollBars(HWND hwnd, int xPage, int yPage, int xMax, int yMax);

LRESULT CALLBACK RendWndProc(HWND, UINT, WPARAM, LPARAM);

#define BKGND_SYS_WINDOW_COLOR	COLOR_WINDOW


BOOL RendwndRegisterClass(void)
{
	WNDCLASSEX wndclass;

	/* Define the app frame window class. */
	wndclass.cbSize              = sizeof(WNDCLASSEX);
	wndclass.style               = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc         = RendWndProc;
	wndclass.cbClsExtra          = 0;
	wndclass.cbWndExtra          = sizeof(HANDLE); /* ptr to local window info. */
	wndclass.hInstance           = hInst;
	wndclass.hIcon               = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor             = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground       = (HBRUSH)(BKGND_SYS_WINDOW_COLOR + 1);
	wndclass.lpszMenuName        = NULL;
	wndclass.lpszClassName       = szRendWndClassName;
	wndclass.hIconSm             = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wndclass))
		return FALSE;

	return TRUE;
}


HWND RendWndCreate(HWND hwndParent)
{
	return CreateWindowEx(
		WS_EX_CLIENTEDGE,           /* extended window style */
		szRendWndClassName,         /* window class name */
		_T(""),                     /* window caption */
		WS_CHILD | WS_HSCROLL | WS_VSCROLL, /* window style */
		0,                          /* initial x position */
		0,                          /* initial y position */
		0,                          /* initial x size */
		0,                          /* initial y size */
		hwndParent,                 /* parent window handle */
		(HMENU)CHILD_ID_RENDWND,    /* window menu handle */
		hInst,                      /* program instance handle */
		NULL);                      /* creation parameters */
}


LRESULT CALLBACK RendWndProc(HWND hwnd, UINT imsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;

	switch (imsg)
	{
	case WM_CREATE:
		return RendWndOnCreate(hwnd, (LPCREATESTRUCT) lParam);

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		RendWndOnPaint(hwnd, hdc, &ps.rcPaint);
		EndPaint(hwnd, &ps);
		return 0;

	case WM_ERASEBKGND:
		return RendWndOnErasebkgnd(hwnd, (HDC) wParam);

	case WM_HSCROLL:
		RendWndOnHScroll(hwnd, LOWORD(wParam), HIWORD(wParam));
		return 0;

	case WM_VSCROLL:
		RendWndOnVScroll(hwnd, LOWORD(wParam), HIWORD(wParam));
		return 0;

	case WM_SIZE:
		RendWndOnSize(hwnd, (int) LOWORD(lParam), (int) HIWORD(lParam), (UINT) wParam);
		return 0;

	case WM_DESTROY:
		RendWndOnDestroy(hwnd);
		return 0;
	}

	return DefWindowProc(hwnd, imsg, wParam, lParam);
}


int RendWndOnCreate(HWND hwnd, LPCREATESTRUCT lpcs)
{
	LPRENDVIEWDATA prvd = (LPRENDVIEWDATA)HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY, sizeof(RENDVIEWDATA));
	if (prvd == NULL)
		return -1;
	SetWindowLong(hwnd, 0, (long)prvd);
	prvd->hRendViewDC = GetDC(hwnd);
	prvd->hRendDC = CreateCompatibleDC(prvd->hRendViewDC);
	if (prvd->hRendDC == NULL)
		return -1;
	prvd->bPreviewMode = bPreviewMode;
	/* Will be set when RendWndResizeImage() is called. */
	prvd->hRendBitmap = NULL;

	/* Not used */
	lpcs;

	return 0;
}


void RendWndOnDestroy(HWND hwnd)
{
	LPRENDVIEWDATA prvd = (LPRENDVIEWDATA)GetWindowLong(hwnd, 0);	
	if (prvd != NULL)
	{
		if (prvd->hRendDC != NULL)
			DeleteDC(prvd->hRendDC);
		if (prvd->hRendViewDC != NULL)
			ReleaseDC(hwnd, prvd->hRendViewDC);
		if (prvd->hRendBitmap != NULL)
			DeleteObject(prvd->hRendBitmap);
		HeapFree(GetProcessHeap(), 0, prvd);
	}
}


void RendWndOnPaint(HWND hwnd, HDC hdc, RECT *prcPaint)
{
	LPRENDVIEWDATA prvd = (LPRENDVIEWDATA)GetWindowLong(hwnd, 0);
	
	assert(prvd != NULL);

	if (prvd->hRendDC != NULL)
	{
		POINT ptScroll = RendWndGetScrollPos(hwnd);
		BitBlt(hdc,
			prcPaint->left,
			prcPaint->top,
			prcPaint->right - prcPaint->left,
			prcPaint->bottom - prcPaint->top,
			prvd->hRendDC,
			prcPaint->left + ptScroll.x,
			prcPaint->top + ptScroll.y,
			SRCCOPY);
	}
}


BOOL RendWndOnErasebkgnd(HWND hwnd, HDC hdc)
{
	LPRENDVIEWDATA	prvd = (LPRENDVIEWDATA) GetWindowLong(hwnd, 0);
	RECT			rcClip, rcClient;
	HPEN			hPen, hOldPen;
	
	assert(prvd != NULL);

	if (prvd->hRendDC != NULL)
	{
		GetClipBox(hdc, &rcClip);
		GetClientRect(hwnd, &rcClient);

		hPen = CreatePen(PS_SOLID, 0, GetSysColor(BKGND_SYS_WINDOW_COLOR));
		hOldPen = SelectObject(hdc, hPen);

		/* Fill the area beyond the image width - if any. */
		if (prvd->Width < rcClip.right)
		{
			Rectangle(hdc, prvd->Width, 0, rcClient.right, rcClient.bottom);
		}

		/* Fill the area beyond the image bottom - if any. */
		if (prvd->Height < rcClip.bottom)
		{
			Rectangle(hdc, 0, prvd->Height, rcClient.right, rcClient.bottom);
		}

		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);
	}

	return TRUE;
}


void RendWndOnSize(HWND hwnd, int cx, int cy, UINT uSizeType)
{
	LPRENDVIEWDATA prvd = (LPRENDVIEWDATA) GetWindowLong(hwnd, 0);

	assert(prvd != NULL);

	if (prvd->hRendDC != NULL)
	{
		RendWndSetScrollBars(hwnd, cx, cy, prvd->Width, prvd->Height);
	}

	/* Not used */
	uSizeType;
}

POINT RendWndGetScrollPos(HWND hwnd)
{
	SCROLLINFO si;
	POINT pt;

	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	GetScrollInfo(hwnd, SB_HORZ, &si);
	pt.x = si.nPos;
	GetScrollInfo(hwnd, SB_VERT, &si);
	pt.y = si.nPos;

	return pt;
}

void RendWndOnHScroll(HWND hwnd, WORD wScroll, WORD wPos)
{
	SCROLLINFO si;
	int nPos;

	// Common info
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_HORZ, &si);
	nPos = si.nPos;

	switch (wScroll)
	{
	case SB_LINELEFT:
		si.nPos -= 1;
		break;
	case SB_LINERIGHT:
		si.nPos += 1;
		break;
	case SB_PAGELEFT:
		si.nPos -= si.nPage;
		break;
	case SB_PAGERIGHT:
		si.nPos += si.nPage;
		break;
	case SB_THUMBTRACK:
		si.nPos = si.nTrackPos;
		break;
	default:
		break;
	}

	si.fMask = SIF_POS;
	SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
	GetScrollInfo(hwnd, SB_HORZ, &si);

	if (si.nPos != nPos)
	{
		ScrollWindow(hwnd, nPos - si.nPos, 0, NULL, NULL);
		UpdateWindow(hwnd);
	}

	/* Not used */
	wPos;
} 

void RendWndOnVScroll(HWND hwnd, WORD wScroll, WORD wPos)
{
	SCROLLINFO si;
	int nPos;

	// Common info
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, &si);
	nPos = si.nPos;

	switch (wScroll)
	{
	case SB_TOP:
		si.nPos = si.nMin;
		break;
	case SB_BOTTOM:
		si.nPos = si.nMax;
		break;
	case SB_LINEUP:
		si.nPos -= 1;
		break;
	case SB_LINEDOWN:
		si.nPos += 1;
		break;
	case SB_PAGEUP:
		si.nPos -= si.nPage;
		break;
	case SB_PAGEDOWN:
		si.nPos += si.nPage;
		break;
	case SB_THUMBTRACK:
		si.nPos = si.nTrackPos;
		break;
	default:
		break;
	}

	si.fMask = SIF_POS;
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
	GetScrollInfo(hwnd, SB_VERT, &si);

	if (si.nPos != nPos)
	{
		ScrollWindow(hwnd, 0, nPos - si.nPos, NULL, NULL);
		UpdateWindow(hwnd);
	}

	/* Not used */
	wPos;
}

void RendWndSetScrollBars(HWND hwnd, int xPage, int yPage, int xMax, int yMax)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nMin = 0;
	si.nMax = xMax;
	si.nPage = xPage;
	SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
	si.nMax = yMax;
	si.nPage = yPage;
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

void RendWndUpdateRect(HWND hwnd, RECT *prc)
{
	RECT rect = *prc;
	POINT ptScroll = RendWndGetScrollPos(hwnd);
	OffsetRect(&rect, -ptScroll.x, -ptScroll.y);
	InvalidateRect(hwndRendView, &rect, FALSE);
}

void RendWndSetPixel(HWND hwnd, Rend2DPixel *pixel)
{
	if (hwnd != NULL)
	{
		LPRENDVIEWDATA prvd = (LPRENDVIEWDATA)GetWindowLong(hwnd, 0);
		assert(prvd != NULL);
		assert(prvd->hRendDC != NULL);
		if (pixel->width > 0)
		{
			HBRUSH hBrush;
			RECT rect;
			rect.left = pixel->x;
			rect.top = pixel->y;
			rect.right = pixel->x + pixel->width;
			rect.bottom = pixel->y + pixel->height;
			hBrush = CreateSolidBrush(RGB(pixel->r, pixel->g, pixel->b));
			FillRect(prvd->hRendDC, &rect, hBrush);
			DeleteObject(hBrush);
		}
	}
}


BOOL RendWndResizeImage(HWND hwnd, int width, int height)
{
	LPRENDVIEWDATA prvd = (LPRENDVIEWDATA)GetWindowLong(hwnd, 0);
	RECT rect;

	assert(prvd != NULL);
	assert(width > 0);
	assert(height > 0);
	
	if ((width == prvd->Width) && (height == prvd->Height))
		return TRUE; /* No change in size. */
	prvd->Width = width;
	prvd->Height = height;
	if (prvd->hRendDC != NULL)
	{
		DeleteDC(prvd->hRendDC);
		prvd->hRendDC = NULL;
	}
	if (prvd->hRendBitmap != NULL)
	{
		DeleteObject(prvd->hRendBitmap);
		prvd->hRendBitmap = NULL;
	}
	InvalidateRect(hwnd, NULL, TRUE); /* Erase the window. */
	prvd->hRendBitmap = CreateCompatibleBitmap(prvd->hRendViewDC,
		prvd->Width, prvd->Height);
	if (prvd->hRendBitmap == NULL)
		return FALSE;
	prvd->hRendDC = CreateCompatibleDC(prvd->hRendViewDC);
	if (prvd->hRendDC == NULL)
		return FALSE;
	SelectObject(prvd->hRendDC, prvd->hRendBitmap);

	GetClientRect(hwnd, &rect);
	RendWndSetScrollBars(hwnd, rect.right, rect.bottom, width - 1, height - 1);

	RendWndClear(hwnd);
	return TRUE;
}


void RendWndClear(HWND hwnd)
{
	HBRUSH hOldBrush;
	LPRENDVIEWDATA prvd = (LPRENDVIEWDATA)GetWindowLong(hwnd, 0);
	assert(prvd != NULL);
	if (prvd->hRendDC != NULL)
	{
		hOldBrush = (HBRUSH)SelectObject(prvd->hRendDC, GetStockObject(BLACK_BRUSH));
		Rectangle(prvd->hRendDC, 0, 0, prvd->Width, prvd->Height);
		SelectObject(prvd->hRendDC, hOldBrush);
	}
}
