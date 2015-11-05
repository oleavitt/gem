/*************************************************************************
*
*  viewport.c - Viewport statement.
*
*  syntax: veiwport [vexpr from [, vexpr at [, fexpr angle]]];
*
*************************************************************************/

#include "local.h"

static void ExecCausticsStmt(Stmt *stmt);
static void DeleteCausticsStmt(Stmt *stmt);

StmtProcs caustics_stmt_procs =
{
	TK_CAUSTICS,
	ExecCausticsStmt,
	DeleteCausticsStmt
};

Stmt *ParseCausticsStmt(void)
{
	int token;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("caustics");
		return NULL;
	}
	stmt->procs = &caustics_stmt_procs;
	if((token = GetToken()) == OP_SEMICOLON)
		return stmt;
	ErrUnknown(token, ";", "caustics");
	DeleteStmt(stmt);
	return NULL;
}

void ExecCausticsStmt(Stmt *stmt)
{
		ray_setup_data->use_fake_caustics = 1;
}

void DeleteCausticsStmt(Stmt *stmt)
{
}