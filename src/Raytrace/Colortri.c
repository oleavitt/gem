/*************************************************************************
*
*  colortri.c
*
*  The colored triangle primitive.
*
*************************************************************************/

#include "ray.h"

static int IntersectColorTriangle(Object *obj, HitData *hits);
static void CalcNormalColorTriangle(Object *obj, Vec3 *P, Vec3 *N);
static int IsInsideColorTriangle(Object *obj, Vec3 *P);
static void CalcUVMapColorTriangle(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsColorTriangle(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformColorTriangle(Object *obj, Vec3 *params, int type);
static void CopyColorTriangle(Object *destobj, Object *srcobj);
static void DeleteColorTriangle(Object *obj);
static void DrawColorTriangle(Object *obj);

unsigned long ray_colortri_tests;
unsigned long ray_colortri_hits;

static ObjectProcs colortri_procs =
{
	OBJ_COLORTRIANGLE,
	IntersectColorTriangle,
	CalcNormalColorTriangle,
	IsInsideColorTriangle,
  CalcUVMapColorTriangle,
  CalcExtentsColorTriangle,
  TransformColorTriangle,
  CopyColorTriangle,
	DeleteColorTriangle,
	DrawColorTriangle
};


void ColorTriangleGetColor(Object *obj, Vec3 *color)
{
  ColorTriangleData *tri = obj->data.colortri;
  /* Color was computed in CalcNormalColorTriangle(). */
  color->x = tri->r;
  color->y = tri->g;
  color->z = tri->b;
}


void SetupColorTriangle(ColorTriangleData *tri)
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

	V3Cross(&N, &V1, &V2);
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
		else if(fabs(N.y) > fabs(N.z))
			tri->axis = Y_AXIS;
		else
			tri->axis = Z_AXIS;
	}
	else if(fabs(N.y) > fabs(N.z))
		tri->axis = Y_AXIS;
	else
		tri->axis = Z_AXIS;
}


/*
 * Compute the surface area (X 2) of a 2D triangle.
 */
double ColorTriangleArea(double x1, double y1, double x2, double y2,
  double x3, double y3)
{
	return fabs(x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
}

Object *Ray_MakeColorTriangle(float *points, float *normals,
	float *colors)
{
	Object *obj;
	ColorTriangleData *tri;

	obj = NewObject();
	if(obj == NULL)
    return NULL;
	tri = (ColorTriangleData *)Malloc(sizeof(ColorTriangleData));
	if(tri == NULL)
  {
    Ray_DeleteObject(obj);
    return NULL;
  }

	/* Get the vertex points. */
	memcpy(tri->pts, points, sizeof(float) * 9);

	/* Get the vertex colors. */
	memcpy(tri->colors, colors, sizeof(float) * 9);

	/* Get the optional vertex normals, if any. */
	if(normals != NULL)
		memcpy(tri->norms, normals, sizeof(float) * 9);
	else  /* Zero them so that SetupColorTriangle() will give them... */
	{     /* triangle's plane normal. */
    tri->norms[0] = tri->norms[1] = tri->norms[2] =
    tri->norms[3] = tri->norms[4] = tri->norms[5] =
  	tri->norms[6] = tri->norms[7] = tri->norms[8] = (float)0.0;
	}

	tri->uv[0] = (float)0.0;
  tri->uv[1] = (float)0.0;
	tri->uv[2] = (float)1.0;
	tri->uv[3] = (float)0.0;
	tri->uv[4] = (float)0.0;
	tri->uv[5] = (float)1.0;

	/* Some assembly required... */
	SetupColorTriangle(tri);

  /* Don't shadow test against self. */
  obj->flags |= OBJ_FLAG_NO_SELF_INTERSECT;
 	obj->data.colortri = tri;
	obj->procs = &colortri_procs;

	return obj;
}


int IntersectColorTriangle(Object *obj, HitData *hits)
{
  ColorTriangleData *tri = obj->data.colortri;
	double d, t, x, y, x0, y0, x1, y1;
	Vec3 P;
	int i, A, B, in;

	ray_colortri_tests++;

	/* Intersect ray with plane containing triangle. */
	P.x = ct.B.x - tri->pts[0];
	P.y = ct.B.y - tri->pts[1];
	P.z = ct.B.z - tri->pts[2];

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
		default:  /* Z_AXIS */
			A = 0;
			B = 1;
			x = ct.B.x + ct.D.x * t;
			y = ct.B.y + ct.D.y * t;
			break;
	}

	/*
	 * Compute 2D point on the projected plane and
	 * see if it is within the bounds of the projected triangle.
	 */
	in = 0;
	for(i = 0; i < 3; i++)
	{
		x0 = tri->pts[i*3+A];
		y0 = tri->pts[i*3+B];
		if(i < 2)
		{
			x1 = tri->pts[(i+1)*3+A];
			y1 = tri->pts[(i+1)*3+B];
		}
		else
		{
			x1 = tri->pts[A];
			y1 = tri->pts[B];
		}

		if((y0 <= y && y1 <= y) ||
			 (y0 >= y && y1 >= y) ||
			 (x0 <= x && x1 <= x))
      continue;

		if(x0 >= x && x1 >= x)
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
		ray_colortri_hits++;
		return 1;
	}
	return 0;
}


