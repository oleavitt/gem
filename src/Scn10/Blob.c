/*************************************************************************
*
*  blob.c - Blob statement.
*
*************************************************************************/

#include "local.h"

typedef struct tag_blobstmtdata
{
	Expr *threshold;
	Stmt *block;
} BlobStmtData;

typedef struct tag_blobspherestmtdata
{
	Expr *loc;
	Expr *rad;
	Expr *field;
} BlobSphereStmtData;

typedef struct tag_blobcylstmtdata
{
	Expr *loc;
	Expr *endpt;
	Expr *rad;
	Expr *field;
} BlobCylStmtData;

typedef struct tag_blobplanestmtdata
{
	Expr *loc;
	Expr *dir;
	Expr *dist;
	Expr *field;
} BlobPlaneStmtData;

static void ExecBlobStmt(Stmt *stmt);
static void DeleteBlobStmt(Stmt *stmt);
static void ExecBlobSphereStmt(Stmt *stmt);
static void DeleteBlobSphereStmt(Stmt *stmt);
static void ExecBlobCylStmt(Stmt *stmt);
static void DeleteBlobCylStmt(Stmt *stmt);
static void ExecBlobPlaneStmt(Stmt *stmt);
static void DeleteBlobPlaneStmt(Stmt *stmt);

static StmtProcs blob_stmt_procs =
{
	TK_BLOB,
	ExecBlobStmt,
	DeleteBlobStmt
};

static StmtProcs blobsphere_stmt_procs =
{
	BLOB_SPHERE,
	ExecBlobSphereStmt,
	DeleteBlobSphereStmt
};

static StmtProcs blobcyl_stmt_procs =
{
	BLOB_CYLINDER,
	ExecBlobCylStmt,
	DeleteBlobCylStmt
};

static StmtProcs blobplane_stmt_procs =
{
	BLOB_PLANE,
	ExecBlobPlaneStmt,
	DeleteBlobPlaneStmt
};

Stmt *ParseBlobStmt(void)
{
	int nparams, prev_object_token, i;
	Param params[2];
	BlobStmtData *bsd;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("blob");
		return NULL;
	}
	bsd = (BlobStmtData *)calloc(1, sizeof(BlobStmtData));
	if (bsd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("blob");
		return NULL;
	}
	stmt->procs = &blob_stmt_procs;
	stmt->data = (void *)bsd;
	bsd->threshold = NULL;
	bsd->block = NULL;
	prev_object_token = cur_object_token;
	cur_object_token = TK_BLOB;
	nparams = ParseParams("EB", "blob", ParseObjectDetails, params);
	cur_object_token = prev_object_token;
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				bsd->threshold = params[i].data.expr;
				break;
			case PARAM_BLOCK:
				bsd->block = params[i].data.block;
				break;
		}
	}
	if (!error_count)
		return stmt;

	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseBlobSphereStmt(void)
{
	int nparams;
	Param params[3];
	BlobSphereStmtData *sd;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("blob");
		return NULL;
	}
	sd = (BlobSphereStmtData *)calloc(1, sizeof(BlobSphereStmtData));
	if (sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("blob");
		return NULL;
	}
	stmt->procs = &blobsphere_stmt_procs;
	stmt->data = (void *)sd;
	sd->loc = NULL;
	sd->rad = NULL;
	sd->field = NULL;
	nparams = ParseParams("EEOE;", "blob", NULL, params);
	if (nparams > 0)
	{
		sd->loc = params[0].data.expr;
		if (nparams > 1)
		{
			sd->rad = params[1].data.expr;
			if(nparams > 2)
				sd->field = params[2].data.expr;
		}
	}
	if (!error_count)
		return stmt;

	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseBlobCylinderStmt(void)
{
	int nparams;
	Param params[4];
	BlobCylStmtData *sd;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("blob");
		return NULL;
	}
	sd = (BlobCylStmtData *)calloc(1, sizeof(BlobCylStmtData));
	if (sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("blob");
		return NULL;
	}
	stmt->procs = &blobcyl_stmt_procs;
	stmt->data = (void *)sd;
	sd->loc = NULL;
	sd->endpt = NULL;
	sd->rad = NULL;
	sd->field = NULL;
	nparams = ParseParams("EEEOE;", "blob", NULL, params);
	if (nparams > 0)
	{
		sd->loc = params[0].data.expr;
		if (nparams > 1)
		{
			sd->endpt = params[1].data.expr;
			if (nparams > 2)
			{
				sd->rad = params[2].data.expr;
				if(nparams > 3)
					sd->field = params[3].data.expr;
			}
		}
	}

	if (!error_count)
		return stmt;

	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseBlobPlaneStmt(void)
{
	int nparams;
	Param params[4];
	BlobPlaneStmtData *sd;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("blob");
		return NULL;
	}
	sd = (BlobPlaneStmtData *)calloc(1, sizeof(BlobPlaneStmtData));
	if (sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("blob");
		return NULL;
	}
	stmt->procs = &blobplane_stmt_procs;
	stmt->data = (void *)sd;
	sd->loc = NULL;
	sd->dir = NULL;
	sd->dist = NULL;
	sd->field = NULL;
	nparams = ParseParams("EEEOE;", "blob", NULL, params);
	if (nparams > 0)
	{
		sd->loc = params[0].data.expr;
		if (nparams > 1)
		{
			sd->dir = params[1].data.expr;
			if (nparams > 2)
			{
				sd->dist = params[2].data.expr;
				if(nparams > 3)
					sd->field = params[3].data.expr;
			}
		}
	}

	if (!error_count)
		return stmt;

	DeleteStmt(stmt);
	return NULL;
}

