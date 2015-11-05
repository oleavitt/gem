/*************************************************************************
*
*  sphere.c - Sphere statement.
*
*  syntax: sphere [vexpr loc [, fexpr radius]] { modifier stmts }
*  syntax: sphere [vexpr loc [, fexpr radius]];
*
*************************************************************************/

#include "local.h"

typedef struct tag_spherestmtdata
{
	Expr *loc;
	Expr *rad;
	Stmt *block;
} SphereStmtData;

static void ExecSphereStmt(Stmt *stmt);
static void DeleteSphereStmt(Stmt *stmt);

StmtProcs sphere_stmt_procs =
{
	TK_SPHERE,
	ExecSphereStmt,
	DeleteSphereStmt
};

Stmt *ParseSphereStmt(void)
{
	int nparams, prev_object_token, i;
	Param params[3];
	SphereStmtData *sd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("sphere");
		return NULL;
	}
	sd = (SphereStmtData *)calloc(1, sizeof(SphereStmtData));
	if(sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("sphere");
		return NULL;
	}
	stmt->procs = &sphere_stmt_procs;
	stmt->data = (void *)sd;
	sd->loc = sd->rad = NULL;
	sd->block = NULL;
	prev_object_token = cur_object_token;
	cur_object_token = TK_SPHERE;
	nparams = ParseParams("OEOEOB", "sphere", ParseObjectDetails, params);
	cur_object_token = prev_object_token;
	for(i = 0; i < nparams; i++)
	{
		switch(params[i].type)
		{
			case PARAM_EXPR:
				if(sd->loc == NULL) /* First one is the location vector. */
					sd->loc = params[i].data.expr;
				else                 /* Second one is the radius. */
					sd->rad = params[i].data.expr;
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

void ExecSphereStmt(Stmt *stmt)
{
	SphereStmtData *sd = (SphereStmtData *)stmt->data;
	Vec3 loc;
	double rad;
	Object *obj, *oldobj;
	V3Set(&loc, 0, 0, 0); rad = 1.0;
	if(sd->loc != NULL)
		ExprEvalVector(sd->loc, &loc);
	if(sd->rad != NULL)
		rad = ExprEvalDouble(sd->rad);
	if((obj = Ray_MakeSphere(&loc, rad)) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		ScnBuild_AddObject(obj);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, sd->block);
		objstack_ptr->curobj = oldobj;
	}
}

void DeleteSphereStmt(Stmt *stmt)
{
	SphereStmtData *sd = (SphereStmtData *)stmt->data;
	if(sd != NULL)
	{
		ExprDelete(sd->loc);
		ExprDelete(sd->rad);
		DeleteStatements(sd->block);
		free(sd);
	}
}