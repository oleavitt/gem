/*************************************************************************
*
*	define.c - Handles the 'define' keyword.
*	Parses object definition statements, executes them, and adds the
*	newly constructed object to symbol table.
*
*************************************************************************/

#include "local.h"

/* Ptr to receive surface from ExecSurfaceStmt(). */
Surface *parse_declsurf = NULL;

/*************************************************************************
*
*	ParseDefine() - Starting point for definition parsing. Called
*	after the 'define' keyword is encountered.
*
*************************************************************************/
int ParseDefine(void)
{
	char nameid[TOKEN_SIZE_MAX];
	int token;
	Stmt *stmt;

	/* Get new identifier name... */
	if ((token = GetNewIdentifier()) == TK_UNKNOWN_ID)
	{
		/* Save id name now. */
		strcpy(nameid, token_buffer);

		/* Get the object being defined... */
		token = GetToken();

		if (token == TK_SURFACE || token == DECL_SURFACE)
		{
			/* 'surface' or <surface name> */
			stmt = (token == DECL_SURFACE) ?
				ParseDeclSurfaceStmt((Surface *)cur_token->data) :
				ParseSurfaceStmt();
			if (stmt != NULL)
			{
				if (!error_count)
				{
					/* Create the surface and store it in the symbol table. */
					parse_declsurfflag = 1;
					ExecStatements(stmt);
					if (parse_declsurf != NULL)
					{
						if (!Symbol_AddLocal(nameid, DECL_SURFACE, 0, (void *)parse_declsurf))
							LogMemError("surface definition");
						parse_declsurf = NULL;
					}
					parse_declsurfflag = 0;
				}
				DeleteStatements(stmt);
			}
		}
		else if (cur_token->flags & TKFLAG_OBJECT)
		{
			/* <object type> or <object name> */
			Object *declobj = NULL;
			if (token == DECL_OBJECT)
				declobj = (Object *)cur_token->data;
			stmt = ParseObject(token, declobj);
			if (stmt != NULL)
			{
				if (!error_count)
				{
					Object *obj;
					/* Create the object and store it in the symbol table. */
					ExecStatements(stmt);
					obj = ScnBuild_RemoveLastObject();
					if (!Symbol_AddLocal(nameid, DECL_OBJECT, TKFLAG_OBJECT, (void *)obj))
						LogMemError("object definition");
				}
				DeleteStatements(stmt);
			}
		}
	}
	else
	{
		ErrUnknown(token, "identifier name", "define");
		/* SkipStatement() */
		return 0;
	}

	return 1;
}