/*************************************************************************
*
*  triangle.c - Triangle statement.
*
*  syntax: triangle vexpr_pt1, vexpr_pt2, vexpr_pt3 [, vexpr_norm1,
*    vexpr_norm2, vexpr_norm3 [, fexpr_u, fexpr_v ]] { modifier stmts }
*
*************************************************************************/

#include "local.h"

typedef struct tag_trianglestmtdata
{
	Expr *pt1, *pt2, *pt3;
	Expr *n1, *n2, *n3;
	Expr *u, *v;
	Stmt *block;
} TriangleStmtData;

static void ExecTriangleStmt(Stmt *stmt);
static void DeleteTriangleStmt(Stmt *stmt);

StmtProcs triangle_stmt_procs =
{
	TK_TRIANGLE,
	ExecTriangleStmt,
	DeleteTriangleStmt
};

Stmt *ParseTriangleStmt(void)
{
	int nparams, prev_object_token, i;
	Param params[9];
	TriangleStmtData *sd;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("triangle");
		return NULL;
	}
	sd = (TriangleStmtData *)calloc(1, sizeof(TriangleStmtData));
	if (sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("triangle");
		return NULL;
	}
	stmt->procs = &triangle_stmt_procs;
	stmt->data = (void *)sd;
	sd->pt1 = sd->pt2 = sd->pt3 = NULL;
	sd->n1 = sd->n2 = sd->n3 = NULL;
	sd->u = sd->v = NULL;
	sd->block = NULL;
	prev_object_token = cur_object_token;
	cur_object_token = TK_TRIANGLE;
	nparams = ParseParams("EEEOEEEOEEB", "triangle", ParseObjectDetails, params);
	cur_object_token = prev_object_token;
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (sd->pt1 == NULL) /* First three are the vertices. */
					sd->pt1 = params[i].data.expr;
				else if (sd->pt2 == NULL)
					sd->pt2 = params[i].data.expr;
				else if (sd->pt3 == NULL)
					sd->pt3 = params[i].data.expr;
				else if (sd->n1 == NULL) /* Next three are the normals. */
					sd->n1 = params[i].data.expr;
				else if (sd->n2 == NULL)
					sd->n2 = params[i].data.expr;
				else if (sd->n3 == NULL)
					sd->n3 = params[i].data.expr;
				else if (sd->u == NULL) /* Last two are the UV map points. */
					sd->u = params[i].data.expr;
				else
					sd->v = params[i].data.expr;
				break;
			case PARAM_BLOCK:
				sd->block = params[i].data.block;
				break;
		}
	}
	if (!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

void ExecTriangleStmt(Stmt *stmt)
{
	TriangleStmtData *sd = (TriangleStmtData *)stmt->data;
	float pts[9], norms[9], uv[6];
	float *ppts, *pnorms, *puv;
	Vec3 v;
	Object *obj, *oldobj;
	ppts = pnorms = puv = NULL;
	assert(sd->pt1 != NULL);
	assert(sd->pt2 != NULL);
	assert(sd->pt3 != NULL);
	ExprEvalVector(sd->pt1, &v);
	pts[0] = (float)v.x; pts[1] = (float)v.y; pts[2] = (float)v.z;
	ExprEvalVector(sd->pt2, &v);
	pts[3] = (float)v.x; pts[4] = (float)v.y; pts[5] = (float)v.z;
	ExprEvalVector(sd->pt3, &v);
	pts[6] = (float)v.x; pts[7] = (float)v.y; pts[8] = (float)v.z;
	ppts = pts;
	if (sd->n1 != NULL)
	{
		assert(sd->n2 != NULL);
		assert(sd->n3 != NULL);
		ExprEvalVector(sd->n1, &v);
		norms[0] = (float)v.x; norms[1] = (float)v.y; norms[2] = (float)v.z;
		ExprEvalVector(sd->n2, &v);
		norms[3] = (float)v.x; norms[4] = (float)v.y; norms[5] = (float)v.z;
		ExprEvalVector(sd->n3, &v);
		norms[6] = (float)v.x; norms[7] = (float)v.y; norms[8] = (float)v.z;
		pnorms = norms;
	}
	if (sd->u != NULL)
	{
		assert(sd->v != NULL);
		ExprEvalVector(sd->u, &v);
		uv[0] = (float)v.x; uv[2] = (float)v.y; uv[4] = (float)v.z;
		ExprEvalVector(sd->v, &v);
		uv[1] = (float)v.x; uv[3] = (float)v.y; uv[5] = (float)v.z;
		puv = uv;
	}
	if ((obj = Ray_MakeTriangle(ppts, pnorms, puv)) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		ScnBuild_AddObject(obj);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, sd->block);
		objstack_ptr->curobj = oldobj;
	}
}

void DeleteTriangleStmt(Stmt *stmt)
{
	TriangleStmtData *sd = (TriangleStmtData *)stmt->data;
	if (sd != NULL)
	{
		ExprDelete(sd->pt1);
		ExprDelete(sd->pt2);
		ExprDelete(sd->pt3);
		ExprDelete(sd->n1);
		ExprDelete(sd->n2);
		ExprDelete(sd->n3);
		ExprDelete(sd->u);
		ExprDelete(sd->v);
		DeleteStatements(sd->block);
		free(sd);
	}
}
