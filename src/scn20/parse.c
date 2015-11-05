/**
 *****************************************************************************
 * @file parse.c
 *  Entry point into the parsing process
 *
 *****************************************************************************
 */

#include "local.h"



/**
 *	Main entry point for parsing the scene file.
 *	We are at the start of a scene description file.
 *
 * @param rsd - RaySetupData* - Receives all of the info needed to set up the renderer.
 */
void parse(RaySetupData * rsd)
{
	int			token;

	if (rsd == NULL)
	{
		assert(0); /* This parameter cannot be NULL */
		return;
	}

	logmsg0("Parsing...");
	
	/* Save a global pointer to the ray setup data. */
	g_rsd = rsd;

	/* Push our first 'global' level on the stack. */
	pcontext_push("global");

	while ((token = gettoken()) != TK_EOF)
	{
		/* First, see if we are declaring variables... */
		if (! parse_vm_decl(token, NULL, 0))
		{
			/* No declarations processed.
			 * Look for other global items.
			 */
			switch (token)
			{
				case TK_MAIN:
					parse_main();
					break;

				case TK_BACKGROUND_SHADER:
				case TK_SURFACE_SHADER:
				case TK_FUNCTION:
					{
						char		name[ MAX_TOKEN_LEN ];
						VMStmt		*newstmt;
						int			func_type = token;

						if ((token = gettoken_GetNewIdentifier()) == TK_UNKNOWN_ID)
						{
							strcpy(name, g_token_buffer);
							newstmt = parse_vm_function(func_type, name);
						}
						else
						{
							gettoken_ErrUnknown(token, "identifier name");
						}
					}
					break;

				case TK_DEFINE:
					parse_define();
					break;

				case TK_UNKNOWN_ID:
					logmsg("Got an unknown ID: %s", g_token_buffer);
					break;

				default:
					gettoken_ErrUnknown(token, NULL);
					break;
			}
		}
		if (g_error_count)
			break;
	}

	/* We're done, clean up. */
	pcontext_pop();
	g_rsd = NULL;
}



/**
 *	Parse and run the main { } block.
 *	The 'main' keyword has just been parsed.
 */
void parse_main(void)
{
	VMStmt *	stmtmain;

	logmsg0("main: Parsing statements...");
	g_compile_mode++;
	stmtmain = parse_vm_function(TK_MAIN, "main");
	g_compile_mode--;

	/*
	 * If we have successfully compiled a list of virtual machine
	 * statements, run them now.
	 */
	if (! g_error_count && (stmtmain != NULL))
	{
		logmsg0("main: Executing statements...");
// TODO:	logmsg0("Press <Esc> to abort.");
		stmtmain->methods->fn(stmtmain);
		vm_delete(stmtmain);
		logmsg0("main: Finished.");
// TODO: Print time elapsed.
	}
}



/**
 * Parse a block of statements.
 *
 *  Called whenever a block of statements is expected. It can be
 *  multiple statements enclosed in { } or a single statement ending with
 *  just the ';'.
 *
 * @return  VMStmt * - Pointer to a list of statements if successful and any are found. NULL otherwise.
 */