void CalcNormalColorTriangle(Object *obj, Vec3 *P, Vec3 *N)
{
	ColorTriangleData *tri = obj->data.colortri;
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
	area = ColorTriangleArea(P1[A], P1[B], P2[A], P2[B], P3[A], P3[B]);
	a1 =   ColorTriangleArea(u, v, P2[A], P2[B], P3[A], P3[B]) / area;
	a2 =   ColorTriangleArea(P1[A], P1[B], u, v, P3[A], P3[B]) / area;
	a3 =   1.0 - a1 - a2;

	/*
	 * ...and use them to interpolate between the vertex normals
	 * to get the real normal and likewise for the vertex colors.
	 */
	N1 = tri->norms;
	N2 = &tri->norms[3];
	N3 = &tri->norms[6];
	N->x = N1[0] * a1 + N2[0] * a2 + N3[0] * a3;
	N->y = N1[1] * a1 + N2[1] * a2 + N3[1] * a3;
	N->z = N1[2] * a1 + N2[2] * a2 + N3[2] * a3;
	V3Normalize(N);

	N1 = tri->colors;
	N2 = &tri->colors[3];
	N3 = &tri->colors[6];
	tri->r = (float)(N1[0] * a1 + N2[0] * a2 + N3[0] * a3);
	tri->g = (float)(N1[1] * a1 + N2[1] * a2 + N3[1] * a3);
	tri->b = (float)(N1[2] * a1 + N2[2] * a2 + N3[2] * a3);
}


void CalcUVMapColorTriangle(Object *obj, Vec3 *P, double *u, double *v)
{
	ColorTriangleData *tri = obj->data.colortri;
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
	area = ColorTriangleArea(P1[A], P1[B], P2[A], P2[B], P3[A], P3[B]);
	a1 =   ColorTriangleArea(pu, pv, P2[A], P2[B], P3[A], P3[B]) / area;
	a2 =   ColorTriangleArea(P1[A], P1[B], pu, pv, P3[A], P3[B]) / area;
	a3 =   1.0 - a1 - a2;

	/*
	 * ...and use them to interpolate between the UV coordinates
	 * for each vertex to get actual UV position.
	 */
	*u = tri->uv[0] * a1 + tri->uv[2] * a2 + tri->uv[4] * a3;
	*v = tri->uv[1] * a1 + tri->uv[3] * a2 + tri->uv[5] * a3;
}



int IsInsideColorTriangle(Object *obj, Vec3 *P)
{
	ColorTriangleData *tri = obj->data.colortri;
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


void CalcExtentsColorTriangle(Object *obj, Vec3 *omin, Vec3 *omax)
{
	ColorTriangleData *tri = obj->data.colortri;

	omin->x = fmin(fmin(tri->pts[0],tri->pts[3]),tri->pts[6]);
	omax->x = fmax(fmax(tri->pts[0],tri->pts[3]),tri->pts[6]);
	omin->y = fmin(fmin(tri->pts[1],tri->pts[4]),tri->pts[7]);
	omax->y = fmax(fmax(tri->pts[1],tri->pts[4]),tri->pts[7]);
	omin->z = fmin(fmin(tri->pts[2],tri->pts[5]),tri->pts[8]);
	omax->z = fmax(fmax(tri->pts[2],tri->pts[5]),tri->pts[8]);
	omin->x -= EPSILON;
	omin->y -= EPSILON;
	omin->z -= EPSILON;
	omax->x += EPSILON;
	omax->y += EPSILON;
	omax->z += EPSILON;
}


void TransformColorTriangle(Object *obj, Vec3 *params, int type)
{
  ColorTriangleData *tri = obj->data.colortri;
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
	SetupColorTriangle(tri);
}


void CopyColorTriangle(Object *destobj, Object *srcobj)
{
	if((destobj->data.colortri = (ColorTriangleData *)
    Malloc(sizeof(ColorTriangleData))) != NULL)
	  memcpy(destobj->data.colortri, srcobj->data.colortri,
      sizeof(ColorTriangleData));
}


void DeleteColorTriangle(Object *obj)
{
	Free(obj->data.colortri, sizeof(ColorTriangleData));
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

void DrawColorTriangle(Object *obj)
{
  ColorTriangleData *tri = obj->data.colortri;
	float *vtx = tri->pts;
	Set_Pt(0, vtx[0], vtx[1], vtx[2]);
	vtx += 3;
	Set_Pt(1, vtx[0], vtx[1], vtx[2]);
	vtx += 3;
	Set_Pt(2, vtx[0], vtx[1], vtx[2]);
	Move_To(0); Line_To(1); Line_To(2); Line_To(0);
}
