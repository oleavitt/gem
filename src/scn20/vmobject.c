/**
 *****************************************************************************
 *  @file vmobject.c
 *  Virtual machine functions for creating objects.
 *
 *  In here are the basic functions for parsing object primitives from
 *  the scene and the VM functions for creating them. The simple
 *  object types are all contained in this file - sphere, disc,
 *  box, torus, and cone.
 *
 *****************************************************************************
 */

#include "local.h"

/**
 * Creates a new object statement and begins the parse process
 * for this object.
 *
 * @param size - size_t - Size of the object to allocate.
 * @param name - const char * - Scene description language name of object.
 * @param token - int - Token value of object.
 * @param methods - VMStmtMethods * - The vm methods for this object.
 *
 * @return VMStmtObj * - Pointer to the new object statement.
 */
VMStmtObj *begin_parse_object(
	size_t			size,
	const char		*name,
	int				token,
	VMStmtMethods	*methods)
{
	VMStmtObj *stmtobj;

	stmtobj = (VMStmtObj *) vm_alloc_stmt(size, methods);
	if (stmtobj == NULL)
	{
		// Alloc failed. Log error and return NULL.
		logmemerror(name);
		return NULL;
	}

	// Save previous state on the stack and set up for this object.
	//
	pcontext_push(name);
	pcontext_setobjtype(token);

	// Add standard object parameters to the local namespace as local variables.
	// These will be bound at runtime to the actual fields in the object.
	//
	// The 'no_shadow' flag
	//
	stmtobj->lv_no_shadow = vm_new_lvalue(TK_FLOAT);
	if (stmtobj->lv_no_shadow != NULL)
		pcontext_addsymbol("no_shadow", DECL_FLOAT, 0,
			(void *) vm_copy_lvalue(stmtobj->lv_no_shadow));

	return stmtobj;
}

/**
 * Finishes the process of parsing an object.
 *
 * @param stmtobj - VMStmtObj * - Pointer to the object statement to finish up.
 *
 * @return VMStmtObj * - Pointer to the finished object statement.
 */
VMStmtObj *finish_parse_object(
	VMStmtObj *stmtobj)
{
	// Restore previous state and return a pointer to the
	// new object statement.
	//
	pcontext_pop();

	return stmtobj;
}



/**
 * Called first whenever an object statement is executed.
 *
 *  'Pushes' the am stack state and performs common initialization for
 *  object statements.
 *
 * @param   stmt - VMStmtObj - The object statement.
 */
void vm_begin_object(VMStmtObj *stmt)
{
	vmstack_push((VMStmt *) stmt);
}

/**
 * Executes the statements in the object statement's block.
 *
 * @param   stmtobj - VMStmtObj - The object statement.
 */
void vm_execute_object_block(VMStmtObj *stmtobj)
{
	VMStmt * stmt;

	// Loop thru the statements in this block.
	//
	for (stmt = stmtobj->block; stmt != NULL; stmt = stmt->next)
		stmt->methods->fn(stmt);
}

/**
 * Called whenever an object vm statement finishes execution.
 *
 *  Performs common post-processing for all Objects, such as adding them
 *  to the renderer database, parent CSG, or symbol table.
 *  The vm stack is 'poped' to its previous state.
 *
 * @param   stmt - VMStmtObj - The object statement
 * @param   obj - Object - The renderer object.
 * @param   success - int - 1 indicates the object is valid, 0 idicates error - just cleanup.
 *
 * @return  int - 1 if succesful, or 0 if not.
 */
int vm_finish_object(VMStmtObj *stmt, Object *obj, int success)
{
	VMStmtObj		*stmtobj = (VMStmtObj *) stmt;
	Object			*parentobj;

	if ( ! success)
	{
		// Caller is indicating that something failed.
		// Just cleanup and delete the object.
		vmstack_pop();
		Ray_DeleteObject(obj);
		return 0;
	}

	// Determine if the 'no_shadow' flag was set.
	//
	if (stmtobj->lv_no_shadow != NULL)
	{
		if (stmtobj->lv_no_shadow->v.x != 0.0)
			obj->flags |= OBJ_FLAG_NO_SHADOW;
	}

	// Restore the previous vm state.
	vmstack_pop();

	// Get the parent object if we are in a parent object.
	parentobj = vmstack_getcurobj();

	if (Ray_ObjectIsCSG(parentobj))
	{
		/* This object is part of a CSG object.
		 * Add this object to the parent CSG object's child list.
		 */
		if (Ray_AddCSGChild(parentobj, obj))
			return 1;
	}
	else if (g_define_mode)
	{
		/* We are within a "define" block.
		 * Store this object in the symbol table for later use.
		 */
		if (pcontext_addsymbol(g_define_name, DECL_OBJECT, 0, obj))
			return 1;
	}
	else if (g_rsd != NULL)
	{
		/* This object is going directly into the renderer database.
		 */
		Ray_AddObject(&g_rsd->objects, obj);
		return 1;
	}

	/* Something failed. */
	Ray_DeleteObject(obj);
	return 0;
}

