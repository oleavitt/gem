/**
 *****************************************************************************
 *  @file vmproc.c
 *  Branching and loop control statements for the virtual machine.
 *
 *****************************************************************************
 */

#include "local.h"


static int s_break_count = 0;

/*************************************************************************/

/*
 * Container for a 'repeat' loop statement.
 */
typedef struct tVMStmtRepeat
{
	VMStmt		vmstmt;
	VMStmt		*block;
	VMExpr		*repeat_count;
} VMStmtRepeat;



/*
 * Methods for the 'repeat' stmt.
 */
static void vm_repeat(VMStmt * stmthead);
static void vm_repeat_cleanup(VMStmt * thisstmt);

static VMStmtMethods s_repeat_stmt_methods =
{
	TK_REPEAT,
	vm_repeat,
	vm_repeat_cleanup
};



VMStmt * parse_vm_repeat(void)
{
	int				token;
	VMStmtRepeat	*newstmt =
		(VMStmtRepeat *) vm_alloc_stmt(sizeof(VMStmtRepeat), &s_repeat_stmt_methods);

	if (newstmt == NULL)
	{
		logmemerror("repeat");
		return NULL;
	}

	/* Save previous state on the stack. */
	pcontext_push("repeat");

	/* parse the repeat count expression. */
	if((token = gettoken()) == OP_LPAREN)
	{
		newstmt->repeat_count = parse_exprtree();
		if((token = gettoken()) == OP_RPAREN)
		{
			/* Parse the repeat statement's body. */
			newstmt->block = parse_vm_block();
		}
		else
			gettoken_ErrUnknown(token, ")");
	}
	else
		gettoken_ErrUnknown(token, "(");

	/* Restore previous state. */
	pcontext_pop();

	return (VMStmt *)newstmt;
}



/**
 *	VM repeat (repeat_count expr) { block }
 */
void vm_repeat(VMStmt *thisstmt)
{
	VMStmtRepeat	*stmtrepeat = (VMStmtRepeat *) thisstmt;
	VMStmt			*stmt;
	int				repeat_count, count;

	vmstack_push(thisstmt);

	/* The actual 'for' statement. */
	repeat_count = (int) fabs(vm_evaldouble(stmtrepeat->repeat_count));
	for (count = 0; count < repeat_count; count++)
	{
	 	/* Loop thru the statements in this block. */
		for (stmt = stmtrepeat->block; stmt != NULL; stmt = stmt->next)
		{
			stmt->methods->fn(stmt);
			if (s_break_count > 0)
			{
				s_break_count--;
				goto break_loop;
			}
		}
	}

	break_loop:

	vmstack_pop();
}

/**
 *	Cleanup function for VM 'repeat' stmt.
 */
void vm_repeat_cleanup(VMStmt *thisstmt)
{
	VMStmtRepeat *	stmtrepeat = (VMStmtRepeat *) thisstmt;

	/* Free the repeat count expression */
	delete_exprtree(stmtrepeat->repeat_count);
	stmtrepeat->repeat_count = NULL;

	/* Recursively free the statements in our block. */
	vm_delete(stmtrepeat->block);
	stmtrepeat->block = NULL;
}

/*************************************************************************/

/*
 * Container for a 'while' loop statement.
 */
typedef struct tVMStmtWhile
{
	VMStmt		vmstmt;
	VMStmt		*block;
	VMExpr		*cond;
} VMStmtWhile;



/*
 * Methods for the 'while' stmt.
 */
static void vm_while(VMStmt * stmthead);
static void vm_while_cleanup(VMStmt * thisstmt);

static VMStmtMethods s_while_stmt_methods =
{
	TK_WHILE,
	vm_while,
	vm_while_cleanup
};



VMStmt * parse_vm_while(void)
{
	int			token;
	VMStmtWhile *	newstmt =
		(VMStmtWhile *) vm_alloc_stmt(sizeof(VMStmtWhile), &s_while_stmt_methods);

	if (newstmt == NULL)
	{
		logmemerror("while");
		return NULL;
	}

	/* Save previous state on the stack. */
	pcontext_push("while");

	/* parse the condition expression. */
	if((token = gettoken()) == OP_LPAREN)
	{
		newstmt->cond = parse_exprtree();
		if((token = gettoken()) == OP_RPAREN)
		{
			/* Parse the this statement's body. */
			newstmt->block = parse_vm_block();
		}
		else
			gettoken_ErrUnknown(token, ")");
	}
	else
		gettoken_ErrUnknown(token, "(");

	/* Restore previous state. */
	pcontext_pop();

	return (VMStmt *)newstmt;
}



/**
 *	VM while (condition expr) { block }
 */
