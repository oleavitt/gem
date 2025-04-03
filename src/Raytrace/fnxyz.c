/**
 *****************************************************************************
 * @file fnxyz.c
 *  The fn(x, y, z) function primitive and its functions.
 *
 *****************************************************************************
 */

#include "ray.h"
#include "scn20.h"

/*************************************************************************
 *  Local stuff...
 */
/* Ptr to function expression used by root polishing routines. */
static VMExpr *fn_expr;
/* Transformed ray base and direction vectors. */
static Vec3 B, D;

/* Tolerance for root (t) accuracy. */
#define FN_RELERROR    1e-10
/* Max # of iterations to use in root polisher routines. */
#define FN_MAXIT       800

static int find_root(double a, double b, double *val);

/* Distance from point hit to side of sample box for normal calculation. */
#define FN_OFFSET       0.01
#define FN_BOXSIZE      (FN_OFFSET * 2.0)


static int IntersectFnxyz(Object *obj, HitData *hits);
static void CalcNormalFnxyz(Object *obj, Vec3 *P, Vec3 *N);
static int IsInsideFnxyz(Object *obj, Vec3 *P);
static void CalcUVMapFnxyz(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsFnxyz(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformFnxyz(Object *obj, Vec3 *params, int type);
static void CopyFnxyz(Object *destobj, Object *srcobj);
static void DeleteFnxyz(Object *obj);
static void DrawFnxyz(Object *obj);

unsigned long ray_fnxyz_tests;
unsigned long ray_fnxyz_hits;

static ObjectProcs fnxyz_procs =
{
	OBJ_FN_XYZ,
	IntersectFnxyz,
	CalcNormalFnxyz,
	IsInsideFnxyz,
  CalcUVMapFnxyz,
  CalcExtentsFnxyz,
  TransformFnxyz,
  CopyFnxyz,
	DeleteFnxyz,
	DrawFnxyz
};


Object *Ray_MakeFnxyz(VMExpr *expr, Vec3 *bmin, Vec3 *bmax, Vec3 *steps)
{
	Object *obj = NewObject();
	if(obj != NULL)
	{
		FnxyzData *imp = (FnxyzData *)Malloc(sizeof(FnxyzData));
		if(imp != NULL)
		{
			double t;

			imp->nrefs = 1;
			/* The function (required!). */
			assert(expr != NULL);
			imp->fn = expr;
			/* Bounds of area in which to search. */
			if(bmin != NULL)
				V3Copy(&imp->bmin, bmin);
			else
				V3Set(&imp->bmin, -1.0, -1.0, -1.0);
			if(bmax != NULL)
				V3Copy(&imp->bmax, bmax);
			else
				V3Set(&imp->bmax, 1.0, 1.0, 1.0);
			/* Number of search intervals for each axis. */
			if(steps != NULL)
			{
				imp->xinc = steps->x;
				imp->yinc = steps->y;
				imp->zinc = steps->z;
			}
			else
			{
				imp->xinc = 32.0;
				imp->yinc = 32.0;
				imp->zinc = 32.0;
			}

			if(imp->bmin.x > imp->bmax.x)
				{ t = imp->bmin.x; imp->bmin.x = imp->bmax.x; imp->bmax.x = t; }
			if(imp->bmin.z > imp->bmax.z)
				{ t = imp->bmin.y; imp->bmin.y = imp->bmax.y; imp->bmax.y = t; }
			if(imp->bmin.z > imp->bmax.z)
				{ t = imp->bmin.z; imp->bmin.z = imp->bmax.z; imp->bmax.z = t; }

			if(imp->xinc < 1.0) imp->xinc = 1.0;
			imp->xinc = (imp->bmax.x - imp->bmin.x) / imp->xinc;
			if(imp->yinc < 1.0) imp->yinc = 1.0;
			imp->yinc = (imp->bmax.y - imp->bmin.y) / imp->yinc;
			if(imp->zinc < 1.0) imp->zinc = 1.0;
			imp->zinc = (imp->bmax.z - imp->bmin.z) / imp->zinc;

	  	obj->data.fnxyz = imp;
  		obj->procs = &fnxyz_procs;
		}
		else
      obj = Ray_DeleteObject(obj);
	}
	return obj;
}


int IntersectFnxyz(Object *obj, HitData *hits)
{
	FnxyzData *imp = obj->data.fnxyz;
	double lo, hi;

	ray_fnxyz_tests++;

	/*
	 * Transform ray base point and direction to function's coordinate
	 * system.
	 */
	V3Copy(&B, &ct.B);
	V3Copy(&D, &ct.D);
	if(obj->T != NULL)
	{
		PointToObject(&B, obj->T);
		DirToObject(&D, obj->T);
	}

	/*
	 * If ray hits bounding box, test for function intersections.
	 * lo and hi represent the search interval along the ray.
	 */
	if(Intersect_Box(&B, &D, &imp->bmin, &imp->bmax, &lo, &hi))
	{
		 /* See if interval is within the valid range of ray... */
		if((hi > ct.tmin) && (lo < ct.tmax))
		{
			int i, nhits, ray_entering;
			Vec3 Ptmp;
			double p, t, sa, sb, sc, ta, tb, tc;
			HitData *hitlist;

			V3Copy(&Ptmp, &rt_O);
			fn_expr = imp->fn;
			hitlist = hits;

			/* Truncate parts of interval that are out of ray's bounds... */
			if(lo < ct.tmin)
				lo = ct.tmin;
			if(hi > ct.tmax)
				hi = ct.tmax;

			/* Slice interval into small steps and check each for a root... */
			sa = (fabs(D.x) > EPSILON) ? fabs(imp->xinc / D.x) : HUGE;
			sb = (fabs(D.y) > EPSILON) ? fabs(imp->yinc / D.y) : HUGE;
			sc = (fabs(D.z) > EPSILON) ? fabs(imp->zinc / D.z) : HUGE;
			if(sb > sc) { t = sb; sb = sc; sc = t; }
			if(sa > sb) { t = sa; sa = sb; sb = t; }
			if(sb > sc) { t = sb; sb = sc; sc = t; }
			ta = lo + sa;
			tb = lo + sb;
			tc = lo + sc;
			nhits = 0;
			while(lo < hi)
			{
				if(ta > tc)
				{
					if(tb > tc)
					{
						p = tc;
						tc += sc;
					}
					else
					{
						p = tb;
						tb += sb;
					}
				}
				else if(ta > tb)
				{
					p = tb;
					tb += sb;
				}
				else
				{
					p = ta;
					ta += sa;
				}
				if(find_root(lo, p, &t))
				{
					if(nhits++ > 0)
						hits = GetNextHit(hits);
					hits->t = t;
					hits->obj = obj;
					if(! ct.calc_all)
						break;   /* from loop */
				}
				lo = p;
			}
			if(nhits)
			{
				ray_fnxyz_hits++;
				ray_entering = nhits & 1;
				for(i = nhits; i != 0; i--)
				{
					ray_entering = 1 - ray_entering;
					hitlist->entering = ray_entering;
					hitlist = hitlist->next;
				}
			}
			V3Copy(&rt_O, &Ptmp);
			return nhits;
		}
	}
	return 0;
}

static int find_root(double a, double b, double *val)
{
	int i;
	double fa, fb, m, fm, lfm;

	/* Get start & end points for interval... */
	rt_O.x = B.x + D.x * a;
	rt_O.y = B.y + D.y * a;
	rt_O.z = B.z + D.z * a;
	fa = vm_evaldouble(fn_expr);
	if(fabs(fa) < FN_RELERROR)
	{
		*val = a;
		return 1;
	}

	rt_O.x = B.x + D.x * b;
	rt_O.y = B.y + D.y * b;
	rt_O.z = B.z + D.z * b;
	fb = vm_evaldouble(fn_expr);
	if(fabs(fb) < FN_RELERROR)
	{
		*val = b;
		return 1;
	}

	/* No sign changes, no roots. */
	if((fa * fb) > 0.0)
		return 0;

	lfm = fa;

	for(i = FN_MAXIT; i != 0; i--)
	{
		m = (fb * a - fa * b) / (fb - fa);
		rt_O.x = B.x + D.x * m;
		rt_O.y = B.y + D.y * m;
		rt_O.z = B.z + D.z * m;
		fm = vm_evaldouble(fn_expr);
		if(fabs(m) > FN_RELERROR)
		{
			if(fabs(fm / m) < FN_RELERROR)
			{
				*val = m;
				return 1;
			}
		}
		else if(fabs(fm) < FN_RELERROR)
		{
			*val = m;
			return 1;
		}

		if((fa * fm) < 0.0)
		{
			b = m;
			fb = fm;
			if((lfm * fm) > 0.0)
				fa /= 2.0;
		}
		else
		{
			a = m;
			fa = fm;
			if((lfm * fm) > 0.0)
				fb /= 2.0;
		}

		lfm = fm;
	}
	return 0;
}

static int sign_change(double a, double b)
{
	double fa, fb;

	rt_O.x = B.x + D.x * a;
	rt_O.y = B.y + D.y * a;
	rt_O.z = B.z + D.z * a;
	fa = vm_evaldouble(fn_expr);
	rt_O.x = B.x + D.x * b;
	rt_O.y = B.y + D.y * b;
	rt_O.z = B.z + D.z * b;
	fb = vm_evaldouble(fn_expr);
	return (fa * fb > 0.0) ? 0 : 1;
}


void CalcNormalFnxyz(Object *obj, Vec3 *P, Vec3 *N)
{
	#define NSIDES 12
	FnxyzData *imp;
	static Vec3 P2, Ps[2], Ds[2];
	static int npts, sc[NSIDES];
	static double t;

	imp = obj->data.fnxyz;
	fn_expr = imp->fn;

	/*
	 * Transform world point, "P", to point in function's coordinate
	 * system, "P2".
	 */
	V3Copy(&P2, P);
	if(obj->T != NULL)
		PointToObject(&P2, obj->T);

	/*
	 * Make "P2" the center of a small cube.
	 * Find where the function intersects two sides of the cube
	 * that share a common face. The two points at the side
	 * intersections plus the middle point "P2" define a triangle.
	 * The vector that is perpendicular to the triangle plane
	 * is the approximated normal for the surface at point "P2".
	 */

	npts = 0;
	memset(sc, 0, sizeof(int) * NSIDES);

	/* Sample the +X side... */
	D.x = 0.0;
	D.y = 0.0;
	D.z = 1.0;
	B.x = P2.x + FN_OFFSET;
	B.y = P2.y - FN_OFFSET;
	B.z = P2.z - FN_OFFSET;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[0], &B);
		V3Copy(&Ds[0], &D);
		npts++;
		sc[0] = 1;
	}

	B.y = P2.y + FN_OFFSET;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
		sc[1] = 1;
	}

	if(npts == 2)
		goto normal;

	D.y = 1.0;
	D.z = 0.0;
	B.y = P2.y - FN_OFFSET;
	B.z = P2.z + FN_OFFSET;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
		sc[2] = 1;
	}

	if(npts == 2)
		goto normal;

	B.z = P2.z - FN_OFFSET;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
		sc[3] = 1;
	}

	if(npts == 2)
		goto normal;

	/* Sample the +Y side... */
	npts = 0;
	D.y = 0.0;
	D.z = 1.0;
	B.y = P2.y + FN_OFFSET;
	if(sc[1])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	B.x = P2.x - FN_OFFSET;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
		sc[4] = 1;
	}

	if(npts == 2)
		goto normal;

	D.x = 1.0;
	D.z = 0.0;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
		sc[5] = 1;
	}

	if(npts == 2)
		goto normal;

	B.z = P2.z + FN_OFFSET;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
		sc[6] = 1;
	}
	if(npts == 2)
		goto normal;

	/* Sample the +Z side... */
	npts = 0;
	D.x = 0.0;
	D.y = 1.0;
	B.x = P2.x + FN_OFFSET;
	B.y = P2.y - FN_OFFSET;
	if(sc[2])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	D.x = 1.0;
	D.y = 0.0;
	B.x = P2.x - FN_OFFSET;
	B.y = P2.y + FN_OFFSET;
	if(sc[6])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	if(npts == 2)
		goto normal;
	B.y = P2.y - FN_OFFSET;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
		sc[7] = 1;
	}

	if(npts == 2)
		goto normal;

	D.x = 0.0;
	D.y = 1.0;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
		sc[8] = 1;
	}

	if(npts == 2)
		goto normal;

	/* Sample the -X side... */
	npts = 0;
	D.y = 0.0;
	D.z = 1.0;
	B.y = P2.y + FN_OFFSET;
	B.z = P2.z - FN_OFFSET;
	if(sc[4])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	D.y = 1.0;
	D.z = 0.0;
	B.y = P2.y - FN_OFFSET;
	B.z = P2.z + FN_OFFSET;
	if(sc[8])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	if(npts == 2)
		goto normal;

	B.z = P2.z - FN_OFFSET;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
		sc[9] = 1;
	}

	if(npts == 2)
		goto normal;
	D.y = 0.0;
	D.z = 1.0;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
		sc[10] = 1;
	}

	if(npts == 2)
		goto normal;

	/* Sample the -Y side... */
	npts = 0;
	B.x = P2.x + FN_OFFSET;
	if(sc[0])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	D.x = 1.0;
	D.z = 0.0;
	B.x = P2.x - FN_OFFSET;
	B.z = P2.z + FN_OFFSET;
	if(sc[7])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	if(npts == 2)
		goto normal;

	D.x = 0.0;
	D.z = 1.0;
	B.z = P2.z - FN_OFFSET;
	if(sc[10])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	if(npts == 2)
		goto normal;

	D.x = 1.0;
	D.z = 0.0;
	if(sign_change(0.0, FN_BOXSIZE))
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
		sc[11] = 1;
	}

	if(npts == 2)
		goto normal;

	/* Sample the -Z side... */
	npts = 0;
	D.x = 0.0;
	D.y = 1.0;
	B.x = P2.x + FN_OFFSET;
	if(sc[3])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	D.x = 1.0;
	D.y = 0.0;
	B.x = P2.x - FN_OFFSET;
	B.y = P2.y + FN_OFFSET;
	if(sc[5])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	if(npts == 2)
		goto normal;

	D.x = 0.0;
	D.y = 1.0;
	B.y = P2.y - FN_OFFSET;
	if(sc[9])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	if(npts == 2)
		goto normal;

	D.x = 1.0;
	D.y = 0.0;
	if(sc[11])
	{
		V3Copy(&Ps[npts], &B);
		V3Copy(&Ds[npts], &D);
		npts++;
	}

	normal:

	if(npts == 2)
	{
		V3Copy(&B, &Ps[0]);
		V3Copy(&D, &Ds[0]);
		(void)find_root(0.0, FN_BOXSIZE, &t);
		Ps[0].x = B.x + D.x * t;
		Ps[0].y = B.y + D.y * t;
		Ps[0].z = B.z + D.z * t;
		V3Copy(&B, &Ps[1]);
		V3Copy(&D, &Ds[1]);
		(void)find_root(0.0, FN_BOXSIZE, &t);
		Ps[1].x = B.x + D.x * t;
		Ps[1].y = B.y + D.y * t;
		Ps[1].z = B.z + D.z * t;
		V3Sub(&B, &Ps[0], &P2);
		V3Sub(&D, &Ps[1], &P2);
		V3Cross(N, &B, &D);
	}
	else
	{
		N->x = 0.0;
		N->y = 0.0;
		N->z = 1.0;
	}

	if(obj->T != NULL)
		NormToWorld(N, obj->T);
	V3Normalize(N);
}


