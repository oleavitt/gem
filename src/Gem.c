/**
 *****************************************************************************
 *  @file gem.c
 *  Top-level application code.
 *
 *****************************************************************************
 */

#include "gempch.h"
#include "gem.h"

const TCHAR g_szAppName[] = _T("Gem");	/* Who we are. */

/* App build info: Compiler name-OS name-Build date */
const TCHAR g_szAppBuild[] = _T(CONFIG_BUILDINFO);

TCHAR g_szAppRegRoot[GEM_APPINFO_MAX];	/* App's main registry root. */
HINSTANCE hInst;				/* App instance. */
HWND hwndGem = NULL;			/* Top level frame window. */
HWND hwndRendView = NULL;		/* Window displaying image being rendered. */
HWND hwndMsgWnd = NULL;			/* Window displaying message output. */
HMENU hGemMainMenu = NULL;		/* Main menu bar. */
HMENU g_hmenuRender = NULL;	/* Popup context menu for renderer view window. */
BOOL bIsRendering;				/* TRUE while render is in progress. */
BOOL bPreviewMode;				/* TRUE if Render|Preview is checked. */
BOOL bShowStatusBar;			/* TRUE if status bar is to be shown. */
BOOL bPause;					/* TRUE if Render|Stop is selected. */
int splitpos;					/* Splitter bar position (% * 100). */
int mouse_Y, max_X, max_Y, adjmax_Y;
int cyTB, cySB;
int newsplipos;
int gem_fileType;				/* SCN?, NFF?, etc. */

OutfileData *ofdOutfile;		/* Output file data. */

static TCHAR  gszFileName[_MAX_PATH];
static TCHAR  gszTitleName[_MAX_FNAME + _MAX_EXT];


/*************************************************************************
*
*  Handler functions for the various UI and OS events.
*  These are called from GemFrameProc() with the needed parameters
*  cracked from WPARAM and LPARAM.
*
*************************************************************************/
int GemOnCreate(HWND hwnd, LPCREATESTRUCT lpCreatestruct); /* WM_CREATE */
void GemOnSize(HWND hwnd, int cx, int cy);  /* WM_SIZE */
void GemOnInitMenu(HMENU hMenu);  /* WM_INITMENU */
BOOL GemOnContextMenu(HWND hwnd, int x, int y, HWND hwndContext);  /* WM_CONTEXTMENU */
void GemOnViewRenderer(HWND hwnd);  /* WM_COMMAND - ID_VIEW_RENDERER */
void GemOnViewStatusbar(HWND hwnd);  /* WM_COMMAND - ID_VIEW_STATUSBAR */
void GemOnFileLoad(HWND hwnd);  /* WM_COMMAND - ID_FILE_LOAD */
void GemOnFileReload(HWND hwnd);  /* WM_COMMAND - ID_FILE_RELOAD */
void GemOnSelfReload(HWND hwnd);  /* WM_USER_SELF_RELOAD */
void GemOnFileClose(HWND hwnd);  /* WM_COMMAND - ID_FILE_CLOSE */
void GemOnRenderStart(HWND hwnd); /* WM_COMMAND - ID_RENDER_START */
void GemOnRenderStop(HWND hwnd); /* WM_COMMAND - ID_RENDER_STOP */
int GemOnQueryEndSession(HWND hwnd); /* WM_QUERYENDSESSION */
void GemOnLButtonDown(HWND hwnd, int y); /* WM_LBUTTONDOWN */
void GemOnLButtonUp(HWND hwnd); /* WM_LBUTTONUP */
void GemOnMouseMove(HWND hwnd, int y); /* WM_MOUSEMOVE */
void GemOnSettingChange(HWND hwnd, WORD wFlag, LPCTSTR lpszMetrics);

