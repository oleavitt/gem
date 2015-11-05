/**
 *****************************************************************************
 *  @file vmvsblty.c
 *  Virtual machine functions for creating the visibility.
 *
 *****************************************************************************
 */

#include "local.h"

/*************************************************************************
*
*	visibility { }
*
*************************************************************************/

/**
 * Container for the visibility statement.
 */
typedef struct tVMStmtVisibility
{
	VMStmt		vmstmt;
	VMStmt		*block;
	VMExpr		*expr_color;
	VMExpr		*expr_distance;
	VMLValue	*lv_color;
	VMLValue	*lv_distance;
} VMStmtVisibility;



/*
 * Methods for the visibility stmt.
 */
static void vm_visibility(VMStmt *thisstmt);
static void vm_visibility_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_visibility_stmt_methods =
{
	0,
	vm_visibility,
	vm_visibility_cleanup
};



/**
 *	Parse the visibility { } block.
 *
 *	The visibility keyword has just been parsed.
 *
 *	@return VMStmt* - ptr to the compiled visibility statement.
 */
VMStmt * parse_vm_visibility(void)
{
	VMStmtVisibility *	newstmt =
		(VMStmtVisibility *) vm_alloc_stmt(sizeof(VMStmtVisibility), &s_visibility_stmt_methods);
	ParamList		params[3];
	int				nparams, i;
	char			name[] = "visibility";

	if (newstmt == NULL)
	{
		logmemerror(name);
		return NULL;
	}

	// Save previous state on the stack.
	//
	pcontext_push(name);
	pcontext_setobjtype(TK_VISIBILITY);

	// Parse the visibility's parameters and body.
	//
	nparams = parse_paramlist("OEOEOB", name, params);
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr_distance == NULL)
					newstmt->expr_distance = params[i].data.expr;
				else
					newstmt->expr_color = params[i].data.expr;
				break;
			case PARAM_BLOCK:
				newstmt->block = params[i].data.block;
				break;
		}
	}

	// Restore previous state.
	//
	pcontext_pop();

	return (VMStmt *) newstmt;
}



int parse_vm_visibility_token(int token, VMStmt **stmtlist)
{
	*stmtlist = NULL;

	switch (token)
	{
		//case TK_VISIBILITY_SHADER:
		//	{
		//		VMVisibilityShader *sh = parse_vm_visibility_shader();
		//		if (sh != NULL)
		//		{
		//			// Add this shader to the list of shaders on
		//			// this surface
		//			//
		//			// TODO: When we start using declared shaders, be sure to use Ray_ShareVMShader().
		//			ray_visibility_shader_list =
		//				Ray_AddShader(ray_visibility_shader_list, (VMShader *) sh);
		//		}
		//	}
		//	break;

		default:
			return 0;
	}

	return 1;
}



/*************************************************************************/

/**
 *	VM visibility { block }
 *
 * @param curstmt - VMStmt * - Base pointer to the statement for this type.
 */
void vm_visibility(VMStmt *curstmt)
{
	VMStmtVisibility *	stmtbkgrnd = (VMStmtVisibility *) curstmt;
	VMStmt *		stmt;
	Vec3			dist_color;
   double      distance;

	// Evaluate any parameters following the visibility token before the block.
	if (stmtbkgrnd->expr_color != NULL)
		vm_evalvector(stmtbkgrnd->expr_color, &dist_color);
	else
		V3Set(&dist_color, 1.0, 1.0, 1.0);
	
	if (stmtbkgrnd->expr_distance != NULL)
      distance = vm_evaldouble(stmtbkgrnd->expr_distance);
	else
		distance = 1000;

	// Loop thru the statements in this block.
	//
	for (stmt = stmtbkgrnd->block; stmt != NULL; stmt = stmt->next)
	{
		stmt->methods->fn(stmt);
	}


	// Set up the visibility colors in the renderer.
	V3Copy(&g_rsd->visibility_color, &dist_color);
	g_rsd->visibility_distance = distance;

	// TODO: Visibility shader.
}

/**
 *	Cleanup function for VM visibility stmt.
 *
 * @param curstmt - VMStmt * - Base pointer to the statement for this type.
 */
void vm_visibility_cleanup(VMStmt *curstmt)
{
	VMStmtVisibility *	stmtbkgrnd = (VMStmtVisibility *) curstmt;

	// Free the parameter initializer expressions.
	//
	delete_exprtree(stmtbkgrnd->expr_color);
	stmtbkgrnd->expr_color = NULL;
	delete_exprtree(stmtbkgrnd->expr_distance);
	stmtbkgrnd->expr_distance = NULL;
	
	// Recursively free the statements in our block.
	//
	vm_delete(stmtbkgrnd->block);
	stmtbkgrnd->block = NULL;
}
