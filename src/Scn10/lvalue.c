/*************************************************************************
*
*  lvalue.c - Statements involving l-value assignment and functions
*    for managing l-value structures.
*
*************************************************************************/

#include "local.h"

static void ExecLVAssignStmt(Stmt *stmt);
static void DeleteLVAssignStmt(Stmt *stmt);

StmtProcs lvassign_stmt_procs =
{
	OP_ASSIGN, /* Includes compound assignment expressions. */
	ExecLVAssignStmt,
	DeleteLVAssignStmt
};

Stmt *ParseLVInitExprStmt(LValue *lv)
{
	int token;
	Stmt *stmt = NULL;

	if((token = GetToken()) == OP_ASSIGN)
	{
		if((stmt = NewStmt()) != NULL)
		{
			if((stmt->data = (void *)ExprParseLVInitializer(lv)) != NULL)
			{
				stmt->procs = &lvassign_stmt_procs;
			}
			else
			{
				DeleteStmt(stmt);
				return NULL;
			}
		}
		else
			LogMemError("initialize");
	}
	else
		UngetToken();

	return stmt;
}

Stmt *ParseExprStmt(void)
{
	Stmt *stmt = NULL;
	Expr *expr = ExprParse();
	if(expr != NULL)
	{
		if((stmt = NewStmt()) != NULL)
		{
			stmt->data = (void *)expr;
			stmt->procs = &lvassign_stmt_procs;
		}
		else
		{
			ExprDelete(expr);
			LogMemError("expr statement");
		}
	}
	return stmt;
}

void ExecLVAssignStmt(Stmt *stmt)
{
	static char result[EXPR_RESULT_SIZE_MAX];
	static void *presult = &result;
	(void)ExprEval((Expr *)stmt->data, presult);
}

void DeleteLVAssignStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}

LValue *LValueNew(int type_token)
{
	LValue *lv = (LValue *)calloc(1, sizeof(LValue));
	if(lv != NULL)
	{
		lv->nrefs = 1;
		lv->type = type_token;
	}
	return lv;
}

void LValueDelete(LValue *lv)
{
	if(lv != NULL)
	{
		if(--lv->nrefs == 0)
			free(lv);
	}
}

LValue *LValueCopy(LValue *lv)
{
	if(lv != NULL)
		lv->nrefs++;
	return lv;
}
