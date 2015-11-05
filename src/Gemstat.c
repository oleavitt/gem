/*************************************************************************
*
*  gemstat.c - Status bar operations.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"

time_t gem_total_elapsed_time;

#define ID_GEM_STATUS_BAR       1000
#define STAT_MSG_MAX            256
#define NUM_PARTS               3

HWND hwndStatusBar;
TCHAR sz[STAT_MSG_MAX];
TCHAR szStatMsgText[STAT_MSG_MAX];
int parts[NUM_PARTS];
time_t start_time;
int hours, mins, secs;

int GemCreateStatusBar(HWND hwndParent)
{
	LoadString(hInst, IDS_STATUS_IDLE, szStatMsgText, STAT_MSG_MAX);
	if ((hwndStatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE,
		szStatMsgText, hwndParent, ID_GEM_STATUS_BAR)) == NULL)
		return 0;
	SendMessage(hwndStatusBar, SB_SETPARTS,
		(WPARAM)NUM_PARTS, (LPARAM)parts);
	LoadString(hInst, IDS_STATUS_ETIME, sz, STAT_MSG_MAX);
	hours = mins = secs = 0;
	wsprintf(szStatMsgText, sz, hours, mins, secs); 
	SendMessage(hwndStatusBar, SB_SETTEXT,
		(WPARAM)1, (LPARAM)szStatMsgText);
	LoadString(hInst, IDS_STATUS_FRAME, sz, STAT_MSG_MAX);
	/* TODO: Update frame counter in status bar. */
	wsprintf(szStatMsgText, sz, 1, 1);
	SendMessage(hwndStatusBar, SB_SETTEXT,
		(WPARAM)2, (LPARAM)szStatMsgText);
	return 1;
}


void GemSizeStatusBar(int cx, int cy)
{
	int cySB, i;
	RECT rect;

	GetWindowRect(hwndStatusBar, &rect);
	cySB = rect.bottom - rect.top;
	MoveWindow(hwndStatusBar, 0, cy - cySB, cx, cySB, TRUE);
	for (i = 1; i <= NUM_PARTS; i++)
		parts[i-1] = cx/NUM_PARTS * i;
	SendMessage(hwndStatusBar, SB_SETPARTS,
		(WPARAM)NUM_PARTS, (LPARAM)parts);
}


void GemUpdateStatusBar(void)
{
	int y = Rend2D_GetY(); /* Get current line being rendered. */
	time_t now;
	double elapsed;
	
	if (bIsRendering)  /* Print the line status. */
	{
		LoadString(hInst, IDS_STATUS_RENDERING, sz, STAT_MSG_MAX);
		wsprintf(szStatMsgText, sz, Rend2D_GetY()+1, renderer.yend); 
	}
	else   /* Print the idle message. */
		LoadString(hInst, IDS_STATUS_IDLE, szStatMsgText, STAT_MSG_MAX);
	SendMessage(hwndStatusBar, SB_SETTEXT,
		(WPARAM)0, (LPARAM)szStatMsgText);

	/* Print the elapsed time. */
	LoadString(hInst, IDS_STATUS_ETIME, sz, STAT_MSG_MAX);
	now = time(NULL);
	elapsed = difftime(now, start_time);
	hours = ((int)elapsed / 3600);
	mins = ((int)elapsed / 60) % 60;
	secs = (int)elapsed % 60;
	wsprintf(szStatMsgText, sz, hours, mins, secs); 
	SendMessage(hwndStatusBar, SB_SETTEXT,
		(WPARAM)1, (LPARAM)szStatMsgText);

	if (y == renderer.ystart) /* Print the frame status. */
	{
		LoadString(hInst, IDS_STATUS_FRAME, sz, STAT_MSG_MAX);
		/* TODO: Get current frame info. */
		wsprintf(szStatMsgText, sz, 1, 1); 
		SendMessage(hwndStatusBar, SB_SETTEXT,
			(WPARAM)2, (LPARAM)szStatMsgText);
	}
}


LRESULT GemStatusBarMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu = NULL;  /* For pop-ups, if any. */
	UINT iStrBase = 0;

	/* See if we have a pop-up, other than the system menu. */

	/* Display the help text in the status bar. */
	MenuHelp(WM_MENUSELECT, wParam, lParam, hMenu, hInst,
		hwndStatusBar, &iStrBase);

	/* Not used */
	hwnd;

	return 0;
}


void GemStartStopWatch(void)
{
	start_time = time(NULL);
}


void GemStopStopWatch(void)
{
	double diff;
	time_t now = time(NULL);
	diff = difftime(now, start_time);
	gem_total_elapsed_time += (int)diff;
}


void GemResetStopWatch(void)
{
	start_time = 0;
	gem_total_elapsed_time = 0;
}
 