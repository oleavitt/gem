/**
 *****************************************************************************
 *  @file define.c
 *  Handles the 'define' keyword.
 *  Parses object definition statements, executes them, and adds the
 *  newly constructed object to symbol table.
 *
 *****************************************************************************
 */

#include "local.h"

/*************************************************************************
*
*	parse_define() - Starting point for definition parsing. Called
*	after the 'define' keyword is encountered.
*
*************************************************************************/
int parse_define(void)
{
	int		token;
	VMStmt	*stmt = NULL;

	/* Get new identifier name... */
	if ((token = gettoken_GetNewIdentifier()) == TK_UNKNOWN_ID)
	{
		/* Set the global state to 'define mode'. */
		g_define_mode++;

		/* Save id name now. */
		strcpy(g_define_name, g_token_buffer);

		/* See if there is an argument list... */

		/* Get the object being defined... */
		token = gettoken();

		switch (token)
		{
			case TK_SURFACE:
				/* 'surface' or <surface name> */
				stmt = parse_vm_surface(NULL);
				break;

			case TK_OBJECT:
			case TK_UNION:
			case TK_DIFFERENCE:
			case TK_INTERSECTION:
			case TK_CLIP:
			case DECL_OBJECT:
				stmt = parse_vm_csg(token);
				break;
		}

		if (stmt != NULL)
		{
			if (!g_error_count)
			{
				/* Run the vm to create the object.
				 * The vm functions for objects that are "define"able
				 * will automatically store the object in the symbol
				 * table if g_define_mode is non-zero.
				 */
				stmt->methods->fn(stmt);
			}
			vm_delete(stmt);
		}

		g_define_mode--;
	}
	else
	{
		gettoken_ErrUnknown(token, "identifier name");
		/* SkipStatement() */
		return 0;
	}

	return 1;
}