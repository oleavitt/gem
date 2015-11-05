/***************************************************************
*
*	Nurbs.c - Scene script interface for NURBS surfaces
*
***************************************************************/

#include "local.h"
#include "nurbs.h"
#include "drawing.h"

typedef struct tag_nurbsstmtdata
{
	Expr *numPatchesU;
	Expr *numPatchesV;
	Expr *orderU;
	Expr *orderV;
	Stmt *block;
} NurbsStmtData;

typedef struct tag_nurbsptstmtdata
{
	Expr *point;
	Expr *weight;
} NurbsPtStmtData;

static void TriFromSurfSample(SurfSample *v0, SurfSample *v1, SurfSample *v2);
void (*DrawTriangle)( SurfSample *, SurfSample *, SurfSample * ) = TriFromSurfSample;

static void ExecNurbsStmt(Stmt *stmt);
static void DeleteNurbsStmt(Stmt *stmt);
static void ExecNurbsPointStmt(Stmt *stmt);
static void DeleteNurbsPointStmt(Stmt *stmt);

static int nurbs_ntris;

static StmtProcs nurbs_stmt_procs =
{
	TK_NURBS,
	ExecNurbsStmt,
	DeleteNurbsStmt
};

static StmtProcs nurbspoint_stmt_procs =
{
	TK_POINT,
	ExecNurbsPointStmt,
	DeleteNurbsPointStmt
};

static NurbSurface nurbs_newsurface;
static int nurbs_nextU, nurbs_nextV, nurbs_npoints;

