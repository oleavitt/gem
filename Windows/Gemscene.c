/*************************************************************************
*
*  gemscene.c - Handles application-level scene file details.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"

TCHAR g_szScnFileName[_MAX_PATH]; /* Scene file name. */

BOOL bAnimation = FALSE;


int BuildScene(LPCTSTR lpszFilename)
{
	int result;

	if(lpszFilename == NULL || *lpszFilename == '\0')
		return 0;  /* No scene file. */

	lstrcpy(g_szScnFileName, lpszFilename);

	/* Send message output from parser to MsgWnd's message function. */
	/* Scn10_SetMsgFunc(MsgWndMessage); */
	scn20_set_msgfn(MsgWndMessage);
	Nff_SetMsgFunc(MsgWndMessage);

	/* Init rsd with current (default) settings. */
	Ray_GetSetup(&rsd);

	/* Parse the scene file. */
	switch (gem_fileType)
	{
		case GEMFILE_NFF:
			result = Nff_Parse(g_szScnFileName, &rsd);

			/* Get renderer settings that override those in the scene file. */

			/* Setup the ray trace renderer. */
			if (!Ray_Setup(&rsd))
				return 0;

			if (result != NFF_OK)
				return 0;
			break;

		default:
			result = scn20_parse(g_szScnFileName, &rsd, pathsdlg_paths);
			/* result = Scn10_Parse(g_szScnFileName, &rsd, pathsdlg_paths); */

			/* Get renderer settings that override those in the scene file. */

			/* Setup the ray trace renderer. */
			if (!Ray_Setup(&rsd))
				return 0;

			if (result != SCN_OK)
				return 0;
			break;
	}

	/* Set run-time animation settings. */
	bAnimation = renddlg_animation_on;
	/* Subtract one since scn frame counters are offset based. */
	/* TODO: Set up animation counters for Scn10 code. */

	/* Setup the screen renderer. */
	Rend2D_GetState(&renderer);
	
	/* Get screen renderer settings from the UI. */
	bPreviewMode = renddlg_preview_mode;
	renderer.preview = (bPreviewMode == TRUE) ? 1 : 0;
	renderer.xstart = 0;
	renderer.xres = renderer.xend = 
		renddlg_use_scn_res ? rsd.xres : renddlg_output_width;
	renderer.ystart = 0;
	renderer.yres = renderer.yend = 
		renddlg_use_scn_res ? rsd.yres : renddlg_output_height;
	renderer.mode = renddlg_aa_adaptive ? REND2D_MODE_ADAPTIVE_ANTIALIAS :
		REND2D_MODE_ONCE_PER_PIXEL;
	renderer.aa_level = renddlg_aa_depth;
	renderer.aa_threshold = renddlg_aa_threshold;
	renderer.jitter = renddlg_jitter_on ?
		(double)renddlg_jitter_percent/100.0 : 0.0;

	renderer.calc_color = RaytracePixel;
	if (renderer.xres < renderer.yres)
	{
		renderer.vmin = (double)renderer.yres / (double)renderer.xres;
		renderer.umax = 1.0;
	}
	else
	{
		renderer.umax = (double)renderer.xres / (double)renderer.yres;
		renderer.vmin = 1.0;
	}
	renderer.umin = -renderer.umax;
	renderer.vmax = -renderer.vmin;

	Rend2D_SetState(&renderer);

	if (!renderer.preview)
	{
		if ((ofdOutfile = Outfile_Create(_T(""), renderer.xres, renderer.yres,
			24, 0)) == NULL)
			return 0;
	}
	else
		ofdOutfile = NULL;

	return 1;
}