/**
 * Frees up all resources used by the base object statement,
 * including the body statements.
 *
 * @param   stmt - VMStmt - Pointer to the object statement container.
 */
void vm_object_cleanup(VMStmt *stmt)
{
	VMStmtObj * stmtobj = (VMStmtObj *) stmt;

	// Free the parameter l-values.
	//
	vm_delete_lvalue(stmtobj->lv_no_shadow);
	stmtobj->lv_no_shadow = NULL;

	// Recursively free the statements in our block.
	//
	vm_delete(stmtobj->block);
	stmtobj->block = NULL;
}

/*************************************************************************
*
*	Sphere
*
*************************************************************************/

/**
 * Container for a 'sphere' object statement.
 */
typedef struct tVMStmtSphere
{
	VMStmtObj	vmstmtobj;
	VMExpr		*expr_center;
	VMExpr		*expr_radius;
	VMLValue	*lv_center;
	VMLValue	*lv_radius;
} VMStmtSphere;



/*
 * Methods for the 'sphere' stmt.
 */
static void vm_sphere(VMStmt *thisstmt);
static void vm_sphere_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_sphere_stmt_methods =
{
	TK_SPHERE,
	vm_sphere,
	vm_sphere_cleanup
};



/**
 *	Parse the sphere object { } block.
 *
 *	The 'sphere' keyword has just been parsed.
 *
 *	@return VMStmt *, ptr to a sphere stmt if we are compiling
 *		for the virtual machine. Returns NULL otherwise.
 */
VMStmt * parse_vm_sphere(void)
{
	// Allocate a new VMStmt struct for this object.
	//
	VMStmtSphere *	newstmt;
	ParamList		params[3];
	int				nparams, i;

	newstmt = (VMStmtSphere *) begin_parse_object(
		sizeof(VMStmtSphere),
		"sphere",
		TK_SPHERE,
		&s_sphere_stmt_methods);

	// Make sure alloc succeeded.
	//
	if (newstmt == NULL)
		return NULL;

	// Add 'center' and 'radius' to the local namespace as local variables.
	// These will be bound at runtime to the actual fields in the object.
	//
	newstmt->lv_center = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_center != NULL)
		pcontext_addsymbol("center", DECL_VECTOR, 0,
			(void *) vm_copy_lvalue(newstmt->lv_center));
	newstmt->lv_radius = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_radius != NULL)
		pcontext_addsymbol("radius", DECL_FLOAT, 0,
			(void *) vm_copy_lvalue(newstmt->lv_radius));

	// Parse the sphere's parameters and body.
	//
	nparams = parse_paramlist("OEOEOB", "sphere", params);
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr_center == NULL)	// First one is the location vector.
					newstmt->expr_center = params[i].data.expr;
				else	// Second one is the radius.
					newstmt->expr_radius = params[i].data.expr;
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
 *	VM sphere { block }
 *
 * @param   curstmt - VMStmt - Pointer to the object statement container.
 */
void vm_sphere(VMStmt *curstmt)
{
	VMStmtSphere *	stmtobj = (VMStmtSphere *) curstmt;
	Vec3			v;
	double			f;
	Object *		newobj;
	
	// Standard object initialization.
	//
	vm_begin_object((VMStmtObj *) curstmt);
	
	// Set the default unit sphere center and radius values.
	//
	V3Set(&v, 0, 0, 0);
	f = 1.0;

	// Evaluate the parameter initializer expressions, if any.
	//
	if (stmtobj->expr_center != NULL)
		vm_evalvector(stmtobj->expr_center, &v);
	if (stmtobj->expr_radius != NULL)
		f = vm_evaldouble(stmtobj->expr_radius);
	
	// Create this object and store it in the VM stack.
	//
	newobj = Ray_MakeSphere(&v, f);
	if (newobj == NULL)
	{
		logmemerror("sphere");
		return;
	}

	vmstack_setcurobj(newobj);

	// Set the parameter lvalues to object's initial values.
	//
	if (stmtobj->lv_center != NULL)
		V3Copy(&stmtobj->lv_center->v, &v);
	if (stmtobj->lv_radius != NULL)
		stmtobj->lv_radius->v.x = f;

	// Run the statements in the object's block.
	//
	vm_execute_object_block((VMStmtObj *) stmtobj);

	// Set the object parameters to the possibly modified values of 
	// our parameter lvalues.
	//
	if (stmtobj->lv_center != NULL)
		V3Copy(&v, &stmtobj->lv_center->v);
	if (stmtobj->lv_radius != NULL)
		f = stmtobj->lv_radius->v.x;

	// Update the new object with the possibly modified parameters.
	//
	Ray_SetSphere(newobj->data.sphere, &v, f);

	// Finish up and insert the object into the renderer database.
	//
	vm_finish_object((VMStmtObj *) curstmt, newobj, 1);
}

