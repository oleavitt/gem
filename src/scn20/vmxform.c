/**
 *****************************************************************************
 * @file vmxform.c
 *  Virtual machine functions for transform operations.
 *
 *****************************************************************************
 */

#include "local.h"



/**
 * Container for a transform statement.
 */
typedef struct tVMStmtXform
{
	VMStmt		vmstmt;
	VMExpr		*expr;
	int			transform_type;
} VMStmtXform;



/*
 * Methods for the transform statement.
 */
static void vm_xform(VMStmt *thisstmt);
static void vm_xform_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_xform_stmt_methods =
{
	TK_NULL,
	vm_xform,
	vm_xform_cleanup
};



/**
 *	Parse a transform statement.
 *
 *	@return VMStmt *, ptr to a transform stmt if successful.
 */
VMStmt *parse_vm_transform(int transform_type_token)
{
	int			token;
	VMStmtXform	*newstmt =
		(VMStmtXform *) vm_alloc_stmt(sizeof(VMStmtXform), &s_xform_stmt_methods);
	if (newstmt == NULL)
	{
		logmemerror(g_token_buffer);
		return NULL;
	}

	newstmt->transform_type = 
		(transform_type_token == TK_ROTATE) ? XFORM_ROTATE :
		(transform_type_token == TK_SCALE) ? XFORM_SCALE :
		XFORM_TRANSLATE;
	
	/* Parse the transform's expression. */
	if ((newstmt->expr = parse_exprtree()) != NULL)
		if ((token = gettoken()) != OP_SEMICOLON)
			gettoken_ErrUnknown(token, ";");

	return (VMStmt *)newstmt;
}



/*************************************************************************/

/**
 *	VM transform statement
 */
void vm_xform(VMStmt *curstmt)
{
	VMStmtXform *	stmt = (VMStmtXform *) curstmt;
	Vec3			v;
	Object			*curobj;

	/* Evaluate the expression. */
	vm_evalvector(stmt->expr, &v);

	/* Apply the transform to whatever object we are in. */
	curobj = vmstack_getcurobj();
	if (curobj != NULL)
		Ray_Transform_Object(curobj, &v, stmt->transform_type);
	/* TODO: Apply this to a surface if in a surface block. */
}

/**
 *	Cleanup function for VM transform stmt.
 */
void vm_xform_cleanup(VMStmt *curstmt)
{
	VMStmtXform *	stmtxform = (VMStmtXform *) curstmt;

	/* Delete the expression. */
	delete_exprtree(stmtxform->expr);
	stmtxform->expr = NULL;
}
