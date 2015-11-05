/*************************************************************************
*
*  polymesh.c - Polymesh statement.
*
*************************************************************************/

#include "local.h"

/* polymesh */
static void ExecPolymeshStmt(Stmt *stmt);
static void DeletePolymeshStmt(Stmt *stmt);

static StmtProcs polymesh_stmt_procs =
{
	TK_POLYMESH,
	ExecPolymeshStmt,
	DeletePolymeshStmt
};



/* vertex */
static void ExecPolymeshVertexStmt(Stmt *stmt);
static void DeletePolymeshVertexStmt(Stmt *stmt);

static StmtProcs polymeshvertex_stmt_procs =
{
	TK_VERTEX,
	ExecPolymeshVertexStmt,
	DeletePolymeshVertexStmt
};

typedef struct tag_vertexstmtdata
{
	Expr *point;
	Expr *normal;
} VertexStmtData;



/* extrude */
static int ParseExtrudeDetails(Stmt **stmt);
static void ExecPolymeshExtrudeStmt(Stmt *stmt);
static void DeletePolymeshExtrudeStmt(Stmt *stmt);

static StmtProcs polymeshextrude_stmt_procs =
{
	TK_EXTRUDE,
	ExecPolymeshExtrudeStmt,
	DeletePolymeshExtrudeStmt
};



/* rel_step */
static Stmt *ParseRelStepStmt(void);
static void ExecRelStepStmt(Stmt *stmt);
static void DeleteRelStepStmt(Stmt *stmt);

static StmtProcs rel_step_stmt_procs =
{
	TK_REL_STEP,
	ExecRelStepStmt,
	DeleteRelStepStmt
};



/* rot_step */
static Stmt *ParseRotStepStmt(void);
static void ExecRotStepStmt(Stmt *stmt);
static void DeleteRotStepStmt(Stmt *stmt);

static StmtProcs rot_step_stmt_procs =
{
	TK_ROT_STEP,
	ExecRotStepStmt,
	DeleteRotStepStmt
};

typedef struct tag_rotstepstmtdata
{
	Expr *center;
	Expr *axis;
	Expr *angle;
	Expr *steps;
} RotStepStmtData;



/* path */
static int ParsePathDetails(Stmt **stmt);
static void ExecPolymeshPathStmt(Stmt *stmt);
static void DeletePolymeshPathStmt(Stmt *stmt);

static StmtProcs polymeshpath_stmt_procs =
{
	TK_PATH,
	ExecPolymeshPathStmt,
	DeletePolymeshPathStmt
};



/* point */
static Stmt *ParsePointStmt(void);
static void ExecPointStmt(Stmt *stmt);
static void DeletePointStmt(Stmt *stmt);

static StmtProcs point_stmt_procs =
{
	TK_POINT,
	ExecPointStmt,
	DeletePointStmt
};



// Ptr to path points array.
static Vec3 *s_ptlist = NULL;
static int s_npoints = 0;
static int s_npoints_alloced = 0;
#define NPTS_PER_CHUNK	16

static int s_nsteps = 0;
static int s_nvertices = 0;


Stmt *ParsePolymeshStmt(int smooth)
{
	int prev_object_token;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("polymesh");
		return NULL;
	}
	stmt->procs = &polymesh_stmt_procs;
	prev_object_token = cur_object_token;
	cur_object_token = TK_POLYMESH;
	stmt->int_data = smooth;
	stmt->data = (void *)ParseBlock("polymesh", ParseObjectDetails);
	cur_object_token = prev_object_token;
	if (!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

// vertex <x, y, z> [, <nx, ny, nz>];
// vertex point [, normal];
Stmt *ParsePolymeshVertexStmt(void)
{
	int nparams;
	Param params[2];
	VertexStmtData *sd;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("vertex");
		return NULL;
	}

	sd = (VertexStmtData *)calloc(1, sizeof(VertexStmtData));
	if (sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("vertex");
		return NULL;
	}
	sd->point = sd->normal = NULL;

	stmt->data = sd;
	stmt->procs = &polymeshvertex_stmt_procs;

	nparams = ParseParams("EOE;", "vertex", NULL, params);
	if (nparams > 0)
	{
		sd->point = params[0].data.expr;
		if (nparams > 1)
			sd->normal = params[1].data.expr;
	}

	if (!error_count)
		return stmt;

	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParsePolymeshExtrudeStmt(void)
{
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("extrude");
		return NULL;
	}

	stmt->procs = &polymeshextrude_stmt_procs;
	stmt->data = (void *)ParseBlock("extrude", ParseExtrudeDetails);

	if (!error_count)
		return stmt;

	DeleteStmt(stmt);
	return NULL;
}

