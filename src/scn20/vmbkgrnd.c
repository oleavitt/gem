/**
 *****************************************************************************
 *  @file vmbkgrnd.c
 *  Virtual machine functions for creating the background.
 *
 *****************************************************************************
 */

#include "local.h"

/*************************************************************************
*
*	background { }
*
*************************************************************************/

/**
 * Container for the background statement.
 */
typedef struct tVMStmtBackground
{
	VMStmt		vmstmt;
	VMStmt		*block;
	VMExpr		*expr_color1;
	VMExpr		*expr_color2;
	VMExpr		*expr_falloff;
	VMLValue	*lv_location;
	VMLValue	*lv_color;
	VMLValue	*lv_falloff;
	int			light_token;
} VMStmtBackground;



/*
 * Methods for the background stmt.
 */
static void vm_background(VMStmt *thisstmt);
static void vm_background_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_background_stmt_methods =
{
	0,
	vm_background,
	vm_background_cleanup
};



/**
 *	Parse the background { } block.
 *
 *	The background keyword has just been parsed.
 *
 *	@return VMStmt* - ptr to the compiled background statement.
 */
VMStmt * parse_vm_background(void)
{
	VMStmtBackground *	newstmt =
		(VMStmtBackground *) vm_alloc_stmt(sizeof(VMStmtBackground), &s_background_stmt_methods);
	ParamList		params[3];
	int				nparams, i;
	char			name[] = "background";

	if (newstmt == NULL)
	{
		logmemerror(name);
		return NULL;
	}

	// Save previous state on the stack.
	//
	pcontext_push(name);
	pcontext_setobjtype(TK_BACKGROUND);

	// Parse the background's parameters and body.
	//
	nparams = parse_paramlist("OEOEOB", name, params);
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr_color1 == NULL) /* color1 */
					newstmt->expr_color1 = params[i].data.expr;
				else /* color2 */
					newstmt->expr_color2 = params[i].data.expr;
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



int parse_vm_background_token(int token, VMStmt **stmtlist)
{
	*stmtlist = NULL;

	switch (token)
	{
		case TK_BACKGROUND_SHADER:
			{
				VMBackgroundShader *sh = parse_vm_background_shader();
				if (sh != NULL)
				{
					// Add this shader to the list of shaders on
					// this surface
					//
					// TODO: When we start using declared shaders, be sure to use Ray_ShareVMShader().
					ray_background_shader_list =
						Ray_AddShader(ray_background_shader_list, (VMShader *) sh);
				}
			}
			break;

		default:
			return 0;
	}

	return 1;
}



/*************************************************************************/

/**
 *	VM background { block }
 *
 * @param curstmt - VMStmt * - Base pointer to the statement for this type.
 */
void vm_background(VMStmt *curstmt)
{
	VMStmtBackground *	stmtbkgrnd = (VMStmtBackground *) curstmt;
	VMStmt *		stmt;
	Vec3			color1, color2;

	// Evaluate any parameters following the background token before the block.
	if (stmtbkgrnd->expr_color1 != NULL)
		vm_evalvector(stmtbkgrnd->expr_color1, &color1);
	else
		V3Set(&color1, 0.0, 0.0, 0.0);
	
	if (stmtbkgrnd->expr_color2 != NULL)
		vm_evalvector(stmtbkgrnd->expr_color2, &color2);
	else
		V3Copy(&color2, &color1);

	// Loop thru the statements in this block.
	//
	for (stmt = stmtbkgrnd->block; stmt != NULL; stmt = stmt->next)
	{
		stmt->methods->fn(stmt);
	}


	// Set up the background colors in the renderer.
	V3Copy(&g_rsd->background_color1, &color1);
	V3Copy(&g_rsd->background_color2, &color2);

	// TODO: Background shader.
}

/**
 *	Cleanup function for VM background stmt.
 *
 * @param curstmt - VMStmt * - Base pointer to the statement for this type.
 */
void vm_background_cleanup(VMStmt *curstmt)
{
	VMStmtBackground *	stmtbkgrnd = (VMStmtBackground *) curstmt;

	// Free the parameter initializer expressions.
	//
	delete_exprtree(stmtbkgrnd->expr_color1);
	stmtbkgrnd->expr_color1 = NULL;
	delete_exprtree(stmtbkgrnd->expr_color2);
	stmtbkgrnd->expr_color2 = NULL;
	
	// Recursively free the statements in our block.
	//
	vm_delete(stmtbkgrnd->block);
	stmtbkgrnd->block = NULL;
}
