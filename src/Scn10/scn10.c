/*************************************************************************
*
*  scn10.c - Top level interface to the parser.
*
*************************************************************************/

#include "local.h"

int Scn10_Initialize(void)
{
	ExprEval_Init();
	return 1;
}

int Scn10_Parse(const char *fname, RaySetupData *rsd,
	const char *searchpaths)
{
	if(!(Error_Init() &&
			 GetToken_Init(fname) &&
			 Symbol_Init() &&
			 ScnBuild_Init(rsd) &&
			 Parse_Init()))
		return SCN_INIT_FAIL;
	
	Image_Initialize();
	FindFileInitialize();
	SCN_SetPaths(&scn_include_paths, searchpaths);

	Parse();
	ScnBuild_CommitScene();

	FindFileClose();
	Parse_Close();
	ScnBuild_Close();
	Symbol_Close();
	GetToken_Close();
	Error_Close();

	return error_count ? SCN_ERR_PARSE : SCN_OK;
}

MSGFN Scn10_SetMsgFunc(MSGFN msgfn)
{
	MSGFN oldmsgfn = Message;
	Message = msgfn;
	return oldmsgfn;
}