/*************************************************************************
*
*  cone.c - Cone and cylinder statements.
*
*  syntax: cone [ vexpr_point1, vexpr_point2, fexpr_radius1,
*    fexpr_radius2 ] { modifier stmts }
*  syntax: cylinder [ vexpr_point1, vexpr_point2, fexpr_radius ]
*    { modifier stmts }
*  syntax: closed_cone [ vexpr_point1, vexpr_point2, fexpr_radius1,
*    fexpr_radius2 ] { modifier stmts }
*  syntax: closed_cylinder [ vexpr_point1, vexpr_point2, fexpr_radius ]
*    { modifier stmts }
*
*************************************************************************/

#include "local.h"

typedef struct tag_conestmtdata
{
	Expr *basept;
	Expr *endpt;
	Expr *baserad;
	Expr *endrad;
	int is_closed, is_cylinder;
	Stmt *block;
} ConeStmtData;

static void ExecConeStmt(Stmt *stmt);
static void DeleteConeStmt(Stmt *stmt);

StmtProcs cone_stmt_procs =
{
	TK_CONE,
	ExecConeStmt,
	DeleteConeStmt
};

Stmt *ParseConeStmt(int token)
{
	int nparams, prev_object_token, i;
	Param params[5];
	ConeStmtData *sd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError(token_buffer);
		return NULL;
	}
	sd = (ConeStmtData *)calloc(1, sizeof(ConeStmtData));
	if(sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError(token_buffer);
		return NULL;
	}
	stmt->procs = &cone_stmt_procs;
	stmt->data = (void *)sd;
	sd->basept = sd->endpt = sd->baserad = sd->endrad = NULL;
	sd->block = NULL;
	sd->is_cylinder = (token == TK_CYLINDER || token == TK_CLOSED_CYLINDER) ? 1 : 0;
	sd->is_closed = (token == TK_CLOSED_CONE || token == TK_CLOSED_CYLINDER) ? 1 : 0;
	prev_object_token = cur_object_token;
	cur_object_token = token;
	if(sd->is_cylinder)
		nparams = ParseParams("OEOEOEOB", "cylinder", ParseObjectDetails, params);
	else
		nparams = ParseParams("OEOEOEOEOB", "cone", ParseObjectDetails, params);
	cur_object_token = prev_object_token;
	for(i = 0; i < nparams; i++)
	{
		switch(params[i].type)
		{
			case PARAM_EXPR:
				if(sd->basept == NULL) /* First one is the base point. */
					sd->basept = params[i].data.expr;
				else if(sd->endpt == NULL) /* Second one is the end point. */
					sd->endpt = params[i].data.expr;
				else if(sd->baserad == NULL) /* Third one is the base radius. */
					sd->baserad = params[i].data.expr;
				else    /* Fourth one is the end radius. */
					sd->endrad = params[i].data.expr;
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

void ExecConeStmt(Stmt *stmt)
{
	ConeStmtData *sd = (ConeStmtData *)stmt->data;
	Vec3 basept, endpt;
	double baserad, endrad;
	Object *obj, *oldobj;
	V3Set(&basept, 0.0, 0.0, -1.0);
	V3Set(&endpt, 0.0, 0.0, 1.0);
	baserad = 1.0;
	endrad = 0.0;
	if(sd->basept != NULL)
		ExprEvalVector(sd->basept, &basept);
	if(sd->endpt != NULL)
		ExprEvalVector(sd->endpt, &endpt);
	if(sd->baserad != NULL)
		baserad = ExprEvalDouble(sd->baserad);
	if(sd->endrad != NULL)
		endrad = ExprEvalDouble(sd->endrad);
	else if(sd->is_cylinder)
		endrad = baserad;
	if((obj = Ray_MakeCone(&basept, &endpt, baserad, endrad, sd->is_closed)) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		ScnBuild_AddObject(obj);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, sd->block);
		objstack_ptr->curobj = oldobj;
	}
}

void DeleteConeStmt(Stmt *stmt)
{
	ConeStmtData *sd = (ConeStmtData *)stmt->data;
	if(sd != NULL)
	{
		ExprDelete(sd->basept);
		ExprDelete(sd->endpt);
		ExprDelete(sd->baserad);
		ExprDelete(sd->endrad);
		DeleteStatements(sd->block);
		free(sd);
	}
}
