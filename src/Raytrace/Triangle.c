/*************************************************************************
*
*  triangle.c
*
*  The triangle primitive.
*
*************************************************************************/

#include "ray.h"

static int IntersectTriangle(Object *obj, HitData *hits);
static void CalcNormalTriangle(Object *obj, Vec3 *P, Vec3 *N);
static int IsInsideTriangle(Object *obj, Vec3 *P);
static void CalcUVMapTriangle(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsTriangle(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformTriangle(Object *obj, Vec3 *params, int type);
static void CopyTriangle(Object *destobj, Object *srcobj);
static void DeleteTriangle(Object *obj);
static void DrawTriangle(Object *obj);

unsigned long ray_triangle_tests;
unsigned long ray_triangle_hits;

static ObjectProcs triangle_procs =
{
	OBJ_TRIANGLE,
	IntersectTriangle,
	CalcNormalTriangle,
	IsInsideTriangle,
  CalcUVMapTriangle,
  CalcExtentsTriangle,
  TransformTriangle,
  CopyTriangle,
	DeleteTriangle,
	DrawTriangle
};


void SetupTriangle(TriangleData *tri)
{
	Vec3 V1, V2, N;
	double len;
	int i;

	/* Compute plane normal. */
	V1.x = tri->pts[3] - tri->pts[0];
	V2.x = tri->pts[6] - tri->pts[0];
	V1.y = tri->pts[4] - tri->pts[1];
	V2.y = tri->pts[7] - tri->pts[1];
	V1.z = tri->pts[5] - tri->pts[2];
	V2.z = tri->pts[8] - tri->pts[2];

	V3Cross(&N, &V2, &V1);
	V3Normalize(&N);
	tri->pnorm[0] = (float)N.x;
	tri->pnorm[1] = (float)N.y;
	tri->pnorm[2] = (float)N.z;

	/* Normalize the vertex normals. */
	for(i = 0; i < 9; i += 3)
	{
		if((tri->norms[i] * N.x +
        tri->norms[i+1] * N.y +
        tri->norms[i+2] * N.z) < 0.0)
		{
			tri->norms[i] = - tri->norms[i];
			tri->norms[i+1] = - tri->norms[i+1];
			tri->norms[i+2] = - tri->norms[i+2];
		}

		len = sqrt(tri->norms[i] * tri->norms[i] +
      tri->norms[i+1] * tri->norms[i+1] +
      tri->norms[i+2] * tri->norms[i+2]);
		if(len > EPSILON)
		{
			tri->norms[i] /= (float)len;
			tri->norms[i+1] /= (float)len;
			tri->norms[i+2] /= (float)len;
		}
		else   /* Use the plane normal for bad vertex normals. */
		{
			tri->norms[i] = (float)N.x;
			tri->norms[i+1] = (float)N.y;
			tri->norms[i+2] = (float)N.z;
		}
	}

	/* Determine axis of greatest projection. */
	if(fabs(N.x) > fabs(N.z))
	{
		if(fabs(N.x) > fabs(N.y))
			tri->axis = X_AXIS;
		else
			tri->axis = Y_AXIS;
	}
	else if(fabs(N.y) > fabs(N.z))
		tri->axis = Y_AXIS;
	else
		tri->axis = Z_AXIS;
}


/*
 * Compute the surface area (X 2) of a 2D triangle.
 */
double TriangleArea(double x1, double y1, double x2, double y2,
  double x3, double y3)
{
	return fabs(x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
}

void FixTriangleNormal(Object *obj, Vec3 *D, Vec3 *N)
{
	TriangleData *tri = obj->data.triangle;

	if((D->x * tri->pnorm[0] +
			D->y * tri->pnorm[1] +
			D->z * tri->pnorm[2]) > 0.0)
	{          /* see if we need to flip... */
		if(V3Dot(D, N) > 0.0) /* we need to flip */
    {
			N->x = -N->x;
			N->y = -N->y;
			N->z = -N->z;
    }
	}
}


Object *Ray_MakeTriangle(float *points, float *normals, float *uvpoints)
{
	Object *obj;
	TriangleData *tri;

	obj = NewObject();
	if(obj == NULL)
    return NULL;
	tri = (TriangleData *)Malloc(sizeof(TriangleData));
	if(tri == NULL)
  {
    Ray_DeleteObject(obj);
    return NULL;
  }

	/* Get the vertex points. */
	memcpy(tri->pts, points, sizeof(float) * 9);

	/* Get the optional vertex normals, if any. */
	if(normals != NULL)
		memcpy(tri->norms, normals, sizeof(float) * 9);
	else  /* Zero them so that SetupTriangle() will give them... */
	{     /* triangle's plane normal. */
    tri->norms[0] = tri->norms[1] = tri->norms[2] =
    tri->norms[3] = tri->norms[4] = tri->norms[5] =
  	tri->norms[6] = tri->norms[7] = tri->norms[8] = (float)0.0;
	}

	/* Get the optional vertex UV map points, if any. */
	/* Points are packed as: u1, v1, u2, v2, u3, v3. */
	if(uvpoints != NULL)
		memcpy(tri->uv, uvpoints, sizeof(float) * 6);
	else
	{
		tri->uv[0] = (float)0.0;
		tri->uv[1] = (float)0.0;
		tri->uv[2] = (float)1.0;
		tri->uv[3] = (float)0.0;
		tri->uv[4] = (float)0.0;
		tri->uv[5] = (float)1.0;
	}

	/* Some assembly required... */
	SetupTriangle(tri);

  /* Don't shadow test against self. */
  obj->flags |= OBJ_FLAG_NO_SELF_INTERSECT;
 	obj->data.triangle = tri;
	obj->procs = &triangle_procs;

	return obj;
}


int IntersectTriangle(Object *obj, HitData *hits)
{
  TriangleData *tri = obj->data.triangle;
	double d, t, u0, v0, u1, v1, u2, v2, a, b;
	Vec3 P;
	float *P1, *P2, *P3;

	ray_triangle_tests++;

	P1 = tri->pts;
	P2 = &tri->pts[3];
	P3 = &tri->pts[6];

	/* Intersect ray with plane containing triangle. */
	P.x = ct.B.x - P1[0];
	P.y = ct.B.y - P1[1];
	P.z = ct.B.z - P1[2];

	d = tri->pnorm[0] * ct.D.x +
			tri->pnorm[1] * ct.D.y +
			tri->pnorm[2] * ct.D.z;

	/* ray is parallel to plane if d == 0 */
	if(fabs(d) < EPSILON)
		return 0;

	t = -(tri->pnorm[0] * P.x +
				tri->pnorm[1] * P.y +
				tri->pnorm[2] * P.z) / d;

	/* 0 (miss) if behind viewer or too small. */
	if(t < ct.tmin || t > ct.tmax)
		return 0;

	hits->entering = (d < 0.0) ? 1 : 0;

	/*
	 * Project the triangle vertices to the 2D plane
	 * that is most perpendicular to the plane normal.
	 * This has already been pre-computed and the result
	 * is stored in "tri->axis".
	 */
	switch(tri->axis)
	{
		case X_AXIS:
			u0 = P.y + ct.D.y * t; v0 = P.z + ct.D.z * t;
			u1 = P2[1] - P1[1]; v1 = P2[2] - P1[2];
			u2 = P3[1] - P1[1]; v2 = P3[2] - P1[2];
			break;
		case Y_AXIS:
			u0 = P.x + ct.D.x * t; v0 = P.z + ct.D.z * t;
			u1 = P2[0] - P1[0]; v1 = P2[2] - P1[2];
			u2 = P3[0] - P1[0]; v2 = P3[2] - P1[2];
			break;
		default:  /* Z_AXIS */
			u0 = P.x + ct.D.x * t; v0 = P.y + ct.D.y * t;
			u1 = P2[0] - P1[0]; v1 = P2[1] - P1[1];
			u2 = P3[0] - P1[0]; v2 = P3[1] - P1[1];
			break;
	}

	a = -1.0;
	if(fabs(u1) < EPSILON)
	{
		b = u0 / u2;
		if((b >= 0.0) && (b <= 1.0))
			a = (v0 - b * v2) / v1;
	}
	else
	{
		b = (v0 * u1 - u0 * v1) / (v2 * u1 - u2 * v1);
		if((b >= 0.0) && (b <= 1.0))
			a = (u0 - b * u2) / u1;
	}

	if((a >= 0.0) && (b >= 0.0) && ((a + b) <= 1.0))
	{
		hits->t = t;
		hits->obj = obj;
		ray_triangle_hits++;
		return 1;
	}
	return 0;
}


void CalcNormalTriangle(Object *obj, Vec3 *P, Vec3 *N)
{
	TriangleData *tri = obj->data.triangle;
	double area, a1, a2, a3, u, v;
	float *P1, *P2, *P3;
	float *N1, *N2, *N3;
	int A, B;

	/*
	 * Project the triangle vertices to the 2D plane
	 * that is most perpendicular to the plane normal.
	 */
	switch(tri->axis)
	{
		case X_AXIS:
			A = 1;
			B = 2;
			u = P->y;
			v = P->z;
			break;
		case Y_AXIS:
			A = 0;
			B = 2;
			u = P->x;
			v = P->z;
			break;
		default:  /* Z_AXIS */
			A = 0;
			B = 1;
			u = P->x;
			v = P->y;
			break;
	}

	P1 = tri->pts;
	P2 = &tri->pts[3];
	P3 = &tri->pts[6];

	/* Determine barycentric coordinates of point... */
	area = TriangleArea(P1[A], P1[B], P2[A], P2[B], P3[A], P3[B]);
	a1 =   TriangleArea(u, v, P2[A], P2[B], P3[A], P3[B]) / area;
	a2 =   TriangleArea(P1[A], P1[B], u, v, P3[A], P3[B]) / area;
	a3 =   1.0 - a1 - a2;

	/*
	 * ...and use them to interpolate between the vertex normals
	 * to get the real normal.
	 */
	N1 = tri->norms;
	N2 = &tri->norms[3];
	N3 = &tri->norms[6];
	N->x = N1[0] * a1 + N2[0] * a2 + N3[0] * a3;
	N->y = N1[1] * a1 + N2[1] * a2 + N3[1] * a3;
	N->z = N1[2] * a1 + N2[2] * a2 + N3[2] * a3;
	V3Normalize(N);
}


void CalcUVMapTriangle(Object *obj, Vec3 *P, double *u, double *v)
{
	TriangleData *tri = obj->data.triangle;
	double area, a1, a2, a3, pu, pv;
	float *P1, *P2, *P3;
	int A, B;

	/*
	 * Project the triangle vertices to the 2D plane
	 * that is most perpendicular to the plane normal.
	 */
	switch(tri->axis)
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
		default:  /* Z_AXIS */
			A = 0;
			B = 1;
			pu = P->x;
			pv = P->y;
			break;
	}

	P1 = tri->pts;
	P2 = &tri->pts[3];
	P3 = &tri->pts[6];

	/* Determine barycentric coordinates of point... */
	area = TriangleArea(P1[A], P1[B], P2[A], P2[B], P3[A], P3[B]);
	a1 =   TriangleArea(pu, pv, P2[A], P2[B], P3[A], P3[B]) / area;
	a2 =   TriangleArea(P1[A], P1[B], pu, pv, P3[A], P3[B]) / area;
	a3 =   1.0 - a1 - a2;

	/*
	 * ...and use them to interpolate between the UV coordinates
	 * for each vertex to get actual UV position.
	 */
	*u = tri->uv[0] * a1 + tri->uv[2] * a2 + tri->uv[4] * a3;
	*v = tri->uv[1] * a1 + tri->uv[3] * a2 + tri->uv[5] * a3;
}



int IsInsideTriangle(Object *obj, Vec3 *P)
{
	TriangleData *tri = obj->data.triangle;
	Vec3 Pt, N;

	Pt.x = P->x - tri->pts[0];
	Pt.y = P->y - tri->pts[1];
	Pt.z = P->z - tri->pts[2];
	N.x = tri->pnorm[0];
	N.y = tri->pnorm[1];
	N.z = tri->pnorm[2];

	if(V3Dot(&Pt, &N) > 0.0 )
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */
	return ( ! (obj->flags & OBJ_FLAG_INVERSE)); /* inside */
}


void CalcExtentsTriangle(Object *obj, Vec3 *omin, Vec3 *omax)
{
	TriangleData *tri = obj->data.triangle;

	omin->x = fmin(fmin(tri->pts[0],tri->pts[3]),tri->pts[6])-EPSILON;
	omax->x = fmax(fmax(tri->pts[0],tri->pts[3]),tri->pts[6])-EPSILON;
	omin->y = fmin(fmin(tri->pts[1],tri->pts[4]),tri->pts[7])-EPSILON;
	omax->y = fmax(fmax(tri->pts[1],tri->pts[4]),tri->pts[7])+EPSILON;
	omin->z = fmin(fmin(tri->pts[2],tri->pts[5]),tri->pts[8])+EPSILON;
	omax->z = fmax(fmax(tri->pts[2],tri->pts[5]),tri->pts[8])+EPSILON;
}


void TransformTriangle(Object *obj, Vec3 *params, int type)
{
  TriangleData *tri = obj->data.triangle;
  Vec3 V;
  int i;

  for(i = 0; i < 9; i += 3)
  {
  	V.x = tri->pts[i];
	  V.y = tri->pts[i+1];
  	V.z = tri->pts[i+2];
	  XformVector(&V, params, type);
  	tri->pts[i] = (float)V.x;
	  tri->pts[i+1] = (float)V.y;
  	tri->pts[i+2] = (float)V.z;
		V.x = tri->norms[i];
		V.y = tri->norms[i+1];
		V.z = tri->norms[i+2];
		XformNormal(&V, params, type);
		tri->norms[i] = (float)V.x;
		tri->norms[i+1] = (float)V.y;
  	tri->norms[i+2] = (float)V.z;
  }
	SetupTriangle(tri);
}


void CopyTriangle(Object *destobj, Object *srcobj)
{
	if((destobj->data.triangle = (TriangleData *)Malloc(sizeof(TriangleData)))
    != NULL)
	  memcpy(destobj->data.triangle, srcobj->data.triangle, sizeof(TriangleData));
}


void DeleteTriangle(Object *obj)
{
	Free(obj->data.triangle, sizeof(TriangleData));
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

void DrawTriangle(Object *obj)
{
	TriangleData *tri = obj->data.triangle;
	float *vtx = tri->pts;
	Set_Pt(0, vtx[0], vtx[1], vtx[2]);
	vtx += 3;
	Set_Pt(1, vtx[0], vtx[1], vtx[2]);
	vtx += 3;
	Set_Pt(2, vtx[0], vtx[1], vtx[2]);
	Move_To(0); Line_To(1); Line_To(2); Line_To(0);
}