/**
 *	Cleanup function for VM 'sphere' stmt.
 *
 * @param   curstmt - VMStmt - Pointer to the object statement container.
 */
void vm_sphere_cleanup(VMStmt *curstmt)
{
	VMStmtSphere *	stmtobj = (VMStmtSphere *) curstmt;

	// Free the parameter initializer expressions.
	//
	delete_exprtree(stmtobj->expr_center);
	stmtobj->expr_center = NULL;
	delete_exprtree(stmtobj->expr_radius);
	stmtobj->expr_radius = NULL;

	// Free the parameter l-values.
	//
	vm_delete_lvalue(stmtobj->lv_center);
	stmtobj->lv_center = NULL;
	vm_delete_lvalue(stmtobj->lv_radius);
	stmtobj->lv_radius = NULL;
	
	// Cleanup the base object statement.
	//
	vm_object_cleanup(curstmt);
}



/*************************************************************************
*
*	Disc
*
*************************************************************************/

/**
 * Container for a 'disc' object statement.
 */
typedef struct tVMStmtDisc
{
	VMStmtObj	vmstmtobj;
	VMExpr		*expr_center;
	VMExpr		*expr_normal;
	VMExpr		*expr_outer_radius;
	VMExpr		*expr_inner_radius;
	VMLValue	*lv_center;
	VMLValue	*lv_normal;
	VMLValue	*lv_outer_radius;
	VMLValue	*lv_inner_radius;
} VMStmtDisc;



/*
 * Methods for the 'disc' stmt.
 */
static void vm_disc(VMStmt *thisstmt);
static void vm_disc_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_disc_stmt_methods =
{
	TK_DISC,
	vm_disc,
	vm_disc_cleanup
};



/**
 *	Parse the disc object { } block.
 *
 *	The 'sphere' keyword has just been parsed.
 *
 *	@return VMStmt *, ptr to a sphere stmt if we are compiling
 *		for the virtual machine. Returns NULL otherwise.
 */
VMStmt * parse_vm_disc(void)
{
	VMStmtDisc *	newstmt;
	ParamList		params[5];
	int				nparams, i;

	newstmt = (VMStmtDisc *) begin_parse_object(
		sizeof(VMStmtDisc),
		"disc",
		TK_DISC,
		&s_disc_stmt_methods);

	// Make sure alloc succeeded.
	//
	if (newstmt == NULL)
		return NULL;

	/*
	 * Add 'center', 'dir' and 'radius' to the local namespace.
	 * These will be bound at runtime to the actual fields in the object.
	 */
	newstmt->lv_center = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_center != NULL)
		pcontext_addsymbol("center", DECL_VECTOR, 0,
			(void *) vm_copy_lvalue(newstmt->lv_center));
	newstmt->lv_normal = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_normal != NULL)
		pcontext_addsymbol("normal", DECL_VECTOR, 0,
			(void *) vm_copy_lvalue(newstmt->lv_normal));
	newstmt->lv_outer_radius = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_outer_radius != NULL)
		pcontext_addsymbol("radius", DECL_FLOAT, 0,
			(void *) vm_copy_lvalue(newstmt->lv_outer_radius));
	newstmt->lv_inner_radius = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_inner_radius != NULL)
		pcontext_addsymbol("inner_radius", DECL_FLOAT, 0,
			(void *) vm_copy_lvalue(newstmt->lv_inner_radius));

	/* Parse the disc's parameters and body. */
	nparams = parse_paramlist("OEOEOEOEOB", "disc", params);
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr_center == NULL) /* First one is the center point. */
					newstmt->expr_center = params[i].data.expr;
				else if (newstmt->expr_normal == NULL) /* Second one is the normal. */
					newstmt->expr_normal = params[i].data.expr;
				else if (newstmt->expr_outer_radius == NULL) /* Third one is the outer radius. */
					newstmt->expr_outer_radius = params[i].data.expr;
				else                 /* Fourth one is the inner radius. */
					newstmt->expr_inner_radius = params[i].data.expr;
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
 *	VM disc { block }
 *
 * @param   curstmt - VMStmt - Pointer to the object statement container.
 */
