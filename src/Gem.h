/*************************************************************************
*
*  gem.h - Main header for the application.
*
*************************************************************************/

#ifndef GEM_H
#define GEM_H

#include "config.h"
#include "nff.h"
#include "winutil.h"

#define MAXFNAME _MAX_PATH

/* resource defines maintained by resource compiler */
#include "resource.h"

#include "rend2dc.h"
#include "raytrace.h"
//#include "scn10.h"
#include "scn20.h"

#include <stdio.h>		/* FILE type */
#include <time.h>		/* time_t type */
#include <assert.h>

/* Registry settings. */
/* Layout. */
#define GEM_REG_LAYOUT				_T("Layout")
#define GEM_REG_LAYOUT_APP_FRAME	_T("App Frame")
/* Search paths. */
#define GEM_REG_PATHS				_T("Search Paths")
#define GEM_REG_PATHS_SEARCH_PATHS	_T("Search Paths")
/* File open. */
#define GEM_REG_FILEOPEN			_T("File Open")
#define GEM_REG_FILEOPEN_LAST_PATH	_T("Last Path")
/* Renderer settings. */
#define GEM_REG_RENDERER			_T("Renderer")
#define GEM_REG_RENDERER_WIDTH		_T("Width")
#define GEM_REG_RENDERER_HEIGHT		_T("Height")
#define GEM_REG_RENDERER_PREVIEW	_T("Preview Mode")
#define GEM_REG_RENDERER_USESCNRES	_T("Use Scene File Res")
#define GEM_REG_RENDERER_AA			_T("Antialiasing")
#define GEM_REG_RENDERER_AATHRESH	_T("AA Threshold")
#define GEM_REG_RENDERER_AADEPTH	_T("AA Depth")
#define GEM_REG_RENDERER_JITTER		_T("Jitter")
#define GEM_REG_RENDERER_JITTERAMT	_T("Jitter Amount")
#define GEM_REG_RENDERER_ANIM		_T("Animation")
#define GEM_REG_RENDERER_ANIMSTART	_T("Start Frame")
#define GEM_REG_RENDERER_ANIMEND	_T("End Frame")

/* File | Open type codes. */
#define GEMFILE_FAIL			0
#define GEMFILE_SCN				1
#define GEMFILE_NFF				2

/* Maximum length of search paths string. */
#define PATHSDLG_PATHS_MAX		1024

/* User message IDs. */
#define WM_USER_SELF_RELOAD		WM_USER

/* Child window IDs. */
#define CHILD_ID_RENDWND        1
#define CHILD_ID_MSGWND         2
#define CHILD_ID_MSGWNDLISTBOX  3
#define CHILD_ID_RENDMSGSPLIT   4

/* Projection plane IDs. */
#define DRAW3D_XY				0
#define DRAW3D_XZ				1
#define DRAW3D_YZ				2

typedef struct tag_outfiledata
{
	TCHAR fname[MAXFNAME];	/* Output file name. */
	int xres, yres;			/* Output image resolution. */
	int bits;				/* Bits per pixel (color depth). */
	int rle;				/* Use run-length compression if non-zero. */
	unsigned char *scanline;	/* Buffer for current scanline. */
	FILE *fp;				/* FILE stream ptr. */
} OutfileData;


/*
 * aboutdlg.c
 */
extern BOOL CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);


/*
 * draw3d.c
 */
extern int Draw3D_Initialize(void);
extern void Draw3D_Close(void);
extern void Draw3D_SetWindow(Vec3 *wmin, Vec3 *wmax, int width,
	int height, int plane, HDC hdc);
extern void Draw3D_SetPt(int pt_ndx, double x, double y, double z);
extern void Draw3D_MoveTo(int pt_ndx);
extern void Draw3D_LineTo(int pt_ndx);


/*
 * gem.c
 */
#define GEM_APPINFO_MAX		256
extern const TCHAR g_szAppName[];	/* App name. */
extern const TCHAR g_szAppBuild[];	/* App build info: Compiler code-OS code-Build date */
extern TCHAR g_szAppRegRoot[];	/* App's main registry root. */
extern HINSTANCE hInst;		/* App instance */
extern HWND hwndRendView;	/* Window displaying image being rendered. */
extern HWND hwndMsgWnd;		/* Window displaying message output. */
extern HWND hwndGem;		/* Top level app frame window. */
extern HMENU hGemMainMenu;	/* Main menu bar. */
extern OutfileData* ofdOutfile;	/* App's output file info. */
extern BOOL bIsRendering;	/* TRUE while render is in progress. */
extern BOOL bPreviewMode;	/* TRUE if Render|Preview is checked. */
extern int splitpos;		/* Splitter bar position. */
extern int gem_fileType;	/* SCN?, NFF?, etc. */