VMStmt * parse_vm_block(void)
{
	int			token;
	int			objtype_token;
	int			result;
	VMStmt *	headstmt;
	VMStmt *	newstmt;
	VMStmt *	laststmt;
	int			endtoken;

	newstmt = headstmt = laststmt = NULL;

	// Parse block body...
	//
	token = gettoken();
	if (token != TK_LEFTBRACE)
		gettoken_Unget();
	endtoken = (token == TK_LEFTBRACE) ? TK_RIGHTBRACE : OP_SEMICOLON;
	if (! g_error_count)
	{
		// Parse all statements until we reach the closing '}'.
		//
		while (((token = gettoken()) != endtoken) &&
			! g_error_count)
		{
			// First, see if the token is a basic vm control statement
			// which can appear in all contexts.
			//
			if (! parse_vm_vm_token(token, &newstmt, endtoken == OP_SEMICOLON))
			{
				// No basic vm tokens were processed.
				// See if we are within an object and attempt to
				// process a token specific to that type.
				//
				objtype_token = pcontext_getobjtype();
				switch (objtype_token)
				{
					case DECL_OBJECT:
					case TK_BLOB:
					case TK_BOX:
					case TK_CLOSED_CONE:
					case TK_CLOSED_CYLINDER:
					case TK_CONE:
					case TK_CYLINDER:
					case TK_DISC:
					case TK_NPOLYGON:
					case TK_FN_XYZ:
					case TK_POLYGON:
					case TK_SPHERE:
					case TK_TORUS:
						result = parse_vm_object_modifier_token(token, &newstmt);
						break;

					case TK_OBJECT:
					case TK_UNION:
					case TK_DIFFERENCE:
					case TK_INTERSECTION:
					case TK_CLIP:
						// CSG objects will contain objects within them, so
						// look for objects here.
						//
						result = parse_vm_object_token(token, &newstmt);
						if (! result)
							result = parse_vm_object_modifier_token(token, &newstmt);
						break;

					case TK_SURFACE:
						result = parse_vm_surface_token(token, &newstmt);
						break;

					case TK_BACKGROUND:
						result = parse_vm_background_token(token, &newstmt);
						break;

					case TK_SURFACE_SHADER:
						result = parse_vm_surface_shader_token(token, &newstmt);
						break;

					default:
						// No object-specific stuff, we are most likely at
						// the global level, try for a global token now.
						//
						result = parse_vm_global_token(token, &newstmt);
						break;
				}

				if (! result)
				{
					// Nothing we can make sense of.
					// Report an error.
					//
					gettoken_ErrUnknown(token, NULL);
					break;
				}
			}

			// Add statement(s) to the function's body block.
			//
			if (newstmt != NULL)
			{
				if (laststmt != NULL)
					laststmt->next = newstmt;
				else
					headstmt = newstmt;
				laststmt = newstmt;
				for (laststmt = newstmt;
					laststmt->next != NULL;
					laststmt = laststmt->next)
						; // Advance laststmt to the end of newly added statements.
			}

			if (endtoken == OP_SEMICOLON)
				break;
		}
	}

	return headstmt;
}



int parse_vm_vm_token(int token, VMStmt ** stmtlist, int no_declare)
{
	*stmtlist = NULL;

	if (! parse_vm_decl(token, stmtlist, no_declare))
	{
		switch (token)
		{
			case TK_IF:
				*stmtlist = parse_vm_if();
				break;
			case TK_REPEAT:
				*stmtlist = parse_vm_repeat();
				break;
			case TK_WHILE:
				*stmtlist = parse_vm_while();
				break;
			case TK_FOR:
				*stmtlist = parse_vm_for();
				break;
			case TK_BREAK:
				*stmtlist = parse_vm_break();
				break;

			/* Null statement. */
			case OP_SEMICOLON:
				break;

			case DECL_FLOAT:
			case DECL_VECTOR:
				gettoken_Unget();
				if ((*stmtlist = parse_vm_expr()) != NULL)
				{
					if ((token = gettoken()) == OP_SEMICOLON)
						break;
				}
				return 0;

			case DECL_FUNCTION:
				if ((*stmtlist = parse_vm_function_call((VMStmt *) g_cur_token->data)) != NULL)
				{
					if ((token = gettoken()) == OP_SEMICOLON)
						break;
				}
				return 0;

			default:
				if (g_cur_token->flags & TKFLAG_SHADER_FN)
				{
					gettoken_Unget();
					if ((*stmtlist = parse_vm_expr()) != NULL)
					{
						if ((token = gettoken()) == OP_SEMICOLON)
							break;
					}
				}
				return 0;
		}
	}

	return 1;
}



int parse_vm_object_token(int token, VMStmt **stmtlist)
{
	*stmtlist = NULL;

	switch (token)
	{
		case TK_BLOB:
			*stmtlist = parse_vm_blob();
			break;

		case TK_BOX:
			*stmtlist = parse_vm_box();
			break;

		case TK_DISC:
			*stmtlist = parse_vm_disc();
			break;

		case TK_FN_XYZ:
			*stmtlist = parse_vm_fnxyz();
			break;

		case TK_SPHERE:
			*stmtlist = parse_vm_sphere();
			break;

		case TK_TORUS:
			*stmtlist = parse_vm_torus();
			break;

		case TK_CONE:
		case TK_CLOSED_CONE:
		case TK_CYLINDER:
		case TK_CLOSED_CYLINDER:
			*stmtlist = parse_vm_cone_or_cylinder(token);
			break;

		// Polygon objects
		case TK_NPOLYGON:
		case TK_POLYGON:
			*stmtlist = parse_vm_polygon(token);
			break;

		// CSG objects
		case TK_OBJECT:
		case TK_UNION:
		case TK_DIFFERENCE:
		case TK_INTERSECTION:
		case TK_CLIP:
		case DECL_OBJECT:
			*stmtlist = parse_vm_csg(token);
			break;

		default:
			return 0;
	}

	return 1;
}



