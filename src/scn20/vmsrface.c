/**
 *****************************************************************************
 *  @file vmsrface.c
 *  Virtual machine functions for sufaces.
 *
 *****************************************************************************
 */

#include "local.h"



/*
 * Container for a 'surface' object statement.
 */
typedef struct tVMStmtSurface
{
	VMStmt			vmstmt;
	VMStmt			*block;
	VMSurfaceShader	*shaders;
	Surface			*src_surface;
	VMLValue		*lv_color;
	VMLValue		*lv_ambient;
	VMLValue		*lv_diffuse;
	VMLValue		*lv_Phong;
	VMLValue		*lv_specular;
	VMLValue		*lv_reflection;
	VMLValue		*lv_transmission;
	VMLValue		*lv_ior;
	VMLValue		*lv_outior;
} VMStmtSurface;



/*
 * Methods for the 'surface' stmt.
 */
static void vm_surface(VMStmt *thisstmt);
static void vm_surface_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_surface_stmt_methods =
{
	TK_SURFACE,
	vm_surface,
	vm_surface_cleanup
};



static VMStmtSurface *s_parent_surface_stmt = NULL;


/**
 *	Parse the surface object { } block.
 *
 *	The 'surface' keyword has just been parsed.
 *
 *	@param	src_surface		[optional] ptr to a surface to share or
 *			inherit from - can be NULL if none.
 *	@return VMStmt *, ptr to a surface stmt or NULL if unsuccessful
 */
VMStmt *parse_vm_surface(Surface *src_surface)
{
	VMStmtSurface * grandparent_surface_stmt;
	VMStmtSurface *	newstmt =
		(VMStmtSurface *) vm_alloc_stmt(sizeof(VMStmtSurface), &s_surface_stmt_methods);

	if (newstmt == NULL)
	{
		logmemerror("surface");
		return NULL;
	}

	/* Save previous state on the stack. */
	pcontext_push("surface");
	pcontext_setobjtype(TK_SURFACE);
	grandparent_surface_stmt = s_parent_surface_stmt;
	s_parent_surface_stmt = newstmt;

	/* Build a surface statement and add it to the VM. */
	newstmt->src_surface = src_surface;
	newstmt->shaders = NULL;

	/*
	 * Add all of the lighting constant values to the local namespace.
	 * These will be bound at runtime to the actual fields in the surface.
	 */
	/* color */
	newstmt->lv_color = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_color != NULL)
		pcontext_addsymbol("color", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_color));
	/* ambient */
	newstmt->lv_ambient = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_ambient != NULL)
		pcontext_addsymbol("ambient", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_ambient));
	/* diffuse */
	newstmt->lv_diffuse = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_diffuse != NULL)
		pcontext_addsymbol("diffuse", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_diffuse));
	/* specular */
	newstmt->lv_specular = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_specular != NULL)
		pcontext_addsymbol("specular", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_specular));
	/* Phong */
	newstmt->lv_Phong = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_Phong != NULL)
		pcontext_addsymbol("Phong", DECL_FLOAT, 0,
			(void *)vm_copy_lvalue(newstmt->lv_Phong));
	/* reflection */
	newstmt->lv_reflection = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_reflection != NULL)
		pcontext_addsymbol("reflection", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_reflection));
	/* transmission */
	newstmt->lv_transmission = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_transmission != NULL)
		pcontext_addsymbol("transmission", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_transmission));
	/* ior */
	newstmt->lv_ior = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_ior != NULL)
		pcontext_addsymbol("ior", DECL_FLOAT, 0,
			(void *)vm_copy_lvalue(newstmt->lv_ior));
	/* outior */
	newstmt->lv_outior = vm_new_lvalue(TK_FLOAT);
	if (newstmt->lv_outior != NULL)
		pcontext_addsymbol("outior", DECL_FLOAT, 0,
			(void *)vm_copy_lvalue(newstmt->lv_outior));

	/* Parse the surface's body. */
	newstmt->block = parse_vm_block();

	/* Restore previous state. */
	pcontext_pop();
	s_parent_surface_stmt = grandparent_surface_stmt;

	return (VMStmt *)newstmt;
}



