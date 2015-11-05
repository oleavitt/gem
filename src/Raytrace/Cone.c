/*************************************************************************
*
*  cone.c - The cone primitive and its functions.
*
*************************************************************************/

#include "ray.h"

unsigned long ray_cone_tests;
unsigned long ray_cone_hits;

/*************************************************************************
 *  Procs for the cone object type.
 */
static int IntersectCone(Object *obj, HitData *hits);
static void CalcNormalCone(Object *obj, Vec3 *P, Vec3 *N);
static int IsInsideCone(Object *obj, Vec3 *Q);
static void CalcUVMapCone(Object *obj, Vec3 *P, double *u, double *v);
static void TransformCone(Object *obj, Vec3 *params, int type);
static void CalcExtentsCone(Object *obj, Vec3 *omin, Vec3 *omax);
static void CopyCone(Object *destobj, Object *srcobj);
static void DeleteCone(Object *obj);
static void DrawCone(Object *obj);

static ObjectProcs cone_procs =
{
  OBJ_CONE,
	IntersectCone,
	CalcNormalCone,
	IsInsideCone,
  CalcUVMapCone,
  CalcExtentsCone,
  TransformCone,
  CopyCone,
  DeleteCone,
  DrawCone
};

static int IntersectCone(Object *obj, HitData *hits)
{
  ConeData *cone;
  Vec3 P, D;
  double ox, oy, oz, dx, dy, dz;
  double a, b, c, d, dy2, ct1, ct2, pt1, pt2, bpt, ept, ray_scale, t[4];
  int valid_hits, i, entering;

  cone = obj->data.cone;

  ray_cone_tests++;

  /* Transform ray into object space. */
  P = ct.B;
  D = ct.D;
  if(obj->T != NULL)
  {
    PointToObject(&P, obj->T);
    DirToObject(&D, obj->T);
    ray_scale = V3Mag(&D);
    if(ray_scale == 0.0)
      return 0;
    D.x /= ray_scale;
    D.y /= ray_scale;
    D.z /= ray_scale;
  }
  V3Sub(&P, &P, &cone->base_pt);
  ox = V3Dot(&P, &cone->rx);
  oy = V3Dot(&P, &cone->ry);
  oz = V3Dot(&P, &cone->rz);
  dx = V3Dot(&D, &cone->rx);
  dy = V3Dot(&D, &cone->ry);
  dz = V3Dot(&D, &cone->rz);

  /* Substitute the ray equation:
   *     ox + t * dx
   *     oy + t * dy
   *     oz + t * dz
   * for "x", "y", and "z" in the cone equation,
   *     x^2 + z^2 - (r - s * y)^2 = 0 ,
   * to get a quadratic in "t" in standard form,
   *     t^2 * a + t * b + c = 0 ,
   * where:
   *     ox, oy, and oz  is the ray's origin point in 3D object space.
   *     dx, dy, and dz  is the ray's normalized direction vector.
   *     r = cone's base radius.
   *     s = cone's slope.
   * Note that since the ray direction vector is normalized, where
   * dx^2 + dy^2 + dz^2 = 1.0, we can optimize in the "a" part. We
   * can replace the (dx^2 + dz^2) part with (1.0 - dy^2) and save
   * one multiply. Also dy^2 is now used twice so we can pre-compute it
   * (dy2) save one more multiply.
   */
  d = cone->base_rad * cone->slope;
  dy2 = dy * dy;

  a = 1.0 - dy2 - dy2*cone->ssq;
  b = 2.0 * (ox*dx + oz*dz + dy*(d - cone->ssq*oy));
  c = ox*ox + oz*oz + oy*(2.0*d - cone->ssq*oy) - cone->brsq;

  /* Solve for "t"... */
  d = b * b - 4.0 * a * c;
  if(d < 0.0)
    return 0;
  d = sqrt(d);
  ct1 = (-b + d) / (2.0 * a);
  ct2 = (-b - d) / (2.0 * a);

  /*
   * Get the intersections of the planes that bound the cone
   * along the Y axis. Cull out any cone intersections that
   * are outside of the bounding planes. If this is a closed
   * cone, include any plane intersections that are within
   * the cone radii.
   */
  if(fabs(dy) > EPSILON)
  {
    pt1 = - oy / dy;
    pt2 = (cone->height - oy) / dy;
    bpt = pt1;
    ept = pt2;
    if(pt1 > pt2) { a = pt1; pt1 = pt2; pt2 = a; }
    if(pt2 < EPSILON)
      return 0;   /* Planes and valid cone region are behind viewer. */
  }
  else  /* Ray is parallel to Y planes. */
  {
    if(oy < 0.0 || oy > cone->height)
      return 0;   /* Outside of plane pair. */
    bpt = -1.0;
    ept = -1.0;
    pt1 = -HUGE;
    pt2 = HUGE;
  }

  valid_hits = 0;
  if(ct1 > pt1 && ct1 < pt2)
  {
    if(obj->T != NULL)
      ct1 /= ray_scale;
    if(ct1 > ct.tmin && ct1 < ct.tmax)
      t[valid_hits++] = ct1;
  }

  if(ct2 > pt1 && ct2 < pt2)
  {
    if(obj->T != NULL)
      ct2 /= ray_scale;
    if(ct2 > ct.tmin && ct2 < ct.tmax)
      t[valid_hits++] = ct2;
  }

  if(cone->closed)   /* Add in plane intersections. */
  {
    pt1 = bpt;
    if(obj->T != NULL)
      bpt /= ray_scale;
    if(bpt > ct.tmin && bpt < ct.tmax)
    {
      a = ox + pt1 * dx;
      b = oz + pt1 * dz;
      if(( a * a + b * b) < cone->brsq )
        t[valid_hits++] = bpt;
    }
    pt2 = ept;
    if(obj->T != NULL)
      ept /= ray_scale;
    if(ept > ct.tmin && ept < ct.tmax)
    {
      a = ox + pt2 * dx;
      b = oz + pt2 * dz;
      c = cone->end_rad * cone->end_rad;
      if(( a * a + b * b) < c )
        t[valid_hits++] = ept;
    }
  }

  if(valid_hits > 0)
  {
    if(valid_hits > 1)
    {
      register j;

      for(i = valid_hits-1; i >= 1; i--)
        for(j = 1; j <= i; j++)
          if(t[j-1] > t[j])
          {
            a = t[j-1];
            t[j-1] = t[j];
            t[j] = a;
          }
    }
    entering = (valid_hits & 1) ?
      (obj->flags & OBJ_FLAG_INVERSE) :
      !(obj->flags & OBJ_FLAG_INVERSE);
    for(i = 0; i < valid_hits; i++)
    {
      hits->t = t[i];
      hits->obj = obj;
      hits->entering = entering;
      entering = 1 - entering;
      hits = GetNextHit(hits);
    }
    ray_cone_hits++;   /* Smack! */
  }

  return valid_hits;

} /* end of IntersectCone() */


