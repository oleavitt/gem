/*****************************************************************************
*
*  disc.c
*  The disc primitive and its functions.
*
*****************************************************************************/

#include "ray.h"

static int IntersectDisc(Object *obj, HitData *hits);
static void CalcNormalDisc(Object *obj, Vec3 *Q, Vec3 *N);
static int IsInsideDisc(Object *obj, Vec3 *P);
static void CalcUVMapDisc(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsDisc(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformDisc(Object *obj, Vec3 *params, int type);
static void CopyDisc(Object *destobj, Object *srcobj);
static void DeleteDisc(Object *obj);
static void DrawDisc(Object *obj);

unsigned long ray_disc_tests;
unsigned long ray_disc_hits;

static ObjectProcs disc_procs =
{
	OBJ_DISC,
	IntersectDisc,
	CalcNormalDisc,
	IsInsideDisc,
	CalcUVMapDisc,
	CalcExtentsDisc,
	TransformDisc,
	CopyDisc,
	DeleteDisc,
	DrawDisc
};

void Ray_SetDisc( DiscData *disc, Vec3 *loc, Vec3 *norm, double or, double ir )
{
	V3Copy( &disc->loc, loc );
	V3Copy( &disc->norm, norm );
	V3Normalize( &disc->norm );

	/* Precompute "d" part of disc's plane equation. */
	disc->d = - V3Dot( &disc->norm, &disc->loc );

	/* Some assembly required... */
	disc->inrsq = fmin( ir, or );
	disc->outrsq = fmax( ir, or );
	disc->inrsq = disc->inrsq * disc->inrsq;
	disc->outrsq = disc->outrsq * disc->outrsq;
}

Object *Ray_MakeDisc( Vec3 *loc, Vec3 *norm, double or, double ir )
{
	Object *obj = NewObject( );
	if ( obj != NULL )
	{
		DiscData *disc = (DiscData *) Malloc( sizeof( DiscData ) );
		if ( disc != NULL )
		{
			Ray_SetDisc( disc, loc, norm, or, ir );

			obj->data.disc = disc;
			obj->procs = &disc_procs;

			/* Don't shadow test against self. */
			obj->flags |= OBJ_FLAG_NO_SELF_INTERSECT;
		}
		else
			obj = Ray_DeleteObject( obj );
	}
	return obj;
}


int IntersectDisc(Object *obj, HitData *hits)
{
  DiscData *disc = obj->data.disc;
  double denom, x, y, z, d, t;
  Vec3 B, D;

  ray_disc_tests++;

  V3Copy(&B, &ct.B);
  V3Copy(&D, &ct.D);
  if(obj->T != NULL)
  {
    PointToObject(&B, obj->T);
    DirToObject(&D, obj->T);
  }

  denom = V3Dot(&disc->norm, &D);

  /* ray is parallel to plane if denom == 0 */
  if(fabs(denom) < EPSILON)
    return 0;

  t = - (V3Dot(&disc->norm, &B) + disc->d) / denom;

  /* 0 (miss) if behind viewer or too small. */
  if(t < ct.tmin || t > ct.tmax)
    return 0;

  /* Get 3D point at ray intersection on plane. */
  x = (B.x + D.x * t) - disc->loc.x;
  y = (B.y + D.y * t) - disc->loc.y;
  z = (B.z + D.z * t) - disc->loc.z;

  /* See if point is within the clipping radii. */
  d = x*x + y*y + z*z;
  if(d > disc->outrsq || d < disc->inrsq)
    return 0;

  hits->t = t;
  hits->entering = (denom < 0.0) ? 1 : 0;
  if(obj->flags & OBJ_FLAG_INVERSE)
    hits->entering = 1 - hits->entering;
  hits->obj = obj;
  ray_disc_hits++;
  return 1;
}


void CalcNormalDisc(Object *obj, Vec3 *Q, Vec3 *N)
{
	V3Copy(N, &obj->data.disc->norm);
	if(obj->T != NULL)
	{
		NormToWorld(N, obj->T);
		V3Normalize(N);
	}

	/* Not used */
	//Q;
}


void CalcUVMapDisc(Object *obj, Vec3 *P, double *u, double *v)
{
  DiscData *d = obj->data.disc;
  Vec3 PO;
  double r;

  V3Copy(&PO, P);

  PO.x -= d->loc.x;
  PO.y -= d->loc.y;
  PO.z -= d->loc.z;
  if(fabs(PO.x) < EPSILON && fabs(PO.z) < EPSILON)
  {
    *u = 0.5;
    *v = 0.5;
    return;
  }

  r = sqrt(d->outrsq - PO.z * PO.z) * 2.0;
  if(fabs(r) > EPSILON)
    *u = PO.x / r + 0.5;
  else
    *u =(PO.x > 0.0) ? 1.0 : 0.0;

  r = sqrt(d->outrsq - PO.x * PO.x) * 2.0;
  if(fabs(r) > EPSILON)
    *v = 0.5 - PO.z / r;
  else
    *v = (PO.z > 0.0) ? 0.0 : 1.0;
}


/*
void CalcUVMapDisc(Object *obj, Vec3 *P, double *u, double *v)
{
  DiscData *d;
  Vec3 PO;
  double dist, pdist, du, dv, uu, vv;

  d = obj->data.disc;

  V3Copy(&PO, P);

  PO.x -= d->loc.x;
  PO.y -= d->loc.y;
  PO.z -= d->loc.z;
  if(fabs(PO.x) < EPSILON && fabs(PO.z) < EPSILON)
  {
    *u = 0.5;
    *v = 0.5;
    return;
  }
  dist = sqrt(d->outrsq);
  pdist = sqrt(PO.x * PO.x + PO.z * PO.z);
  du = PO.x / pdist;
  dv = PO.z / pdist;
  PO.x /= dist;
  PO.z /= dist;
  pdist /= dist;
  if(fabs(PO.x) > fabs(PO.z))
  {
    uu = (PO.x > 0.0) ? 1.0 : -1.0;
    vv = asin(dv) / (PI * 0.25);
    pdist *= sqrt(fabs(PO.x));
  }
  else
  {
    uu = asin(du) / (PI * 0.25);
    vv = (PO.z > 0.0) ? 1.0 : -1.0;
    pdist *= sqrt(fabs(PO.z));
  }
  uu *= pdist;
  vv *= pdist;
  *u = (1.0 + LERP(pdist, PO.x, uu)) * 0.5;
  *v = (1.0 - LERP(pdist, PO.z, vv)) * 0.5;
}
*/

int IsInsideDisc(Object *obj, Vec3 *Q)
{
  DiscData *disc = obj->data.disc;
  Vec3 P;

  V3Copy(&P, Q);
  if(obj->T != NULL)
    PointToObject(&P, obj->T);

  if(V3Dot(&P, &disc->norm) + disc->d > 0.0)
    return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */
  return (!(obj->flags & OBJ_FLAG_INVERSE)); /* inside */
}

void CalcExtentsDisc(Object *obj, Vec3 *omin, Vec3 *omax)
{
  DiscData *disc = obj->data.disc;
  double r, d;

  r = sqrt(disc->outrsq);
  d = fabs(sin(acos(disc->norm.x))) * r + EPSILON;
  omin->x = disc->loc.x - d;
  omax->x = disc->loc.x + d;
  d = fabs(sin(acos(disc->norm.y))) * r + EPSILON;
  omin->y = disc->loc.y - d;
  omax->y = disc->loc.y + d;
  d = fabs(sin(acos(disc->norm.z))) * r + EPSILON;
  omin->z = disc->loc.z - d;
  omax->z = disc->loc.z + d;

  if(obj->T != NULL)
    BBoxToWorld(omin, omax, obj->T);
}


void TransformDisc(Object *obj, Vec3 *params, int type)
{
	if (obj->T == NULL)
		obj->T = Ray_NewXform();
	XformXforms(obj->T, params, type);

#if 0
	if(obj->T == NULL)
	{
		DiscData *d = obj->data.disc;

		if(obj->flags & OBJ_FLAG_UV)   /* UV mapping needs the matrix. */
		{
			obj->T = Ray_NewXform();
			XformXforms(obj->T, params, type);
		}
		else
		{
			switch(type)
			{
				case XFORM_SCALE:
					if(V3IsIsotropic(params))
					{
						double rad = sqrt(d->inrsq);
						rad *= params->x;
						d->inrsq = rad * rad;
						rad = sqrt(d->outrsq);
						rad *= params->x;
						d->outrsq = rad * rad;
						XformVector(&d->loc, params, type);
						/* Recompute the "d" part of plane equation. */
						d->d = - V3Dot(&d->norm, &d->loc);
					}
					else
					{
						obj->T = Ray_NewXform();
						XformXforms(obj->T, params, type);
					}
					break;
				case XFORM_SHEAR:
					obj->T = Ray_NewXform();
					XformXforms(obj->T, params, type);
					break;
				default:
					XformVector(&d->loc, params, type);
					XformNormal(&d->norm, params, type);
					/* Recompute the "d" part of plane equation. */
					d->d = - V3Dot(&d->norm, &d->loc);
					break;
			}
		}
	}
	else
		XformXforms(obj->T, params, type);
#endif
}


void CopyDisc(Object *destobj, Object *srcobj)
{
	if((destobj->data.disc = (DiscData *)Malloc(sizeof(DiscData))) != NULL)
	  memcpy(destobj->data.disc, srcobj->data.disc, sizeof(DiscData));
}


void DeleteDisc(Object *obj)
{
  Free(obj->data.disc, sizeof(DiscData));
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

void DrawDisc(Object *obj)
{
	DiscData *d = obj->data.disc;
	int i;
	Vec3 P, P2, P3;
	double r;
	#define NUM_STEPS  12

	V3Set(&P, 1.0, 0.0, 0.0);
	V3Cross(&P2, &P, &d->norm);
	if(ISZERO(P2.x) && ISZERO(P2.y) && ISZERO(P2.z))
	{
		V3Set(&P, 0.0, 1.0, 0.0);
		V3Cross(&P2, &P, &d->norm);
	}
	V3Normalize(&P2);
	V3Copy(&P3, &P2);
	r = sqrt(d->outrsq);
	P2.x *= r; P2.y *= r; P2.z *= r;
	for(i = 0; i < NUM_STEPS; i++)
	{
		P.x = P2.x + d->loc.x; P.y = P2.y + d->loc.y; P.z = P2.z + d->loc.z;
		if(obj->T != NULL)
			PointToWorld(&P, obj->T);
		Set_Pt(i, P.x, P.y, P.z);
		RotatePoint3D(&P2, 30.0*DTOR, &d->norm);
	}

	Move_To(0);
	Line_To(1);
	Line_To(2);
	Line_To(3);
	Line_To(4);
	Line_To(5);
	Line_To(6);
	Line_To(7);
	Line_To(8);
	Line_To(9);
	Line_To(10);
	Line_To(11);
	Line_To(0);

	if(d->inrsq > EPSILON)
	{
		r = sqrt(d->inrsq);
		P3.x *= r; P3.y *= r; P3.z *= r;
		for(i = 0; i < NUM_STEPS; i++)
		{
			P.x = P3.x + d->loc.x; P.y = P3.y + d->loc.y; P.z = P3.z + d->loc.z;
			if(obj->T != NULL)
				PointToWorld(&P, obj->T);
			Set_Pt(i, P.x, P.y, P.z);
			RotatePoint3D(&P3, 30.0*DTOR, &d->norm);
		}
		Move_To(0);
		Line_To(1);
		Line_To(2);
		Line_To(3);
		Line_To(4);
		Line_To(5);
		Line_To(6);
		Line_To(7);
		Line_To(8);
		Line_To(9);
		Line_To(10);
		Line_To(11);
		Line_To(0);
	}
}