/*************************************************************************
*
*  Misc. helper functions.
*
*************************************************************************/
void ResetGem(void);
void CloseGem(void);
BOOL GemColdStart(void);
BOOL GemIdleAction(void);
void ParseCmdLine(LPTSTR lpszCmdLine);
void GemUpdateCaption(int nCurLine, int nTotalLines);
void GemDraw3DHorzLine(HDC hdc, int y, int x1, int x2);

/*************************************************************************
*
*  Callback functions. These are passed to the OS as pointers for the
*  OS to call us when appropriate.
*
*************************************************************************/
LRESULT CALLBACK GemFrameProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK CloseEnumProc(HWND, LPARAM);


/*************************************************************************
*
*  The main entry point for the application.
*
*************************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPTSTR lpCmdLine, int cmdShow)
{
	WNDCLASSEX wndclass;
	HACCEL hAccel;
	MSG msg;
	RECT rect;
	int nShow;

	memset(&msg, 0, sizeof(MSG));

	hInst = hInstance;

	/* Set the registry root for this app. */
	wsprintf(g_szAppRegRoot, _T("OBL\\%s\\%d.%d\\"),
		g_szAppName, CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR);

	if (!hPrevInstance)
	{
		/* Define the app frame window class. */
		wndclass.cbSize = sizeof(WNDCLASSEX);
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = GemFrameProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInstance;
		wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GEM));
		wndclass.hCursor = LoadCursor(hInst, MAKEINTRESOURCE(IDC_SPLITCURSOR));
		wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = g_szAppName;
		wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GEMSM));

		/* Register this class and child window classes. */
		if (!(RegisterClassEx(&wndclass) &&
			RendwndRegisterClass() &&
			MsgWndRegisterClass()))
			return 0;
	}

	InitCommonControls();

	hGemMainMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_GEM_MENU));

	/* Restore app window states from registry. */
	GemRegGetAppFrameState(&rect, &nShow);

	/* Create the app frame window. */
	hwndGem = CreateWindowEx(
		0L,                         /* extended window style */
		g_szAppName,                  /* window class name */
		g_szAppName,                  /* window caption */
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,  /* window style */
		rect.left,                  /* initial x position */
		rect.top,                   /* initial y position */
		rect.right - rect.left,     /* initial x size */
		rect.bottom - rect.top,     /* initial y size */
		HWND_DESKTOP,               /* parent window handle */
		hGemMainMenu,               /* window menu handle */
		hInst,                      /* program instance handle */
		lpCmdLine);                 /* creation parameters */

	/* Load accelerators. */
	hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_GEM_ACCEL));

	if (!GemCreateStatusBar(hwndGem))
		return 0;
	if (!bShowStatusBar)
		ShowWindow(hwndStatusBar, SW_HIDE);

	ShowWindow(hwndRendView, SW_SHOW);
	ShowWindow(hwndMsgWnd, SW_SHOW);

	/* Display the window. */
	ShowWindow(hwndGem, (nShow == 0) ? cmdShow : SW_MAXIMIZE);
	UpdateWindow(hwndGem);

	GetClientRect(hwndGem, &rect);
	GemOnSize(hwndGem, rect.right, rect.bottom);

	for (;;)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;
			if (!TranslateAccelerator(hwndGem, hAccel, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else if (!GemIdleAction())	/* If we're not busy... */
			WaitMessage();			/* Let the system go idle. */
	}

	return msg.wParam;
}


