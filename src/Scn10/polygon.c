/*************************************************************************
*
*  polygon.c - Polygon statement.
*
*************************************************************************/

#include "local.h"

static void ExecPolygonStmt(Stmt *stmt);
static void DeletePolygonStmt(Stmt *stmt);
static void ExecPolygonVertexStmt(Stmt *stmt);
static void DeletePolygonVertexStmt(Stmt *stmt);

static StmtProcs polygon_stmt_procs =
{
	TK_POLYGON,
	ExecPolygonStmt,
	DeletePolygonStmt
};

static StmtProcs polygonvertex_stmt_procs =
{
	TK_VERTEX,
	ExecPolygonVertexStmt,
	DeletePolygonVertexStmt
};

Stmt *ParsePolygonStmt(void)
{
	int prev_object_token;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("polygon");
		return NULL;
	}
	stmt->procs = &polygon_stmt_procs;
	prev_object_token = cur_object_token;
	cur_object_token = TK_POLYGON;
	stmt->data = (void *)ParseBlock("polygon", ParseObjectDetails);
	cur_object_token = prev_object_token;
	if(!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParsePolygonVertexStmt(void)
{
	int nparams;
	Param params;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("vertex");
		return NULL;
	}
	stmt->procs = &polygonvertex_stmt_procs;
	nparams = ParseParams("E;", "vertex", NULL, &params);
	assert(nparams == 1);
	stmt->data = (void *)params.data.expr;
	if(!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

void ExecPolygonStmt(Stmt *stmt)
{
	Object *obj, *oldobj;
	assert(stmt->data != NULL);
	if((obj = Ray_MakePolygon(NULL, 0)) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, (Stmt *)stmt->data);
		if(Ray_PolygonFinish(obj))
			ScnBuild_AddObject(obj);
		else
			Ray_DeleteObject(obj);
		objstack_ptr->curobj = oldobj;
	}
}

void ExecPolygonVertexStmt(Stmt *stmt)
{
	Vec3 pt;
	assert(stmt->data != NULL);
	ExprEvalVector((Expr *)stmt->data, &pt);
	Ray_PolygonAddVertex(objstack_ptr->curobj, &pt);
}

void DeletePolygonStmt(Stmt *stmt)
{
	DeleteStatements((Stmt *)stmt->data);
}

void DeletePolygonVertexStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}

