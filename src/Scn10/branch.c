/*************************************************************************
*
*  branch.c - Branching statements - if-else, break, continue.
*
*************************************************************************/

#include "local.h"

typedef struct tag_ifdata
{
	Expr *expr;
	Stmt *block;
	Stmt *elseblock;
} IfData;

static void ExecIfStmt(Stmt *stmt);
static void ExecBreakStmt(Stmt *stmt);
static void ExecContinueStmt(Stmt *stmt);
static void DeleteIfStmt(Stmt *stmt);
static void DeleteBreakStmt(Stmt *stmt);
static void DeleteContinueStmt(Stmt *stmt);

StmtProcs if_stmt_procs =
{
	TK_IF,
	ExecIfStmt,
	DeleteIfStmt
};

StmtProcs break_stmt_procs =
{
	TK_BREAK,
	ExecBreakStmt,
	DeleteBreakStmt
};

StmtProcs continue_stmt_procs =
{
	TK_CONTINUE,
	ExecContinueStmt,
	DeleteContinueStmt
};

Stmt *ParseIfStmt(int (*ParseDetails)(Stmt **stmt))
{
	Stmt *stmt;
	IfData *id;
	if((stmt = NewStmt()) != NULL)
	{
		if((id = (IfData *)calloc(1, sizeof(IfData))) != NULL)
		{
			stmt->procs = &if_stmt_procs;
			stmt->data = (void *)id;
			if((id->expr = ExprParse()) != NULL)
			{
				id->block = ParseBlock("if", ParseDetails);
				if(GetToken() == TK_ELSE)
				{
					id->elseblock = ParseBlock("else", ParseDetails);
				}
				else
				{
					UngetToken();
					id->elseblock = NULL;
				}
				return stmt;
			}
		}
		else
			LogMemError("if");
	}
	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseBreakStmt(void)
{
	Stmt *stmt;
	if((stmt = NewStmt()) != NULL)
	{
		int token;
		stmt->procs = &break_stmt_procs;
		stmt->data = (void *)ExprParse();
		if((token = GetToken()) == OP_SEMICOLON)
			return stmt;
		else
			ErrUnknown(token, ";", "break");
	}
	else
		LogMemError("break");
	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseContinueStmt(void)
{
	Stmt *stmt;
	if((stmt = NewStmt()) != NULL)
	{
		int token;
		stmt->procs = &continue_stmt_procs;
		stmt->data = NULL;
		if((token = GetToken()) == OP_SEMICOLON)
			return stmt;
		else
			ErrUnknown(token, ";", "continue");
	}
	else
		LogMemError("continue");
	DeleteStmt(stmt);
	return NULL;
}


void ExecIfStmt(Stmt *stmt)
{
	IfData *id = (IfData *)stmt->data;
	Stmt *s;
	for(s = (fabs(ExprEvalDouble(id->expr)) > EPSILON) ?
		id->block : id->elseblock;
		s != NULL; s = s->next)
	{
		s->procs->Exec(s);
		if(s->break_count)
		{
			stmt->break_count = s->break_count;
			s->break_count = 0;
			return;
		}
		if(s->continue_flag)
		{
			stmt->continue_flag = 1;
			s->continue_flag = 0;
			return;
		}
	}
}

void ExecBreakStmt(Stmt *stmt)
{
	Expr *expr = (Expr *)stmt->data;
	if(expr != NULL)
	{
		int cnt = (int)ExprEvalDouble(expr);
		stmt->break_count = (cnt > 0) ? cnt : 0;
	}
	else
		stmt->break_count = 1;
}

void ExecContinueStmt(Stmt *stmt)
{
	stmt->continue_flag = 1;
}


void DeleteIfStmt(Stmt *stmt)
{
	IfData *id = (IfData *)stmt->data;
	ExprDelete(id->expr);
	DeleteStatements(id->block);
	DeleteStatements(id->elseblock);
	free(id);
}

void DeleteBreakStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}

void DeleteContinueStmt(Stmt *stmt)
{
}
