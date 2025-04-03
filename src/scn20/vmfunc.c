/**
 *****************************************************************************
 * @file vmfunc.c
 *  Virtual machine functions for creating functions.
 *
 *****************************************************************************
 */

#include "local.h"

/**
 * Container for a function statement.
 */
typedef struct tVMStmtFunction
{
	VMStmt			vmstmt;
	VMStmt			*block;
	VMArgument		*arglist;
	VMLValue       **lv_list;
	int				lv_list_len, lv_list_len_alloced;
	/*int				ref_count;*/
	/* TODO: return type mechanism */
} VMStmtFunction;



/**
 * Container for a function call statement.
 */
typedef struct tVMStmtFunctionCall
{
	VMStmt			vmstmt;
	VMStmtFunction	*function;
	VMArgument		*arglist;
	/* TODO: return type mechanism */
} VMStmtFunctionCall;



/*
 * Methods for the function stmt.
 */
static void vm_function(VMStmt * thisstmt);
static void vm_function_cleanup(VMStmt * thisstmt);
static void vm_function_call(VMStmt * thisstmt);
static void vm_function_call_cleanup(VMStmt * thisstmt);

static VMStmtMethods s_func_stmt_methods =
{
	TK_FUNCTION,
	vm_function,
	vm_function_cleanup
};

static VMStmtMethods s_func_call_stmt_methods =
{
	TK_FUNCTION,
	vm_function_call,
	vm_function_call_cleanup
};



// Points to the current function statement being parsed
// by parse_vm_function().
//
static VMStmtFunction * s_cur_function_stmt = NULL;

#define LV_LIST_ALLOC_INCR       16

/**
 *  Parse a function or shader declaration
 */
VMStmt * parse_vm_function(int function_type_token, const char *function_name)
{
	VMStmtFunction *	thisstmt;

	assert(s_cur_function_stmt == NULL); // No re-entry!

	thisstmt = (VMStmtFunction *) vm_alloc_stmt(sizeof(VMStmtFunction), &s_func_stmt_methods);
	if (thisstmt != NULL)
	{
		s_cur_function_stmt = thisstmt;
		thisstmt->arglist = NULL;
		thisstmt->lv_list = NULL;
		thisstmt->lv_list_len = 0;
		thisstmt->lv_list_len_alloced = 0;
		/*ref_count = 1;*/
	}
	else
	{
		logmemerror(function_name);
		return NULL;
	}

	// Push this function's state onto the stack.
	//
	pcontext_push(function_name);

	if (strcmp(function_name, "main") != 0)
		g_function_mode = 1;

	// Parse argument list and add their names to the local symbols.
	//
	thisstmt->arglist = parse_vm_argument_declarations();

	if (!g_error_count)
	{
		// Forward declare this function in symbol table now so that
		// it can be referred to recursively.
		// Do not declare "main" since it is implicitly called as the
		// entry point for the VM.
		//
		if (g_function_mode)
			pcontext_addsymbol(function_name, DECL_FUNCTION, 1, (void *)thisstmt);

		// Parse function body
		//
		thisstmt->block = parse_vm_block();
	}

	if (g_error_count)
	{
		// There were errors while parsing this function.
		// Ditch it and return NULL.
		// In g_function_mode, the partially complete function statement is
		// in the symbol table and will be deleted there. 
		//
		if ( ! g_function_mode)
			vm_delete((VMStmt *)thisstmt);
		thisstmt = NULL;
	}

	// Restore the previous state.
	//
	g_function_mode = 0;
	pcontext_pop();

	s_cur_function_stmt = NULL;

	return (VMStmt *)thisstmt;
}



/**
 *	The name of a function that is in the symbol table has just been parsed.
 *	Parse the argument expressions for the function call and build a
 *	function call statement.
 */