static void CalcNormalCone(Object *obj, Vec3 *Q, Vec3 *N)
{
  ConeData *cone;
  Vec3 P;
  double x, y, z, s;
  int on_side;

  cone = obj->data.cone;

  /* Transform ray into object space. */
  P = *Q;
  if(obj->T != NULL)
    PointToObject(&P, obj->T);
  V3Sub(&P, &P, &cone->base_pt);
  x = V3Dot(&P, &cone->rx);
  y = V3Dot(&P, &cone->ry);
  z = V3Dot(&P, &cone->rz);


  if(cone->closed)  /* See if point is on an end-cap... */
  {
    if(y < EPSILON)
    {
      V3Sub(N, &cone->base_pt, &cone->end_pt);
      on_side = 0;
    }
    else if(fabs(y - cone->height) < EPSILON)
    {
      V3Sub(N, &cone->end_pt, &cone->base_pt);
      on_side = 0;
    }
    else on_side = 1;
  }
  else on_side = 1;


  if(on_side)
  {
    /*
     * Take partial derivatives of the cone equation,
     * x^2 + z^2 - (r - s * y)^2, with respect to
     * "x", "y", and "z", where:
     * s = cone's slope
     * r = cone's base radius
     * fx(x,y,z) = 2.0 * x
     * fy(x,y,z) = 2.0 * s * (r - s * y)
     * fz(x,y,z) = 2.0 * z
     * Since the normal is normalized any scalar scaling is
     * irrelevant, thus, the 2.0 constant can be omitted.
     */
    s = cone->slope;
    P.x = x;
    P.y = s *(cone->base_rad - s * y);
    P.z = z;

    /* Transform normal into world space. */
    N->x = P.x * cone->rx.x + P.y * cone->ry.x + P.z * cone->rz.x;
    N->y = P.x * cone->rx.y + P.y * cone->ry.y + P.z * cone->rz.y;
    N->z = P.x * cone->rx.z + P.y * cone->ry.z + P.z * cone->rz.z;
  }

  /* Transform normal into world space. */
  if(obj->T != NULL)
    NormToWorld(N, obj->T);
  V3Normalize(N);
} /* end of CalcNormalCone() */