void vm_disc(VMStmt *curstmt)
{
	VMStmtDisc *	stmtobj = (VMStmtDisc *) curstmt;
	Vec3			center, normal;
	double			outer_radius, inner_radius;
	Object *		newobj;
	
	vm_begin_object((VMStmtObj *) curstmt);
	
	// Create this object and store it in the VM stack.
	//
	V3Set(&center, 0, 0, 0);
	V3Set(&normal, 0, 0, 1);
	outer_radius = 1.0;
	inner_radius = 0.0;
	if (stmtobj->expr_center != NULL)
		vm_evalvector(stmtobj->expr_center, &center);
	if (stmtobj->expr_normal != NULL)
		vm_evalvector(stmtobj->expr_normal, &normal);
	if (stmtobj->expr_outer_radius != NULL)
		outer_radius = vm_evaldouble(stmtobj->expr_outer_radius);
	if (stmtobj->expr_inner_radius != NULL)
		inner_radius = vm_evaldouble(stmtobj->expr_inner_radius);
	newobj = Ray_MakeDisc(&center, &normal, outer_radius, inner_radius);
	if (newobj == NULL)
	{
		logmemerror("disc");
		return;
	}

	vmstack_setcurobj(newobj);

	// Set our intrinsic lvalues to object's initial values.
	//
	if (stmtobj->lv_center != NULL)
		V3Copy(&stmtobj->lv_center->v, &center);
	if (stmtobj->lv_normal != NULL)
		V3Copy(&stmtobj->lv_normal->v, &normal);
	if (stmtobj->lv_outer_radius != NULL)
		stmtobj->lv_outer_radius->v.x = outer_radius;
	if (stmtobj->lv_inner_radius != NULL)
		stmtobj->lv_inner_radius->v.x = inner_radius;

	// Run the statements in the object's block.
	//
	vm_execute_object_block((VMStmtObj *) stmtobj);

	// Set the object parameters to the possibly new values of 
	// our intrinsic lvalues.
	//
	if (stmtobj->lv_center != NULL)
		V3Copy(&center, &stmtobj->lv_center->v);
	if (stmtobj->lv_normal != NULL)
		V3Copy(&normal, &stmtobj->lv_normal->v);
	if (stmtobj->lv_outer_radius != NULL)
		outer_radius = stmtobj->lv_outer_radius->v.x;
	if (stmtobj->lv_inner_radius != NULL)
		inner_radius = stmtobj->lv_inner_radius->v.x;
	Ray_SetDisc(newobj->data.disc, &center, &normal, outer_radius, inner_radius);

	vm_finish_object((VMStmtObj *) curstmt, newobj, 1);
}

/**
 *	Cleanup function for VM 'disc' stmt.
 *
 * @param   curstmt - VMStmt - Pointer to the object statement container.
 */
void vm_disc_cleanup(VMStmt *curstmt)
{
	VMStmtDisc *	stmtobj = (VMStmtDisc *) curstmt;

	// Free the parameter initializer expressions.
	//
	delete_exprtree(stmtobj->expr_center);
	stmtobj->expr_center = NULL;
	delete_exprtree(stmtobj->expr_normal);
	stmtobj->expr_normal = NULL;
	delete_exprtree(stmtobj->expr_outer_radius);
	stmtobj->expr_outer_radius = NULL;
	delete_exprtree(stmtobj->expr_inner_radius);
	stmtobj->expr_inner_radius = NULL;

	// Free the parameter l-values.
	//
	vm_delete_lvalue(stmtobj->lv_center);
	stmtobj->lv_center = NULL;
	vm_delete_lvalue(stmtobj->lv_normal);
	stmtobj->lv_normal = NULL;
	vm_delete_lvalue(stmtobj->lv_outer_radius);
	stmtobj->lv_outer_radius = NULL;
	vm_delete_lvalue(stmtobj->lv_inner_radius);
	stmtobj->lv_inner_radius = NULL;
	
	// Cleanup the base object statement.
	//
	vm_object_cleanup(curstmt);
}



/*************************************************************************
*
*	Box
*
*************************************************************************/

/**
 * Container for a 'box' object statement.
 */
typedef struct tVMStmtBox
{
	VMStmtObj	vmstmtobj;
	VMExpr		*expr_bmin;
	VMExpr		*expr_bmax;
	VMLValue	*lv_bmin;
	VMLValue	*lv_bmax;
} VMStmtBox;



/*
 * Methods for the 'box' stmt.
 */
static void vm_box(VMStmt *thisstmt);
static void vm_box_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_box_stmt_methods =
{
	TK_BOX,
	vm_box,
	vm_box_cleanup
};



/**
 *	Parse the box object { } block.
 *
 *	The 'box' keyword has just been parsed.
 *
 *	@return VMStmt *, ptr to a box stmt if we are compiling
 *		for the virtual machine. Returns NULL otherwise.
 */
VMStmt * parse_vm_box(void)
{
	VMStmtBox *	newstmt;
	ParamList		params[3];
	int				nparams, i;

	newstmt = (VMStmtBox *) begin_parse_object(
		sizeof(VMStmtBox),
		"box",
		TK_BOX,
		&s_box_stmt_methods);

	// Make sure alloc succeeded.
	//
	if (newstmt == NULL)
		return NULL;

	/*
	 * Add 'bmin' and 'bmax' to the local namespace.
	 * These will be bound at runtime to the actual fields in the object.
	 */
	newstmt->lv_bmin = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_bmin != NULL)
		pcontext_addsymbol("bmin", DECL_VECTOR, 0,
			(void *) vm_copy_lvalue(newstmt->lv_bmin));
	newstmt->lv_bmax = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_bmax != NULL)
		pcontext_addsymbol("bmax", DECL_VECTOR, 0,
			(void *) vm_copy_lvalue(newstmt->lv_bmax));

	/* Parse the box's parameters and body. */
	nparams = parse_paramlist("OEOEOB", "box", params);
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr_bmin == NULL) /* First one is the bmin point. */
					newstmt->expr_bmin = params[i].data.expr;
				else                 /* Second one is the bmax point. */
					newstmt->expr_bmax = params[i].data.expr;
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
 *	VM box { block }
 *
 * @param   curstmt - VMStmt - Pointer to the object statement container.
 */
