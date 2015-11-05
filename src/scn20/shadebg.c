/**
 *****************************************************************************
 *  @file shadebg.c
 *  The background shader.
 *  See vm.h for the VMBackgroundShader type.
 *
 *****************************************************************************
 */

#include "local.h"

// Methods for the 'background_shader' stmt.
//
static void vm_background_shader(VMStmt *thisstmt);
static void vm_background_shader_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_background_shader_stmt_methods =
{
	TK_BACKGROUND_SHADER,
	vm_background_shader,
	vm_background_shader_cleanup
};



/**
 *	Parse the background_shader { } block.
 *
 *	The 'background_shader' keyword has just been parsed.
 *
 *	@return VMBackgroundShader *, ptr to a VMBackgroundShader.
 */
VMBackgroundShader * parse_vm_background_shader(void)
{
	ParamList				params[1];
	VMBackgroundShader *	newstmt =
		(VMBackgroundShader *) vm_alloc_shader(sizeof(VMBackgroundShader), &s_background_shader_stmt_methods);

	if (newstmt == NULL)
	{
		logmemerror("background_shader");
		return NULL;
	}

	// Save previous state on the stack.
	//
	pcontext_push("background_shader");
	pcontext_setobjtype(TK_BACKGROUND_SHADER);

	// Add all of the built-in variable names to the local namespace.
	// These will be bound at runtime to the actual fields in the shader.
	//
	newstmt->lv_D = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_D != NULL)
		pcontext_addsymbol("D", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_D));

	newstmt->lv_color = vm_new_lvalue(TK_VECTOR);
	if (newstmt->lv_color != NULL)
		pcontext_addsymbol("color", DECL_VECTOR, 0,
			(void *)vm_copy_lvalue(newstmt->lv_color));

	// Parse the shader's body.
	//
	if (parse_paramlist("B", "background_shader", params))
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



/*************************************************************************/

/**
 *	background_shader { block }
 *	This is the runtime function for this shader.
 */
void vm_background_shader(VMStmt *curstmt)
{
	VMBackgroundShader	*sh = (VMBackgroundShader *) curstmt;
	VMStmt				*stmt;
	Vec3				*color = (Vec3*) sh->vmshaderstmt.tmp_data;
	
	vmstack_push(curstmt);
	
	// Initialize the shader's color parameter with the
	// current background color.
	//
	V3Copy(&sh->lv_color->v, color);
	
	// Initialize the available runtime environment values to current
	// state of the renderer.
	// These are read only. Any changes to them in the shader are not stored.
	//
	if (sh->lv_D != NULL)
		V3Copy(&sh->lv_D->v, &rt_D);

	// Loop thru the statements in this block.
	//
	for (stmt = sh->vmshaderstmt.block; stmt != NULL; stmt = stmt->next)
	{
		stmt->methods->fn(stmt);
	}

	// Set the color to the possibly changed shader color value. 
	//
	V3Copy(color, &sh->lv_color->v);

	vmstack_pop();
}

/**
 *	Cleanup function for VM 'background_shader' stmt.
 */
void vm_background_shader_cleanup(VMStmt *curstmt)
{
	VMBackgroundShader *	sh = (VMBackgroundShader *) curstmt;

	// Recursively free the statements in our block. 
	//
	vm_delete(sh->vmshaderstmt.block);
	sh->vmshaderstmt.block = NULL;

	// Free up the lvalue objects we've associated with this context. 
	//
	vm_delete_lvalue(sh->lv_D);
	sh->lv_D = NULL;

	vm_delete_lvalue(sh->lv_color);
	sh->lv_color = NULL;
}
