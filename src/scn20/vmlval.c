/**
 *****************************************************************************
 *  @file vmlval.c
 *  Statements involving l-value assignment and functions
 *  for managing l-value structures.
 *
 *****************************************************************************
 */

#include "local.h"

/**
 * Container for the lvalue statement.
 */
typedef struct tVMStmtLValue
{
	VMStmt		vmstmt;
	VMExpr		*expr;
} VMStmtLValue;



static void vm_lvalue(VMStmt *thisstmt);
static void vm_lvalue_cleanup(VMStmt *thisstmt);

VMStmtMethods s_lvalue_stmt_methods =
{
	OP_ASSIGN, /* Includes compound assignment expressions. */
	vm_lvalue,
	vm_lvalue_cleanup
};



VMStmt *parse_vm_lvalue_init(VMLValue *lv)
{
	int				token;
	VMStmtLValue	*thisstmt = NULL;

	if ((token = gettoken()) == OP_ASSIGN)
	{
		thisstmt =
			(VMStmtLValue *) vm_alloc_stmt(sizeof(VMStmtLValue), &s_lvalue_stmt_methods);
		if (thisstmt != NULL)
		{
			if ((thisstmt->expr = parse_vm_lvalue_expr(lv)) == NULL)
			{
				vm_delete((VMStmt *)thisstmt);
				return NULL;
			}
		}
		else
			logmemerror("initialize");
	}
	else
		gettoken_Unget();

	return (VMStmt *) thisstmt;
}



VMStmt *parse_vm_expr(void)
{
	VMStmtLValue	*thisstmt = NULL;
	VMExpr			*expr = parse_exprtree();
	if (expr != NULL)
	{
		thisstmt =
			(VMStmtLValue *) vm_alloc_stmt(sizeof(VMStmtLValue), &s_lvalue_stmt_methods);
		if (thisstmt != NULL)
		{
			thisstmt->expr = expr;
		}
		else
		{
			delete_exprtree(expr);
			logmemerror("expr statement");
		}
	}

	return (VMStmt *) thisstmt;
}



void vm_lvalue(VMStmt *thisstmt)
{
	VMStmtLValue	*lvstmt = (VMStmtLValue *) thisstmt;
	static char		result[EXPR_RESULT_SIZE_MAX];
	static void		*presult = &result;

	/* Evaluate the assignment expression */
	vm_evalexpr(lvstmt->expr, presult);
}



void vm_lvalue_cleanup(VMStmt *thisstmt)
{
	VMStmtLValue	*lvstmt = (VMStmtLValue *) thisstmt;

	/* Free the assignment expression */
	delete_exprtree(lvstmt->expr);
}



VMLValue *vm_new_lvalue(int type_token)
{
	VMLValue *lv = (VMLValue *)calloc(1, sizeof(VMLValue));
	if (lv != NULL)
	{
		lv->nrefs = 1;
		lv->type = type_token;
	}

	return lv;
}

void vm_delete_lvalue(VMLValue *lv)
{
	if (lv != NULL)
	{
		if(--lv->nrefs == 0)
			free(lv);
	}
}

VMLValue *vm_copy_lvalue(VMLValue *lv)
{
	if (lv != NULL)
		lv->nrefs++;

	return lv;
}