void vm_box(VMStmt *curstmt)
{
	VMStmtBox *	stmtobj = (VMStmtBox *) curstmt;
	Vec3			bmin, bmax;
	Object *		newobj;
	
	vm_begin_object((VMStmtObj *) curstmt);
	
	/* Create this object and store it in the VM stack. */
	V3Set(&bmin, -1, -1, -1);
	V3Set(&bmax, 1, 1, 1);
	if (stmtobj->expr_bmin != NULL)
		vm_evalvector(stmtobj->expr_bmin, &bmin);
	if (stmtobj->expr_bmax != NULL)
		vm_evalvector(stmtobj->expr_bmax, &bmax);
	newobj = Ray_MakeBox(&bmin, &bmax);
	if (newobj == NULL)
	{
		logmemerror("box");
		return;
	}

	vmstack_setcurobj(newobj);

	/* Set our intrinsic lvalues to object's initial values. */
	if (stmtobj->lv_bmin != NULL)
		V3Copy(&stmtobj->lv_bmin->v, &bmin);
	if (stmtobj->lv_bmax != NULL)
		V3Copy(&stmtobj->lv_bmax->v, &bmax);

	// Run the statements in the object's block.
	//
	vm_execute_object_block((VMStmtObj *) stmtobj);

	/* Set the object parameters to the possibly new values of 
	 * our intrinsic lvalues.
	 */
	if (stmtobj->lv_bmin != NULL)
		V3Copy(&bmin, &stmtobj->lv_bmin->v);
	if (stmtobj->lv_bmax != NULL)
		V3Copy(&bmax, &stmtobj->lv_bmax->v);
	Ray_SetBox(newobj->data.box, &bmin, &bmax);

	vm_finish_object((VMStmtObj *) curstmt, newobj, 1);
}

/**
 *	Cleanup function for VM 'box' stmt.
 *
 * @param   curstmt - VMStmt - Pointer to the object statement container.
 */
void vm_box_cleanup(VMStmt *curstmt)
{
	VMStmtBox *	stmtobj = (VMStmtBox *) curstmt;

	// Free the parameter initializer expressions.
	//
	delete_exprtree(stmtobj->expr_bmin);
	stmtobj->expr_bmin = NULL;
	delete_exprtree(stmtobj->expr_bmax);
	stmtobj->expr_bmax = NULL;

	// Free the parameter l-values.
	//
	vm_delete_lvalue(stmtobj->lv_bmin);
	stmtobj->lv_bmin = NULL;
	vm_delete_lvalue(stmtobj->lv_bmax);
	stmtobj->lv_bmax = NULL;
	
	// Cleanup the base object statement.
	//
	vm_object_cleanup(curstmt);
}



/*************************************************************************
*
*	Torus
*
*************************************************************************/

/**
 * Container for a 'torus' object statement.
 */
typedef struct tVMStmtTorus
{
	VMStmtObj	vmstmtobj;
	VMExpr		*expr_center;
	VMExpr		*expr_rmajor;
	VMExpr		*expr_rminor;
	VMLValue	*lv_center;
	VMLValue	*lv_rmajor;
	VMLValue	*lv_rminor;
} VMStmtTorus;



/*
 * Methods for the 'torus' stmt.
 */
static void vm_torus(VMStmt *thisstmt);
static void vm_torus_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_torus_stmt_methods =
{
	TK_TORUS,
	vm_torus,
	vm_torus_cleanup
};



/**
 *	Parse the torus object { } block.
 *
 *	The 'torus' keyword has just been parsed.
 *
 *	@return VMStmt *, ptr to a torus stmt if we are compiling
 *		for the virtual machine. Returns NULL otherwise.
 */