int IsInsideFnxyz(Object *obj, Vec3 *P)
{
	FnxyzData *imp = obj->data.fnxyz;
	Vec3 Ptmp;
	int out;

	V3Copy(&Ptmp, &rt_O);

	/*
	 * Transform world point, "P", to point in function's coordinate
	 * system, "rt_O".
	 */
	V3Copy(&rt_O, P);
	if(obj->T != NULL)
		PointToObject(&rt_O, obj->T);
	out = (vm_evaldouble(imp->fn) > 0.0) ? 1 : 0;
	V3Copy(&rt_O, &Ptmp);
	if(out)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */
	return(! (obj->flags & OBJ_FLAG_INVERSE)); /* inside */
}


void CalcUVMapFnxyz(Object *obj, Vec3 *P, double *u, double *v)
{
	*u = *v = 0.0;

	/* Not used */
	//P; obj;
}


void CalcExtentsFnxyz(Object *obj, Vec3 *omin, Vec3 *omax)
{
	FnxyzData *imp = obj->data.fnxyz;

	omin->x = imp->bmin.x;
	omax->x = imp->bmax.x;
	omin->y = imp->bmin.y;
	omax->y = imp->bmax.y;
	omin->z = imp->bmin.z;
	omax->z = imp->bmax.z;

	if (obj->T != NULL)
		BBoxToWorld(omin, omax, obj->T);
}


