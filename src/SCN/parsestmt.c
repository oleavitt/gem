/***************************************************************
*
*	parsestmt.c
*
***************************************************************/

#include "local.h"
#include "scn.h"

/***************************************************************
*
*	ParseStatement()
*	Parse a complete statement.
*	Patterns:
*		[text]; - semicolon terminates.
*		[text] { [nested statements] } - matching right curly bracket terminates.
*
***************************************************************/
void ScanTopLevelStatement(void)
{
	/* Read text into buffer... */
	/* if '{' nest++... */
	/* if '}' nest--... */
}