int IsInsideCone(Object *obj, Vec3 *Q)
{
  ConeData *cone;
  Vec3 P;
  double x, y, z, a;

  cone = obj->data.cone;

  P = *Q;
  if(obj->T != NULL)
    PointToObject(&P, obj->T);
  V3Sub(&P, &P, &cone->base_pt);
  x = V3Dot(&P, &cone->rx);
  y = V3Dot(&P, &cone->ry);
  z = V3Dot(&P, &cone->rz);

  a = cone->base_rad - y * cone->slope;
  if((x*x + z*z - a*a) > 0.0 || y < 0.0 || y > cone->height)
    return (obj -> flags & OBJ_FLAG_INVERSE); /* not inside */
  return ( ! (obj -> flags & OBJ_FLAG_INVERSE)); /* inside */
} /* end of cone_inside() */


void CalcUVMapCone(Object *obj, Vec3 *P, double *u, double *v)
{
	ConeData *c = obj->data.cone;
	Vec3 P0, P1;
	double rad;

	V3Sub(&P0, P, &c->base_pt );
	P1.x = V3Dot(&P0, &c->rx);
	P1.y = V3Dot(&P0, &c->ry);
	P1.z = V3Dot(&P0, &c->rz);

	*v = P1.y / c->height;

	rad = sqrt(P1.x * P1.x + P1.z * P1.z);

	if(rad > EPSILON)
	{
		P1.z /= rad;

		if(fabs(P1.z) > 1.0)
			P1.z = (P1.z < 0.0) ? -1.0 : 1.0;
		*u = acos(P1.z) / TWOPI;
		if(P1.x > 0.0)
			*u = 1.0 - *u;
	}
  else *u = 0.0;
}


void CalcExtentsCone(Object *obj, Vec3 *omin, Vec3 *omax)
{
  ConeData *cone;
  double a, b, brad, erad;

  cone = obj->data.cone;

  brad = fabs(cone->base_rad);
  erad = fabs(cone->end_rad);

  a = cone->base_pt.x - sin(acos(cone->ry.x)) * brad;
  b = cone->end_pt.x - sin(acos(cone->ry.x)) * erad;
  omin->x = min(a, b);
  a = cone->base_pt.x + sin(acos(cone->ry.x)) * brad;
  b = cone->end_pt.x + sin(acos(cone->ry.x)) * erad;
  omax->x = max(a, b);

  a = cone->base_pt.y - sin(acos(cone->ry.y)) * brad;
  b = cone->end_pt.y - sin(acos(cone->ry.y)) * erad;
  omin->y = min(a, b);
  a = cone->base_pt.y + sin(acos(cone->ry.y)) * brad;
  b = cone->end_pt.y + sin(acos(cone->ry.y)) * erad;
  omax->y = max(a, b);

  a = cone->base_pt.z - sin(acos(cone->ry.z)) * brad;
  b = cone->end_pt.z - sin(acos(cone->ry.z)) * erad;
  omin->z = min(a, b);
  a = cone->base_pt.z + sin(acos(cone->ry.z)) * brad;
  b = cone->end_pt.z + sin(acos(cone->ry.z)) * erad;
  omax->z = max(a, b);

	omin->x -= EPSILON;
	omin->y -= EPSILON;
	omin->z -= EPSILON;
	omax->x += EPSILON;
	omax->y += EPSILON;
	omax->z += EPSILON;

  if(obj->T != NULL)
    BBoxToWorld(omin, omax, obj->T);

} /* end of CalcExtentsCone() */


static void TransformCone(Object *obj, Vec3 *params, int type)
{
  if(obj->T == NULL)
    obj->T = Ray_NewXform();
  XformXforms(obj->T, params, type);
} /* end of TransformCone() */


/*************************************************************************
*
*  CopyCone - Copy cone-specific data from srcobj to destobj.
*  Cone data can be reference copied since there are no dependencies
*  on a single object instance.
*
*************************************************************************/
void CopyCone(Object *destobj, Object *srcobj)
{
	destobj->data.cone = srcobj->data.cone;
  destobj->data.cone->nrefs++;
}


