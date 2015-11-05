/**
 *****************************************************************************
 *  @file vm.c
 *  Top level and other misc functions for the VM.
 *
 *****************************************************************************
 */

#include "local.h"



int vm_init(void)
{
	if (!vmstack_init())
		return 0;
	if (!vmexpr_init())
		return 0;

	return 1;
}



int vm_reset(void)
{
	if (!vmexpr_init())
		return 0;

	return 1;
}



int vm_close(void)
{
	vmstack_close();

	return 1;
}



/**
 *	Delete a statement list.
 *
 *	This function will be called recursively by the 'cleanup' functions
 *	of statements that have control blocks.
 *
 *	@param	stmtlist VMStmt *, head of a list of statements to delete.
 */
void vm_delete(VMStmt * stmtlist)
{
	while (stmtlist != NULL)
	{
		VMStmt * stmt = stmtlist;

		stmtlist = stmtlist->next;

		stmt->methods->cleanup(stmt);
		vm_free_stmt(stmt);
	}
}

VMShader * vm_alloc_shader(size_t size, VMStmtMethods *methods)
{
	VMShader *shader = (VMShader *) vm_alloc_stmt(size, methods);

	if (shader != NULL)
	{
		shader->tmp_data = NULL;
		shader->tmp_arglist = NULL; // TODO shader: Arguments for parameterized shaders
		shader->nrefs = 1;
	}

	return shader;
}

#ifndef NDEBUG
int num_stmts_allocd = 0;
#endif // NDEBUG

VMStmt * vm_alloc_stmt(size_t size, VMStmtMethods *methods)
{
	VMStmt *stmt = (VMStmt *) calloc(1, size);

	if (stmt != NULL)
	{
#ifndef NDEBUG
		num_stmts_allocd++;
#endif // NDEBUG
		stmt->next = NULL;
		stmt->methods = methods;
	}

	return stmt;
}

void vm_free_stmt(VMStmt *stmt)
{
#ifndef NDEBUG
	if (stmt != NULL)
	{
		num_stmts_allocd--;
	}
#endif // NDEBUG
	free(stmt);
}