VMStmt * parse_vm_torus(void)
{
	VMStmtTorus *	newstmt;
	ParamList		params[4];
	int				nparams, i;

	newstmt = (VMStmtTorus *) begin_parse_object(
		sizeof(VMStmtTorus),
		"torus",
		TK_TORUS,
		&s_torus_stmt_methods);

	// Make sure alloc succeeded.
	//
	if (newstmt == NULL)
		return NULL;

	// Add 'center', 'rmajor and 'rminor' to the local namespace.
	// These will be bound at runtime to the actual fields in the object.
	//
	newstmt->lv_center = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_center != NULL)
		pcontext_addsymbol("center", DECL_VECTOR, 0,
			(void *) vm_copy_lvalue(newstmt->lv_center));
	newstmt->lv_rmajor = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_rmajor != NULL)
		pcontext_addsymbol("rmajor", DECL_FLOAT, 0,
			(void *) vm_copy_lvalue(newstmt->lv_rmajor));
	newstmt->lv_rminor = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_rminor != NULL)
		pcontext_addsymbol("rminor", DECL_FLOAT, 0,
			(void *) vm_copy_lvalue(newstmt->lv_rminor));

	// Parse the torus's parameters and body.
	//
	nparams = parse_paramlist("OEOEOEOB", "torus", params);
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr_center == NULL) /* First one is the center. */
					newstmt->expr_center = params[i].data.expr;
				else if (newstmt->expr_rmajor == NULL) /* Second one is the major radius. */
					newstmt->expr_rmajor = params[i].data.expr;
				else                 /* Third one is the minor radius. */
					newstmt->expr_rminor = params[i].data.expr;
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
 *	VM torus { block }
 *
 * @param   curstmt - VMStmt - Pointer to the object statement container.
 */
void vm_torus(VMStmt *curstmt)
{
	VMStmtTorus *	stmtobj = (VMStmtTorus *) curstmt;
	Vec3			center;
	double			rmajor, rminor;
	Object *		newobj;
	
	vm_begin_object((VMStmtObj *) curstmt);
	
	/* Create this object and store it in the VM stack. */
	V3Set(&center, 0, 0, 0);
	rmajor = 0.5;
	rminor = 0.5;
	if (stmtobj->expr_center != NULL)
		vm_evalvector(stmtobj->expr_center, &center);
	if (stmtobj->expr_rmajor != NULL)
		rmajor = vm_evaldouble(stmtobj->expr_rmajor);
	if (stmtobj->expr_rminor != NULL)
		rminor = vm_evaldouble(stmtobj->expr_rminor);
	newobj = Ray_MakeTorus(&center, rmajor, rminor);
	if (newobj == NULL)
	{
		logmemerror("torus");
		return;
	}

	vmstack_setcurobj(newobj);

	/* Set our intrinsic lvalues to object's initial values. */
	if (stmtobj->lv_center != NULL)
		V3Copy(&stmtobj->lv_center->v, &center);
	if (stmtobj->lv_rmajor != NULL)
		stmtobj->lv_rmajor->v.x = rmajor;
	if (stmtobj->lv_rminor != NULL)
		stmtobj->lv_rminor->v.x = rminor;

	// Run the statements in the object's block.
	//
	vm_execute_object_block((VMStmtObj *) stmtobj);

	/* Set the object parameters to the possibly new values of 
	 * our intrinsic lvalues.
	 */
	if (stmtobj->lv_center != NULL)
		V3Copy(&center, &stmtobj->lv_center->v);
	if (stmtobj->lv_rmajor != NULL)
		rmajor = stmtobj->lv_rmajor->v.x;
	if (stmtobj->lv_rminor != NULL)
		rminor = stmtobj->lv_rminor->v.x;
	Ray_SetTorus(newobj->data.torus, &center, rmajor, rminor);

	vm_finish_object((VMStmtObj *) curstmt, newobj, 1);
}

/**
 *	Cleanup function for VM 'torus' stmt.
 *
 * @param   curstmt - VMStmt - Pointer to the object statement container.
 */
void vm_torus_cleanup(VMStmt *curstmt)
{
	VMStmtTorus *	stmtobj = (VMStmtTorus *) curstmt;

	// Free the parameter initializer expressions.
	//
	delete_exprtree(stmtobj->expr_center);
	stmtobj->expr_center = NULL;
	delete_exprtree(stmtobj->expr_rmajor);
	stmtobj->expr_rmajor = NULL;
	delete_exprtree(stmtobj->expr_rminor);
	stmtobj->expr_rminor = NULL;

	// Free the parameter l-values.
	//
	vm_delete_lvalue(stmtobj->lv_center);
	stmtobj->lv_center = NULL;
	vm_delete_lvalue(stmtobj->lv_rmajor);
	stmtobj->lv_rmajor = NULL;
	vm_delete_lvalue(stmtobj->lv_rminor);
	stmtobj->lv_rminor = NULL;
	
	// Cleanup the base object statement.
	//
	vm_object_cleanup(curstmt);
}



/*************************************************************************
*
*	Cone, Closed Cone, Cylinder, Closed Cylinder
*
*************************************************************************/

/**
 * Container for a 'cylinder', 'closed_cylinder', 'cone' or
 * 'closed_cone' object statement.
 */
typedef struct tVMStmtCone
{
	VMStmtObj	vmstmtobj;
	VMExpr		*expr_basept;
	VMExpr		*expr_endpt;
	VMExpr		*expr_baserad;
	VMExpr		*expr_endrad;
	VMLValue	*lv_basept;
	VMLValue	*lv_endpt;
	VMLValue	*lv_baserad;
	VMLValue	*lv_endrad;
	int			closed;
	int			cylinder;
} VMStmtCone;



