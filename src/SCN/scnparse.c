/***************************************************************
*
*	scnparse.c
*
***************************************************************/

#include "local.h"
#include "scn.h"
#include "parsecontext.h"

static void SCNParseMain(void);
static void SCNParseInclude(void);

/***************************************************************
*
*	SCNParse()
*	Main entry point SCN parse.
*
***************************************************************/
void SCNParse(void)
{
	/* Initialization. */
	/* Parse. */
	SCNParseMain();
	/* Cleanup. */
}

/***************************************************************
*
*	SCNParseMain()
*	Parse main level statements in the main file.
*
***************************************************************/
void SCNParseMain(void)
{
	/*
	 * Initialize a 'main level' context.
	 * Context contains:
	 *	Global variables for entire SCN file.
	 */
	/* include "filename" */
	/* SCNParseInclude() */
}

/***************************************************************
*
*	SCNParseInclude()
*	Parse main level statements within an include file.
*
***************************************************************/
void SCNParseInclude(void)
{
	/* Push context stack. */
	/*
	 * Initialize an 'include' context.
	 * Context contains:
	 *	Local & 'private' variables for include file.
	 */

	/* include "filename" */
	/* SCNParseInclude() */

	/* Pop context stack. */
}