int ParseExtrudeDetails(Stmt **stmt)
{
	int token = GetToken();
	*stmt = NULL;

	switch (token)
	{
		case TK_REL_STEP:
			*stmt = ParseRelStepStmt();
			break;

		case TK_ROT_STEP:
			*stmt = ParseRotStepStmt();
			break;

		default:
			return token;
	}
	return TK_NULL;
}

Stmt *ParseRelStepStmt(void)
{
	int nparams;
	Param params;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("rel_step");
		return NULL;
	}

	stmt->procs = &rel_step_stmt_procs;

	nparams = ParseParams("E;", "rel_step", NULL, &params);
	assert(nparams == 1);
	stmt->data = (void *)params.data.expr;

	if (!error_count && (stmt->data != NULL))
		return stmt;

	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseRotStepStmt(void)
{
	int nparams;
	Param params[4];
	RotStepStmtData *sd;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("rot_step");
		return NULL;
	}

	sd = (RotStepStmtData *)calloc(1, sizeof(RotStepStmtData));
	if (sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("rot_step");
		return NULL;
	}
	sd->center = sd->axis = sd->angle = sd->steps = NULL;

	stmt->procs = &rot_step_stmt_procs;
	stmt->data = (void *)sd;

	nparams = ParseParams("EEEOE;", "rot_step", NULL, params);
	if (nparams > 0)
	{
		/* First one is the center point for rotation. */
		sd->center = params[0].data.expr;
		if (nparams > 1)
		{
			/* Second one is the rotation axis. */
			sd->axis = params[1].data.expr;
			if (nparams > 2)
			{
				/* Third one is the angle in degrees. */
				sd->angle = params[2].data.expr;
				if (nparams > 3)
				{
					/* Fourth one is the number of steps to make the turn. */
					sd->steps = params[3].data.expr;
				}
			}
		}
	}
	
	if (!error_count)
		return stmt;
	
	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParsePolymeshPathStmt(int smooth)
{
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("path");
		return NULL;
	}

	smooth; // TODO:

	stmt->procs = &polymeshpath_stmt_procs;
	stmt->data = (void *)ParseBlock("path", ParsePathDetails);

	if (!error_count)
		return stmt;

	DeleteStmt(stmt);
	return NULL;
}

int ParsePathDetails(Stmt **stmt)
{
	int token = GetToken();
	*stmt = NULL;

	switch (token)
	{
		case TK_POINT:
			*stmt = ParsePointStmt();
			break;

		default:
			return token;
	}

	return TK_NULL;
}

Stmt *ParsePointStmt(void)
{
	int nparams;
	Param params;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("point");
		return NULL;
	}

	stmt->procs = &point_stmt_procs;

	nparams = ParseParams("E;", "point", NULL, &params);
	assert(nparams == 1);
	stmt->data = (void *)params.data.expr;

	if (!error_count && (stmt->data != NULL))
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
		if (stmt->int_data)
			obj->flags |= OBJ_FLAG_SMOOTH;
		obj->surface = Ray_ShareSurface(default_surface);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;

		s_nsteps = 0;
		s_nvertices = 0;
		ExecBlock(stmt, (Stmt *)stmt->data);

		// Finish the object and add it to the renderer.
		if ((obj = Ray_FinishMesh()) != NULL)
			ScnBuild_AddObject(obj);

		// Clean up the stack.
		objstack_ptr->curobj = oldobj;
	}
}

void ExecPolymeshVertexStmt(Stmt *stmt)
{
	VertexStmtData *sd = (VertexStmtData *)stmt->data;
	Vec3 p;
	MeshVertex *v;

	assert(sd != NULL);

	/* Vertex point */
	ExprEvalVector(sd->point, &p);
	v = Ray_NewMeshVertex(&p);
	if (sd->normal != NULL)
	{
		/* Vertex normal */
		ExprEvalVector(sd->normal, &p);
		v->nx = (float)p.x;
		v->ny = (float)p.y;
		v->nz = (float)p.z;
		v->flags |= MESH_VERTEX_HAS_NORMAL;
	}

	if (Ray_AddMeshVertex(objstack_ptr->curobj->data.mesh, v))
		s_nvertices++;
}

