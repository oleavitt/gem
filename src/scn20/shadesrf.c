/**
 *****************************************************************************
 *  @file shadesrf.c
 *  The surface shader.
 *  See vm.h for the VMSurfaceShader type.
 *
 *****************************************************************************
 */

#include "local.h"

// Methods for the 'surface_shader' stmt.
//
static void vm_surface_shader(VMStmt *thisstmt);
static void vm_surface_shader_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_surface_shader_stmt_methods =
{
	TK_SURFACE_SHADER,
	vm_surface_shader,
	vm_surface_shader_cleanup
};



/**
 *	Parse the surface_shader { } block.
 *
 *	The 'surface_shader' keyword has just been parsed.
 *
 *	@return VMSurfaceShader *, ptr to a VMSurfaceShader.
 */
VMSurfaceShader * parse_vm_surface_shader(void)
{
	ParamList			params[1];
	VMSurfaceShader *	newstmt =
		(VMSurfaceShader *) vm_alloc_shader(sizeof(VMSurfaceShader), &s_surface_shader_stmt_methods);

	if (newstmt == NULL)
	{
		logmemerror("surface_shader");
		return NULL;
	}

	// Save previous state on the stack.
	//
	pcontext_push("surface_shader");
	pcontext_setobjtype(TK_SURFACE_SHADER);

	// Add all of the built-in variable names to the local namespace.
	// These will be bound at runtime to the actual fields in the shader.
	///
	newstmt->lv_W = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_W != NULL)
		pcontext_addsymbol("W", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_W));
	newstmt->lv_D = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_D != NULL)
		pcontext_addsymbol("D", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_D));
	newstmt->lv_N = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_N != NULL)
		pcontext_addsymbol("N", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_N));
	newstmt->lv_O = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_O != NULL)
		pcontext_addsymbol("O", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_O));
	newstmt->lv_ON = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_ON != NULL)
		pcontext_addsymbol("ON", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_ON));
	newstmt->lv_u = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_u != NULL)
		pcontext_addsymbol("u", DECL_FLOAT, 0,
			(void *)vm_copy_lvalue(newstmt->lv_u));
	newstmt->lv_v = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_v != NULL)
		pcontext_addsymbol("v", DECL_FLOAT, 0,
			(void *)vm_copy_lvalue(newstmt->lv_v));

	newstmt->lv_color = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_color != NULL)
		pcontext_addsymbol("color", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_color));
	newstmt->lv_ka = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_ka != NULL)
		pcontext_addsymbol("ka", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_ka));
	newstmt->lv_kd = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_kd != NULL)
		pcontext_addsymbol("kd", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_kd));
	newstmt->lv_ks = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_ks != NULL)
		pcontext_addsymbol("ks", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_ks));
	newstmt->lv_Phong = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_Phong != NULL)
		pcontext_addsymbol("Phong", DECL_FLOAT, 0,
			(void *)vm_copy_lvalue(newstmt->lv_Phong));
	newstmt->lv_kr = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_kr != NULL)
		pcontext_addsymbol("kr", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_kr));
	newstmt->lv_kt = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_kt != NULL)
		pcontext_addsymbol("kt", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_kt));
	newstmt->lv_ior = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_ior != NULL)
		pcontext_addsymbol("ior", DECL_FLOAT, 0,
			(void *)vm_copy_lvalue(newstmt->lv_ior));
	newstmt->lv_outior = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_outior != NULL)
		pcontext_addsymbol("outior", DECL_FLOAT, 0,
			(void *)vm_copy_lvalue(newstmt->lv_outior));

	// Parse the shader's body.
	//
	if (parse_paramlist("B", "surface_shader", params))
		newstmt->vmshaderstmt.block = params[0].data.block;

	// Restore previous state.
	//
	pcontext_pop();

	// If there were errors parsing this, delete this shader and
	// return NULL.
	//
	if (g_error_count)
	{
		vm_delete((VMStmt *) newstmt);
		newstmt = NULL;
	}

	return newstmt;
}



int parse_vm_surface_shader_token(int token, VMStmt **stmtlist)
{
	*stmtlist = NULL;

	switch (token)
	{
// TODO:		case FN_TRACE_RAY:
//			break;
		default:
			return 0;
	}

	return 1;
}


/*************************************************************************/

/**
 *	surface_shader { block }
 *	This is the runtime function for this shader.
 */
