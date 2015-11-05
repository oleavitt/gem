/*************************************************************************
*
*  polymesh.c - Polymesh statement.
*
*************************************************************************/

#include "local.h"
static void ExecPolymeshStmt(Stmt *stmt);
static void DeletePolymeshStmt(Stmt *stmt);
static void ExecPolymeshVertexStmt(Stmt *stmt);
static void DeletePolymeshVertexStmt(Stmt *stmt);
static void ExecPolymeshExtrudeStmt(Stmt *stmt);
static void DeletePolymeshExtrudeStmt(Stmt *stmt);

static StmtProcs polymesh_stmt_procs =
{
	TK_POLYMESH,
	ExecPolymeshStmt,
	DeletePolymeshStmt
};

static StmtProcs polymeshvertex_stmt_procs =
{
	TK_VERTEX,
	ExecPolymeshVertexStmt,
	DeletePolymeshVertexStmt
};

Stmt *ParsePolymeshStmt(void)
{
	int prev_object_token;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("polymesh");
		return NULL;
	}
	stmt->procs = &polymesh_stmt_procs;
	prev_object_token = cur_object_token;
	cur_object_token = TK_POLYMESH;
	stmt->data = (void *)ParseBlock("polymesh", ParseObjectDetails);
	cur_object_token = prev_object_token;
	if(!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParsePolymeshVertexStmt(void)
{
	int nparams;
	Param params;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("vertex");
		return NULL;
	}
	stmt->procs = &polymeshvertex_stmt_procs;
	nparams = ParseParams("E;", "vertex", NULL, &params);
	assert(nparams == 1);
	stmt->data = (void *)params.data.expr;
	if(!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

void ExecPolymeshStmt(Stmt *stmt)
{
	Object *obj, *oldobj;
	assert(stmt->data != NULL);
	if ((obj = Ray_BeginMesh()) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, (Stmt *)stmt->data);
		if (Ray_FinishMesh(obj))
			ScnBuild_AddObject(obj);
		else
			Ray_DeleteObject(obj);
		objstack_ptr->curobj = oldobj;
	}
}

void ExecPolymeshVertexStmt(Stmt *stmt)
{
	Vec3 pt;
	MeshVertex *v;
	assert(stmt->data != NULL);
// TODO:	ExprEvalVector((Expr *)stmt->data, &pt);
	v = Ray_NewMeshVertex(&pt);
	Ray_AddMeshVertex(objstack_ptr->curobj->data.mesh, &v);
}

void DeletePolymeshStmt(Stmt *stmt)
{
// TODO:	DeleteStatements((Stmt *)stmt->data);
}

void DeletePolymeshVertexStmt(Stmt *stmt)
{
// TODO:	ExprDelete((Expr *)stmt->data);
}
