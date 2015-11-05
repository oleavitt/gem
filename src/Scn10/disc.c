/*************************************************************************
*
*  disc.c - Disc statement.
*
*  syntax: disc [ vexpr_center, vexpr_normal, fexpr_outer_radius,
*    fexpr_inner_radius ] { modifier stmts }
*
*************************************************************************/

#include "local.h"

typedef struct tag_discstmtdata
{
	Expr *center;
	Expr *normal;
	Expr *outerrad;
	Expr *innerrad;
	Stmt *block;
} DiscStmtData;

static void ExecDiscStmt(Stmt *stmt);
static void DeleteDiscStmt(Stmt *stmt);

StmtProcs disc_stmt_procs =
{
	TK_DISC,
	ExecDiscStmt,
	DeleteDiscStmt
};

Stmt *ParseDiscStmt(void)
{
	int nparams, prev_object_token, i;
	Param params[5];
	DiscStmtData *sd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("disc");
		return NULL;
	}
	sd = (DiscStmtData *)calloc(1, sizeof(DiscStmtData));
	if(sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("disc");
		return NULL;
	}
	stmt->procs = &disc_stmt_procs;
	stmt->data = (void *)sd;
	sd->center = sd->normal = sd->outerrad = sd->innerrad = NULL;
	sd->block = NULL;
	prev_object_token = cur_object_token;
	cur_object_token = TK_DISC;
	nparams = ParseParams("OEOEOEOEOB", "disc", ParseObjectDetails, params);
	cur_object_token = prev_object_token;
	for(i = 0; i < nparams; i++)
	{
		switch(params[i].type)
		{
			case PARAM_EXPR:
				if(sd->center == NULL) /* First one is the center point. */
					sd->center = params[i].data.expr;
				else if(sd->normal == NULL) /* Second one is the plane normal. */
					sd->normal = params[i].data.expr;
				else if(sd->outerrad == NULL) /* Third one is the outside radius. */
					sd->outerrad = params[i].data.expr;
				else    /* Fourth one is the inside radius. */
					sd->innerrad = params[i].data.expr;
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

void ExecDiscStmt(Stmt *stmt)
{
	DiscStmtData *sd = (DiscStmtData *)stmt->data;
	Vec3 center, normal;
	double outerrad, innerrad;
	Object *obj, *oldobj;
	V3Set(&center, 0.0, 0.0, 0.0);
	V3Set(&normal, 0.0, 0.0, 1.0);
	outerrad = 1.0;
	innerrad = 0.0;
	if(sd->center != NULL)
		ExprEvalVector(sd->center, &center);
	if(sd->normal != NULL)
		ExprEvalVector(sd->normal, &normal);
	if(sd->outerrad != NULL)
		outerrad = ExprEvalDouble(sd->outerrad);
	if(sd->innerrad != NULL)
		innerrad = ExprEvalDouble(sd->innerrad);
	if((obj = Ray_MakeDisc(&center, &normal, outerrad, innerrad)) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		ScnBuild_AddObject(obj);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, sd->block);
		objstack_ptr->curobj = oldobj;
	}
}

void DeleteDiscStmt(Stmt *stmt)
{
	DiscStmtData *sd = (DiscStmtData *)stmt->data;
	if(sd != NULL)
	{
		ExprDelete(sd->center);
		ExprDelete(sd->normal);
		ExprDelete(sd->outerrad);
		ExprDelete(sd->innerrad);
		DeleteStatements(sd->block);
		free(sd);
	}
}