/*
 * Methods for the 'cone' stmt.
 */
static void vm_cone(VMStmt *thisstmt);
static void vm_cone_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_cone_stmt_methods =
{
	TK_CONE,
	vm_cone,
	vm_cone_cleanup
};



/**
 *	Parse the cone/cylinder object { } block.
 *
 *	The 'cone', 'closed_cone', 'cylinder', or 'closed_cylinder' keyword
 *  has just been parsed.
 *
 *  @param token - int identfies which of the four possible types of
 *     cones/cylinders we are parsing.
 *
 *	@return VMStmt *, ptr to a cone stmt if we are compiling
 *		for the virtual machine. Returns NULL otherwise.
 */
VMStmt * parse_vm_cone_or_cylinder(int token)
{
	VMStmtCone *	newstmt;
	ParamList		params[5];
	int				nparams, i;
	char *			name;
	int				cylinder = 0, closed = 0;;

	switch (token)
	{
		case TK_CONE:
			name = "cone";
			break;
		case TK_CLOSED_CONE:
			name = "closed_cone";
			closed = 1;
			break;
		case TK_CYLINDER:
			name = "cylinder";
			cylinder = 1;
			break;
		case TK_CLOSED_CYLINDER:
			name = "closed_cylinder";
			cylinder = 1;
			closed = 1;
			break;
		default:
			assert(0);
			return NULL;
	}

	newstmt = (VMStmtCone *) begin_parse_object(
		sizeof(VMStmtCone),
		name,
		TK_CONE,
		&s_cone_stmt_methods);

	// Make sure alloc succeeded.
	//
	if (newstmt == NULL)
		return NULL;

	// Set the 'closed' and 'cylinder' flags.
	//
	newstmt->closed = closed;
	newstmt->cylinder = cylinder;

	// Add 'basept', 'endpt', 'baserad' and 'endrad' to the local namespace.
	// These will be bound at runtime to the actual fields in the object.
	//
	newstmt->lv_basept = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_basept != NULL)
		pcontext_addsymbol("basept", DECL_VECTOR, 0,
			(void *) vm_copy_lvalue(newstmt->lv_basept));
	newstmt->lv_endpt = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_endpt != NULL)
		pcontext_addsymbol("endpt", DECL_VECTOR, 0,
			(void *) vm_copy_lvalue(newstmt->lv_endpt));
	if (cylinder)
	{
		newstmt->lv_baserad = vm_new_lvalue(TK_FLOAT);
		if (newstmt->lv_baserad != NULL)
			pcontext_addsymbol("radius", DECL_FLOAT, 0,
				(void *) vm_copy_lvalue(newstmt->lv_baserad));
		newstmt->lv_endrad = NULL;
	}
	else
	{
		newstmt->lv_baserad = vm_new_lvalue(TK_FLOAT);
		if (newstmt->lv_baserad != NULL)
			pcontext_addsymbol("baserad", DECL_FLOAT, 0,
				(void *) vm_copy_lvalue(newstmt->lv_baserad));
		newstmt->lv_endrad = vm_new_lvalue(TK_FLOAT);
		if (newstmt->lv_endrad != NULL)
			pcontext_addsymbol("endrad", DECL_FLOAT, 0,
				(void *) vm_copy_lvalue(newstmt->lv_endrad));
	}

	// Parse the cone/cylinder's parameters and body.
	//
	if (cylinder)
		nparams = parse_paramlist("OEOEOEOB", name, params);
	else
		nparams = parse_paramlist("OEOEOEOEOB", name, params);
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr_basept == NULL) /* Base point */
					newstmt->expr_basept = params[i].data.expr;
				else if (newstmt->expr_endpt == NULL) /* End point */
					newstmt->expr_endpt = params[i].data.expr;
				else if (newstmt->expr_baserad == NULL) /* Base radius */
					newstmt->expr_baserad = params[i].data.expr;
				else                              /* End radius (cone only) */
					newstmt->expr_endrad = params[i].data.expr;
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
 *	VM cone/cylinder/closed_cone/closed_cylinder { block }
 *
 * @param   curstmt - VMStmt - Pointer to the object statement container.
 */