/*
 * gemfile.c
 */
extern void GemFileInitialize(HWND hwnd);
extern int GemFileOpenDlg(HWND hwnd, LPCTSTR lpszFilename,
  LPCTSTR lpszTitle);


/*
 * gemray.c
 */
extern int RaytracePixel(double u, double v,
  unsigned char *r, unsigned char *g, unsigned char *b);
extern BOOL CheckRayError(void);
extern RaySetupData rsd;       /* Setup info for the ray-tracer. */
extern Rend2D renderer;        /* Setup info for the 2D renderer. */


/*
 * gemreg.c
 */
extern void GemRegGetAppFrameState(RECT *pRect, int *pnShow);
extern void GemRegSaveAppFrameState(HWND hwnd);
extern void GemRegGetSearchPaths(void);
extern void GemRegSaveSearchPaths(void);


/*
 * gemscene.c
 */
extern BOOL bAnimation;
extern TCHAR g_szScnFileName[]; /* Scene file name. */
extern int BuildScene(LPCTSTR lpszFileName);
extern void GemLogMsg(LPCTSTR lpszMsg, int type);


/*
 * gemstat.c
 */
extern HWND hwndStatusBar;
extern time_t gem_total_elapsed_time;
extern int GemCreateStatusBar(HWND hwndParent);
extern void GemSizeStatusBar(int cx, int cy);
extern void GemUpdateStatusBar(void);
extern LRESULT GemStatusBarMenuSelect(HWND hwnd,
	WPARAM wParam, LPARAM lParam);
extern void GemStartStopWatch(void);
extern void GemStopStopWatch(void);
extern void GemResetStopWatch(void);


/*
 * msgwnd.c
 */
extern BOOL MsgWndRegisterClass(void);
extern HWND MsgWndCreate(HWND hwndClient);
extern void MsgWndClear(HWND hwnd);
extern void MsgWndMessage(LPCTSTR lpszMsg);
extern void MsgWndVMessage(const LPCTSTR szFmt, ...);

/*
 * outfile.c
 */
extern OutfileData *Outfile_Create(LPCTSTR lpszFilename, int xres, int yres,
  int bits, int rle);
extern void Outfile_Close(OutfileData *ofd);
extern void Outfile_PutPixel(OutfileData *ofd, unsigned char r, 
  unsigned char g, unsigned char b);
extern void Outfile_MakeName(LPTSTR lpszOutpath, LPCTSTR lpszInpath);


/*
 * pathsdlg.c
 */
/* String containing search paths. */
extern TCHAR pathsdlg_paths[];

extern BOOL PathsDlgColdStart(void);
extern BOOL CALLBACK PathsDlgProc(HWND, UINT, WPARAM, LPARAM);


/*
 * renddlg.c
 */
extern int renddlg_use_scn_res;
extern int renddlg_preview_mode;
extern int renddlg_output_width;
extern int renddlg_output_height;
extern int renddlg_aa_none;
extern int renddlg_aa_adaptive;
extern int renddlg_aa_threshold;
extern int renddlg_aa_depth;
extern int renddlg_jitter_on;
extern int renddlg_jitter_percent;
extern int renddlg_animation_on;
extern int renddlg_start_frame;
extern int renddlg_end_frame;

extern BOOL RendDlgColdStart(void);
extern BOOL CALLBACK RendDlgProc(HWND, UINT, WPARAM, LPARAM);


/*
 * rendwnd.c
 */
extern BOOL RendwndRegisterClass(void);
extern HWND RendWndCreate(HWND hwndClient);
extern void RendWndSetPixel(HWND hwnd, Rend2DPixel *pixel);
extern void RendWndUpdateRect(HWND hwnd, RECT *prc);
extern BOOL RendWndResizeImage(HWND hwnd, int width, int height);
extern void RendWndClear(HWND hwnd);

#endif  /* GEM_H */
