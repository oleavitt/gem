/**
 *****************************************************************************
 *  @file scn20.c
 *  Implementation of top level stuff for SCN 2.0
 *
 *****************************************************************************
 */

#include "local.h"

/**
 *	Initializes the SCN 2.0 library.
 *	Called when the app starts up.
 */
int scn20_initialize(void)
{
	if (!vm_init())
		return 0;

	return 1;
}

/**
 *	Resets the SCN 2.0 library.
 *	Called when the app starts a new scene or animation frame.
 */
int scn20_reset(void)
{
	vm_reset();
	return 1;
}

/**
 *	Closes the SCN 2.0 library.
 *	Called when the app closes.
 */
int scn20_close(void)
{
	vm_close();
	return 1;
}

/**
 *	Starts the parsing of a scene description file.
 */
int scn20_parse(
	const char *	fname,
	RaySetupData *	rsd,
	const char *	searchpaths
	)
{
	g_scn20error = SCN_OK;

	if (!error_initialize())
		return SCN_INIT_FAIL;
	FindFileInitialize();
	if (!gettoken_Init(fname))
		return SCN_INIT_FAIL;
	if (!pcontext_init())
		return SCN_INIT_FAIL;

	
	Image_Initialize();
	SCN_SetPaths(&scn_include_paths, searchpaths);
	
	parse(rsd);
//	ScnBuild_CommitScene();

	pcontext_close();
	gettoken_Close();
	FindFileClose();
	error_close();

	return g_error_count ? SCN_ERR_PARSE : SCN_OK;
}

/**
 *	Supplies a user defined message output function to the parser..
 */
MSGFN scn20_set_msgfn(MSGFN msgfn)
{
	MSGFN oldmsgfn = Scn20Message;
	Scn20Message = msgfn;
	return oldmsgfn;
}