VMStmt *parse_vm_function_call(VMStmt *stmtfunc)
{
	VMStmtFunctionCall	*thisstmt = NULL;
	VMStmtFunction		*function_stmt = (VMStmtFunction *) stmtfunc;
	int					token;
	VMArgument			*arg_func;
	VMArgument			*arg_call, *lastarg;
	VMExpr				*expr;

	thisstmt = (VMStmtFunctionCall *) vm_alloc_stmt(sizeof(VMStmtFunctionCall), &s_func_call_stmt_methods);
	if (thisstmt != NULL)
	{
		thisstmt->arglist = NULL;
		thisstmt->function = function_stmt;
	}
	else
	{
		logmemerror(g_token_buffer);
		return NULL;
	}

	/* NOTE: 'function_stmt' may be incomplete if it is only forward
	 * declared at this point. This will be the case in recursive function
	 * calls. It won't have a body yet, but the argument list should
	 * be there.
	 */

	token = gettoken_Expect(OP_LPAREN, "(");
	if (token == OP_LPAREN)
	{
		/* For each function argument l-value, expect an expression
		 * in this list, and add it to a local argument list.
		 */
		lastarg = NULL;
		for (arg_func = function_stmt->arglist;
			arg_func != NULL;
			arg_func = arg_func->next)
		{
			/* Parse the expression. */
			expr = parse_exprtree();

			/* Add this argument element to the function call arg list. */
			arg_call = (VMArgument *) calloc(1, sizeof(VMArgument));
			if (arg_call != NULL)
			{
				arg_call->expr = expr;
				if (lastarg != NULL)
					lastarg->next = arg_call;
				else
					thisstmt->arglist = arg_call;
				lastarg = arg_call;
			}
			else
			{
				delete_exprtree(expr);
				logmemerror("function arg");
				goto bail_out_of_here;
			}

			/* If there are more args, expect a comma. */
			if (arg_func->next != NULL)
			{
				gettoken_Expect(OP_COMMA, ",");
			}
		}

		/* Finally, parse the closing ')'. */
		token = gettoken_Expect(OP_RPAREN, ")");
	}

bail_out_of_here:
	if (g_error_count)
	{
		vm_delete((VMStmt *) thisstmt);
		thisstmt = NULL;
	}

	return (VMStmt *) thisstmt;
}



VMArgument *parse_vm_argument_declarations(void)
{
	int			token, type, got_comma = 0;
	VMArgument	*arglist = NULL, *lastarg = NULL;
	VMArgument	*arg;
	char		name[MAX_TOKEN_LEN];

	token = gettoken();
	if (token == OP_LPAREN)
	{
		while ((token = gettoken()) != OP_RPAREN)
		{
			switch (token)
			{
				case TK_FLOAT:
				case TK_VECTOR:
					got_comma = 0;
					type = token;
					token = gettoken_GetNewIdentifier();
					if (token == TK_UNKNOWN_ID)
					{
						strcpy(name, g_token_buffer);
						arg = (VMArgument *) calloc(1, sizeof(VMArgument));
						if (arg != NULL)
						{
							arg->lv = vm_new_lvalue(type);
							if (arg->lv != NULL)
							{
								if (!pcontext_addsymbol(name,
										(type == TK_FLOAT) ? DECL_FLOAT : DECL_VECTOR,
										0,
										(void *)arg->lv))
								{
									vm_delete_arglist(arg);
									goto fail_alloc;
								}
							}
							else
							{
								vm_delete_arglist(arg);
								goto fail_alloc;
							}
						}
						else
							goto fail_alloc;
					}
					else
						goto fail_error;
					break;

				default:
					vm_delete_arglist(arglist);
					gettoken_ErrUnknown(token, "argument type");
					return NULL;
			}

			if (arg != NULL)
			{
				if (lastarg != NULL)
					lastarg->next = arg;
				else
					arglist = arg;
				lastarg = arg;
			}

			token = gettoken();
			if (token == OP_COMMA)
			{
				got_comma = 1;
			}
			else if (token == OP_RPAREN)
			{
				gettoken_Unget();
			}
			else
			{
				vm_delete_arglist(arglist);
				gettoken_ErrUnknown(token, "',' or ')'");
				return NULL;
			}
		}		
	}
	else
		gettoken_Unget();

	return arglist;

fail_error:
	vm_delete_arglist(arglist);
	gettoken_ErrUnknown(token, "identifier name");
	return NULL;

fail_alloc:
	vm_delete_arglist(arglist);
	logmemerror("function argument list");
	return NULL;
}



void vm_function_add_to_statelist(int type_token, void *lv)
{
	if ((type_token == DECL_FLOAT) ||
		(type_token == DECL_VECTOR))
	{
		assert(s_cur_function_stmt != NULL);
		if (s_cur_function_stmt != NULL)
		{
			if (s_cur_function_stmt->lv_list_len ==
				s_cur_function_stmt->lv_list_len_alloced)
			{
				int newsize = s_cur_function_stmt->lv_list_len_alloced +
					LV_LIST_ALLOC_INCR;
				VMLValue ** newlist = (VMLValue **) calloc(newsize, sizeof(VMLValue *));
				if (newlist != NULL)
				{
					if (s_cur_function_stmt->lv_list != NULL)
					{
						memcpy(newlist, s_cur_function_stmt->lv_list,
							s_cur_function_stmt->lv_list_len * sizeof(VMLValue *));
						free(s_cur_function_stmt->lv_list);
					}
					s_cur_function_stmt->lv_list_len_alloced = newsize;
					s_cur_function_stmt->lv_list = newlist;
				}
			}
			
			if (s_cur_function_stmt->lv_list != NULL)
			{
				s_cur_function_stmt->lv_list[s_cur_function_stmt->lv_list_len] =
					(VMLValue *)lv;
				s_cur_function_stmt->lv_list_len++;
			}
		}
	}       
}