void ExecBlobStmt(Stmt *stmt)
{
	BlobStmtData *bsd = (BlobStmtData *)stmt->data;
	double threshold;
	Object *obj, *oldobj;
	threshold = ExprEvalDouble(bsd->threshold);
	if ((obj = Ray_MakeBlob(threshold)) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, bsd->block);
		if (Ray_BlobFinish(obj))
			ScnBuild_AddObject(obj);
		else
			Ray_DeleteObject(obj);
		objstack_ptr->curobj = oldobj;
	}
}

void ExecBlobSphereStmt(Stmt *stmt)
{
	BlobSphereStmtData *sd = (BlobSphereStmtData *)stmt->data;
	double rad, field;
	Vec3 loc;
	rad = 1.0;
	field = 1.0;
	assert(sd->loc != NULL);
	ExprEvalVector(sd->loc, &loc);
	if (sd->rad != NULL)
		rad = ExprEvalDouble(sd->rad);
	if (sd->field != NULL)
		field = ExprEvalDouble(sd->field);
	Ray_BlobAddSphere(objstack_ptr->curobj, &loc, rad, field);
}

void ExecBlobCylStmt(Stmt *stmt)
{
	BlobCylStmtData *sd = (BlobCylStmtData *)stmt->data;
	double rad, field;
	Vec3 loc, endpt;

	rad = 1.0;
	field = 1.0;
	assert(sd->loc != NULL);
	assert(sd->endpt != NULL);
	ExprEvalVector(sd->loc, &loc);
	ExprEvalVector(sd->endpt, &endpt);
	if (sd->rad != NULL)
		rad = ExprEvalDouble(sd->rad);
	if (sd->field != NULL)
		field = ExprEvalDouble(sd->field);
	Ray_BlobAddCylinder(objstack_ptr->curobj, &loc, &endpt, rad, field);
}

void ExecBlobPlaneStmt(Stmt *stmt)
{
	BlobPlaneStmtData *sd = (BlobPlaneStmtData *)stmt->data;
	double dist, field;
	Vec3 loc, dir;

	dist = 1.0;
	field = 1.0;
	assert(sd->loc != NULL);
	assert(sd->dir != NULL);
	ExprEvalVector(sd->loc, &loc);
	ExprEvalVector(sd->dir, &dir);
	if (sd->dist != NULL)
		dist = ExprEvalDouble(sd->dist);
	if (sd->field != NULL)
		field = ExprEvalDouble(sd->field);
	Ray_BlobAddPlane(objstack_ptr->curobj, &loc, &dir, dist, field);
}

void DeleteBlobStmt(Stmt *stmt)
{
	BlobStmtData *bsd = (BlobStmtData *)stmt->data;
	if (bsd != NULL)
	{
		ExprDelete(bsd->threshold);
		DeleteStatements(bsd->block);
		free(bsd);
	}
}

void DeleteBlobSphereStmt(Stmt *stmt)
{
	BlobSphereStmtData *sd = (BlobSphereStmtData *)stmt->data;
	if (sd != NULL)
	{
		ExprDelete(sd->loc);
		ExprDelete(sd->rad);
		ExprDelete(sd->field);
		free(sd);
	}
}

void DeleteBlobCylStmt(Stmt *stmt)
{
	BlobCylStmtData *sd = (BlobCylStmtData *)stmt->data;
	if (sd != NULL)
	{
		ExprDelete(sd->loc);
		ExprDelete(sd->endpt);
		ExprDelete(sd->rad);
		ExprDelete(sd->field);
		free(sd);
	}
}

void DeleteBlobPlaneStmt(Stmt *stmt)
{
	BlobPlaneStmtData *sd = (BlobPlaneStmtData *)stmt->data;
	if (sd != NULL)
	{
		ExprDelete(sd->loc);
		ExprDelete(sd->dir);
		ExprDelete(sd->dist);
		ExprDelete(sd->field);
		free(sd);
	}
}

