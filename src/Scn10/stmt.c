/*************************************************************************
*
*  stmt.c - Functions for managing Stmt structures.
*
*************************************************************************/

#include "local.h"

/*************************************************************************
*
*  Null statement - Plug procs for undefined statement structures.
*
*************************************************************************/
static void ExecNullStmt(Stmt *stmt);
static void DeleteNullStmt(Stmt *stmt);

StmtProcs null_stmt_procs =
{
	TK_NULL,
	ExecNullStmt,
	DeleteNullStmt
};

void ExecNullStmt(Stmt *stmt)
{
}

void DeleteNullStmt(Stmt *stmt)
{
}

/*************************************************************************
*
*  ExecStatements()
*  Main entry point to execute a compiled list of statements.
*
*************************************************************************/
void ExecStatements(Stmt *stmts)
{
	while(stmts != NULL)
	{
		/* TODO: Put system cooperation call here. */
		stmts->break_count = 0;
		stmts->procs->Exec(stmts);
		if(stmts->break_count)
			break;
		stmts = stmts->next;
	}
}

void ExecBlock(Stmt *stmt, Stmt *block)
{
	Stmt *s;
	for(s = block; s != NULL; s = s->next)
	{
		s->procs->Exec(s);
		if(s->break_count)
		{
			stmt->break_count = s->break_count;
			s->break_count = 0;
			break;
		}
		if(s->continue_flag)
		{
			s->continue_flag = 0;
			stmt->continue_flag = 1;
			break;
		}
	}
}

void DeleteStatements(Stmt *stmts)
{
	Stmt *s;
	while(stmts != NULL)
	{
		s = stmts;
		stmts = stmts->next;
		DeleteStmt(s);
	}
}

Stmt *NewStmt(void)
{
	Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
	if(stmt != NULL)
	{
		stmt->procs = &null_stmt_procs;
		stmt->next = NULL;		
		stmt->data = NULL;		
	}
	return stmt;
}

void DeleteStmt(Stmt *stmt)
{
	if(stmt != NULL)
	{
		stmt->procs->Delete(stmt); /* Statement type-specific cleanup. */
		free(stmt);
	}
}
