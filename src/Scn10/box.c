/*************************************************************************
*
*  box.c - Box statement.
*
*  syntax: box [vexpr_point1, vexpr_point2] { modifier stmts }
*  syntax: box [vexpr_point1, vexpr_point2];
*
*************************************************************************/

#include "local.h"

typedef struct tag_boxstmtdata
{
	Expr *bmin;
	Expr *bmax;
	Stmt *block;
} BoxStmtData;

static void ExecBoxStmt(Stmt *stmt);
static void DeleteBoxStmt(Stmt *stmt);

StmtProcs box_stmt_procs =
{
	TK_BOX,
	ExecBoxStmt,
	DeleteBoxStmt
};

Stmt *ParseBoxStmt(void)
{
	int nparams, prev_object_token, i;
	Param params[3];
	BoxStmtData *sd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("box");
		return NULL;
	}
	sd = (BoxStmtData *)calloc(1, sizeof(BoxStmtData));
	if(sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("box");
		return NULL;
	}
	stmt->procs = &box_stmt_procs;
	stmt->data = (void *)sd;
	sd->bmin = sd->bmax = NULL;
	sd->block = NULL;
	prev_object_token = cur_object_token;
	cur_object_token = TK_BOX;
	nparams = ParseParams("OEOEOB", "box", ParseObjectDetails, params);
	cur_object_token = prev_object_token;
	for(i = 0; i < nparams; i++)
	{
		switch(params[i].type)
		{
			case PARAM_EXPR:
				if(sd->bmin == NULL) /* First one is the bmin vertex. */
					sd->bmin = params[i].data.expr;
				else                 /* Second one is the bmax vertex. */
					sd->bmax = params[i].data.expr;
				break;
			case PARAM_BLOCK:
				sd->block = params[i].data.block;
				break;
		}
	}
	if(!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

void ExecBoxStmt(Stmt *stmt)
{
	BoxStmtData *sd = (BoxStmtData *)stmt->data;
	Vec3 bmin, bmax;
	Object *obj, *oldobj;
	V3Set(&bmin, -1.0, -1.0, -1.0);
	V3Set(&bmax, 1.0, 1.0, 1.0);
	if(sd->bmin != NULL)
		ExprEvalVector(sd->bmin, &bmin);
	if(sd->bmax != NULL)
		ExprEvalVector(sd->bmax, &bmax);
	if((obj = Ray_MakeBox(&bmin, &bmax)) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		ScnBuild_AddObject(obj);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, sd->block);
		objstack_ptr->curobj = oldobj;
	}
}

void DeleteBoxStmt(Stmt *stmt)
{
	BoxStmtData *sd = (BoxStmtData *)stmt->data;
	if(sd != NULL)
	{
		ExprDelete(sd->bmin);
		ExprDelete(sd->bmax);
		DeleteStatements(sd->block);
		free(sd);
	}
}