void vm_surface_shader(VMStmt *curstmt)
{
	VMSurfaceShader *	sh = (VMSurfaceShader *) curstmt;
	VMStmt *			stmt;
	Surface				*surf = (Surface *) sh->vmshaderstmt.tmp_data;

	vmstack_push(curstmt);
	
	// Initialize the shader's lighting parameters with the
	// current values in the surface.
	//
	V3Copy(&sh->lv_color->v, &surf->color);
	V3Copy(&sh->lv_ka->v, &surf->ka);
	V3Copy(&sh->lv_kd->v, &surf->kd);
	V3Copy(&sh->lv_ks->v, &surf->ks);
	sh->lv_Phong->v.x = surf->spec_power;
	V3Copy(&sh->lv_kr->v, &surf->kr);
	V3Copy(&sh->lv_kt->v, &surf->kt);
	sh->lv_ior->v.x = surf->ior;
	sh->lv_outior->v.x = surf->outior;
	
	// Initialize the available runtime environment values to current
	// state of the renderer.
	// These are read only. Any changes to them in the shader are not stored.
	//
	V3Copy(&sh->lv_W->v, &rt_W);
	V3Copy(&sh->lv_D->v, &rt_D);
	V3Copy(&sh->lv_N->v, &rt_WN);
	V3Copy(&sh->lv_O->v, &rt_O);
	sh->lv_u->v.x = rt_u;
	sh->lv_v->v.x = rt_v;

	// Copy the transformed surface normal. This variable may be altered by use of the bump() surface function.
	//
	V3Copy(&sh->lv_ON->v, &rt_ON);

	// Loop thru the statements in this block.
	//
	for (stmt = sh->vmshaderstmt.block; stmt != NULL; stmt = stmt->next)
	{
		stmt->methods->fn(stmt);
	}

	// Save the possibly modified lighting parameter values from the shader
	// back to the surface.
	//
	V3Copy(&surf->color, &sh->lv_color->v);
	V3Copy(&surf->ka, &sh->lv_ka->v);
	V3Copy(&surf->kd, &sh->lv_kd->v);
	V3Copy(&surf->ks, &sh->lv_ks->v);
	surf->spec_power = sh->lv_Phong->v.x;
	V3Copy(&surf->kr, &sh->lv_kr->v);
	V3Copy(&surf->kt, &sh->lv_kt->v);
	surf->ior = sh->lv_ior->v.x;
	surf->outior = sh->lv_outior->v.x;

	vmstack_pop();
}

/**
 *	Cleanup function for VM 'surface_shader' stmt.
 */
void vm_surface_shader_cleanup(VMStmt *curstmt)
{
	VMSurfaceShader *	sh = (VMSurfaceShader *) curstmt;

	// Recursively free the statements in our block.
	//
	vm_delete(sh->vmshaderstmt.block);
	sh->vmshaderstmt.block = NULL;

	// Free up the lvalue objects we've associated with this context.
	//
	vm_delete_lvalue(sh->lv_W);
	sh->lv_W = NULL;
	vm_delete_lvalue(sh->lv_D);
	sh->lv_D = NULL;
	vm_delete_lvalue(sh->lv_N);
	sh->lv_N = NULL;
	vm_delete_lvalue(sh->lv_O);
	sh->lv_O = NULL;
	vm_delete_lvalue(sh->lv_ON);
	sh->lv_ON = NULL;
	vm_delete_lvalue(sh->lv_u);
	sh->lv_u = NULL;
	vm_delete_lvalue(sh->lv_v);
	sh->lv_v = NULL;

	vm_delete_lvalue(sh->lv_color);
	sh->lv_color = NULL;
	vm_delete_lvalue(sh->lv_ka);
	sh->lv_ka = NULL;
	vm_delete_lvalue(sh->lv_kd);
	sh->lv_kd = NULL;
	vm_delete_lvalue(sh->lv_ks);
	sh->lv_ks = NULL;
	vm_delete_lvalue(sh->lv_Phong);
	sh->lv_Phong = NULL;
	vm_delete_lvalue(sh->lv_kr);
	sh->lv_kr = NULL;
	vm_delete_lvalue(sh->lv_kt);
	sh->lv_kt = NULL;
	vm_delete_lvalue(sh->lv_ior);
	sh->lv_ior = NULL;
	vm_delete_lvalue(sh->lv_outior);
	sh->lv_outior = NULL;
}