int parse_vm_global_token(int token, VMStmt **stmtlist)
{
	*stmtlist = NULL;

	if (! parse_vm_object_token(token, stmtlist))
	{
		switch (token)
		{
			case TK_BACKGROUND:
				*stmtlist = parse_vm_background();
				break;

			case TK_LIGHT:
				*stmtlist = parse_vm_light(token);
				break;

			case TK_VIEWPORT:
				*stmtlist = parse_vm_viewport();
				break;

         case TK_VISIBILITY:
            *stmtlist = parse_vm_visibility();
            break;

			default:
				return 0;
		}
	}

	return 1;
}

int parse_vm_decl(int token, VMStmt **stmtlist, int no_declare)
{
	VMStmt *localstmtlist = NULL;

	switch (token)
	{
		/* Declarations - May return one or more stmts or NULL. */
		case TK_FLOAT:
			if (no_declare)
				gettoken_Error(g_token_buffer, "Declarations not allowed here");
			else
				localstmtlist = parse_vm_float_decl();
			break;
		case TK_VECTOR:
			if (no_declare)
				gettoken_Error(g_token_buffer, "Declarations not allowed here");
			else
				localstmtlist = parse_vm_vector_decl();
			break;

		default:
			return 0;
	}

	if (stmtlist == NULL)
	{
		VMStmt		*stmt;

		/* Execute the statements now.
		 * These are being declared outside of 'main()'.
		 */
		for (stmt = localstmtlist; stmt != NULL; stmt = stmt->next)
			stmt->methods->fn(stmt);
		vm_delete(localstmtlist);
	}
	else
		*stmtlist = localstmtlist;


	return 1;
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
VMStmt *parse_vm_float_decl(void)
{
	int			token;
	char		lv_nameid[ MAX_TOKEN_LEN ];
	VMLValue	*lv;
	VMStmt		*stmtlist = NULL, *laststmt = NULL, *stmt = NULL;

	token = gettoken_GetNewIdentifier();
	for (;;)
	{
		switch (token)
		{
			case TK_UNKNOWN_ID:
				if ((lv = vm_new_lvalue(TK_FLOAT)) != NULL)
				{
					strcpy(lv_nameid, g_token_buffer);
					stmt = parse_vm_lvalue_init(lv);
					if (! pcontext_addsymbol(lv_nameid, DECL_FLOAT, 0, (void *) lv))
						goto alloc_fail;
				}
				else
					goto alloc_fail;
				break;

			default:
				gettoken_ErrUnknown(token, "identifier name");
				vm_delete(stmtlist);
				return NULL;
		}

		if (stmt != NULL)
		{
			if (laststmt != NULL)
				laststmt->next = stmt;
			else
				stmtlist = stmt;
			laststmt = stmt;
			stmt = NULL;
		}
		token = gettoken();
		if (token == OP_SEMICOLON)
			break;
		if (token == OP_COMMA)
			token = gettoken_GetNewIdentifier();
		else
		{
			gettoken_ErrUnknown(token, ";");
			vm_delete(stmtlist);
			return NULL;
		}
	}

	return stmtlist;

	alloc_fail:
	logmemerror("float");
	vm_delete(stmtlist);
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
VMStmt *parse_vm_vector_decl(void)
{
	int			token;
	char		lv_nameid[ MAX_TOKEN_LEN ];
	VMLValue	*lv;
	VMStmt		*stmtlist = NULL, *laststmt = NULL, *stmt = NULL;

	token = gettoken_GetNewIdentifier();
	for (;;)
	{
		switch (token)
		{
			case TK_UNKNOWN_ID:
				if ((lv = vm_new_lvalue(TK_VECTOR)) != NULL)
				{
					strcpy(lv_nameid, g_token_buffer);
					stmt = parse_vm_lvalue_init(lv);
					if (! pcontext_addsymbol(lv_nameid, DECL_VECTOR, 0, (void *) lv))
						goto alloc_fail;
				}
				else
					goto alloc_fail;
				break;

			default:
				gettoken_ErrUnknown(token, "identifier name");
				vm_delete(stmtlist);
				return NULL;
		}

		if (stmt != NULL)
		{
			if (laststmt != NULL)
				laststmt->next = stmt;
			else
				stmtlist = stmt;
			laststmt = stmt;
			stmt = NULL;
		}
		token = gettoken();
		if (token == OP_SEMICOLON)
			break;
		if (token == OP_COMMA)
			token = gettoken_GetNewIdentifier();
		else
		{
			gettoken_ErrUnknown(token, ";");
			vm_delete(stmtlist);
			return NULL;
		}
	}

	return stmtlist;

	alloc_fail:
	logmemerror("vector");
	vm_delete(stmtlist);
	return NULL;
}
