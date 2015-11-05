/**
 *****************************************************************************
 *  @file vmlight.c
 *  Virtual machine functions for creating light source objects.
 *
 *****************************************************************************
 */

#include "local.h"

/*************************************************************************
*
*	light { }
*
*************************************************************************/

/**
 * Container for the light statement.
 */
typedef struct tVMStmtLight
{
	VMStmt		vmstmt;
	VMStmt		*block;
	VMExpr		*expr_location;
	VMExpr		*expr_color;
	VMExpr		*expr_falloff;
	VMLValue	*lv_location;
	VMLValue	*lv_color;
	VMLValue	*lv_falloff;
	int			light_token;
} VMStmtLight;



/*
 * Methods for the light stmt.
 */
static void vm_light(VMStmt *thisstmt);
static void vm_light_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_light_stmt_methods =
{
	0,
	vm_light,
	vm_light_cleanup
};



/**
 *	Parse the light { } block.
 *
 *	A light source object keyword has just been parsed.
 *
 *  @param light_type_token - int - The type of light we are parsing.
 *
 *	@return VMStmt* - ptr to a light source object statement.
 */
VMStmt * parse_vm_light(int light_type_token)
{
	VMStmtLight *	newstmt =
		(VMStmtLight *) vm_alloc_stmt(sizeof(VMStmtLight), &s_light_stmt_methods);
	ParamList		params[5];
	int				nparams, i;
	char *			name;

	switch (light_type_token)
	{
		case TK_LIGHT: // Basic point light source
			name = "light";
			break;
		default:
			assert(0);
			return NULL;
	}

	if (newstmt == NULL)
	{
		logmemerror(name);
		return NULL;
	}

	/* Save previous state on the stack. */
	pcontext_push(name);
	pcontext_setobjtype(light_type_token);

	/* Save the light source type token. */
	newstmt->light_token = light_type_token;

	/*
	 * Add the appropriate light parameter variables to the local namespace.
	 * These will be bound at runtime to the actual fields in the object.
	 */
	newstmt->lv_color = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_color != NULL)
		pcontext_addsymbol("color", DECL_VECTOR, 0,
			(void *) vm_copy_lvalue(newstmt->lv_color));
	// if (light_type_token != TK_INFINITE_LIGHT)
	// {
		newstmt->lv_location = vm_new_lvalue(TK_VECTOR);
			if (newstmt->lv_location != NULL)
				pcontext_addsymbol("location", DECL_VECTOR, 0,
					(void *) vm_copy_lvalue(newstmt->lv_location));
		newstmt->lv_falloff = vm_new_lvalue(TK_FLOAT);
			if (newstmt->lv_falloff != NULL)
				pcontext_addsymbol("falloff", DECL_FLOAT, 0,
					(void *) vm_copy_lvalue(newstmt->lv_falloff));
	// }

	/* Parse the light's parameters and body. */
	if (light_type_token == TK_LIGHT)
	{
		nparams = parse_paramlist("OEOEOEOB", name, params);
		for (i = 0; i < nparams; i++)
		{
			switch (params[i].type)
			{
				case PARAM_EXPR:
					if (newstmt->expr_location == NULL) /* location */
						newstmt->expr_location = params[i].data.expr;
					else if (newstmt->expr_color == NULL) /* color */
						newstmt->expr_color = params[i].data.expr;
					else /* falloff */
						newstmt->expr_falloff = params[i].data.expr;
					break;
				case PARAM_BLOCK:
					newstmt->block = params[i].data.block;
					break;
			}
		}
	}

	/* Restore previous state. */
	pcontext_pop();

	return (VMStmt *) newstmt;
}




/*************************************************************************/

/**
 *	VM light { block }
 *
 * @param curstmt - VMStmt * - Base pointer to the statement for this type.
 */
void vm_light(VMStmt *curstmt)
{
	VMStmtLight *	stmtlite = (VMStmtLight *) curstmt;
	VMStmt *		stmt;
	Light *			newlite;
	Vec3			location, color;
	double			falloff;

	/* Set default values for the light. */
	V3Set(&location, 0.0, 0.0, 0.0);
	V3Set(&color, 1.0, 1.0, 1.0);
	falloff = 0.0;

	// Evaluate any parameters following the light token before the block.
	if (stmtlite->expr_location != NULL)
		vm_evalvector(stmtlite->expr_location, &location);
	if (stmtlite->expr_color != NULL)
		vm_evalvector(stmtlite->expr_color, &color);
	if (stmtlite->expr_falloff != NULL)
		falloff = vm_evaldouble(stmtlite->expr_falloff);

	/* Loop thru the statements in this block. */
	for (stmt = stmtlite->block; stmt != NULL; stmt = stmt->next)
	{
		stmt->methods->fn(stmt);
	}

	/* Get the possibly new values of our parameters.
	 */
	if (stmtlite->lv_location != NULL)
		V3Copy(&location, &stmtlite->lv_location->v);
	if (stmtlite->lv_color != NULL)
		V3Copy(&color, &stmtlite->lv_color->v);
	if (stmtlite->lv_falloff != NULL)
		falloff = stmtlite->lv_falloff->v.x;

	/* Create the light source and add it the renderer database. */
	newlite = Ray_MakePointLight(&location, &color, falloff);
	if ((newlite != NULL) && (g_rsd != NULL))
		Ray_AddLight(&g_rsd->lights, newlite);
	else
		logmemerror("light");
}

/**
 *	Cleanup function for VM light stmt.
 *
 * @param curstmt - VMStmt * - Base pointer to the statement for this type.
 */
void vm_light_cleanup(VMStmt *curstmt)
{
	VMStmtLight *	stmtlite = (VMStmtLight *) curstmt;

	// Free the parameter initializer expressions.
	//
	delete_exprtree(stmtlite->expr_location);
	stmtlite->expr_location = NULL;
	delete_exprtree(stmtlite->expr_color);
	stmtlite->expr_color = NULL;
	delete_exprtree(stmtlite->expr_falloff);
	stmtlite->expr_falloff = NULL;

	// Free the parameter l-values.
	//
	vm_delete_lvalue(stmtlite->lv_location);
	stmtlite->lv_location = NULL;
	vm_delete_lvalue(stmtlite->lv_color);
	stmtlite->lv_color = NULL;
	vm_delete_lvalue(stmtlite->lv_falloff);
	stmtlite->lv_falloff = NULL;
	
	// Recursively free the statements in our block.
	//
	vm_delete(stmtlite->block);
	stmtlite->block = NULL;
}