Stmt *ParseNurbsStmt(void)
{
	int nparams;
	Param params[5];
	NurbsStmtData *sd;
	int prev_object_token;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("nurbs");
		return NULL;
	}
	sd = (NurbsStmtData *)calloc(1, sizeof(NurbsStmtData));
	if(sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("nurbs");
		return NULL;
	}
	stmt->procs = &nurbs_stmt_procs;
	stmt->data = (void *)sd;
	sd->numPatchesU = sd->numPatchesV = sd->orderU = sd->orderV = NULL;
	sd->block = NULL;
	prev_object_token = cur_object_token;
	cur_object_token = TK_NURBS;
	nparams = ParseParams("EEEEB", "nurbs", ParseObjectDetails, params);
	cur_object_token = prev_object_token;
	assert(nparams == 5);
	assert(params[0].type == PARAM_EXPR);
	sd->numPatchesU = params[0].data.expr;
	assert(params[1].type == PARAM_EXPR);
	sd->numPatchesV = params[1].data.expr;
	assert(params[2].type == PARAM_EXPR);
	sd->orderU = params[2].data.expr;
	assert(params[3].type == PARAM_EXPR);
	sd->orderV = params[3].data.expr;
	assert(params[4].type == PARAM_BLOCK);
	sd->block = params[4].data.block;
	if(!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseNurbsPointStmt(void)
{
	int nparams;
	Param params[2];
	NurbsPtStmtData *sd;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("point");
		return NULL;
	}
	sd = (NurbsPtStmtData *)calloc(1, sizeof(NurbsPtStmtData));
	if (sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("point");
		return NULL;
	}
	sd->point = sd->weight = NULL;
	stmt->procs = &nurbspoint_stmt_procs;
	stmt->data = (void *)sd;
	nparams = ParseParams("EOE;", "point", NULL, params);
	assert(nparams >= 1);
	assert(params[0].type == PARAM_EXPR);
	sd->point = params[0].data.expr;
	if (nparams > 1)
	{
		assert(params[1].type == PARAM_EXPR);
		sd->weight = params[1].data.expr;
	}
	if (!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

#if 0
static void generateTorus(double majorRadius, double minorRadius)
{
/*	Object *obj;
	Vec3 v1, v2;
*/    /* These define the shape of a unit torus centered about the origin. */
    double xvalues[] = { 0.0, -1.0, -1.0, -1.0, 0.0, 1.0, 1.0, 1.0, 0.0 };
    double yvalues[] = { 1.0, 1.0, 0.0, -1.0, -1.0, -1.0, 0.0, 1.0, 1.0 };
    double zvalues[] = { 0.0, 1.0, 1.0, 1.0, 0.0, -1.0, -1.0, -1.0, 0.0 };
    double offsets[] = { -1.0, -1.0, 0.0, 1.0, 1.0, 1.0, 0.0, -1.0, -1.0 };

    /* Piecewise Bezier knot vector for a quadratic curve with four segments */
    double knots[] = { 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4 };

    long i, j, npts;

    double r2over2 = sqrt(2.0) / 2.0;
    double weight;

    /* Set up the dimension and orders of the surface */

	npts = 9;
    nurbs_newsurface.numU = npts;	    /* A circle is formed from nine points */
    nurbs_newsurface.numV = npts;
    nurbs_newsurface.orderU = 3;	    /* Quadratic in both directions */
    nurbs_newsurface.orderV = 3;

    /* After the dimension and orders are set, AllocNurb creates the dynamic
     * storage for the control points and the knot vectors */

	AllocNurb(&nurbs_newsurface, NULL, NULL );

	for (i = 0; i < npts; i++)
	{
		for (j = 0; j < npts; j++)
		{
			weight = ((j & 1) ? r2over2 : 1.0) * ((i & 1) ? r2over2 : 1.0);
			/* Notice how the weights are pre-multiplied with the x, y and z values */
			nurbs_newsurface.points[i][j].x = xvalues[j]
						* (majorRadius + offsets[i] * minorRadius) * weight;
			nurbs_newsurface.points[i][j].y = yvalues[j]
						* (majorRadius + offsets[i] * minorRadius) * weight;
			nurbs_newsurface.points[i][j].z = (zvalues[i] * minorRadius) * weight;
			nurbs_newsurface.points[i][j].w = weight;

/*			V3Set(&v1, nurbs_newsurface.points[i][j].x, nurbs_newsurface.points[i][j].y, nurbs_newsurface.points[i][j].z);
			obj = Ray_MakeSphere(&v1, 0.05);
			ScnBuild_AddObject(obj);
			if (i > 0)
			{
				V3Set(&v2, nurbs_newsurface.points[i-1][j].x, nurbs_newsurface.points[i-1][j].y, nurbs_newsurface.points[i-1][j].z);
				obj = Ray_MakeCone(&v1, &v2, 0.025, 0.025, 0);
				ScnBuild_AddObject(obj);
			}
			if (j > 0)
			{
				V3Set(&v2, nurbs_newsurface.points[i][j-1].x, nurbs_newsurface.points[i][j-1].y, nurbs_newsurface.points[i][j-1].z);
				obj = Ray_MakeCone(&v1, &v2, 0.025, 0.025, 0);
				ScnBuild_AddObject(obj);
			}
*/		}
	}

    /* The knot vectors define piecewise Bezier segments (the same in both U and V). */

	for (i = 0; i < nurbs_newsurface.numU + nurbs_newsurface.orderU; i++)
		nurbs_newsurface.kvU[i] = nurbs_newsurface.kvV[i] = (double) knots[i];
}
#endif

void ExecNurbsStmt(Stmt *stmt)
{
	NurbsStmtData *sd = (NurbsStmtData *)stmt->data;
	Object *obj, *prev_bound_objects;
	int i, j, orderU, orderV, numPatchesU, numPatchesV, success = 0;
	double *kn;

	nurbs_ntris = 0;
	nurbs_nextU = nurbs_nextV = nurbs_npoints = 0;

	if ((obj = Ray_MakeCSG(OBJ_CSGGROUP)) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		ScnBuild_PushObjStack();
		objstack_ptr->curobj = obj;
		prev_bound_objects = bound_objects;
		bound_objects = NULL;

		assert(sd->numPatchesU != NULL);
		numPatchesU = (int)ExprEvalDouble(sd->numPatchesU);
		assert(sd->numPatchesV != NULL);
		numPatchesV = (int)ExprEvalDouble(sd->numPatchesV);
		assert(sd->orderU != NULL);
		orderU = (int)ExprEvalDouble(sd->orderU);
		assert(sd->orderV != NULL);
		orderV = (int)ExprEvalDouble(sd->orderV);

		if (numPatchesU > 0 && numPatchesV > 0 && orderU > 2 && orderV > 2)
		{
			nurbs_newsurface.numU = numPatchesU * (orderU - 1) + 1;
			nurbs_newsurface.numV = numPatchesV * (orderV - 1) + 1;
			nurbs_newsurface.orderU = orderU;
			nurbs_newsurface.orderV = orderV;
			AllocNurb(&nurbs_newsurface, NULL, NULL);

			kn = nurbs_newsurface.kvU;
			*kn++ = 0.0;
			for (j = 0; j <= numPatchesU; j++)
				for (i = 1; i < orderU; i++)
					*kn++ = j;
			*kn = j-1;
			kn = nurbs_newsurface.kvV;
			*kn++ = 0.0;
			for (j = 0; j <= numPatchesV; j++)
				for (i = 1; i < orderV; i++)
					*kn++ = j;
			*kn = j-1;

			ExecBlock(stmt, sd->block);
			if (nurbs_nextV >= nurbs_newsurface.numV)
			{
				/* Set up the subdivision tolerance (facets span about four pixels) */
				SubdivTolerance = 4.0;
				DrawSubdivision(&nurbs_newsurface);
				/* TODO: DrawEvaluation(&nurbs_newsurface);*/	/* Alternate drawing method */
				FreeNurb(&nurbs_newsurface);

				LogMessage("NURBS: Generated %d triangles.", nurbs_ntris);
				success = 1;
			}
			else
				LogMessage("NURBS: All %d points must be defined. Got %d, need %d more.",
					nurbs_newsurface.numU * nurbs_newsurface.numV,
					nurbs_npoints,
					nurbs_newsurface.numU * nurbs_newsurface.numV - nurbs_npoints);
		}

		if (Ray_FinishCSG(obj, objstack_ptr->objlist, bound_objects))
		{
			ScnBuild_PopObjStack();
			if (success)
				ScnBuild_AddObject(obj);
			else
			{
				Ray_DeleteObject(obj);
				if (orderU < 3 || orderV < 3)
					LogMessage("NURBS: Order must be at least 3");
				if (numPatchesU < 1 || numPatchesV < 1)
					LogMessage("NURBS: Invalid dimensions. Must have at least one, or more patches");
			}
		}
		else
		{
			Ray_DeleteObject(obj);
			ScnBuild_PopObjStack();
		}
		bound_objects = prev_bound_objects;
	}
}

void ExecNurbsPointStmt(Stmt *stmt)
{
	NurbsPtStmtData *sd = (NurbsPtStmtData *)stmt->data;
	Vec3 pt;
	Point4 *rpt;
	double weight = 1.0;
	assert(sd != NULL);
	assert(sd->point != NULL);
	if (nurbs_nextV >= nurbs_newsurface.numV)
	{
		LogMessage("NURBS: Extra point ignored. %d x %d points max.",
			nurbs_newsurface.numU, nurbs_newsurface.numV);
		return;
	}
	ExprEvalVector(sd->point, &pt);
	if (sd->weight != NULL)
		weight = ExprEvalDouble(sd->weight);
	rpt = &nurbs_newsurface.points[nurbs_nextV][nurbs_nextU];
	rpt->x = pt.x * weight;
	rpt->y = pt.y * weight;
	rpt->z = pt.z * weight;
	rpt->w = weight;
	nurbs_npoints++;
	nurbs_nextU++;
	if (nurbs_nextU >= nurbs_newsurface.numU)
	{
		nurbs_nextU = 0;
		nurbs_nextV++;
	}
}

void DeleteNurbsStmt(Stmt *stmt)
{
	NurbsStmtData *sd = (NurbsStmtData *)stmt->data;
	if (sd != NULL)
	{
		ExprDelete(sd->numPatchesU);
		ExprDelete(sd->numPatchesV);
		ExprDelete(sd->orderU);
		ExprDelete(sd->orderV);
		DeleteStatements(sd->block);
		free(sd);
	}
}

void DeleteNurbsPointStmt(Stmt *stmt)
{
	NurbsPtStmtData *sd = (NurbsPtStmtData *)stmt->data;
	if (sd != NULL)
	{
		ExprDelete(sd->point);
		ExprDelete(sd->weight);
		free(sd);
	}
}

/*
 * These functions are called upon by the NURBS tessellation code.
 */
void TriFromSurfSample(SurfSample *v0, SurfSample *v1, SurfSample *v2)
{
	Object *obj;
	float pts[9], norms[9], uv[6];
	pts[0] = (float)v0->point.x; pts[1] = (float)v0->point.y; pts[2] = (float)v0->point.z;
	pts[3] = (float)v1->point.x; pts[4] = (float)v1->point.y; pts[5] = (float)v1->point.z;
	pts[6] = (float)v2->point.x; pts[7] = (float)v2->point.y; pts[8] = (float)v2->point.z;
	norms[0] = (float)v0->normal.x; norms[1] = (float)v0->normal.y; norms[2] = (float)v0->normal.z;
	norms[3] = (float)v1->normal.x; norms[4] = (float)v1->normal.y; norms[5] = (float)v1->normal.z;
	norms[6] = (float)v2->normal.x; norms[7] = (float)v2->normal.y; norms[8] = (float)v2->normal.z;
	uv[0] = (float)v0->u; uv[1] = (float)v0->v;
	uv[2] = (float)v1->u; uv[3] = (float)v1->v;
	uv[4] = (float)v2->u; uv[5] = (float)v2->v;
	if ((obj = Ray_MakeTriangle(pts, norms, uv)) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		ScnBuild_AddObject(obj);
		nurbs_ntris++;
	}
}

void ScreenProject(Point4 *worldPt, Point3 *screenPt)
{
    screenPt->x = worldPt->x / worldPt->w * 100 + 200;
    screenPt->y = worldPt->y / worldPt->w * 100 + 200;
    screenPt->z = worldPt->z / worldPt->w * 100 + 200;
}
