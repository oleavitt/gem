/**
 *****************************************************************************
 * @file vmfnxyz.c
 *	Virtual machine functions for creating fn(x, y, z) objects.
 *
 *****************************************************************************
 */

#include "local.h"

/*************************************************************************
*
*	fn_xyz
*
*************************************************************************/

/*
 * Container for a 'fn_xyz' object statement.
 */
typedef struct tVMStmtFnxyz
{
	VMStmtObj	vmstmtobj;
	VMExpr		*expr_fn;
	VMExpr		*expr_bmin;
	VMExpr		*expr_bmax;
	VMExpr		*expr_steps;
} VMStmtFnxyz;



/*
 * Methods for the 'fn_xyz' stmt.
 */
static void vm_fnxyz(VMStmt *thisstmt);
static void vm_fnxyz_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_fnxyz_stmt_methods =
{
	TK_FN_XYZ,
	vm_fnxyz,
	vm_fnxyz_cleanup
};



/**
 *	Parse the fn_xyz object { } block.
 *
 *	The 'fn_xyz' keyword has just been parsed.
 *
 *	@return VMStmt *, ptr to a complete object stmt if successful. NULL otherwise.
 */
VMStmt * parse_vm_fnxyz(void)
{
	VMStmtFnxyz *	newstmt;
	ParamList		params[5];
	int				nparams, i;
	char			name[] = "fn_xyz";

	newstmt = (VMStmtFnxyz *) begin_parse_object(
		sizeof(VMStmtFnxyz),
		name,
		TK_FN_XYZ,
		&s_fnxyz_stmt_methods);

	// Make sure alloc succeeded.
	//
	if (newstmt == NULL)
		return NULL;

	// Parse the objects's parameters and body.
	// The function, low bound and high bound must be given.
	// The fourth parameter, a vector specifying the x, y, and z steps
	// is optional. This defaults to <32, 32, 32>.
	//
	nparams = parse_paramlist("EEEOEOB", name, params);

	// Evaluate the parameters.
	//
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr_fn == NULL) // The fn(x, y, z) function.
					newstmt->expr_fn = params[i].data.expr;
				else if (newstmt->expr_bmin == NULL) // The bound box min parameter.
					newstmt->expr_bmin = params[i].data.expr;
				else if (newstmt->expr_bmax == NULL) // The bound box max parameter.
					newstmt->expr_bmax = params[i].data.expr;
				else if (newstmt->expr_steps == NULL) // The steps parameter.
					newstmt->expr_steps = params[i].data.expr;
				break;
			case PARAM_BLOCK:
				newstmt->vmstmtobj.block = params[i].data.block;
				break;
		}
	}

	return (VMStmt *) finish_parse_object( (VMStmtObj *) newstmt);
}



/*************************************************************************/

/**
 *	VM fn_xyz <threshold> { block }
 */
void vm_fnxyz(VMStmt *curstmt)
{
	VMStmtFnxyz *	stmtobj = (VMStmtFnxyz *) curstmt;
	Vec3			bmin, bmax, steps;
	int				success = 0;
	Object *		newobj;
	
	vm_begin_object((VMStmtObj *) curstmt);
	
	// Evaulate the parameters.
	V3Set(&bmin, -1.0, -1.0, -1.0);
	V3Set(&bmax, 1.0, 1.0, 1.0);
	V3Set(&steps, 32.0, 32.0, 32.0);
	
	if (stmtobj->expr_bmin != NULL)
		vm_evalvector(stmtobj->expr_bmin, &bmin);
	if (stmtobj->expr_bmax != NULL)
		vm_evalvector(stmtobj->expr_bmax, &bmax);
	if (stmtobj->expr_steps != NULL)
		vm_evalvector(stmtobj->expr_steps, &steps);

	// Create this object and store it in the VM stack.
	//
	newobj = Ray_MakeFnxyz(stmtobj->expr_fn, &bmin, &bmax, &steps);
	if (newobj != NULL)
	{
		// The renderer now has the fn(x, y, z) expr, set the stmt's ptr to
		// NULL so that vm_fnxyz_cleanup() will not delete it.
		// The renderer will delete it when finished.
		//
		stmtobj->expr_fn = NULL;
		success = 1;

		vmstack_setcurobj(newobj);

		// Run the statements in the object's block.
		//
		vm_execute_object_block((VMStmtObj *) stmtobj);
	}
	else
		logmemerror("fn_xyz");

	// Post process the object.
	//
	vm_finish_object((VMStmtObj *) curstmt, newobj, success);
}

/**
 *	Cleanup function for VM 'fn_xyz' stmt.
 */
void vm_fnxyz_cleanup(VMStmt *curstmt)
{
	VMStmtFnxyz *	stmtobj = (VMStmtFnxyz *) curstmt;

	// Free the parameter expressions.
	//
	delete_exprtree(stmtobj->expr_fn);
	stmtobj->expr_fn = NULL;
	delete_exprtree(stmtobj->expr_bmin);
	stmtobj->expr_bmin = NULL;
	delete_exprtree(stmtobj->expr_bmax);
	stmtobj->expr_bmax = NULL;
	delete_exprtree(stmtobj->expr_steps);
	stmtobj->expr_steps = NULL;

	// Cleanup the base object statement.
	//
	vm_object_cleanup(curstmt);
}