static void DeleteCone(Object *obj)
{
	assert(obj->data.cone->nrefs > 0);
	if (--obj->data.cone->nrefs == 0)
		Free(obj->data.cone, sizeof(ConeData));
}

Object *Ray_MakeCone(
	Vec3		*base,
	Vec3		*end,
	double		base_rad,
	double		end_rad,
	int			closed)
{
	Object *obj;
	ConeData *cone;

	obj = NewObject();
	if (obj == NULL)
		return NULL;

	/* Allocate the cone data structure... */
	cone = (ConeData *)Malloc(sizeof(ConeData));
	if (cone != NULL)
	{
		cone->nrefs = 1;
		Ray_SetCone(cone, base, end, base_rad, end_rad, closed);
		obj->data.cone = cone;
		obj->procs = &cone_procs;
	}
	else
		obj = Ray_DeleteObject(obj);

	return obj;
} /* end of Ray_MakeCone() */

void Ray_SetCone(
	ConeData	*cone,
	Vec3		*base,
	Vec3		*end,
	double		base_rad,
	double		end_rad,
	int			closed)
{
	Vec3 D;
	double height;

	cone->base_pt = *base;
	cone->end_pt = *end;
	cone->base_rad = (float)base_rad;
	cone->end_rad = (float)end_rad;

	/* Some assembly required... */

	/* Precompute height and slope. */
	V3Sub(&D, &cone->end_pt, &cone->base_pt);
	height = V3Mag(&D);
	if (height < EPSILON)
		height = EPSILON;
	cone->slope = (cone->base_rad - cone->end_rad) / (float)height;
	cone->ssq = cone->slope * cone->slope;

	/*
	 * Compute rotation matrix based on direction from base point
	 * to end point.
	 */
	DirToMatrix(&D, &cone->rx, &cone->ry, &cone->rz);

	cone->height = (float)height;
	cone->brsq = cone->base_rad * cone->base_rad;
	cone->closed = closed;
}

/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

void DrawCone(Object *obj)
{
	ConeData *c = obj->data.cone;
	int i;
	Vec3 P, P2, P3, D;
	#define NUM_STEPS  12

  V3Sub(&D, &c->end_pt, &c->base_pt);
	D.x /= c->height;
	D.y /= c->height;
	D.z /= c->height;
	V3Set(&P, 1.0, 0.0, 0.0);
	V3Cross(&P2, &P, &D);
	if(ISZERO(P2.x) && ISZERO(P2.y) && ISZERO(P2.z))
	{
		V3Set(&P, 0.0, 1.0, 0.0);
		V3Cross(&P2, &P, &D);
	}
	V3Normalize(&P2);
	V3Copy(&P3, &P2);
	P2.x *= c->base_rad; P2.y *= c->base_rad; P2.z *= c->base_rad;
	for(i = 0; i < NUM_STEPS; i++)
	{
		V3Add(&P, &P2, &c->base_pt);
		if(obj->T != NULL)
			PointToWorld(&P, obj->T);
		Set_Pt(i, P.x, P.y, P.z);
		RotatePoint3D(&P2, 30.0*DTOR, &D);
	}
	P3.x *= c->end_rad; P3.y *= c->end_rad; P3.z *= c->end_rad;
	for( ; i < NUM_STEPS*2; i++)
	{
		V3Add(&P, &P3, &c->end_pt);
		if(obj->T != NULL)
			PointToWorld(&P, obj->T);
		Set_Pt(i, P.x, P.y, P.z);
		RotatePoint3D(&P3, 30.0*DTOR, &D);
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

	Move_To(12);
	Line_To(13);
	Line_To(14);
	Line_To(15);
	Line_To(16);
	Line_To(17);
	Line_To(18);
	Line_To(19);
	Line_To(20);
	Line_To(21);
	Line_To(22);
	Line_To(23);
	Line_To(12);

	Move_To(0); Line_To(12);
	Move_To(1); Line_To(13);
	Move_To(2); Line_To(14);
	Move_To(3); Line_To(15);
	Move_To(4); Line_To(16);
	Move_To(5); Line_To(17);
	Move_To(6); Line_To(18);
	Move_To(7); Line_To(19);
	Move_To(8); Line_To(20);
	Move_To(9); Line_To(21);
	Move_To(10); Line_To(22);
	Move_To(11); Line_To(23);
}