void CopyFnxyz(Object *destobj, Object *srcobj)
{
	FnxyzData *imp;

	imp = srcobj->data.fnxyz;
	assert(imp != NULL);
	destobj->data.fnxyz = imp;
	imp->nrefs++;
}


void TransformFnxyz(Object *obj, Vec3 *params, int type)
{
	if (obj->T == NULL)
		obj->T = Ray_NewXform();
	XformXforms(obj->T, params, type);
}


void DeleteFnxyz(Object *obj)
{
	FnxyzData *imp = obj->data.fnxyz;

	if (--imp->nrefs == 0)
	{
		delete_exprtree(imp->fn);
		Free(imp, sizeof(FnxyzData));
	}
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

void DrawFnxyz(Object *obj)
{
	FnxyzData *imp = obj->data.fnxyz;

  /* If bbox only. */
	if(obj->T != NULL)
	{
	  Vec3 pt;
		V3Set(&pt, imp->bmin.x, imp->bmin.y, imp->bmin.z);
		PointToWorld(&pt, obj->T);
		Set_Pt(0, pt.x, pt.y, pt.z);
		V3Set(&pt, imp->bmax.x, imp->bmin.y, imp->bmin.z);
		PointToWorld(&pt, obj->T);
		Set_Pt(1, pt.x, pt.y, pt.z);
		V3Set(&pt, imp->bmin.x, imp->bmax.y, imp->bmin.z);
		PointToWorld(&pt, obj->T);
		Set_Pt(2, pt.x, pt.y, pt.z);
		V3Set(&pt, imp->bmax.x, imp->bmax.y, imp->bmin.z);
		PointToWorld(&pt, obj->T);
		Set_Pt(3, pt.x, pt.y, pt.z);
		V3Set(&pt, imp->bmin.x, imp->bmin.y, imp->bmax.z);
		PointToWorld(&pt, obj->T);
		Set_Pt(4, pt.x, pt.y, pt.z);
		V3Set(&pt, imp->bmax.x, imp->bmin.y, imp->bmax.z);
		PointToWorld(&pt, obj->T);
		Set_Pt(5, pt.x, pt.y, pt.z);
		V3Set(&pt, imp->bmin.x, imp->bmax.y, imp->bmax.z);
		PointToWorld(&pt, obj->T);
		Set_Pt(6, pt.x, pt.y, pt.z);
		V3Set(&pt, imp->bmax.x, imp->bmax.y, imp->bmax.z);
		PointToWorld(&pt, obj->T);
		Set_Pt(7, pt.x, pt.y, pt.z);
	}
	else
	{
		Set_Pt(0, imp->bmin.x, imp->bmin.y, imp->bmin.z);
		Set_Pt(1, imp->bmax.x, imp->bmin.y, imp->bmin.z);
		Set_Pt(2, imp->bmin.x, imp->bmax.y, imp->bmin.z);
		Set_Pt(3, imp->bmax.x, imp->bmax.y, imp->bmin.z);
		Set_Pt(4, imp->bmin.x, imp->bmin.y, imp->bmax.z);
		Set_Pt(5, imp->bmax.x, imp->bmin.y, imp->bmax.z);
		Set_Pt(6, imp->bmin.x, imp->bmax.y, imp->bmax.z);
		Set_Pt(7, imp->bmax.x, imp->bmax.y, imp->bmax.z);
	}
	Move_To(0);
	Line_To(1);
	Line_To(3);
	Line_To(2);
	Line_To(0);
	Line_To(4);
	Line_To(5);
	Line_To(7);
	Line_To(6);
	Line_To(4);
	Move_To(1);
	Line_To(5);
	Move_To(3);
	Line_To(7);
	Move_To(2);
	Line_To(6);

  /* Else, draw the actual object. */
}