void ExecPolymeshExtrudeStmt(Stmt *stmt)
{
	ExecBlock(stmt, (Stmt *)stmt->data);
}

void ExecRelStepStmt(Stmt *stmt)
{
	Vec3 pt, offset;
	MeshVertex *v, *vbase, **ppvprev, **ppvcur;
	MeshTri *tri;
	MeshData *mesh = objstack_ptr->curobj->data.mesh;
	int i;
	int startpt = mesh->nvertices - s_nvertices;
	int endpt = mesh->nvertices;

	assert(stmt->data != NULL);
	ExprEvalVector((Expr *)stmt->data, &offset);

	/* Make copies of the base points and transform them... */
	for (i = startpt; i < endpt; i++)
	{
		vbase = *(mesh->vertices + i);
		pt.x = vbase->x + offset.x;
		pt.y = vbase->y + offset.y;
		pt.z = vbase->z + offset.z;
		v = Ray_NewMeshVertex(&pt);
		if (!Ray_AddMeshVertex(mesh, v))
			return;
	}

	/* Build the connecting triangles... */
	ppvprev = mesh->vertices + startpt;
	ppvcur = ppvprev + s_nvertices;
	for (i = 1; i < s_nvertices; i++)
	{
		tri = Ray_NewMeshTri(ppvprev[0], ppvprev[1], ppvcur[1]);
		Ray_AddMeshTri(mesh, tri);
		tri = Ray_NewMeshTri(ppvcur[1], ppvcur[0], ppvprev[0]);
		Ray_AddMeshTri(mesh, tri);
		ppvprev++;
		ppvcur++;
	}

	s_nsteps++;
}

void ExecRotStepStmt(Stmt *stmt)
{
	RotStepStmtData *sd = (RotStepStmtData *)stmt->data;
	Vec3 center, axis;
	double angle = 90.0;
	int steps = 1;
	Vec3 pt;
	MeshVertex *v, *vbase, **ppvprev, **ppvcur;
	MeshTri *tri;
	MeshData *mesh = objstack_ptr->curobj->data.mesh;
	int i, j;
	double theta;
	int startpt, endpt, segstart, segend;

	V3Set(&center, 0.0, 0.0, 0.0);
	V3Set(&axis, 0.0, 0.0, 1.0);

	if (sd->center != NULL)
		ExprEvalVector(sd->center, &center);
	if (sd->axis != NULL)
		ExprEvalVector(sd->axis, &axis);
	if (sd->angle != NULL)
		angle = ExprEvalDouble(sd->angle);
	if (sd->steps != NULL)
		steps = (int)ExprEvalDouble(sd->steps);

	startpt = mesh->nvertices - s_nvertices;
	endpt = mesh->nvertices;

	for (j = 1; j <= steps; j++)
	{
		theta = RAD((angle * (double)j) / (double)steps);

		/* Make copies of the base points and transform them... */
		segstart = mesh->nvertices - s_nvertices;
		segend = mesh->nvertices;
		for (i = startpt; i < endpt; i++)
		{
			vbase = *(mesh->vertices + i);

			/* Rotate the vertex around the given center point and axis. */
			pt.x = (double)vbase->x - center.x;
			pt.y = (double)vbase->y - center.y;
			pt.z = (double)vbase->z - center.z;
			RotatePoint3D(&pt, theta, &axis);
			pt.x += center.x;
			pt.y += center.y;
			pt.z += center.z;
			v = Ray_NewMeshVertex(&pt);
			v->flags = vbase->flags;
			if (vbase->flags & MESH_VERTEX_HAS_NORMAL)
			{
				/* Rotate the normal, too. */
				pt.x = (double)vbase->nx;
				pt.y = (double)vbase->ny;
				pt.z = (double)vbase->nz;
				RotatePoint3D(&pt, theta, &axis);
				v->nx = (float)pt.x;
				v->ny = (float)pt.y;
				v->nz = (float)pt.z;
			}
			if (!Ray_AddMeshVertex(mesh, v))
				return;
		}

		/* Build the connecting triangles... */
		/* TODO: Consolidate some of this functionality into a separate function. */
		ppvprev = mesh->vertices + segstart;
		ppvcur = mesh->vertices + segend;
		for (i = 1; i < s_nvertices; i++)
		{
			tri = Ray_NewMeshTri(ppvprev[0], ppvprev[1], ppvcur[1]);
			Ray_AddMeshTri(mesh, tri);
			tri = Ray_NewMeshTri(ppvcur[1], ppvcur[0], ppvprev[0]);
			Ray_AddMeshTri(mesh, tri);
			ppvprev++;
			ppvcur++;
		}

		s_nsteps++;
	}
}