/*************************************************************************
*
*  Window proc for the main top-level window of the app.
*  A pointer to this function is set in the window class (WNDCLASSEX)
*  struct when the window class is registered.
*  The OS calls this function each time it's this window's turn to
*  process an event from the event queue.
*
*************************************************************************/
LRESULT CALLBACK GemFrameProc(HWND hwnd, UINT imsg, WPARAM wParam, LPARAM lParam)
{
	switch (imsg)
	{
		case WM_CREATE:
			return GemOnCreate(hwnd, (LPCREATESTRUCT)lParam);

		case WM_INITMENU:
			GemOnInitMenu((HMENU)wParam);
			return 0;

		case WM_MENUSELECT:
			return GemStatusBarMenuSelect(hwnd, wParam, lParam);

		case WM_CONTEXTMENU:
			if (GemOnContextMenu(
				hwnd,
				LOWORD(lParam),
				HIWORD(lParam),
				(HWND)wParam))
				return 0;
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ID_FILE_LOAD:
					GemOnFileLoad(hwnd);
					return 0;

				case ID_FILE_RELOAD:
					GemOnFileReload(hwnd);
					return 0;

				case ID_FILE_EXIT:
					SendMessage(hwnd, WM_CLOSE, 0, 0);
					return 0;

				case ID_VIEW_STATUSBAR:
					GemOnViewStatusbar(hwnd);
					return 0;

				case ID_RENDER_STOP:
					GemOnRenderStop(hwnd);
					return 0;

				case ID_RENDER_START:
					GemOnRenderStart(hwnd);
					return 0;

				case ID_RENDER_SETTINGS:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_RENDER_SETTINGS),
						hwnd, (DLGPROC)RendDlgProc);
					CheckMenuItem(hGemMainMenu, ID_RENDER_PREVIEW,
						renddlg_preview_mode ? MF_CHECKED : MF_UNCHECKED);
					return 0;

				case ID_FILE_SEARCHPATHS:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_SEARCH_PATHS),
						hwnd, (DLGPROC)PathsDlgProc);
					return 0;

				case ID_RENDER_PREVIEW:
					renddlg_preview_mode = (renddlg_preview_mode == FALSE) ? TRUE : FALSE;
					CheckMenuItem(hGemMainMenu, ID_RENDER_PREVIEW,
						renddlg_preview_mode ? MF_CHECKED : MF_UNCHECKED);
					return 0;

				case ID_HELP_ABOUT:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_GEM_ABOUT),
						hwnd, (DLGPROC)AboutDlgProc);
					return 0;

				default:
					break;
			}
			break;

		case WM_USER_SELF_RELOAD:
			GemOnSelfReload(hwnd);
			return 0;

		case WM_SETTINGCHANGE:
			GemOnSettingChange(hwnd, (WORD)wParam, (LPCTSTR)lParam);
			return 0;

		case WM_SIZE:
			GemOnSize(hwnd, LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_LBUTTONDOWN:
			GemOnLButtonDown(hwnd, HIWORD(lParam));
			return 0;

		case WM_LBUTTONUP:
			GemOnLButtonUp(hwnd);
			return 0;

		case WM_MOUSEMOVE:
			GemOnMouseMove(hwnd, HIWORD(lParam));
			return 0;

		case WM_CLOSE:
		case WM_QUERYENDSESSION:
			if (!GemOnQueryEndSession(hwnd))
				return 0;  /* Not yet okay to close. */
			break;

		case WM_DESTROY:
			CloseGem();
			GemRegSaveSearchPaths();
			GemRegSaveAppFrameState(hwnd);
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hwnd, imsg, wParam, lParam);
}


