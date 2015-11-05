/**
 *****************************************************************************
 *  @file vmviewpt.c
 *  Virtual machine functions for the viewport.
 *
 *****************************************************************************
 */

#include "local.h"



/*
 * Container for a 'viewport' object statement.
 */
typedef struct tVMStmtViewport
{
	VMStmt		vmstmt;
	VMStmt		*block;
	VMExpr		*expr_from;
	VMExpr		*expr_at;
	VMExpr		*expr_angle;
	VMLValue	*lv_from;
	VMLValue	*lv_at;
	VMLValue	*lv_angle;
} VMStmtViewport;



/*
 * Methods for the 'viewport' stmt.
 */
static void vm_viewport(VMStmt *thisstmt);
static void vm_viewport_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_viewport_stmt_methods =
{
	TK_VIEWPORT,
	vm_viewport,
	vm_viewport_cleanup
};



/**
 *	Parse the viewport { } block.
 *
 *	The 'viewport' keyword has just been parsed.
 *
 *	@return VMStmt *, ptr to a viewport stmt if we are compiling
 *		for the virtual machine. Returns NULL otherwise.
 */
VMStmt * parse_vm_viewport(void)
{
	VMStmtViewport *	newstmt =
		(VMStmtViewport *) vm_alloc_stmt(sizeof(VMStmtViewport), &s_viewport_stmt_methods);
	ParamList			params[4];
	int					nparams, i;

	if (newstmt == NULL)
	{
		logmemerror("viewport");
		return NULL;
	}

	/* Save previous state on the stack. */
	pcontext_push("viewport");
	pcontext_setobjtype(TK_VIEWPORT);

	/*
	 * Add 'from', 'at' and 'angle' to the local namespace.
	 * These will be bound at runtime to the actual fields in the object.
	 */
	newstmt->lv_from = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_from != NULL)
		pcontext_addsymbol("from", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_from));
	newstmt->lv_at = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_at != NULL)
		pcontext_addsymbol("at", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_at));
	newstmt->lv_angle = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_angle != NULL)
		pcontext_addsymbol("angle", DECL_FLOAT, 0,
			(void *)vm_copy_lvalue(newstmt->lv_angle));

	/* Parse the viewport's parameters and body. */
	nparams = parse_paramlist("OEOEOEOB", "viewport", params);
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr_from == NULL) /* First one is the from vector. */
					newstmt->expr_from = params[i].data.expr;
				else if (newstmt->expr_at == NULL) /* Second one is the at vector. */
					newstmt->expr_at = params[i].data.expr;
				else                 /* Third one is the angle. */
					newstmt->expr_angle = params[i].data.expr;
				break;
			case PARAM_BLOCK:
				newstmt->block = params[i].data.block;
				break;
		}
	}

	/* Restore previous state. */
	pcontext_pop();

	return (VMStmt *)newstmt;
}



/*
int parse_vm_viewport_token(int token, VMStmt **stmtlist)
{
	*stmtlist = NULL;

	switch (token)
	{
		default:
			return parse_vm_object_modifier_token(token, stmtlist);
	}

	return 1;
}
*/


/*************************************************************************/

/**
 *	VM viewport [from, at, angle] { block }
 */
void vm_viewport(VMStmt *curstmt)
{
	VMStmtViewport *	stmtobj = (VMStmtViewport *) curstmt;
	VMStmt *			stmt;
	
	vmstack_push(curstmt);
	
	/* Evaluate any parameters we have provided. */
	if (stmtobj->expr_from != NULL)
		vm_evalvector(stmtobj->expr_from, &g_rsd->viewport.LookFrom);
	if (stmtobj->expr_at != NULL)
		vm_evalvector(stmtobj->expr_at, &g_rsd->viewport.LookAt);
	if (stmtobj->expr_angle != NULL)
		g_rsd->viewport.ViewAngle = vm_evaldouble(stmtobj->expr_angle);

	/* Set our intrinsic lvalues to viewport's initial values. */
	if (stmtobj->lv_from != NULL)
		V3Copy(&stmtobj->lv_from->v, &g_rsd->viewport.LookFrom);
	if (stmtobj->lv_at != NULL)
		V3Copy(&stmtobj->lv_at->v, &g_rsd->viewport.LookAt);
	if (stmtobj->lv_angle != NULL)
		stmtobj->lv_angle->v.x = g_rsd->viewport.ViewAngle;

	/* Loop thru the statements in this block. */
	for (stmt = stmtobj->block; stmt != NULL; stmt = stmt->next)
	{
		stmt->methods->fn(stmt);
	}

	/* Set the viewport parameters to the possibly new values of 
	 * our intrinsic lvalues.
	 */
	if (stmtobj->lv_from != NULL)
		V3Copy(&g_rsd->viewport.LookFrom, &stmtobj->lv_from->v);
	if (stmtobj->lv_at != NULL)
		V3Copy(&g_rsd->viewport.LookAt, &stmtobj->lv_at->v);
	if (stmtobj->lv_angle != NULL)
		g_rsd->viewport.ViewAngle = stmtobj->lv_angle->v.x;

	vmstack_pop();
}

/**
 *	Cleanup function for VM 'viewport' stmt.
 */
void vm_viewport_cleanup(VMStmt *curstmt)
{
	VMStmtViewport *	stmtobj = (VMStmtViewport *) curstmt;

	// Free the parameter initializer expressions.
	//
	delete_exprtree(stmtobj->expr_from);
	stmtobj->expr_from = NULL;
	delete_exprtree(stmtobj->expr_at);
	stmtobj->expr_at = NULL;
	delete_exprtree(stmtobj->expr_angle);
	stmtobj->expr_angle = NULL;
	
	// Free up the lvalue objects we've associated with this context.
	//
	vm_delete_lvalue(stmtobj->lv_from);
	stmtobj->lv_from = NULL;
	vm_delete_lvalue(stmtobj->lv_at);
	stmtobj->lv_at = NULL;
	vm_delete_lvalue(stmtobj->lv_angle);
	stmtobj->lv_angle = NULL;

	// Recursively free the statements in our block.
	//
	vm_delete(stmtobj->block);
	stmtobj->block = NULL;
}