void vm_while(VMStmt *thisstmt)
{
	VMStmtWhile	*stmtwhile = (VMStmtWhile *) thisstmt;
	VMStmt		*stmt;

	vmstack_push(thisstmt);

	/* The actual 'while' statement. */
	while (vm_evaldouble(stmtwhile->cond) != 0.0)
	{
	 	/* Loop thru the statements in this block. */
		for (stmt = stmtwhile->block; stmt != NULL; stmt = stmt->next)
		{
			stmt->methods->fn(stmt);
			if (s_break_count > 0)
			{
				s_break_count--;
				goto break_loop;
			}
		}
	}

	break_loop:

	vmstack_pop();
}

/**
 *	Cleanup function for VM 'while' stmt.
 */
void vm_while_cleanup(VMStmt *thisstmt)
{
	VMStmtWhile *	stmtwhile = (VMStmtWhile *) thisstmt;

	/* Free the control expressions */
	delete_exprtree(stmtwhile->cond);
	stmtwhile->cond = NULL;

	/* Recursively free the statements in our block. */
	vm_delete(stmtwhile->block);
	stmtwhile->block = NULL;
}

/*************************************************************************/

/*
 * Container for a 'for' loop statement.
 */
typedef struct tVMStmtFor
{
	VMStmt		vmstmt;
	VMStmt *	block;
	VMExpr *	init;
	VMExpr *	cond;
	VMExpr *	incr;
} VMStmtFor;



/*
 * Methods for the 'for' stmt.
 */
static void vm_for(VMStmt * stmthead);
static void vm_for_cleanup(VMStmt * thisstmt);

static VMStmtMethods s_for_stmt_methods =
{
	TK_FOR,
	vm_for,
	vm_for_cleanup
};



VMStmt * parse_vm_for(void)
{
	int			token;
	VMStmtFor *	newstmt = 
		(VMStmtFor *) vm_alloc_stmt(sizeof(VMStmtFor), &s_for_stmt_methods);

	if (newstmt == NULL)
	{
		logmemerror("for");
		return NULL;
	}

	/* Save previous state on the stack. */
	pcontext_push("for");

	/* parse the initializer, condition and increment expressions. */
	if((token = gettoken()) == OP_LPAREN)
	{
		newstmt->init = parse_exprtree();
		if((token = gettoken()) == OP_SEMICOLON)
		{
			newstmt->cond = parse_exprtree();
			if((token = gettoken()) == OP_SEMICOLON)
			{
				newstmt->incr = parse_exprtree();
				if((token = gettoken()) == OP_RPAREN)
				{
					/* Parse the for statement's body. */
					newstmt->block = parse_vm_block();
				}
				else
					gettoken_ErrUnknown(token, ")");
			}
			else
				gettoken_ErrUnknown(token, ";");
		}
		else
			gettoken_ErrUnknown(token, ";");
	}
	else
		gettoken_ErrUnknown(token, "(");

	/* Restore previous state. */
	pcontext_pop();

	return (VMStmt *)newstmt;
}



/**
 *	VM for (init expr; cond expr; incr expr) { block }
 */
void vm_for(VMStmt *thisstmt)
{
	VMStmtFor *	stmtfor = (VMStmtFor *) thisstmt;
	VMStmt *	stmt;

	vmstack_push(thisstmt);

	/* The actual 'for' statement. */
	for (stmtfor->init->fn(stmtfor->init);
		 vm_evaldouble(stmtfor->cond) != 0.0;
		 stmtfor->incr->fn(stmtfor->incr))
	{
	 	/* Loop thru the statements in this block. */
		for (stmt = stmtfor->block; stmt != NULL; stmt = stmt->next)
		{
			stmt->methods->fn(stmt);
			if (s_break_count > 0)
			{
				s_break_count--;
				goto break_loop;
			}
		}
	}

	break_loop:

	vmstack_pop();
}

/**
 *	Cleanup function for VM 'for' stmt.
 */
void vm_for_cleanup(VMStmt *thisstmt)
{
	VMStmtFor *	stmtfor = (VMStmtFor *) thisstmt;

	/* Free the control expressions */
	delete_exprtree(stmtfor->init);
	stmtfor->init = NULL;
	delete_exprtree(stmtfor->cond);
	stmtfor->cond = NULL;
	delete_exprtree(stmtfor->incr);
	stmtfor->incr = NULL;

	/* Recursively free the statements in our block. */
	vm_delete(stmtfor->block);
	stmtfor->block = NULL;
}

/*************************************************************************/

/*
 * Container for an 'if/else' statement.
 */
typedef struct tVMStmtIf
{
	VMStmt		vmstmt;
	VMStmt		*block;
	VMStmt		*elseblock;
	VMExpr		*cond;
} VMStmtIf;



/*
 * Methods for the 'if/else' statement.
 */
static void vm_if(VMStmt * stmthead);
static void vm_if_cleanup(VMStmt * thisstmt);

static VMStmtMethods s_if_stmt_methods =
{
	TK_IF,
	vm_if,
	vm_if_cleanup
};



