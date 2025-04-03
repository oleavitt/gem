/**
 *****************************************************************************
 * @file polygon.c
 *  The polygon primitive.
 *
 *****************************************************************************
 */

#include "ray.h"

unsigned long ray_polygon_tests;
unsigned long ray_polygon_hits;

static int IntersectPolygon(Object *obj, HitData *hits);
static void CalcNormalPolygon(Object *obj, Vec3 *P, Vec3 *N);
static int IsInsidePolygon(Object *obj, Vec3 *P);
static void CalcUVMapPolygon(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsPolygon(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformPolygon(Object *obj, Vec3 *params, int type);
static void CopyPolygon(Object *destobj, Object *srcobj);
static void DeletePolygon(Object *obj);
static void DrawPolygon(Object *obj);

static ObjectProcs polygon_procs =
{
	OBJ_POLYGON,
	IntersectPolygon,
	CalcNormalPolygon,
	IsInsidePolygon,
	CalcUVMapPolygon,
	CalcExtentsPolygon,
	TransformPolygon,
	CopyPolygon,
	DeletePolygon,
	DrawPolygon
};


/*
 * Compute the surface area (X 2) of a 2D triangle.
 */
double TriArea(double x1, double y1, double x2, double y2,
	double x3, double y3)
{
	return fabs(x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
}


void SetupPolygon(PolygonData *ply)
{
	Vec3 V1, V2, N;
	float *pts;

	assert(ply != NULL);
	assert(ply->npts >= 3);

	pts = ply->pts;

	assert(pts != NULL);

	/* Compute plane normal. (based on first three vertices) */
	V1.x = pts[3] - pts[0];
	V2.x = pts[6] - pts[0];
	V1.y = pts[4] - pts[1];
	V2.y = pts[7] - pts[1];
	V1.z = pts[5] - pts[2];
	V2.z = pts[8] - pts[2];

	V3Cross(&N, &V1, &V2);
	V3Normalize(&N);
	ply->nx = (float)N.x;
	ply->ny = (float)N.y;
	ply->nz = (float)N.z;

	/* Determine axis of greatest projection. */
	if(fabs(N.x) > fabs(N.z))
	{
		if(fabs(N.x) > fabs(N.y))
			ply->axis = X_AXIS;
		else
			ply->axis = Y_AXIS;
	}
	else if(fabs(N.y) > fabs(N.z))
		ply->axis = Y_AXIS;
	else
		ply->axis = Z_AXIS;
}


Object *Ray_MakePolygon(float *pts, int npts)
{
	Object *obj = NewObject();

	if (obj != NULL)
	{
		PolygonData *polygon = (PolygonData *)Malloc(sizeof(PolygonData));
		if (polygon != NULL)
		{
			if (npts >= 3)
			{
				polygon->npts = npts;
				if((polygon->pts = (float *)Calloc(npts * 3, sizeof(float))) !=
					NULL)
				{
					if (pts != NULL)
					{
						// A point list was given. Copy the points into the
						// polygon point list and initialize it.
						//
						memcpy(polygon->pts, pts, npts * 3 * sizeof(float));
						SetupPolygon(polygon);
					}
				}
				else
				{
					// Alloc failed.
					Ray_DeleteObject(obj);
					return NULL;
				}
			}
			else
			{
				// Start with an empty polygon.
				// Points will be added later.
				//
				polygon->pts = NULL;
				polygon->npts = 0;
			}
			obj->data.polygon = polygon;
			obj->procs = &polygon_procs;

			// Don't shadow test against self.
			//
			obj->flags |= OBJ_FLAG_NO_SELF_INTERSECT;
		}
		else // Alloc failed
			obj = Ray_DeleteObject(obj);
	}

	return obj;
}

/**
 * Makes an n-sided polygon of unit radius on the XY plane at point <0, 0, 0>.
 *
 * @param npts - int - The number of points. Must be at least 3.
 *
 * @return Object* - A new polygon object if successful. NULL otherwise.
 */
Object *Ray_MakeNPolygon(int npts)
{
	Object *obj;

	// Gotta have at least 3 points!
	//
	if (npts < 3)
		return NULL;

	// Make a polygon with an empty point list with room for npts.
	//
	obj = Ray_MakePolygon(NULL, npts);
	if (obj != NULL)
	{
		int				n;
		PolygonData *	ply = obj->data.polygon;
		float *			ppt = ply->pts;
		double			theta, angle;

		// Generate the n-gon.
		//
		angle = TWOPI / (double)npts;
		theta = 0.0;
		for (n = 0; n < npts; n++)
		{
			*ppt++ = (float) sin(theta);
			*ppt++ = (float) cos(theta);
			*ppt++ = 0.0; // z is always 0.0
			theta += angle;
		}

		// Setup the polygon.
		// We can take a shortcut here since we know that the polygon
		// is flat on the XY plane.

		// The dominant axis.
		ply->axis = Z_AXIS;

		// The plane normal.
		ply->nx = 0.0;
		ply->ny = 0.0;
		ply->nz = 1.0;
	}

	return obj;
}

int Ray_PolygonAddVertex(Object *obj, Vec3 *point)
{
	PolygonData *ply;
	float *newpts, *oldpts;

	assert(obj != NULL);
	assert(obj->data.polygon != NULL);
	ply = obj->data.polygon;
	if((newpts = (float *)Calloc((ply->npts + 1) * 3, sizeof(float))) != NULL)
	{
		oldpts = ply->pts;
		ply->pts = newpts;
		if(oldpts != NULL)
		{
			int oldsize = ply->npts * 3;
			memcpy(newpts, oldpts, oldsize * sizeof(float));
			Free(oldpts, oldsize * sizeof(float));
			newpts += oldsize;
		}
		ply->npts++;
		*newpts++ = (float)point->x;
		*newpts++ = (float)point->y;
		*newpts = (float)point->z;
		return 1;
	}
	return 0;
}

int Ray_PolygonFinish(Object *obj)
{
	PolygonData *ply;

	assert(obj != NULL);
	assert(obj->data.polygon != NULL);
	ply = obj->data.polygon;
	if(ply->npts < 3)
		return 0;
	SetupPolygon(ply);
	return 1;
}


int IntersectPolygon(Object *obj, HitData *hits)
{
	PolygonData *ply;
	double d, t, x, y, x0, y0, x1, y1;
	Vec3 P;
	int i, A, B, in;

	ray_polygon_tests++;

	ply = obj->data.polygon;

	/* Intersect ray with plane containing polygon. */
	P.x = ct.B.x - ply->pts[0];
	P.y = ct.B.y - ply->pts[1];
	P.z = ct.B.z - ply->pts[2];

	d = ply->nx * ct.D.x +
			ply->ny * ct.D.y +
			ply->nz * ct.D.z;

	/* ray is parallel to plane if denom == 0 */
	if(fabs(d) < EPSILON)
		return 0;

	t = -(ply->nx * P.x + ply->ny * P.y + ply->nz * P.z) / d;

	/* 0 (miss) if behind viewer or too small. */
	if(t < ct.tmin || t > ct.tmax)
		return 0;

	hits->entering = (d < 0.0) ? 1 : 0;

	/*
	 * Project the polygon vertices to the 2D plane
	 * that is most perpendicular to the plane normal.
	 * This has already been pre-computed and the result
	 * is stored in "ply->axis".
	 */
	switch(ply->axis)
	{
		case X_AXIS:
			A = 1;
			B = 2;
			x = ct.B.y + ct.D.y * t;
			y = ct.B.z + ct.D.z * t;
			break;
		case Y_AXIS:
			A = 0;
			B = 2;
			x = ct.B.x + ct.D.x * t;
			y = ct.B.z + ct.D.z * t;
			break;
		default:  /* Z axis. */
			A = 0;
			B = 1;
			x = ct.B.x + ct.D.x * t;
			y = ct.B.y + ct.D.y * t;
			break;
	}

	/*
	 * Compute 2D point on the projected plane and
	 * see if it is within the bounds of the projected polygon.
	 */
	in = 0;
	for(i = 0; i < ply->npts; i++)
	{
		x0 = ply->pts[i*3+A];
		y0 = ply->pts[i*3+B];
		if(i < ply->npts - 1)
		{
			x1 = ply->pts[(i+1)*3+A];
			y1 = ply->pts[(i+1)*3+B];
		}
		else
		{
			x1 = ply->pts[A];
			y1 = ply->pts[B];
		}

		if((y0 < y && y1 < y) ||
			 (y0 > y && y1 > y) ||
			 (x0 < x && x1 < x)) continue;

		if(x0 > x && x1 > x)
		{
			in = 1 - in;
			continue;
		}

		d = (y0 - y) / (y0 - y1);
		d = x0 * (1.0 - d) + x1 * d;

		if(x < d)
			in = 1 - in;
	}

	if(in)
	{
		hits->t = t;
		hits->obj = obj;
		ray_polygon_hits++;
		return 1;
	}
	return 0;
}


void CalcNormalPolygon(Object *obj, Vec3 *P, Vec3 *N)
{
	PolygonData *p = obj->data.polygon;
	
	N->x = p->nx;
	N->y = p->ny;
	N->z = p->nz;

	//P; // Not used
}


int IsInsidePolygon(Object *obj, Vec3 *P)
{
	PolygonData *p = obj->data.polygon;
	if(((P->x - p->pts[0]) * p->nx +
		  (P->y - p->pts[1]) * p->ny +
		  (P->z - p->pts[2]) * p->nz) > 0.0)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */
	return (!(obj->flags & OBJ_FLAG_INVERSE)); /* inside */
}


void CalcUVMapPolygon(Object *obj, Vec3 *P, double *u, double *v)
{
  PolygonData *p = obj->data.polygon;
	double area, a1, a2, a3, pu, pv;
	float *P1, *P2, *P3;
	int A, B;

	/*
	 * Project the triangle vertices to the 2D plane
	 * that is most perpendicular to the plane normal.
	 * We use the triangle formed by the first three vertices
	 * of the polygon.
	 */
	switch(p->axis )
	{
		case X_AXIS:
			A = 1;
			B = 2;
			pu = P->y;
			pv = P->z;
			break;
		case Y_AXIS:
			A = 0;
			B = 2;
			pu = P->x;
			pv = P->z;
			break;
		default:  /* Z_AXIS. */
			A = 0;
			B = 1;
			pu = P->x;
			pv = P->y;
			break;
	}

	P1 = p->pts;
	P2 = &p->pts[3];
	P3 = &p->pts[6];

	/* Determine barycentric coordinates of point... */
	area = TriArea(P1[A], P1[B], P2[A], P2[B], P3[A], P3[B]);
	a1 =   TriArea(pu, pv, P2[A], P2[B], P3[A], P3[B]) / area;
	a2 =   TriArea(P1[A], P1[B], pu, pv, P3[A], P3[B]) / area;
	a3 =   1.0 - a1 - a2;

	/*
	 * ...and use them to interpolate between the UV coordinates
	 * for each vertex to get actual UV position.
	 */
	*u = 1.0 - a1;
	*v = a3;
}


void CalcExtentsPolygon(Object *obj, Vec3 *omin, Vec3 *omax)
{
  PolygonData *p = obj->data.polygon;
	int i;
	float *pts;

	pts = p->pts;

	omin->x = *pts;
	omax->x = *pts++;
	omin->y = *pts;
	omax->y = *pts++;
	omin->z = *pts;
	omax->z = *pts++;

	for(i = 1; i < p->npts; i++)
	{
		omin->x = fmin(omin->x, *pts);
		omax->x = fmax(omax->x, *pts);
		pts++;
		omin->y = fmin(omin->y, *pts);
		omax->y = fmax(omax->y, *pts);
		pts++;
		omin->z = fmin(omin->z, *pts);
		omax->z = fmax(omax->z, *pts);
		pts++;
	}
	omin->x -= EPSILON;
	omin->y -= EPSILON;
	omin->z -= EPSILON;
	omax->x += EPSILON;
	omax->y += EPSILON;
	omax->z += EPSILON;
}


void TransformPolygon(Object *obj, Vec3 *params, int type)
{
  PolygonData *p = obj->data.polygon;
	Vec3 V;
	int i;
	float *old_pts, *new_pts;

	old_pts = new_pts = p->pts;
	for(i = 0; i < p->npts; i++)
	{
		V.x = *old_pts++;
		V.y = *old_pts++;
		V.z = *old_pts++;
		XformVector(&V, params, type);
		*new_pts++ = (float)V.x;
		*new_pts++ = (float)V.y;
		*new_pts++ = (float)V.z;
	}
	SetupPolygon(p);
}


void CopyPolygon(Object *destobj, Object *srcobj)
{
	PolygonData *srcpolygon, *destpolygon;

	srcpolygon = srcobj->data.polygon;
	destpolygon = (PolygonData *)Malloc(sizeof(PolygonData));
	destobj->data.polygon = destpolygon;
	if(destpolygon != NULL)
	{
		*destpolygon = *srcpolygon;
		destpolygon->pts = (float *)Malloc(sizeof(float) * srcpolygon->npts * 3);
		if(destpolygon->pts != NULL)
			memcpy(destpolygon->pts, srcpolygon->pts, sizeof(float) *
				srcpolygon->npts * 3);
	}
}


void DeletePolygon(Object *obj)
{
	PolygonData *polygon = obj->data.polygon;
	if(polygon != NULL)
	{
		Free(polygon->pts, sizeof(float) * polygon->npts * 3);
		Free(polygon, sizeof(PolygonData));
	}
}


void MatrixTransformPolygon(PolygonData *p, Xform *T)
{
	Vec3 V;
	int i;
	float *old_pts, *new_pts;

	old_pts = new_pts = p->pts;
	for(i = 0; i < p->npts; i++)
	{
		V.x = *old_pts++;
		V.y = *old_pts++;
		V.z = *old_pts++;
		PointToWorld(&V, T);
		*new_pts++ = (float)V.x;
		*new_pts++ = (float)V.y;
		*new_pts++ = (float)V.z;
	}
	SetupPolygon(p);
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

void DrawPolygon(Object *obj)
{
	PolygonData *p = obj->data.polygon;
	Vec3 V;
	int i;
	float *pts;

	pts = p->pts;
	V.x = *pts++;
	V.y = *pts++;
	V.z = *pts++;
	Set_Pt(0, V.x, V.y, V.z);
	Move_To(0);
	for(i = 1; i < p->npts; i++)
	{
		V.x = *pts++;
		V.y = *pts++;
		V.z = *pts++;
		Set_Pt(i, V.x, V.y, V.z);
		Line_To(i);
	}
	Line_To(0);
}
