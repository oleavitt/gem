/*************************************************************************
*
*  parse.c - Parses the top level of scene file.
*
*************************************************************************/

#include "local.h"

static int ParseStmt(Stmt **stmt, int (*ParseDetails)(Stmt **stmt));
static Stmt *ParseFloatDecl(void);
static Stmt *ParseVectorDecl(void);

/* Flags to tell everybody that we're in a declaration. */
int parse_declsurfflag = 0;

/*************************************************************************
*
*  Initialization and cleanup.
*
*************************************************************************/
int Parse_Init(void)
{
	ExprEval_Init();
	parse_declsurfflag = 0;
	parse_declsurf = NULL;
	return 1;
}

void Parse_Close(void)
{
}

int ParseStmt(Stmt **stmt, int (*ParseDetails)(Stmt **stmt))
{
	int token;
	token = GetToken();
	*stmt = NULL;
	if (cur_token->flags & TKFLAG_OBJECT)
	{
/*		char nameid[TOKEN_SIZE_MAX];
*/		Object *declobj = NULL;

		if (token == DECL_OBJECT)
			declobj = (Object *)cur_token->data;

		/*
		 * See if we are defining an object name...
		 */
/*		if (GetNewIdentifier() == TK_UNKNOWN_ID)
			strcpy(nameid, token_buffer);
		else
		{
			UngetToken();
			*nameid = '\0';
		}
*/
		*stmt = ParseObject(token, declobj);

		/* If we are defining an object type... */
/*		if (*nameid != '\0' && *stmt != NULL)
		{
			if (!error_count)
			{
				Object *obj;
*/				/* Create the object and store it in the symbol table. */
/*				ExecStatements(*stmt);
				obj = ScnBuild_RemoveLastObject();
				if (!Symbol_AddLocal(nameid, DECL_OBJECT, TKFLAG_OBJECT, (void *)obj))
					LogMemError("object definition");
			}
			DeleteStatements(*stmt);
			*stmt = NULL;
		}
*/
		return TK_NULL;
	}
	else if (token == TK_SURFACE || token == DECL_SURFACE)
	{
/*		char nameid[TOKEN_SIZE_MAX];
		Surface *declsurf;

		assert(parse_declsurfflag == 0);

		if (token == DECL_SURFACE)
			declsurf = (Surface *)cur_token->data;
*/
		/*
		 * See if we are defining a surface name...
		 */
/*		if (GetNewIdentifier() == TK_UNKNOWN_ID)
		{
			strcpy(nameid, token_buffer);
			parse_declsurfflag = 1;
		}
		else
		{
			UngetToken();
			*nameid = '\0';
		}
*/
		*stmt = (token == TK_SURFACE) ? ParseSurfaceStmt() :
			ParseDeclSurfaceStmt((Surface *)cur_token->data);

		/* If we are defining a surface type... */
/*		if (*nameid != '\0' && *stmt != NULL)
		{
			if (!error_count)
			{
*/				/* Create the surface and store it in the symbol table. */
/*				parse_declsurfflag = 1;
				ExecStatements(*stmt);
				if (parse_declsurf != NULL)
				{
					if (!Symbol_AddLocal(nameid, DECL_SURFACE, 0, (void *)parse_declsurf))
						LogMemError("surface definition");
					parse_declsurf = NULL;
				}
				parse_declsurfflag = 0;
			}
			DeleteStatements(*stmt);
			*stmt = NULL;
		}
*/
		return TK_NULL;
	}

	switch (token)
	{
		/*
		 * If 'stmt' is not NULL, it will point to one or more initializer
		 * expressions which will behave like regular assignment expressions
		 * when executed.
		 */
		case TK_FLOAT:
			*stmt = ParseFloatDecl();
			break;
		case TK_VECTOR:
			*stmt = ParseVectorDecl();
			break;

		case TK_DEFINE:
			ParseDefine();
			break;

		case TK_LOAD_COLOR_MAP:
			*stmt = ParseLoadColorMapStmt();
			break;

		case TK_IF:
			*stmt = ParseIfStmt(ParseDetails);
			break;
		case TK_BREAK:
			*stmt = ParseBreakStmt();
			break;
		case TK_CONTINUE:
			*stmt = ParseContinueStmt();
			break;

		case TK_WHILE:
			*stmt = ParseWhileStmt(ParseDetails);
			break;
		case TK_DO:
			*stmt = ParseDoWhileStmt(ParseDetails);
			break;
		case TK_FOR:
			*stmt = ParseForStmt(ParseDetails);
			break;
		case TK_REPEAT:
			*stmt = ParseRepeatStmt(ParseDetails);
			break;

		case TK_CAUSTICS:
			*stmt = ParseCausticsStmt();
			break;
		case TK_BACKGROUND:
			*stmt = ParseBackgroundStmt();
			break;
		case TK_LIGHT:
			*stmt = ParseLightStmt();
			break;
		case TK_INFINITE_LIGHT:
			*stmt = ParseInfiniteLightStmt();
			break;

		case TK_ANAGLYPH:
			*stmt = ParseAnaglyphStmt();
			break;
		case TK_VIEWPORT:
			*stmt = ParseViewportStmt();
			break;

		case TK_MESSAGE:
			*stmt = ParseMessageStmt();
			break;

		default:
			UngetToken();
			if ((*stmt = ParseExprStmt()) != NULL)
			{
				if ((token = GetToken()) == OP_SEMICOLON)
					break;
				return token;
			}
			GetToken();
			return token;
	}
	return TK_NULL;
}