/*************************************************************************
*
*  App initialization and shutdown functions.
*
*************************************************************************/
/*************************************************************************
*
*  Called when application is first started. (Top frame window creation)
*
*************************************************************************/
BOOL GemColdStart(void)
{
	bIsRendering = FALSE;
	ofdOutfile = NULL;
	bShowStatusBar = TRUE;

	if (!(RendDlgColdStart() && PathsDlgColdStart()))
		return FALSE;

	GemResetStopWatch();

	Rend2D_Init();
	if (!Ray_Initialize())
	{
		MessageBox(GetFocus(),
			_T("Unable to initialize ray trace module."),
			g_szAppName, MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	if (!scn20_initialize())
	{
		MessageBox(GetFocus(),
			_T("Unable to initialize scene description language module."),
			g_szAppName, MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	bPreviewMode = renddlg_preview_mode;
	bAnimation = renddlg_animation_on;

	return TRUE;
}


/*************************************************************************
*
*  Called when app is about to exit. (Top frame window destruction)
*
*************************************************************************/
void CloseGem(void)
{
	ResetGem();
	scn20_close();
	hwndRendView = NULL;
	hwndGem = NULL;
}


void ResetGem(void)
{
	bIsRendering = FALSE;
	scn20_reset();
	/* Close the output file. */
	Outfile_Close(ofdOutfile);
	ofdOutfile = NULL;
	/* Free up renderer resources. */
	Ray_Close();
	Rend2D_Close();
#ifndef NDEBUG
	{
		TCHAR msg[80];
		if (ray_mem_used)
		{
			wsprintf(msg, _T("ray_mem_used = %u"), ray_mem_used);
			MessageBox(GetFocus(), msg, g_szAppName, MB_OK | MB_ICONEXCLAMATION);
		}

		if (num_stmts_allocd)
		{
			wsprintf(msg, _T("num_stmts_allocd = %u"), num_stmts_allocd);
			MessageBox(GetFocus(), msg, g_szAppName, MB_OK | MB_ICONEXCLAMATION);
		}
	}
#endif // NDEBUG
}


/*************************************************************************
*
*  Idle action handler.
*  If a rendering is in progress, render a pixel each time this
*  function is called. 
*
*************************************************************************/
BOOL GemIdleAction(void)
{
	if (Rend2D_GetStatus() == REND2D_STATUS_RENDERING)
	{
		Rend2DPixel pixel;
		if (Rend2D_DoPixel(&pixel) == REND2D_STATUS_RENDERING)
		{
			if (bIsRendering == FALSE)
			{
				Rend2D_GetState(&renderer);
				ModifyMenu(hGemMainMenu, ID_RENDER_START, 0,
					ID_RENDER_STOP, _T("&Stop"));
				bIsRendering = TRUE;
				GemUpdateCaption(0, 1);
				GemStartStopWatch();
				GemUpdateStatusBar();
			}
			RendWndSetPixel(hwndRendView, &pixel);
			if (pixel.width <= 1)
			{
				if (ofdOutfile != NULL)
					Outfile_PutPixel(ofdOutfile, pixel.r, pixel.g, pixel.b);
			}
			if (pixel.x + pixel.width >= renderer.xend)
			{
				RECT rect;
				rect.left = renderer.xstart;
				rect.right = renderer.xend;
				rect.top = pixel.y;
				rect.bottom = pixel.y + pixel.height;
				RendWndUpdateRect(hwndRendView, &rect);
				GemUpdateStatusBar();
				GemUpdateCaption(pixel.y + 1, (renderer.yend + 1) - renderer.ystart);
			}
		}
		return TRUE;
	}
	if (bIsRendering == TRUE)
	{
		ModifyMenu(hGemMainMenu, ID_RENDER_STOP, 0,
			ID_RENDER_START, _T("&Start"));
		bIsRendering = FALSE;
		Outfile_Close(ofdOutfile);
		ofdOutfile = NULL;
		GemUpdateCaption(0, 1);
		GemUpdateStatusBar();
		GemStopStopWatch();
		if ((bAnimation == TRUE) && (bPause == FALSE))
		{
			/*
			 * TODO:
			 * Check for next animation frame. If there's more start the next
			 * frame.
			 */
			/*	PostMessage(hwndGem, WM_USER_SELF_RELOAD, 0, 0); */
		}
	}
	return FALSE;
}


/*************************************************************************
*
*     Message handlers.
*
*************************************************************************/
/*************************************************************************
*
*  WM_CREATE
*
*************************************************************************/
int GemOnCreate(HWND hwnd, LPCREATESTRUCT lpCreatestruct)
{
	if (!GemColdStart())
		return -1;

	if (((hwndRendView = RendWndCreate(hwnd)) == NULL) ||
		((hwndMsgWnd = MsgWndCreate(hwnd)) == NULL))
		return -1;

	g_hmenuRender = LoadMenu(hInst, MAKEINTRESOURCE(IDR_GEM_RENDWND_MENU));
	g_hmenuRender = GetSubMenu(g_hmenuRender, 0);
	GemFileInitialize(hwndGem);
	GemRegGetSearchPaths();

	bPause = FALSE;

	/*
	 * If a file was given on the command-line, read it in
	 * and render it when the message loop starts.
	 */
	lstrcpy(gszFileName, (LPCTSTR)lpCreatestruct->lpCreateParams);
	if (gszFileName[0])
	{
		PostMessage(hwnd, WM_USER_SELF_RELOAD, 0, 0);
	}
	return 0;
}

/*************************************************************************
*
*  WM_INITMENU
*
*************************************************************************/
void GemOnInitMenu(HMENU hMenu)
{
	CheckMenuItem(hMenu, ID_RENDER_PREVIEW,
		renddlg_preview_mode ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_VIEW_STATUSBAR,
		bShowStatusBar ? MF_CHECKED : MF_UNCHECKED);
}

/*************************************************************************
*
*  WM_CONTEXTMENU
*
*************************************************************************/
BOOL GemOnContextMenu(HWND hwnd, int x, int y, HWND hwndContext)
{
	if (hwndContext == hwndRendView)
	{
		TrackPopupMenu(
			g_hmenuRender,
			TPM_RIGHTBUTTON,
			x, y,
			0, hwnd, NULL);
		return TRUE;
	}
	else if (hwndContext == hwndMsgWnd)
	{
		/* TODO: Clear, Hide, etc. */
		return TRUE;
	}
	return FALSE;
}

/*************************************************************************
*
*  WM_SETTINGCHANGE
*
*************************************************************************/
void GemOnSettingChange(HWND hwnd, WORD wFlag, LPCTSTR lpszMetrics)
{
	GemOnSize(hwnd, max_X, max_Y);

	/* Not used */
	lpszMetrics;
	wFlag;
}

/*************************************************************************
*
*  WM_SIZE
*
*************************************************************************/
void GemOnSize(HWND hwnd, int cx, int cy)
{
	RECT rect;
	int cyMW, cyRW, cySplit, x, y;

	cySplit = GetSystemMetrics(SM_CYSIZEFRAME);
	max_X = cx;
	max_Y = cy;

	/* Reposition the tool bar if visible. */
	cyTB = 0; /* No tool bar for now. */

	/* Reposition the status bar if visible. */
	if (IsWindowVisible(hwndStatusBar))
	{
		GetWindowRect(hwndStatusBar, &rect);
		cySB = rect.bottom - rect.top;
		GemSizeStatusBar(cx, cy);
	}
	else
		cySB = 0;

	adjmax_Y = cy - cySB - cyTB;

	/* Calculate new window positions. */
	ShowWindow(hwndRendView, (splitpos == 0) ? SW_HIDE : SW_SHOW);
	ShowWindow(hwndMsgWnd, (splitpos == 10000) ? SW_HIDE : SW_SHOW);
	if (IsWindowVisible(hwndMsgWnd) && IsWindowVisible(hwndRendView))
	{
		x = 0;
		y = (int)(((long)splitpos * (long)adjmax_Y) / 10000L);
		cyMW = adjmax_Y - y;
		MoveWindow(hwndMsgWnd, x, y+cyTB+cySplit, cx, cyMW-cySplit, TRUE);
		y = cyTB;
		cyRW = adjmax_Y - cyMW;
		MoveWindow(hwndRendView, x, y, cx, cyRW, TRUE);
	}
	else if (IsWindowVisible(hwndRendView))
	{
		HDC hdc = GetDC(hwnd);
		cyRW = adjmax_Y - cySplit;
		MoveWindow(hwndRendView, 0, cyTB, cx, cyRW, TRUE);
		GemDraw3DHorzLine(hdc, max_Y-cySB-2, 0, cx);
		ReleaseDC(hwnd, hdc);
	}
	else
	{
		HDC hdc = GetDC(hwnd);
		cyMW = adjmax_Y - cySplit;
		MoveWindow(hwndMsgWnd, 0, cyTB + cySplit, cx, cyMW, TRUE);
		GemDraw3DHorzLine(hdc, cyTB, 0, cx);
		ReleaseDC(hwnd, hdc);
	}

	/* Save our new size settings now. */
	GemRegSaveAppFrameState(hwnd);
}


/*************************************************************************
*
*  WM_QUERYENDSESSION
*
*************************************************************************/
int GemOnQueryEndSession(HWND hwnd)
{
	if (bIsRendering)
	{
		if (MessageBox(hwnd, _T("Render still in progress. Okay to close?"),
			g_szAppName, MB_OKCANCEL | MB_ICONQUESTION) != IDOK)
			return 0;
	}
	return 1;
}


/*************************************************************************
*
*     Command handlers.
*
*************************************************************************/
/*************************************************************************
*
*  File | Open
*
*************************************************************************/
void GemOnFileLoad(HWND hwnd)
{
	if ((gem_fileType = GemFileOpenDlg(hwnd, gszFileName, gszTitleName)) !=
		GEMFILE_FAIL)
	{
		/* Clear the message window. */
		MsgWndClear(hwndMsgWnd);
		PostMessage(hwndGem, WM_USER_SELF_RELOAD, 0, 0);
	}
}


/*************************************************************************
*
*  File | Reload
*
*************************************************************************/
void GemOnFileReload(HWND hwnd)
{
	if (bIsRendering && !bPreviewMode)
	{
		SetFocus(hwnd);
		if (MessageBox(hwnd,
			_T("Render still in progress. OK to stop current job?"),
			g_szAppName, MB_OKCANCEL | MB_ICONINFORMATION) != IDOK)
			return;
	}
	/* Reset run-time animation frame counter. */
	/* TODO: Reset Scn10 code frame counter. */
	/* Clear the message window. */
	MsgWndClear(hwndMsgWnd);
	PostMessage(hwndGem, WM_USER_SELF_RELOAD, 0, 0);
}


/*************************************************************************
*
*  WM_USER_SELF_RELOAD
*
*************************************************************************/
void GemOnSelfReload(HWND hwnd)
{
	if (bIsRendering && !bPreviewMode)
	{
		SetFocus(hwnd);
		if (MessageBox(hwnd,
			_T("Render still in progress. OK to stop current job?"),
			g_szAppName, MB_OKCANCEL | MB_ICONINFORMATION) != IDOK)
			return;
	}
	ResetGem();
	Rend2D_Init();
	if (!Ray_Initialize())
	{
		MessageBox(hwnd, _T("Unable to initialize ray trace module."),
			g_szAppName, MB_OK | MB_ICONSTOP);
		SendMessage(hwnd, WM_CLOSE, 0, 0);
		return;
	}
	if (BuildScene(gszFileName))
	{
		if (GetFileTitle(gszFileName, gszTitleName, _MAX_FNAME + _MAX_EXT))
			lstrcpy(gszTitleName, "???");
		GemUpdateCaption(0, 1);
		if (RendWndResizeImage(hwndRendView,
			renderer.xres, renderer.yres))
		{
			PostMessage(hwndGem, WM_COMMAND, (WPARAM)ID_RENDER_START, 0);
		}
		else
		{
			MessageBox(hwnd, _T("Insufficient resources for renderer view."),
				g_szAppName, MB_OK | MB_ICONSTOP);
		}
	}
	else
	{
		bAnimation = FALSE;
		ResetGem();
	}
}

/*************************************************************************
*
*  WM_LBUTTONDOWN
*
*************************************************************************/
void GemOnLButtonDown(HWND hwnd, int y)
{
	SetCapture(hwnd);
	mouse_Y = y;
}

/*************************************************************************
*
*  WM_LBUTTONUP
*
*************************************************************************/
void GemOnLButtonUp(HWND hwnd)
{
	if (GetCapture() == hwnd)
	{
		ReleaseCapture();
	}
}

/*************************************************************************
*
*  WM_MOUSEMOVE
*
*************************************************************************/
void GemOnMouseMove(HWND hwnd, int y)
{
	if (GetCapture() == hwnd)
	{
		if (mouse_Y != y)
		{
			mouse_Y = y;
			splitpos = (int)(((long)(y - cyTB) * 10000L) / (long)adjmax_Y);
			if(splitpos < 250)
				splitpos = 0;
			else if(splitpos > 9750)
				splitpos = 10000;
			GemOnSize(hwnd, max_X, max_Y);
		}
	}
}


/*************************************************************************
*
*  View | Status Bar
*
*************************************************************************/
void GemOnViewStatusbar(HWND hwnd)
{
	RECT rect;

	/* Toggle the status bar and the menu check. */
	if (IsWindowVisible(hwndStatusBar))
	{
		ShowWindow(hwndStatusBar, SW_HIDE);
		bShowStatusBar = FALSE;
		CheckMenuItem(hGemMainMenu, ID_VIEW_STATUSBAR, MF_UNCHECKED);
	}
	else
	{
		ShowWindow(hwndStatusBar, SW_SHOW);
		bShowStatusBar = TRUE;
		CheckMenuItem(hGemMainMenu, ID_VIEW_STATUSBAR, MF_CHECKED);
	}

	/* Readjust window layout. */
	GetClientRect(hwnd, &rect);
	PostMessage(hwnd, WM_SIZE, 0, MAKELPARAM(rect.right, rect.bottom));
}


/*************************************************************************
*
*  Render | Start
*
*************************************************************************/
void GemOnRenderStart(HWND hwnd)
{
	assert(hwndRendView != NULL);
	Rend2D_Start();
	GemUpdateStatusBar();
	bPause = FALSE;

	/* Not used */
	hwnd;
}


/*************************************************************************
*
*  Render | Stop (pause)
*
*************************************************************************/
void GemOnRenderStop(HWND hwnd)
{
	Rend2D_Stop();
	bPause = TRUE;
	
	/* Not used */
	hwnd;
}


/*************************************************************************
*
*  Misc. utility functions.
*
*************************************************************************/

/*************************************************************************
*
*  Read scene file name from command-line.
*
*************************************************************************/
void ParseCmdLine(LPTSTR lpszCmdLine)
{
	TCHAR c, *p, *p2;
	p2 = g_szScnFileName;
	if (lpszCmdLine != NULL)
	{
		for (p = lpszCmdLine; *p != '\0'; p++)
		{
			c = *p;
			if (!isspace(c))
				*p2++ = c;
			else
				break;
		}
	}
	*p2 = '\0';
}


void GemUpdateCaption(int nCurLine, int nTotalLines)
{
	TCHAR szCaption[64 + _MAX_FNAME + _MAX_EXT];
	int nPercent;

	if (bIsRendering)
	{
		/* Calc percentage of lines completed. */
		nPercent = (nCurLine * 100) / nTotalLines;
		wsprintf(szCaption, _T("%s [%d%%] - %s"),
			gszTitleName, nPercent, g_szAppName);
	}
	else
	{
		wsprintf(szCaption, _T("%s - %s"),
			gszTitleName, g_szAppName);
	}
	SetWindowText(hwndGem, szCaption);
}

void GemDraw3DHorzLine(HDC hdc, int y, int x1, int x2)
{
	HPEN pen, oldpen;

	pen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DDKSHADOW));
	oldpen = (HPEN)SelectObject(hdc, pen);
	MoveToEx(hdc, x1, y, NULL); LineTo(hdc, x2, y);
	SelectObject(hdc, oldpen);
	DeleteObject(pen);
	y++;
	pen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DHILIGHT));
	SelectObject(hdc, pen);
	MoveToEx(hdc, x1, y, NULL); LineTo(hdc, x2, y);
	SelectObject(hdc, oldpen);
	DeleteObject(pen);
}