void ExecPolymeshPathStmt(Stmt *stmt)
{
	ExecBlock(stmt, (Stmt *)stmt->data);

	// Clean up the points array.
	if (s_ptlist != NULL)
		free(s_ptlist);
	s_ptlist = NULL;
	s_npoints = 0;
	s_npoints_alloced = 0;
}

void ExecPointStmt(Stmt *stmt)
{
	Vec3 pt, offset;
	MeshVertex *v, *vbase, **ppvprev, **ppvcur;
	MeshTri *tri;
	MeshData *mesh = objstack_ptr->curobj->data.mesh;
	int i;
	int startpt = mesh->nvertices - s_nvertices;
	int endpt = mesh->nvertices;

	assert(stmt->data != NULL);
	ExprEvalVector((Expr *)stmt->data, &offset);

	// Save this point in the point array, allocating if needed.
	if (s_npoints >= s_npoints_alloced)
	{
		Vec3 *oldlist = s_ptlist;

		s_ptlist = (Vec3 *)calloc(s_npoints_alloced + NPTS_PER_CHUNK,
			sizeof(Vec3));
		if ((s_ptlist != NULL) && (oldlist != NULL))
		{
			memcpy(s_ptlist, oldlist, s_npoints * sizeof(Vec3));
		}
	}
	
	assert(s_ptlist != NULL);
	if (s_ptlist != NULL)
	{
		V3Copy(&s_ptlist[s_npoints], &offset);
		s_npoints++;
	}

	// Make copies of the base points.
	// These will be transformed in the next point after rotations
	// are determined.
	for (i = 0; i < s_nvertices; i++)
	{
		vbase = *(mesh->vertices + i);
		pt.x = vbase->x;
		pt.y = vbase->y;
		pt.z = vbase->z;
		v = Ray_NewMeshVertex(&pt);
		if (!Ray_AddMeshVertex(mesh, v))
			return;
	}

	// If we have segment(s) (more than one point)...
	if (s_npoints > 1)
	{
		// Rotate the previous set of vertices based on the
		// median direction vector of this segment and the previous
		// segment. Use just this segment's direction if this is
		// the first segment (second point).

		// Apply the rotations, and then translate the vertices to
		// the location of the previous point.
		ppvprev = mesh->vertices + startpt;
		for (i = 0; i < s_nvertices; i++)
		{
			v = *ppvprev;

			ppvprev++;
		}

		// Build the connecting triangles...
		ppvprev = mesh->vertices + startpt;
		ppvcur = ppvprev + s_nvertices;
		for (i = 1; i < s_nvertices; i++)
		{
			tri = Ray_NewMeshTri(ppvprev[0], ppvprev[1], ppvcur[1]);
			Ray_AddMeshTri(mesh, tri);
			tri = Ray_NewMeshTri(ppvcur[1], ppvcur[0], ppvprev[0]);
			Ray_AddMeshTri(mesh, tri);
			ppvprev++;
			ppvcur++;
		}
	}
}

void DeletePolymeshStmt(Stmt *stmt)
{
	DeleteStatements((Stmt *)stmt->data);
}

void DeletePolymeshVertexStmt(Stmt *stmt)
{
	VertexStmtData *sd = (VertexStmtData *)stmt->data;
	if (sd != NULL)
	{
		ExprDelete(sd->point);
		ExprDelete(sd->normal);
		free(sd);
	}
}

void DeletePolymeshExtrudeStmt(Stmt *stmt)
{
	DeleteStatements((Stmt *)stmt->data);
}

void DeleteRelStepStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}

void DeleteRotStepStmt(Stmt *stmt)
{
	RotStepStmtData *sd = (RotStepStmtData *)stmt->data;
	if (sd != NULL)
	{
		ExprDelete(sd->center);
		ExprDelete(sd->axis);
		ExprDelete(sd->angle);
		ExprDelete(sd->steps);
		free(sd);
	}
}

void DeletePolymeshPathStmt(Stmt *stmt)
{
	DeleteStatements((Stmt *)stmt->data);
}

void DeletePointStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}