/*************************************************************************
*
*  Parse() - Called after initialization to parse the input file.
*
*************************************************************************/
void Parse(void)
{
	int token;
	Stmt *stmt;

	while ((token = ParseStmt(&stmt, NULL)) != TK_EOF)
	{
		if (stmt != NULL)
		{
			if (!error_count)
				ExecStatements(stmt);
			DeleteStatements(stmt);
		}
		else if(token != TK_NULL)
		{
			/* Parse any top-level (non block) tokens here. */
			switch (token)
			{
    		case TK_INCLUDE_FILE_PATHS:
			   	do
		  		{
    				ExpectToken(TK_QUOTESTRING, "\"quoted name\"", NULL);
			    	SCN_AddPath(&scn_include_paths, token_buffer);
		   			token = GetToken();
    			}
			    while (token == OP_COMMA);
		   		UngetToken();
    			break;
				default:
					ErrUnknown(token, NULL, NULL);
					break;
			}
		}
	}
}

/*************************************************************************
*
*	ParseBlock() - Parses a { } block or single ; terminated statement.
*	Returns a list of compiled statements if successful.
*
*************************************************************************/
Stmt *ParseBlock(const char *block_name, int (*ParseDetails)(Stmt **stmt))
{
	Stmt *s = NULL, *stmtlist = NULL, *stmtlast = NULL;
	int token, one_liner;

	if ((token = GetToken()) != TK_LEFTBRACE)
	{
		UngetToken();
		one_liner = 1;
	}
	else
	{
		Symbol_Push();
		one_liner = 0;
	}

	while (1)
	{
		if (ParseDetails != NULL)
		{
			token = ParseDetails(&s);
			if ((s == NULL) && (token != TK_NULL))
			{
				if ((token == TK_RIGHTBRACE) && (!one_liner))
				{
					Symbol_Pop();
					break;
				}
				if ((token == OP_SEMICOLON) && one_liner)
					break;
				if (token == TK_EOF)
					break;
				UngetToken();
				token = ParseStmt(&s, ParseDetails);
			}
		}
		else
		{
			token = ParseStmt(&s, ParseDetails);
		}
		
		if (s != NULL)
		{
			if (stmtlast != NULL)
				stmtlast->next = s;
			else
				stmtlist = s;
			for (stmtlast = s; stmtlast->next != NULL; stmtlast = stmtlast->next)
				; /* Advance stmtlast to the end of newly added statements. */
		}
		else if (token != TK_NULL)
		{
			if ((token == TK_RIGHTBRACE) && (!one_liner))
			{
				Symbol_Pop();
				break;
			}
			if ((token == OP_SEMICOLON) && one_liner)
				break;
			ErrUnknown(token, NULL, block_name);
			if (token == TK_EOF)
				break;
		}
		if (one_liner)
			break;
	}

	return stmtlist;
}

