/*****************************************************************************
*
*  TORUS.C
*  The torus primitive and its functions.
*  Torus is on the XZ plane for the purpose of simplified calculations.
*
*****************************************************************************/

#include "ray.h"

static int IntersectTorus(Object *obj, HitData *hits);
static void CalcNormalTorus(Object *obj, Vec3 *P, Vec3 *N);
static int IsInsideTorus(Object *obj, Vec3 *P);
static void CalcUVMapTorus(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsTorus(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformTorus(Object *obj, Vec3 *params, int type);
static void CopyTorus(Object *destobj, Object *srcobj);
static void DeleteTorus(Object *obj);
static void DrawTorus(Object *obj);

unsigned long ray_torus_tests;
unsigned long ray_torus_hits;

static ObjectProcs torus_procs =
{
	OBJ_TORUS,
	IntersectTorus,
	CalcNormalTorus,
	IsInsideTorus,
  CalcUVMapTorus,
  CalcExtentsTorus,
  TransformTorus,
  CopyTorus,
	DeleteTorus,
	DrawTorus
};


Object *Ray_MakeTorus(Vec3 *loc, double rmajor, double rminor)
{
	Object *obj = NewObject();
	if (obj != NULL)
	{
		TorusData *tor = (TorusData *)Malloc(sizeof(TorusData));
		if (tor != NULL)
		{
			Ray_SetTorus(tor, loc, rmajor, rminor);
			obj->data.torus = tor;
			obj->procs = &torus_procs;
		}
		else
			obj = Ray_DeleteObject(obj);
	}

	return obj;
}


void Ray_SetTorus(TorusData *torus, Vec3 *loc, double rmajor, double rminor)
{
	V3Copy(&torus->loc, loc);
	torus->R = (float)rmajor;
	torus->r = (float)rminor;

	/* Some assembly required... */
	if (torus->R < EPSILON)
		torus->R = (float)EPSILON;
	torus->R2 = torus->R * torus->R;  /* Precompute major radius squared. */
	torus->r2 = torus->r * torus->r;  /* Precompute minor radius squared. */
	/* Precompute (R^2 - r^2) part of the torus eq. */
	torus->dr = torus->R2 - torus->r2;
	/* For the bounding sphere test. */
	torus->bound_rsq = (torus->R + torus->r) * (torus->R + torus->r);
}


int IntersectTorus(Object *obj, HitData *hits)
{
	TorusData *tor;
	double c[5];  /* Coeffs for quartic in "t". */
	double t[4];  /* The roots (up to 4 "t" values). */
	double ox, oy, oz ,dx, dy, dz;  /* Transformed ray origin & direction. */
	double c0, c1, c2, c3; /* Constants for the "t" equation. */
	int nhits;
	Vec3 B, D;
	double ray_scale;

	ray_torus_tests++;

	tor = obj->data.torus;

	/* Transform ray in to the torus' coordinate system... */
	V3Copy(&B, &ct.B);
	V3Copy(&D, &ct.D);
	if (obj->T != NULL)
	{
		PointToObject(&B, obj->T);
		DirToObject(&D, obj->T);
		ray_scale = V3Mag(&D);
		if (ray_scale < EPSILON)
			return 0;
		D.x /= ray_scale;
		D.y /= ray_scale;
		D.z /= ray_scale;
	}
	else
		ray_scale = 1;

	ox = B.x - tor->loc.x;
	oy = B.y - tor->loc.y;
	oz = B.z - tor->loc.z;
	dx = D.x;
	dy = D.y;
	dz = D.z;

	/* Precompute some constants for the "t" equations... */
	c0 = dx * ox + dy * oy;
	c1 = c0 + dz * oz;
	c2 = ox * ox + oy * oy;
	c3 = c2 + oz * oz;

	/* Check bounds... */
	{
		double t1, t2, b, c, d, z1, z2;

		/* Does ray hit sphere containing torus? */
		b = 2.0 * c1;
		c = c3 - tor->bound_rsq;
		d = b * b - c * 4.0;
		if(d < EPSILON) return 0;
		d = sqrt(d);
		t1 = (-b + d) / 2.0;
		t2 = (-b - d) / 2.0;
		if((t1 < ct.tmin && t2 < ct.tmin) ||
			 (t1 > ct.tmax && t2 > ct.tmax))
			 return 0;

		/*
		 * Do hits on sphere straddle one or both planes containing torus?
		 * If not return a miss.
		 */
		z1 = oz + t1 * dz;
		z2 = oz + t2 * dz;
		if((z1 > tor->r && z2 > tor->r) ||
			 (z1 < -tor->r && z2 < -tor->r))
			return 0;

		/*
		 * If the minor radius is less than the major radius, the torus
		 * has a hole. Get ray intersections with the plane pair and see
		 * if both hits are within the inner diameter of the hole.
		 * If so, return a miss.
		 */
		if (tor->r < tor->R && dz > EPSILON)
		{
			double x, y, rsq;

			t1 = (tor->r - oz) / dz;
			x = ox + t1 * dx;
			y = oy + t1 * dy;
			rsq = tor->R - tor->r;
			rsq = rsq * rsq;
			if((x*x + y*y) < rsq)
			{
				t2 = (-tor->r - oz) / dz;
				x = ox + t2 * dx;
				y = oy + t2 * dy;
				if((x*x + y*y) < rsq)
        	return 0;
			}
		}
	}

	/*
	 * Note: All of the (dx^2 + dy^2 + dz^2) parts of the "t" equation are
	 * equal to one because the ray direction vector is normalized.
	 */
	c3 += tor->dr; /* Add on the pre-computed (R^2 - r^2) constant. */

	c[4] = 1.0;
	c[3] = 4.0 * c1;
	c[2] = 2.0 * c3 + 4.0 * (c1 * c1 - tor->R2 * (1.0 - dz * dz));
	c[1] = 4.0 * c1 * c3 - 8.0 * tor->R2 * c0;
	c[0] = c3 * c3 - 4.0 * tor->R2 * c2;

	nhits = SolvePoly(c, t, 4, ct.tmin, ct.tmax);
	if (nhits > 0)
	{
		int i;

		for (i = 0; i < nhits; i++)
		{
			if (i > 0)
				hits = GetNextHit(hits);
			hits->t = t[i] / ray_scale;
			hits->obj = obj;
			hits->entering = (((nhits - i) & 1) == 0);
		}
		ray_torus_hits++;
		return nhits;
	}

	return 0;
}


void CalcNormalTorus(Object *obj, Vec3 *P, Vec3 *N)
{
	TorusData *tor;
	double px, py, pz, a;
	Vec3 P2;

	tor = obj->data.torus;

	V3Copy(&P2, P);
	if(obj->T != NULL)
		PointToObject(&P2, obj->T);

	px = P2.x - tor->loc.x;
	py = P2.y - tor->loc.y;
	pz = P2.z - tor->loc.z;

	a = px * px + py * py + pz * pz + tor->R2 - tor->r2;

	N->z = pz * a;
	N->x = px * (a -= 2.0 * tor->R2);
	N->y = py * a;

	if(obj->T != NULL)
		NormToWorld(N, obj->T);
	V3Normalize(N);
}

/*
void CalcUVMapTorus(Object *obj, Vec3 *P, double *u, double *v)
{
	TorusData *tor;
	Vec3 P2;
	double rad;

	tor = obj->data.torus;

	V3Copy(&P2, P);
	if(obj->T != NULL)
		PointToObject(&P2, obj->T);

	V3Sub(&P2, &P2, &tor->loc);
	rad = sqrt(P2.x * P2.x + P2.y * P2.y);
	P2.y /= rad;

	if(fabs(P2.y ) > 1.0)
		*u = (P2.y < 0.0) ? 0.5 : 0.0;
	else
		*u = acos(P2.y) / TWOPI;
	if(P2.x > 0.0)
		*u = 1.0 - *u;

	P2.z /= tor->r;
	if(fabs(P2.z) < 1.0)
		*v = asin(P2.z) / TWOPI;
	else
		*v = (P2.z < 0.0) ? -0.25 : 0.25;

	if(rad > tor->R)
		*v = 0.5 - *v;
	else if(P2.z < 0.0)
		*v += 1.0;
}
*/

void CalcUVMapTorus(Object *obj, Vec3 *P, double *u, double *v)
{
	TorusData *tor;
	Vec3 P2;
  double x, y, z, rx, ry, r;

	tor = obj->data.torus;

	V3Sub(&P2, P, &tor->loc);
  x = rx = P2.x;
  y = ry = P2.y;
  z = P2.z;
  r = sqrt(x * x + y * y);
  if(r > EPSILON)
  {
	  rx /= r;
    ry /= r;
    if(fabs(ry) > 1.0)
      ry = (ry < 0.0) ? -1.0 : 1.0;
    *u = acos(ry) / TWOPI;
    if(rx > 0.0)
      *u = 1.0 - *u;
		x = P2.x - rx * tor->R;
		y = P2.y - ry * tor->R;
    r = tor->r;
	  x /= r;
		y /= r;
		r = -(x * rx + y * ry);
    if(fabs(r) > 1.0)
      r = (r < 0.0) ? -1.0 : 1.0;
		*v = acos(r) / TWOPI;
    if(z > 0.0)
      *v = 1.0 - *v;
  }
  else
  {
    *u = 0.0;
		*v = 0.0;
  }
}


int IsInsideTorus(Object *obj, Vec3 *P)
{
	TorusData *tor;
	double px, py, pz, dist, zdist, ring;
	Vec3 P2;

	tor = obj->data.torus;

	V3Copy(&P2, P);
	if(obj->T != NULL)
		PointToObject(&P2, obj->T);

	px = P2.x - tor->loc.x;
	py = P2.y - tor->loc.y;
	pz = P2.z - tor->loc.z;

	zdist = px * px + py * py;
	dist = zdist + pz * pz;
	ring = dist + tor->dr;

	if(ring * ring - 4.0 * tor->R2 * zdist > 0.0)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */
	return (!(obj->flags & OBJ_FLAG_INVERSE)); /* inside */
}


void CalcExtentsTorus(Object *obj, Vec3 *omin, Vec3 *omax)
{
	TorusData *tor = obj->data.torus;

	omin->x = tor->loc.x - tor->R - tor->r - EPSILON;
	omax->x = tor->loc.x + tor->R + tor->r + EPSILON;
	omin->y = tor->loc.y - tor->R - tor->r - EPSILON;
	omax->y = tor->loc.y + tor->R + tor->r + EPSILON;
	omin->z = tor->loc.z - tor->r - EPSILON;
	omax->z = tor->loc.z + tor->r + EPSILON;

	if(obj->T != NULL)
		BBoxToWorld(omin, omax, obj->T);
}


void TransformTorus(Object *obj, Vec3 *params, int type)
{
	if (obj->T == NULL)
		obj->T = Ray_NewXform();
	XformXforms(obj->T, params, type);

#if 0
	if(obj->T == NULL)
	{
		TorusData *tor = obj->data.torus;

		if(type == XFORM_SCALE)
		{
			if(V3IsIsotropic(params))
			{
				XformVector(&tor->loc, params, type);
				tor->R *= (float)params->x;
				tor->r *= (float)params->x;
				/* Recompute the fields affected by scaled radii. */
				tor->R2 = tor->R * tor->R;
				tor->r2 = tor->r * tor->r;
				tor->dr = tor->R2 - tor->r2;
				tor->bound_rsq = (tor->R + tor->r) * (tor->R + tor->r);
			}
			else
			{
				obj->T = Ray_NewXform();
				XformXforms(obj->T, params, type);
			}
		}
		else if(type == XFORM_TRANSLATE)
		{
			XformVector(&tor->loc, params, type);
		}
		else
		{
			obj->T = Ray_NewXform();
			XformXforms(obj->T, params, type);
		}
	}
	else
	{
		XformXforms(obj->T, params, type);
	}
#endif
}


void CopyTorus(Object *destobj, Object *srcobj)
{
	destobj->data.torus = (TorusData *)Malloc(sizeof(TorusData));
	memcpy(destobj->data.torus, srcobj->data.torus, sizeof(TorusData));
}


void DeleteTorus(Object *obj)
{
	Free(obj->data.torus, sizeof(TorusData));
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

#define SIN30 0.5
#define COS30 0.8660254

static double stbl[12] =
	{ 0.0, SIN30, COS30, 1.0, COS30, SIN30, 
	  0.0, -SIN30, -COS30, -1.0, -COS30, -SIN30 };
static double ctbl[12] =
	{ 1.0, COS30, SIN30, 0.0, -SIN30, -COS30,
	  -1.0, -COS30, -SIN30, 0.0, SIN30, COS30 };
static double xpts[12], zpts[12];

void DrawTorus(Object *obj)
{
	TorusData *tor = obj->data.torus;
	Vec3 P;
	int i, j, n, n2;

	n = 0;
	for(i = 0; i < 12; i++)
	{
		xpts[i] = ctbl[i] * tor->r + tor->R;
		zpts[i] = stbl[i] * tor->r;
		V3Set(&P, xpts[i] + tor->loc.x, tor->loc.y, zpts[i] + tor->loc.z);
		if(obj->T != NULL)
			PointToWorld(&P, obj->T);
		Set_Pt(n, P.x, P.y, P.z);
		n++;
	}
	for(i = 1; i < 12; i++)
	{
		for(j = 0; j < 12; j++)
		{
			V3Set(&P,
				xpts[j] * ctbl[i] + tor->loc.x,
				xpts[j] * stbl[i] + tor->loc.y,
				zpts[j] + tor->loc.z);
			if(obj->T != NULL)
				PointToWorld(&P, obj->T);
			Set_Pt(n, P.x, P.y, P.z);
			n++;
		}
	}

	n = 0;
	n2 = 12;
	for(i = 0; i < 12; i++)
	{
		if(i == 11)
			n2 = 0;
		Move_To(n);
		for(j = 0; j < 6; j++)
		{
			Line_To(n2);
			n++; n2++;
			Line_To(n2); Line_To(n);
			n++; n2++;
			Line_To((j == 5) ? n - 12 : n);
		}
	}
}