/*
void vm_function_addref(VMStmt *stmtfunc)
{
	VMStmtFunction	*stmt = (VMStmtFunction *) stmtfunc;

	if (stmt != NULL)
		stmt->ref_count++;
}
*/


/**
 *	VM function name[([argument(s)])] { block }
 */
void vm_function(VMStmt *thisstmt)
{
	VMStmtFunction	*stmtfunc = (VMStmtFunction *) thisstmt;
	VMStmt			*stmt;

	vmstack_push(thisstmt);

	/* Loop thru the statements in this block. */
	for (stmt = stmtfunc->block; stmt != NULL; stmt = stmt->next)
	{
		stmt->methods->fn(stmt);
	}

	vmstack_pop();
}



/**
 *	Cleanup function for VM function() stmt.
 *	Called when this function is deleted from the symbol table.
 */
void vm_function_cleanup(VMStmt * thisstmt)
{
	VMStmtFunction *	stmtfunc = (VMStmtFunction *) thisstmt;

	/* Recursively free the statements in our block. */
	vm_delete(stmtfunc->block);
	stmtfunc->block = NULL;

	/* Free up the argument l-vlaues for this function. */
	vm_delete_arglist(stmtfunc->arglist);
	stmtfunc->arglist = NULL;
}



/**
 *	VM function call function name([argument(s)]);
 */
void vm_function_call(VMStmt * thisstmt)
{
	VMArgument			*arg_call, *arg_func;
	VMStmtFunctionCall	*stmt = (VMStmtFunctionCall *) thisstmt;

	assert(stmt->function != NULL);
	if (stmt->function != NULL)
	{
		assert(stmt->function->vmstmt.methods->token == TK_FUNCTION);

		vmstack_save_lvalues(stmt->function->lv_list, stmt->function->lv_list_len);
	
		/* Evaluate the argument expressions and pass the results to the
		 * associated l-values in the function.
		 */
		for (arg_call = stmt->arglist, arg_func = stmt->function->arglist;
			(arg_call != NULL) && (arg_func != NULL);
			arg_call = arg_call->next, arg_func = arg_func->next)
		{
			if (arg_func->lv->type == TK_FLOAT)
				arg_func->lv->v.x = vm_evaldouble(arg_call->expr);
			else if (arg_func->lv->type == TK_VECTOR)
				vm_evalvector(arg_call->expr, &arg_func->lv->v);
			else
			{
				assert(0); /* ??? arg type */
				/* TODO: strings, objects, surfaces, etc. as args. */
			}
		}

		/* Both lists must have the same # of args!
		 * Both should be NULL by the time we get here.
		 */
		assert((arg_call == NULL) && (arg_func == NULL));

		/* Run the function. */
		// TODO: This could be a shader as well
		stmt->function->vmstmt.methods->fn((VMStmt *)stmt->function);

		vmstack_restore_lvalues(stmt->function->lv_list, stmt->function->lv_list_len);
	}
}



/**
 *	Cleanup function for VM function call stmt.
 */
void vm_function_call_cleanup(VMStmt * thisstmt)
{
	VMStmtFunctionCall	*stmt = (VMStmtFunctionCall *) thisstmt;

	/* Free up the argument expressions for this function call.
	 * NOTE: The 'function' stmt is not freed here as it may be used
	 * in other places.
	 * The function is owned by a symbol table and will be freed
	 * when that symbol table is freed.
	 */
	vm_delete_arglist(stmt->arglist);
	stmt->arglist = NULL;
}



/**
 *	Free up the argument list any expressions or l-values in it.
 */
void vm_delete_arglist(VMArgument *arglist)
{
	VMArgument *arg;

	while (arglist != NULL)
	{
		arg = arglist;
		arglist = arglist->next;
		delete_exprtree(arg->expr);
		vm_delete_lvalue(arg->lv);
		free(arg);
	}
}