void vm_cone(VMStmt *curstmt)
{
	VMStmtCone *	stmtobj = (VMStmtCone *) curstmt;
	Vec3			basept, endpt;
	double			baserad, endrad;
	Object *		newobj;
	
	vm_begin_object((VMStmtObj *) curstmt);
	
	/* Create this object and store it in the VM stack. */
	V3Set(&basept, 0, 0, 0);
	V3Set(&endpt, 0, 0, 1);
	baserad = 1;
	endrad = stmtobj->cylinder ? 1.0 : 0.0;
	if (stmtobj->expr_basept != NULL)
		vm_evalvector(stmtobj->expr_basept, &basept);
	if (stmtobj->expr_endpt != NULL)
		vm_evalvector(stmtobj->expr_endpt, &endpt);
	if (stmtobj->expr_baserad != NULL)
		baserad = vm_evaldouble(stmtobj->expr_baserad);
	if (stmtobj->expr_endrad != NULL)
		endrad = vm_evaldouble(stmtobj->expr_endrad);
	newobj = Ray_MakeCone(&basept, &endpt, baserad, endrad, stmtobj->closed);
	if (newobj == NULL)
	{
		logmemerror("cone");
		return;
	}

	vmstack_setcurobj(newobj);

	// Initialize our local lvalues to object's initial parameters.
	//
	if (stmtobj->lv_basept != NULL)
		V3Copy(&stmtobj->lv_basept->v, &basept);
	if (stmtobj->lv_endpt != NULL)
		V3Copy(&stmtobj->lv_endpt->v, &endpt);
	if (stmtobj->lv_baserad != NULL)
		stmtobj->lv_baserad->v.x = baserad;
	if (stmtobj->lv_endrad != NULL)
		stmtobj->lv_endrad->v.x = endrad;
	if (stmtobj->cylinder)
		endrad = baserad;

	// Run the statements in the object's block.
	//
	vm_execute_object_block((VMStmtObj *) stmtobj);

	// Set the object parameters to the possibly new values of 
	// our local lvalues.
	//
	if (stmtobj->lv_basept != NULL)
		V3Copy(&basept, &stmtobj->lv_basept->v);
	if (stmtobj->lv_endpt != NULL)
		V3Copy(&endpt, &stmtobj->lv_endpt->v);
	if (stmtobj->lv_baserad != NULL)
		baserad = stmtobj->lv_baserad->v.x;
	if (stmtobj->lv_endrad != NULL)
		endrad = stmtobj->lv_endrad->v.x;
	if (stmtobj->cylinder)
		endrad = baserad;
	Ray_SetCone(newobj->data.cone, &basept, &endpt, baserad, endrad, stmtobj->closed);

	vm_finish_object((VMStmtObj *) curstmt, newobj, 1);
}

/**
 *	Cleanup function for VM 'cone' stmt.
 *
 * @param   curstmt - VMStmt - Pointer to the object statement container.
 */
void vm_cone_cleanup(VMStmt *curstmt)
{
	VMStmtCone *	stmtobj = (VMStmtCone *) curstmt;

	// Free the parameter initializer expressions.
	//
	delete_exprtree(stmtobj->expr_basept);
	stmtobj->expr_basept = NULL;
	delete_exprtree(stmtobj->expr_endpt);
	stmtobj->expr_endpt = NULL;
	delete_exprtree(stmtobj->expr_baserad);
	stmtobj->expr_baserad = NULL;
	delete_exprtree(stmtobj->expr_endrad);
	stmtobj->expr_endrad = NULL;

	// Free the parameter l-values.
	//
	vm_delete_lvalue(stmtobj->lv_basept);
	stmtobj->lv_basept = NULL;
	vm_delete_lvalue(stmtobj->lv_endpt);
	stmtobj->lv_endpt = NULL;
	vm_delete_lvalue(stmtobj->lv_baserad);
	stmtobj->lv_baserad = NULL;
	vm_delete_lvalue(stmtobj->lv_endrad);
	stmtobj->lv_endrad = NULL;

	// Cleanup the base object statement.
	//
	vm_object_cleanup(curstmt);
}



/*************************************************************************/


/**
 *	Parse tokens that are common to all objects, such as
 *	surfaces, flags, etc.
 *
 * @param  stmtlist - Receives pointer to a list of statements if successful and any are found. NULL otherwise.
 *
 * @return  int - 1 to indicate token was processed, 0 if not recognized.
 */
int parse_vm_object_modifier_token(int token, VMStmt **stmtlist)
{
	int objtype_token = pcontext_getobjtype();
	
	*stmtlist = NULL;

	switch (token)
	{
		case DECL_SURFACE:
			*stmtlist = parse_vm_surface((Surface *) g_cur_token->data);
			break;
		case TK_SURFACE:
			*stmtlist = parse_vm_surface(NULL);
			break;

		case TK_TRANSLATE:
		case TK_SCALE:
		case TK_ROTATE:
			*stmtlist = parse_vm_transform(token);
			break;

		case TK_VERTEX:
			if (objtype_token == TK_POLYGON)
			{
				// Polygon vertices.
				//
				*stmtlist = parse_vm_polygon_vertex();
				break;
			}
			return 0;

		case TK_CYLINDER:
		case TK_PLANE:
		case TK_SPHERE:
			if (objtype_token == TK_BLOB)
			{
				// Blob elements.
				//
				*stmtlist = parse_vm_blob_element(token);
				break;
			}
			return 0;

		default:
			return 0;
	}

	// We processed the token.
	//
	return 1;
}
