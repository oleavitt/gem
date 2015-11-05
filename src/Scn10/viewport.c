/*************************************************************************
*
*  viewport.c - Viewport statement.
*
*  syntax: veiwport [vexpr from [, vexpr at [, fexpr angle]]];
*
*************************************************************************/

#include "local.h"

typedef struct tag_viewportstmtdata
{
	Expr *fromleft;
	Expr *fromright;
	Expr *at;
	Expr *angle;
} ViewportStmtData;

static void ExecAnaglyphStmt(Stmt *stmt);
static void ExecViewportStmt(Stmt *stmt);
static void DeleteViewportStmt(Stmt *stmt);

StmtProcs anaglyph_stmt_procs =
{
	TK_ANAGLYPH,
	ExecAnaglyphStmt,
	DeleteViewportStmt
};

StmtProcs viewport_stmt_procs =
{
	TK_VIEWPORT,
	ExecViewportStmt,
	DeleteViewportStmt
};

static Stmt *AllocViewportStmt(void)
{
	ViewportStmtData *vsd;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("viewport");
		return NULL;
	}
	vsd = (ViewportStmtData *)calloc(1, sizeof(ViewportStmtData));
	if (vsd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("viewport");
		return NULL;
	}
	stmt->data = (void *)vsd;
	vsd->fromleft = vsd->fromright = vsd->at = vsd->angle = NULL;
	return stmt;
}

Stmt *ParseViewportStmt(void)
{
	int token;
	Stmt *stmt = AllocViewportStmt();
	ViewportStmtData *vsd;

	if (stmt == NULL)
		return NULL;

	vsd = (ViewportStmtData *)stmt->data;
	stmt->procs = &viewport_stmt_procs;
	vsd->fromleft = ExprParse();
	if ((token = GetToken()) == OP_SEMICOLON)
		return stmt;
	if (token == OP_COMMA)
	{
		vsd->at = ExprParse();
		if ((token = GetToken()) == OP_SEMICOLON)
			return stmt;
		if (token == OP_COMMA)
		{
			vsd->angle = ExprParse();
			if((token = GetToken()) == OP_SEMICOLON)
				return stmt;
		}
	}
	ErrUnknown(token, ";", "viewport");
	DeleteStmt(stmt);
	return NULL;
}


Stmt *ParseAnaglyphStmt(void)
{
	int token;
	Stmt *stmt = AllocViewportStmt();
	ViewportStmtData *vsd;

	if (stmt == NULL)
		return NULL;

	vsd = (ViewportStmtData *)stmt->data;
	stmt->procs = &anaglyph_stmt_procs;
	vsd->fromleft = ExprParse();
	if ((token = GetToken()) == OP_COMMA)
	{
		vsd->fromright = ExprParse();
		if ((token = GetToken()) == OP_SEMICOLON)
			return stmt;
		if (token == OP_COMMA)
		{
			vsd->at = ExprParse();
			if ((token = GetToken()) == OP_SEMICOLON)
				return stmt;
			if (token == OP_COMMA)
			{
				vsd->angle = ExprParse();
				if((token = GetToken()) == OP_SEMICOLON)
					return stmt;
			}
		}
	}
	ErrUnknown(token, ";", "anaglyph");
	DeleteStmt(stmt);
	return NULL;
}

void ExecViewportStmt(Stmt *stmt)
{
	ViewportStmtData *vsd = (ViewportStmtData *)stmt->data;
	if (vsd->fromleft != NULL)
		ExprEvalVector(vsd->fromleft, &ray_setup_data->viewport.LookFrom);
	if (vsd->at != NULL)
		ExprEvalVector(vsd->at, &ray_setup_data->viewport.LookAt);
	if (vsd->angle != NULL)
		ray_setup_data->viewport.ViewAngle = ExprEvalDouble(vsd->angle);
	ray_setup_data->projection_mode = VIEWPORT_PERSPECTIVE;
}

void ExecAnaglyphStmt(Stmt *stmt)
{
	ViewportStmtData *vsd = (ViewportStmtData *)stmt->data;
	if (vsd->fromleft != NULL)
		ExprEvalVector(vsd->fromleft, &ray_setup_data->viewport.LookFrom);
	if (vsd->fromright != NULL)
		ExprEvalVector(vsd->fromright, &ray_setup_data->right_eye_lookfrom);
	if (vsd->at != NULL)
		ExprEvalVector(vsd->at, &ray_setup_data->viewport.LookAt);
	if (vsd->angle != NULL)
		ray_setup_data->viewport.ViewAngle = ExprEvalDouble(vsd->angle);
	ray_setup_data->projection_mode = VIEWPORT_ANAGLYPH;
}

void DeleteViewportStmt(Stmt *stmt)
{
	ViewportStmtData *vsd = (ViewportStmtData *)stmt->data;
	if (vsd != NULL)
	{
		ExprDelete(vsd->fromleft);
		ExprDelete(vsd->fromright);
		ExprDelete(vsd->at);
		ExprDelete(vsd->angle);
		free(vsd);
	}
}