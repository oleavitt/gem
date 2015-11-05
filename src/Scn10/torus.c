/*************************************************************************
*
*  torus.c - Torus statement.
*
*  syntax: torus [ vexpr_center, fexpr_majorrad, fexpr_minorrad ]
*    { modifier stmts }
*
*************************************************************************/

#include "local.h"

typedef struct tag_torusstmtdata
{
	Expr *center;
	Expr *majrad;
	Expr *minrad;
	Stmt *block;
} TorusStmtData;

static void ExecTorusStmt(Stmt *stmt);
static void DeleteTorusStmt(Stmt *stmt);

StmtProcs torus_stmt_procs =
{
	TK_TORUS,
	ExecTorusStmt,
	DeleteTorusStmt
};

Stmt *ParseTorusStmt(void)
{
	int nparams, prev_object_token, i;
	Param params[4];
	TorusStmtData *sd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("torus");
		return NULL;
	}
	sd = (TorusStmtData *)calloc(1, sizeof(TorusStmtData));
	if(sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("torus");
		return NULL;
	}
	stmt->procs = &torus_stmt_procs;
	stmt->data = (void *)sd;
	sd->center = sd->majrad = sd->minrad = NULL;
	sd->block = NULL;
	prev_object_token = cur_object_token;
	cur_object_token = TK_TORUS;
	nparams = ParseParams("OEOEOEOB", "torus", ParseObjectDetails, params);
	cur_object_token = prev_object_token;
	for(i = 0; i < nparams; i++)
	{
		switch(params[i].type)
		{
			case PARAM_EXPR:
				if(sd->center == NULL) /* First one is the center point. */
					sd->center = params[i].data.expr;
				else if(sd->majrad == NULL) /* Second one is the major radius. */
					sd->majrad = params[i].data.expr;
				else    /* Third one is the minor radius. */
					sd->minrad = params[i].data.expr;
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

void ExecTorusStmt(Stmt *stmt)
{
	TorusStmtData *sd = (TorusStmtData *)stmt->data;
	Vec3 center;
	double majrad, minrad;
	Object *obj, *oldobj;
	V3Set(&center, 0.0, 0.0, 0.0);
	majrad = 0.5;
	minrad = 0.5;
	if(sd->center != NULL)
		ExprEvalVector(sd->center, &center);
	if(sd->majrad != NULL)
		majrad = ExprEvalDouble(sd->majrad);
	if(sd->minrad != NULL)
		minrad = ExprEvalDouble(sd->minrad);
	if((obj = Ray_MakeTorus(&center, majrad, minrad)) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		ScnBuild_AddObject(obj);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, sd->block);
		objstack_ptr->curobj = oldobj;
	}
}

void DeleteTorusStmt(Stmt *stmt)
{
	TorusStmtData *sd = (TorusStmtData *)stmt->data;
	if(sd != NULL)
	{
		ExprDelete(sd->center);
		ExprDelete(sd->majrad);
		ExprDelete(sd->minrad);
		DeleteStatements(sd->block);
		free(sd);
	}
}
