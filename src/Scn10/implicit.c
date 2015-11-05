/*************************************************************************
*
*  implicit.c - Implicit function f(x, y, z) statement.
*
*  syntax: implicit fexpr_fn, vexpr_bmin, vexpr_bmax [, vexpr_steps]
*    { modifier stmts }
*
*************************************************************************/

#include "local.h"

typedef struct tag_implicitstmtdata
{
	Expr *fn;
	Expr *bmin;
	Expr *bmax;
	Expr *steps;
	Stmt *block;
} ImplicitStmtData;

static void ExecImplicitStmt(Stmt *stmt);
static void DeleteImplicitStmt(Stmt *stmt);

StmtProcs implicit_stmt_procs =
{
	TK_IMPLICIT,
	ExecImplicitStmt,
	DeleteImplicitStmt
};

Stmt *ParseImplicitStmt(void)
{
	int nparams, prev_object_token, i;
	Param params[5];
	ImplicitStmtData *sd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("implicit");
		return NULL;
	}
	sd = (ImplicitStmtData *)calloc(1, sizeof(ImplicitStmtData));
	if(sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("implicit");
		return NULL;
	}
	stmt->procs = &implicit_stmt_procs;
	stmt->data = (void *)sd;
	sd->fn = sd->bmin = sd->bmax = sd->steps = NULL;
	sd->block = NULL;
	prev_object_token = cur_object_token;
	cur_object_token = TK_IMPLICIT;
	nparams = ParseParams("EEEOEOB", "implicit", ParseObjectDetails, params);
	cur_object_token = prev_object_token;
	for(i = 0; i < nparams; i++)
	{
		switch(params[i].type)
		{
			case PARAM_EXPR:
				if(sd->fn == NULL) /* First one is the function. */
					sd->fn = params[i].data.expr;
				else if(sd->bmin == NULL) /* Second one is the lo bound. */
					sd->bmin = params[i].data.expr;
				else if(sd->bmax == NULL) /* Third one is the hi bound. */
					sd->bmax = params[i].data.expr;
				else                 /* Fourth one is the xyz steps. */
					sd->steps = params[i].data.expr;
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

void ExecImplicitStmt(Stmt *stmt)
{
	ImplicitStmtData *sd = (ImplicitStmtData *)stmt->data;
	Vec3 bmin, bmax, steps;
	Object *obj, *oldobj;
	V3Set(&bmin, -1.0, -1.0, -1.0);
	V3Set(&bmax, 1.0, 1.0, 1.0);
	assert(sd->fn != NULL);
	assert(sd->bmin != NULL);
	assert(sd->bmax != NULL);
	ExprEvalVector(sd->bmin, &bmin);
	ExprEvalVector(sd->bmax, &bmax);
	if(sd->steps != NULL)
		ExprEvalVector(sd->steps, &steps);
	else
		V3Set(&steps, 32.0, 32.0, 32.0);
	if((obj = Ray_MakeImplicit(sd->fn, &bmin, &bmax, &steps)) != NULL)
	{
		/* The ray tracer now has the fn expr, set the stmt's ptr to */
		/* NULL so that DeleteImplicitStmt() will not delete it. */
		/* The ray tracer will delete it when finished. */
		sd->fn = NULL; 
		obj->surface = Ray_ShareSurface(default_surface);
		ScnBuild_AddObject(obj);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, sd->block);
		objstack_ptr->curobj = oldobj;
	}
}

void DeleteImplicitStmt(Stmt *stmt)
{
	ImplicitStmtData *sd = (ImplicitStmtData *)stmt->data;
	if(sd != NULL)
	{
		ExprDelete(sd->fn);
		ExprDelete(sd->bmax);
		ExprDelete(sd->bmin);
		ExprDelete(sd->steps);
		DeleteStatements(sd->block);
		free(sd);
	}
}