/*************************************************************************
*
*	ParseObject() - See if token is an object type. If so, parse it.
*	Returns a Stmt ptr if an object was successfully processed.
*
*************************************************************************/
Stmt *ParseObject(int token, Object *baseobj)
{
	Stmt *stmt = NULL;

	switch (token)
	{
		case TK_BOX:
			stmt = ParseBoxStmt();
			break;
		case TK_BLOB:
			stmt = ParseBlobStmt();
			break;
		case TK_CLOSED_CONE:
		case TK_CLOSED_CYLINDER:
		case TK_CONE:
		case TK_CYLINDER:
			stmt = ParseConeStmt(token);
			break;
		case TK_DISC:
			stmt = ParseDiscStmt();
			break;
		case TK_HEIGHT_FIELD:
		case TK_SMOOTH_HEIGHT_FIELD:
			stmt = ParseHeightFieldStmt(token == TK_SMOOTH_HEIGHT_FIELD);
			break;
		case TK_IMPLICIT:
			stmt = ParseImplicitStmt();
			break;
		case TK_NURBS:
			stmt = ParseNurbsStmt();
			break;
		case TK_POLYGON:
			stmt = ParsePolygonStmt();
			break;
		case TK_POLYMESH:
		case TK_SMOOTH_POLYMESH:
			stmt = ParsePolymeshStmt(token == TK_SMOOTH_POLYMESH);
			break;
		case TK_SPHERE:
			stmt = ParseSphereStmt();
			break;
		case TK_TORUS:
			stmt = ParseTorusStmt();
			break;
		case TK_TRIANGLE:
			stmt = ParseTriangleStmt();
			break;
		case TK_OBJECT:
		case TK_UNION:
		case TK_DIFFERENCE:
		case TK_INTERSECTION:
		case TK_CLIP:
			stmt = ParseCSGStmt(token);
			break;
		case DECL_OBJECT:
			stmt = ParseDeclObjectStmt(baseobj);
			break;
		default:
			break;
	}

	return stmt;
}

/*************************************************************************
*
*  The "float" declaration statement.
*
*  If declaration is followed by an initializer expression ParseXXXDecl()
*  will return a pointer to a Stmt containing the expression, otherwise
*  NULL is returned.
*
*************************************************************************/
Stmt *ParseFloatDecl(void)
{
	int token;
	char lv_nameid[TOKEN_SIZE_MAX];
	LValue *lv;
	Stmt *stmtlist = NULL, *laststmt = NULL, *stmt = NULL;
	token = GetNewIdentifier();
	while (1)
	{
		switch (token)
		{
			case TK_UNKNOWN_ID:
				if ((lv = LValueNew(TK_FLOAT)) != NULL)
				{
					strcpy(lv_nameid, token_buffer);
					stmt = ParseLVInitExprStmt(lv);
					if (!Symbol_AddLocal(lv_nameid, DECL_FLOAT, 0, (void *)lv))
						goto alloc_fail;
				}
				else
					goto alloc_fail;
				break;

			default:
				ErrUnknown(token, "identifier name", "float");
				DeleteStatements(stmtlist);
				return NULL;
		}
		if (stmt != NULL)
		{
			if(laststmt != NULL)
				laststmt->next = stmt;
			else
				stmtlist = stmt;
			laststmt = stmt;
			stmt = NULL;
		}
		token = GetToken();
		if (token == OP_SEMICOLON)
			break;
		if (token == OP_COMMA)
			token = GetNewIdentifier();
		else
		{
			ErrUnknown(token, ";", "float");
			DeleteStatements(stmtlist);
			return NULL;
		}
	}

	return stmtlist;

	alloc_fail:
	LogMemError("float");
	DeleteStatements(stmtlist);
	return NULL;
}

/*************************************************************************
*
*  The "vector" declaration statement.
*
*  If declaration is followed by an initializer expression ParseXXXDecl()
*  will return a pointer to a Stmt containing the expression, otherwise
*  NULL is returned.
*
*************************************************************************/
Stmt *ParseVectorDecl(void)
{
	int token;
	char lv_nameid[TOKEN_SIZE_MAX];
	LValue *lv;
	Stmt *stmtlist = NULL, *laststmt = NULL, *stmt = NULL;
	token = GetNewIdentifier();
	while (1)
	{
		switch (token)
		{
			case TK_UNKNOWN_ID:
				if ((lv = LValueNew(TK_VECTOR)) != NULL)
				{
					strcpy(lv_nameid, token_buffer);
					stmt = ParseLVInitExprStmt(lv);
					if (!Symbol_AddLocal(lv_nameid, DECL_VECTOR, 0, (void *)lv))
						goto alloc_fail;
				}
				else
					goto alloc_fail;
				break;

			default:
				ErrUnknown(token, "identifier name", "vector");
				DeleteStatements(stmtlist);
				return NULL;
		}
		if (stmt != NULL)
		{
			if(laststmt != NULL)
				laststmt->next = stmt;
			else
				stmtlist = stmt;
			laststmt = stmt;
			stmt = NULL;
		}
		token = GetToken();
		if (token == OP_SEMICOLON)
			break;
		if (token == OP_COMMA)
			token = GetNewIdentifier();
		else
		{
			ErrUnknown(token, ";", "vector");
			DeleteStatements(stmtlist);
			return NULL;
		}
	}

	return stmtlist;

	alloc_fail:
	LogMemError("vector");
	DeleteStatements(stmtlist);
	return NULL;
}