int parse_vm_surface_token(int token, VMStmt **stmtlist)
{
	*stmtlist = NULL;

	if (s_parent_surface_stmt == NULL)
	{
		/* We must be currently parsing a surface stmt body to use
		 * this function.
		 */
		assert(0);
		return 0;
	}

	switch (token)
	{
		case TK_SURFACE_SHADER:
			{
				VMSurfaceShader *sh = parse_vm_surface_shader();
				if (sh != NULL)
				{
					/* Add this shader to the list of shaders on
					 * this surface
					 */
					if (s_parent_surface_stmt->shaders != NULL)
					{
						VMStmt *shprev =
							(VMStmt *) s_parent_surface_stmt->shaders;
						while (shprev->next != NULL)
							shprev = shprev->next;
						shprev->next = (VMStmt *) sh;
					}
					else
						s_parent_surface_stmt->shaders = sh;
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
 *	VM surface statement method
 *  surface { block }
 */
void vm_surface(VMStmt *curstmt)
{
	VMStmtSurface	*stmttex = (VMStmtSurface *) curstmt;
	VMStmt			*stmt;
	VMStmt			*sh;
	Surface			*surface;
	Object			*curobj;

	vmstack_push(curstmt);
	
	if (stmttex->src_surface != NULL)
	{
		/* We are reusing a custom defined surface.
		 * If there is block attached, this means we are intending to modify it
		 * locally. In that case, we need to create a new instance that is a
		 * copy of the defined surface.
		 * Otherwise, to be memory-efficient, we share the same surface data.
		 */
		surface = (stmttex->block != NULL) ?
			Ray_CloneSurface(stmttex->src_surface) :
			Ray_ShareSurface(stmttex->src_surface);
	}
	else
		surface = Ray_NewSurface();

	if (surface == NULL)
	{
		logmemerror("surface");
		return;
	}

	curobj = vmstack_getcurobj();

	// Add in all of the shaders that were parsed for this surface.
	// TODO: When we start using declared shaders, be sure to use Ray_ShareVMShader().
	//
	for (sh = (VMStmt *) stmttex->shaders; sh != NULL; sh = sh->next)
		surface->shaders = Ray_AddShader(surface->shaders, (VMShader *) sh);

	if (stmttex->block != NULL)
	{
		/* Set our intrinsic lvalues to surface's initial values. */
		if (stmttex->lv_color != NULL)
			V3Copy(&stmttex->lv_color->v, &surface->color);
		if (stmttex->lv_ambient != NULL)
			V3Copy(&stmttex->lv_ambient->v, &surface->ka);
		if (stmttex->lv_diffuse != NULL)
			V3Copy(&stmttex->lv_diffuse->v, &surface->kd);
		if (stmttex->lv_specular != NULL)
			V3Copy(&stmttex->lv_specular->v, &surface->ks);
		if (stmttex->lv_Phong != NULL)
			stmttex->lv_Phong->v.x = surface->spec_power;
		if (stmttex->lv_reflection != NULL)
			V3Copy(&stmttex->lv_reflection->v, &surface->kr);
		if (stmttex->lv_transmission != NULL)
			V3Copy(&stmttex->lv_transmission->v, &surface->kt);
		if (stmttex->lv_ior != NULL)
			stmttex->lv_ior->v.x = surface->ior;
		if (stmttex->lv_outior != NULL)
			stmttex->lv_outior->v.x = surface->outior;

		/* Loop thru the statements in this block. */
		for (stmt = stmttex->block; stmt != NULL; stmt = stmt->next)
		{
			stmt->methods->fn(stmt);
		}

		/* Set the surface parameters to the possibly new values of 
		 * our intrinsic lvalues.
		 */
		if (stmttex->lv_color != NULL)
			V3Copy(&surface->color, &stmttex->lv_color->v);
		if (stmttex->lv_ambient != NULL)
			V3Copy(&surface->ka, &stmttex->lv_ambient->v);
		if (stmttex->lv_diffuse != NULL)
			V3Copy(&surface->kd, &stmttex->lv_diffuse->v);
		if (stmttex->lv_specular != NULL)
			V3Copy(&surface->ks, &stmttex->lv_specular->v);
		if (stmttex->lv_Phong != NULL)
			surface->spec_power = stmttex->lv_Phong->v.x;
		if (stmttex->lv_reflection != NULL)
			V3Copy(&surface->kr, &stmttex->lv_reflection->v);
		if (stmttex->lv_transmission != NULL)
			V3Copy(&surface->kt, &stmttex->lv_transmission->v);
		if (stmttex->lv_ior != NULL)
			surface->ior = stmttex->lv_ior->v.x;
		if (stmttex->lv_outior != NULL)
			surface->outior = stmttex->lv_outior->v.x;
	}

	/* Now put our newly created surface somewhere depending upon
	 * current state.
	 */
	if (curobj != NULL)
	{
		/* Assign it directly to an object. */
		/* TODO: Support for layered surfaces! */
		Ray_DeleteSurface(curobj->surface);
		curobj->surface = surface;

		/* If surface is transmissive, indicate this in the object flags. */
		if (!V3IsZero(&surface->kt))
			curobj->flags |= OBJ_FLAG_TRANSMISSIVE;
	}
	else if (g_define_mode)
	{
		/* It's a "define"d symbol store it in the symbol table. */
		pcontext_addsymbol(g_define_name, DECL_SURFACE, 0, surface);
	}
	else
	{
		/* TODO: Set default surface. */
		Ray_DeleteSurface(surface);
		assert(0); /* Not implemented yet. */
	}

	vmstack_pop();
}

/**
 *	Cleanup method for VM 'surface' stmt.
 */
void vm_surface_cleanup(VMStmt *curstmt)
{
	VMStmtSurface *	stmttex = (VMStmtSurface *) curstmt;

	/* Recursively free the statements in our block. */
	vm_delete(stmttex->block);
	stmttex->block = NULL;
	vm_delete_lvalue(stmttex->lv_color);
	stmttex->lv_color = NULL;
	vm_delete_lvalue(stmttex->lv_ambient);
	stmttex->lv_ambient = NULL;
	vm_delete_lvalue(stmttex->lv_diffuse);
	stmttex->lv_diffuse = NULL;
	vm_delete_lvalue(stmttex->lv_specular);
	stmttex->lv_specular = NULL;
	vm_delete_lvalue(stmttex->lv_Phong);
	stmttex->lv_Phong = NULL;
	vm_delete_lvalue(stmttex->lv_reflection);
	stmttex->lv_reflection = NULL;
	vm_delete_lvalue(stmttex->lv_transmission);
	stmttex->lv_transmission = NULL;
	vm_delete_lvalue(stmttex->lv_ior);
	stmttex->lv_ior = NULL;
	vm_delete_lvalue(stmttex->lv_outior);
	stmttex->lv_outior = NULL;
}