VMStmt * parse_vm_if(void)
{
	int			token;
	VMStmtIf *	newstmt =
		(VMStmtIf *) vm_alloc_stmt(sizeof(VMStmtIf), &s_if_stmt_methods);

	if (newstmt == NULL)
	{
		logmemerror("if");
		return NULL;
	}

	/* Save previous state on the stack. */
	pcontext_push("if");

	/* parse the  if condition expression. */
	if ((token = gettoken()) == OP_LPAREN)
	{
		newstmt->cond = parse_exprtree();
		if ((token = gettoken()) == OP_RPAREN)
		{
			/* Parse the if statement's 'true' body. */
			newstmt->block = parse_vm_block();

			/* Now look for an 'else' block...*/
			if (gettoken() == TK_ELSE)
			{
				/* Parse the if statement's 'else' body. */
				newstmt->elseblock = parse_vm_block();
			}
			else
			{
				gettoken_Unget();
				newstmt->elseblock = NULL;
			}
		}
		else
			gettoken_ErrUnknown(token, ")");
	}
	else
		gettoken_ErrUnknown(token, "(");

	/* Restore previous state. */
	pcontext_pop();

	return (VMStmt *) newstmt;
}



/**
 *	VM if (cond expr) { block } else { block }
 */
void vm_if(VMStmt *thisstmt)
{
	VMStmtIf	*stmtif = (VMStmtIf *) thisstmt;
	VMStmt		*stmt;

	vmstack_push(thisstmt);

	/* The actual 'if/else' statement. */
	if (vm_evaldouble(stmtif->cond) != 0.0)
	{
	 	/* Loop thru the statements in the 'if' block. */
		for (stmt = stmtif->block; stmt != NULL; stmt = stmt->next)
		{
			stmt->methods->fn(stmt);
			if (s_break_count > 0)
				break;
		}
	}
	else
	{
	 	/* Loop thru the statements in the 'else' block. */
		for (stmt = stmtif->elseblock; stmt != NULL; stmt = stmt->next)
		{
			stmt->methods->fn(stmt);
			if (s_break_count > 0)
				break;
		}
	}

	vmstack_pop();
}

/**
 *	Cleanup function for VM 'if/else' stmt.
 */
void vm_if_cleanup(VMStmt *thisstmt)
{
	VMStmtIf *	stmtif = (VMStmtIf *) thisstmt;

	/* Free the control expression */
	delete_exprtree(stmtif->cond);
	stmtif->cond = NULL;

	/* Recursively free the statements in our blocks. */
	vm_delete(stmtif->block);
	stmtif->block = NULL;
	vm_delete(stmtif->elseblock);
	stmtif->elseblock = NULL;
}

/*************************************************************************/

/*
 * Container for a 'break' statement.
 */
typedef struct tVMStmtBreak
{
	VMStmt		vmstmt;
	VMExpr		*break_count;
} VMStmtBreak;



/*
 * Methods for the 'break' stmt.
 */
static void vm_break(VMStmt * stmt);
static void vm_break_cleanup(VMStmt * thisstmt);

static VMStmtMethods s_break_stmt_methods =
{
	TK_BREAK,
	vm_break,
	vm_break_cleanup
};



VMStmt * parse_vm_break(void)
{
	int				token;
	VMStmtBreak	*newstmt = 
		(VMStmtBreak *) vm_alloc_stmt(sizeof(VMStmtBreak), &s_break_stmt_methods);

	if (newstmt == NULL)
	{
		logmemerror("break");
		return NULL;
	}

	/* Save previous state on the stack. */
	pcontext_push("break");

	/* parse the optional break count expression. */
	token = gettoken();
	if (token == OP_LPAREN)
	{
		newstmt->break_count = parse_exprtree();
		if ((token = gettoken()) == OP_RPAREN)
		{
			if ((token = gettoken()) != OP_SEMICOLON)
				gettoken_ErrUnknown(token, ";");
		}
		else
			gettoken_ErrUnknown(token, ")");
	}
	else if (token != OP_SEMICOLON)
		gettoken_ErrUnknown(token, "(expr) or ;");

	/* Restore previous state. */
	pcontext_pop();

	return (VMStmt *)newstmt;
}



/**
 *	VM break (break_count expr);
 */
void vm_break(VMStmt *thisstmt)
{
	VMStmtBreak	*stmt = (VMStmtBreak *) thisstmt;

	/*
	 * If the optional break count expression is there, evaluate it
	 * for a break count, else just set the break count to one to
	 * break out of just one loop like the C break statement.
	 */
	if (stmt->break_count != NULL)
		s_break_count = (int) fabs(vm_evaldouble(stmt->break_count));
	else
		s_break_count = 1;
}

/**
 *	Cleanup function for VM 'break' stmt.
 */
void vm_break_cleanup(VMStmt *thisstmt)
{
	VMStmtBreak *	stmt = (VMStmtBreak *) thisstmt;

	/* Free the break count expression */
	delete_exprtree(stmt->break_count);
	stmt->break_count = NULL;
}

