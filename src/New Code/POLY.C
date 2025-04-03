/*************************************************************************
*
*  poly.c
*  The polynomial primitive and its functions.
*
*************************************************************************/

#include "ray.h"

/* Pool for temporary memory allocation. */
static mempool *poly_mem;
#define POLY_MEM_POOL_BLOCK_SIZE  (sizeof(TERM) * 128)

static VOID build_poly(DBL *tcoef);
static VOID bp(int ord, DBL o, DBL d, DBL *coef);
static DBL power(DBL x, int e);
static int implicit_root(DBL a, DBL b, DBL *val);
static DBL eval_poly3(DBL x, DBL y, DBL z);
static DBL eval_poly(TERM *termlist, DBL *xpow, DBL *ypow, DBL *zpow);

/* Current parameter being parsed. */
static PARAMS *param;

/* Ray base and direction vectors. */
static VECTOR B, D;
/* Polynomial data. */
static POLYNOMIAL *ply;
/* Temp buffer for pre-computed powers of x, y, and z. */
static DBL *xpow, *ypow, *zpow;

/* Coefficient scaling factors. (MAX_ORDER max!) */
static DBL fact[] = 	{
	 1.0,		1.0,		2.0,
	 6.0,		24.0,		120.0,
	 720.0,		5040.0,		40320.0,
	 362880.0,	3628800.0,	39916800.0,
	 479001600.0
	};

/* Tolerance for root (t) accuracy. */
#define PLY_RELERROR    1e-10
/* Max # of iterations to use in root polisher routines. */
#define PLY_MAXIT       800

int poly_intersect(OBJECT *obj, RAY *ray, HIT *hits)
	{
	DBL lo, hi;
	int i, nhits, ray_entering;
	/* The polynomial after arranging for t (for parametric types). */
	static DBL tcoef[MAX_ORDER + 1];
	/* Recieves the roots of the polynomial (for parametric types). */
	static DBL t_array[MAX_ORDER];


	ply = (POLYNOMIAL *)obj->data;

	polynomial_tests++;

	/* Check bounding object, if any. */
	if(NULL != ply->bound)
		if(! intersect[ply->bound->type]( ply->bound, ray, hits))
			return 0; /* miss bounding object */

	/*
	 * Transform ray base point and direction cosines to coordinate
	 * system of polynomial.
	 */
	VCopy(&B, ray->O);
	VCopy(&D, ray->D);
	if(NULL != obj->T)
		{
		PointToObject(&B, obj->T);
		DirToObject(&D, obj->T);
		}

	if(intersect_box(&B, &D, &ply->bmin, &ply->bmax, &lo, &hi))
		{
		if(( hi > ray->min_dist) &&(lo < ray->max_dist))
			{
			if(lo < ray->min_dist)
				lo = ray->min_dist;
			if(hi > ray->max_dist)
				hi = ray->max_dist;
			xpow = ply->xpow;
			ypow = ply->ypow;
			zpow = ply->zpow;
			if(ply->order <= MAX_ORDER && ply->solver != TK_IMPLICIT)
				{
				for(i = 0; i <= ply->order; i++)
					tcoef[i] = 0.0;
				build_poly(tcoef);
				nhits = SolveAndSortPoly (
					tcoef,
					t_array,
					ply->order,
					ply->solver,
					ray->calc_all,
					lo,
					hi);
				if(nhits > 0)
					{
					ray_entering =(( nhits & 1) == 0) ? 1 : 0;
					for(i = 0; i < nhits; i++)
						{
						if(i > 0)
							new_hit(hits);
						hits->t = t_array[i];
						hits->obj = obj;
						hits->entering = ray_entering;
						ray_entering = 1 - ray_entering;
						}
					polynomial_hits++;
					}
				}
			else   /* Use implicit solver. */
				{
				DBL p, t, sa, sb, sc, ta, tb, tc;
				HIT *hitlist;

				hitlist = hits;

				/* Slice interval into small steps and check each for a root... */
				sa =(fabs(D.x) > EPSILON) ? fabs(ply->xinc / D.x) : HUGE;
				sb =(fabs(D.y) > EPSILON) ? fabs(ply->yinc / D.y) : HUGE;
				sc =(fabs(D.z) > EPSILON) ? fabs(ply->zinc / D.z) : HUGE;
				if(sb > sc) SWAP(sb, sc);
				if(sa > sb) SWAP(sa, sb);
				if(sb > sc) SWAP(sb, sc);
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
					if(implicit_root(lo, p, &t))
						{
						if(nhits++ > 0)
							new_hit(hits);
						hits->t = t;
						hits->obj = obj;
						if(! ray->calc_all)
							break;   /* from loop */
						}
					lo = p;
					}
				if(nhits)
					{
					polynomial_hits++;
					ray_entering = nhits & 1;
					for(i = nhits; i != 0; i--)
						{
						ray_entering = 1 - ray_entering;
						hitlist->entering = ray_entering;
						hitlist = hitlist->next;
						}
					}
				}
			return nhits;
			}
		}

	return 0;

	} /* end of poly_intersect() */

/*
 * build_poly
 *
 *    Build up the polynomial in t
 *
 *    Remember 	x = x->a + x->b * t
 *      	y = y->a + y->b * t
 *	    	z = z->a + z->b * t
 *
 *    and a general term is of the form
 *
 *         term = coef * (x^l) * (y^m) * (z^n)
 *
 *	The reader will bare in mind that speed is important here.
 */
static VOID build_poly(DBL *tcoef)
	{
	int 	i, j, l, m, n;
	DBL t1, t2, ecoef, *cf, *lf, *mf, *nf;
	POWER *pw;
	TERM *term;

	for (pw = ply->xpws; pw != NULL; pw = pw->next)
		bp(pw->pw, B.x, D.x, pw->coefs);

	for (pw = ply->ypws; pw != NULL; pw = pw->next)
		bp(pw->pw, B.y, D.y, pw->coefs);

	for (pw = ply->zpws; pw != NULL; pw = pw->next)
		bp(pw->pw, B.z, D.z, pw->coefs);

	for (term = ply->trmlist; term != NULL; term = term->next)
		{
		ecoef = term->coef;
		switch (term->type)
			{
		case 0:
			tcoef[0] += ecoef;
			break;
		case _X:
			l = term->xp;
			lf = term->xcoefs + l;
			cf = tcoef + l;
			for (i = l; i >= 0; i--)
				*cf-- += *lf-- * ecoef;
			break;
		case _Y:
			l = term->yp;
			lf = term->ycoefs + l;
			cf = tcoef + l;
			for (i = l; i >= 0; i--)
				*cf-- += *lf-- * ecoef;
			break;
		case _Z:
			l = term->zp;
			lf = term->zcoefs + l;
			cf = tcoef + l;
			for (i = l; i >= 0; i--)
				*cf-- += *lf-- * ecoef;
			break;
		case _X | _Y:
			m = term->xp;
			n = term->yp;
			mf = term->xcoefs + m;
			lf = term->ycoefs + n;
			cf = tcoef + m;
			while (m >= 0)
				{
				t1 = ecoef * *mf--;
				nf = lf;
				cf += n;
				for (i = n; i >= 0; i--)
					*cf-- += *nf-- * t1;
				m--;
				}
			break;
		case _Y | _Z:
			m = term->yp;
			n = term->zp;
			mf = term->ycoefs + m;
			lf = term->zcoefs + n;
			cf = tcoef + m;
			while (m >= 0)
				{
				t1 = ecoef * *mf--;
				nf = lf;
				cf += n;
				for (i = n; i >= 0; i--)
					*cf-- += *nf-- * t1;
				m--;
				}
			break;
		case _X | _Z:
			m = term->xp;
			n = term->zp;
			mf = term->xcoefs + m;
			lf = term->zcoefs + n;
			cf = tcoef + m;
			while (m >= 0)
				{
				t1 = ecoef * *mf--;
				nf = lf;
				cf += n;
				for (i = n; i >= 0; i--)
					*cf-- += *nf-- * t1;
				m--;
				}
			break;
		case _X | _Y | _Z:
			l = term->xp;
			m = term->yp;
			n = term->zp;
			lf = term->xcoefs + l;
			mf = term->ycoefs - 1;
			nf = term->zcoefs - 1;
			cf = tcoef + l;
			while (l >= 0)
				{
				t1 = ecoef * *lf--;
				mf += m + 1;
				cf += m;
				for (i = m; i >= 0; i--)
					{
					t2 = t1 * *mf--;
					cf += n;
					nf += n + 1;
					for (j = n; j >= 0; j--)
						*cf-- += *nf-- * t2;
					}
				l--;
				}
			break;
		default:
			fatal("Bad type in build_poly().\n");
			break;
			}
		}
	}

/*
 * bp
 *
 *	constructs the coefficients for a polynomial of order ord in
 * 	variables o and d
 */
static VOID bp(int ord, DBL o, DBL d, DBL *coef)
	{
	DBL	*start, *end;
	DBL cof, delta;

	start = coef;
	end = coef + ord;

	if(o != 0.0)
		{
		switch(ord)
			{
			case 1:
				coef[0] = o;
				coef[1] = d;
				break;
			case 2:
				coef[0] = o * o;
				coef[1] = 2.0 * o * d;
				coef[2] = d * d;
				break;
			default:
				*coef = power(o, ord);
				delta = d / o;
				cof = *coef * delta * fact[ord];
				while (++coef <= end)
					{
					*coef = cof / (fact[(int)(end - coef)] * fact[(int)(coef - start)]);
					cof = cof * delta;
					}
				break;
			}
		}
	else
		{
		while (coef < end)
			*coef++ = 0.0;
		*coef = power(d, ord);
		}
	}

/*
 *  power - Return x raised to integer e power.
 */
static DBL power(DBL x, int e)
	{
	DBL a, b;

	b = x;

	switch (e)
		{
		case 0:
			return(1.0);
		case 1:
			return(b);
		case 2:
			return(b * b);
		case 3:
			return(b * b * b);
		case 4:
			b *= b;
			return(b * b);
		case 5:
			b *= b;
			return(b * b * x);
		case 6:
			b *= b;
			return(b * b * b);
		default:
			e -= 6;
			a = b * b;
			a = a * a * a;
			for (;;)
				{
				if (e & 1)
					{
					a *= b;
					if ((e /= 2) == 0)
						break;
					}
				else
					e /= 2;
				b *= b;
				}
			return(a);
		}
	}

/*
 * eval_poly
 *
 *	Evaluate the polynomial defined by termlist for x y and z
 */
static DBL eval_poly(TERM *termlist, DBL *xpow, DBL *ypow, DBL *zpow)
	{
	DBL val;
	TERM *t;

	val = 0.0;
	for (t = termlist; t != NULL; t = t->next)
		val += t->coef * xpow[t->xp] * ypow[t->yp] * zpow[t->zp];

	return val;
	} /* end of eval_poly() */

static DBL eval_poly3(DBL x, DBL y, DBL z)
	{
	register int i;

	xpow[0] = ypow[0] = zpow[0] = 1.0;
	xpow[1] = x;
	ypow[1] = y;
	zpow[1] = z;
	for(i = 2; i <= ply->maxxp; i++)
		xpow[i] = xpow[i - 1] * x;
	for(i = 2; i <= ply->maxyp; i++)
		ypow[i] = ypow[i - 1] * y;
	for(i = 2; i <= ply->maxzp; i++)
		zpow[i] = zpow[i - 1] * z;

	return eval_poly(ply->trmlist, xpow, ypow, zpow);
	} /* end of eval_poly3() */

static int implicit_root(DBL a, DBL b, DBL *val)
	{
	int i;
	DBL fa, fb, m, fm, lfm, x, y, z;

	/* Get start & end points for interval... */
	x = B.x + D.x * a;
	y = B.y + D.y * a;
	z = B.z + D.z * a;
	fa = eval_poly3(x, y, z);
	if(fabs(fa) < PLY_RELERROR)
		{
		*val = a;
		return 1;
		}

	x = B.x + D.x * b;
	y = B.y + D.y * b;
	z = B.z + D.z * b;
	fb = eval_poly3(x, y, z);
	if(fabs(fb) < PLY_RELERROR)
		{
		*val = b;
		return 1;
		}

	/* No sign changes, no roots. */
	if((fa * fb) > 0.0)
		return 0;

	lfm = fa;

	for(i = PLY_MAXIT; i != 0; i--)
		{
		m = (fb * a - fa * b) / (fb - fa);
		x = B.x + D.x * m;
		y = B.y + D.y * m;
		z = B.z + D.z * m;
		fm = eval_poly3(x, y, z);

		if(fabs(m) > PLY_RELERROR)
			{
			if(fabs(fm / m) < PLY_RELERROR)
				{
				*val = m;
				return 1;
				}
			}
		else if(fabs(fm) < PLY_RELERROR)
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
	} /* end of implicit_root() */

VOID poly_normal(OBJECT *obj, VECTOR *Q, VECTOR *N)
	{
	POLYNOMIAL *ply;
	VECTOR P;
	DBL	x, y, z;
	int i;

	ply = (POLYNOMIAL *)obj->data;
	xpow = ply->xpow;
	ypow = ply->ypow;
	zpow = ply->zpow;

	/*
	 * Transform world point, "Q", to polynomial's coordinate
	 * system, "P".
	 */
	VCopy(&P, Q);
	if(NULL != obj->T)
		PointToObject(&P, obj->T);

	xpow[0] = ypow[0] = zpow[0] = 1.0;
	xpow[1] = x = P.x;
	ypow[1] = y = P.y;
	zpow[1] = z = P.z;
	for(i = 2; i <= ply->maxxp; i++)
		xpow[i] = xpow[i - 1] * x;
	for(i = 2; i <= ply->maxyp; i++)
		ypow[i] = ypow[i - 1] * y;
	for(i = 2; i <= ply->maxzp; i++)
		zpow[i] = zpow[i - 1] * z;

	N->x = eval_poly(ply->dxlist, xpow, ypow, zpow);
	N->y = eval_poly(ply->dylist, xpow, ypow, zpow);
	N->z = eval_poly(ply->dzlist, xpow, ypow, zpow);

	if(NULL != obj->T)
		NormToWorld(N, obj->T);
	VNorm(N);
	} /* end of poly_normal() */

int poly_inside(OBJECT *obj, VECTOR *Q)
	{
	POLYNOMIAL *ply;
	VECTOR P;

	ply = (POLYNOMIAL *)obj->data;
	xpow = ply->xpow;
	ypow = ply->ypow;
	zpow = ply->zpow;

	/*
	 * Transform world point, "Q", to polynomial's coordinate
	 * system, "P".
	 */
	VCopy(&P, Q);
	if(NULL != obj->T)
		PointToObject(&P, obj->T);

	/* First, see if point is within bounds of polynomial... */
	if(P.x < ply->bmin.x || P.x > ply->bmax.x)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */

	if(P.y < ply->bmin.y || P.y > ply->bmax.y)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */

	if(P.z < ply->bmin.z || P.z > ply->bmax.z)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */

	if(eval_poly3(P.x, P.y, P.z) > 0.0)
		return(obj->flags & OBJ_FLAG_INVERSE); /* not inside */
	return(! (obj->flags & OBJ_FLAG_INVERSE)); /* inside */
	} /* end of poly_inside() */

VOID poly_extents(OBJECT *obj, VECTOR *omin, VECTOR *omax)
	{
	POLYNOMIAL *ply;
	ply = (POLYNOMIAL *)obj->data;

	/* If user supplied a bounding object, use its extents. */
	if(NULL != ply->bound)
		{
		extents[ply->bound->type]( ply->bound, omin, omax);
		return;
		}

	omin->x = ply->bmin.x;
	omax->x = ply->bmax.x;
	omin->y = ply->bmin.y;
	omax->y = ply->bmax.y;
	omin->z = ply->bmin.z;
	omax->z = ply->bmax.z;

	if(NULL != obj->T)
		BBoxToWorld(omin, omax, obj->T);

	} /* end of poly_extents() */


VOID clone_poly(OBJECT *destobj, OBJECT *srcobj)
	{
	POLYNOMIAL *destply, *srcply;

	srcply = (POLYNOMIAL *)srcobj->data;
	destply = (POLYNOMIAL *)rmalloc(sizeof(POLYNOMIAL));
	destobj->data = destply;
	memcpy(destply, srcply, sizeof(POLYNOMIAL));
	destply->bound = clone_object(srcply->bound);

	} /* end of clone_poly() */


VOID xform_poly(OBJECT *obj, VECTOR *params, int type)
	{
	POLYNOMIAL *ply = (POLYNOMIAL *)obj->data;

	if(NULL == obj->T)
		obj->T = NewXform();
	XformXforms(obj->T, params, type);

	if(NULL != ply->bound)
		xform_object(ply->bound, params, type);
	} /* end of xform_poly() */

/*************************************************************************
*
*  parse_poly_terms - Parses in the polynomial equation from a parameter
*    list and returns a list of terms for the equation in expanded form.
*
*************************************************************************/

static int pwr_flag;
static TERM * parse_poly_expr(VOID);

static TERM *new_term(VOID)
	{
	TERM *t;

	t = (TERM *)mpalloc(sizeof(TERM), poly_mem);
	t->next = NULL;
	t->coef = 1.0;
	t->xp = 0;
	t->yp = 0;
	t->zp = 0;
	return t;
	} /* end of new_term() */

static TERM * parse_atom(VOID)
	{
	TERM *t;

	assert(NULL != param);
	if (OP_LPAREN == param->type)
		{
		param = param->next;
		t = parse_poly_expr();
		assert(NULL != param);
		assert(OP_RPAREN == param->type);
		}
	else
		{
		t = new_term();
		switch (param->type)
			{
			case CV_CONST:
				t->coef *= param->V.x;
				break;
			case DECL_LVALUE:
				t->coef *= param->data.lvalue->V.x;
				break;
			case RT_OX:
				t->xp++;
				break;
			case RT_OY:
				t->yp++;
				break;
			case RT_OZ:
				t->zp++;
				break;
			}
		}
	if (pwr_flag)
		if (RT_OX == param->type || RT_OY == param->type ||
				RT_OZ == param->type || t->coef < 0.0)
			error("Polynomial: Invalid exponent. Must be positive integer.");
	param = param->next;
	return t;
	} /* end of parse_atom() */

static TERM * parse_factor(VOID)
	{
	TERM *tlist, *t;
	int sign;

	sign = 0;
	while (OP_MINUS == param->type || OP_PLUS == param->type)
		{
		if (OP_MINUS == param->type)
			sign = 1 - sign;
		param = param->next;
		}
	assert(NULL != param);
	tlist = parse_atom();
	if (sign)
		for (t = tlist; NULL != t; t = t->next)
			t->coef = - t->coef;
	return tlist;
	} /* end of parse_factor() */

static TERM * parse_power(VOID)
	{
	TERM *tlist, *pt, *rlist, *llist, *rt, *lt, *t;
	int pwr;

	tlist = parse_factor();
	rlist = tlist;
	for (llist = tlist; NULL != param && OP_POW == param->type; llist = tlist)
		{
		param = param->next;
		assert(NULL != param);
		pwr_flag++;
		pt = parse_factor();
		pwr_flag--;
		pwr = (int)pt->coef;
		if (pwr == 0)
			{
			tlist = new_term();
			break;
			}
		else if (pwr > 1)
			{
			do
				{
				tlist = NULL;
				for (lt = llist; NULL != lt; lt = lt->next)
					for (rt = rlist; NULL != rt; rt = rt->next)
						{
						t = new_term();
						t->next = tlist;
						tlist = t;
						t->coef = lt->coef * rt->coef;
						t->xp = lt->xp + rt->xp;
						t->yp = lt->yp + rt->yp;
						t->zp = lt->zp + rt->zp;
						}

				llist = tlist;
				}
			while (pwr-- > 2);
			}
		}

	return tlist;
	} /* end of parse_power() */

static TERM * parse_term(VOID)
	{
	TERM *tlist, *rlist, *llist, *rt, *lt, *t;

	tlist = parse_power();
	for (llist = tlist; NULL != param && OP_MULT == param->type; llist = tlist)
		{
		param = param->next;
		assert(NULL != param);
		rlist = parse_power();
		tlist = NULL;
		for (lt = llist; NULL != lt; lt = lt->next)
			for (rt = rlist; NULL != rt; rt = rt->next)
				{
				t = new_term();
				t->next = tlist;
				tlist = t;
				t->coef = lt->coef * rt->coef;
				t->xp = lt->xp + rt->xp;
				t->yp = lt->yp + rt->yp;
				t->zp = lt->zp + rt->zp;
				}
		}

	return tlist;
	} /* end of parse_term() */

static TERM * parse_poly_expr(VOID)
	{
	TERM *tlist, *rt, *lt, *t;
	int op;

	/*
	 * "lt" will point to the end of the list,
	 * which will have more than one term if sub-expressions
	 * were processed. "tlist" is the start.
	 */
	tlist = parse_term();
	lt = tlist;
	while(NULL != param && (OP_PLUS == (op = param->type) || OP_MINUS == op))
		{
		param = param->next;
		assert(NULL != param);
		rt = parse_term();
		if (pwr_flag)
			lt->coef += (OP_MINUS == op) ? -rt->coef : rt->coef;
		else
			{
			/* Add on the new term list. */
			for( ; NULL != lt->next; lt = lt->next)
					;  /* seek end of list */
			lt->next = rt;
			lt = rt;
			/* Negate the new list if subtracting. */
			if (OP_MINUS == op)
				for (t = rt; NULL != t; t = t->next)
					t->coef = - t->coef;
			}
		}
	return tlist;
	} /* end of parse_poly_expr() */

static TERM * parse_poly_terms(VOID)
	{
	TERM *tmp_termlist, *termlist, *t, *t2;

	poly_mem = new_mem_pool(POLY_MEM_POOL_BLOCK_SIZE);
	
	/*
	 * Will remain NULL if no expression is processed
	 * or an error occurs.
	 */
	termlist = NULL;

	/* Parse the expression... */
	assert(NULL != param);
	pwr_flag = 0;
	tmp_termlist = parse_poly_expr();

	/* Combine like terms... */
	for (t = tmp_termlist; t != NULL; t = t->next)
		for (t2 = t->next; t2 != NULL; t2 = t2->next)
			if (t2->xp == t->xp && t2->yp == t->yp && t2->zp == t->zp)
				{
				t->coef += t2->coef;
				t2->coef = 0.0;  /* flag extra term for deletion */
				}

	/*
	 * Allocate run-time term list and copy the
	 * contents of the temp term list to it, excluding
	 * zeroed out terms.
	 */
	t2 = NULL;
	for (t = tmp_termlist; NULL != t; t = t->next)
		{
		if (t->coef != 0.0)
			{
			if (NULL != t2)
				{
				t2->next = (TERM *)rmalloc(sizeof(TERM));
				t2 = t2->next;
				}
			else
				{
				t2 = (TERM *)rmalloc(sizeof(TERM));
				termlist = t2;
				}
			*t2 = *t;
			t2->next = NULL;
			}
		}

	poly_mem = delete_mem_pool(poly_mem);

	return termlist;

	}  /* end of parse_poly_terms() */

PARAMS * compile_poly_params(VOID)
	{
	PARAMS *plist, *p, *cp;
	int token, paren_level;

	/* Get the polynomial's expression... */
	paren_level = 0;
	token = 0;
	cp = p = plist = NULL;
	while(1)
		{
		if(NULL != p)
			{
			if(NULL == cp)
				plist = p;
			else
				cp->next = p;
			p->type = token;
			cp = p;
			p = NULL;
			}
		token = GetToken();
		switch(token)
			{
			case OP_LPAREN:
				paren_level += 2;
			case OP_RPAREN:
				paren_level--;
				if(paren_level < 0)
					error("Polynomial expression syntax: ')' without matching '('");
			case CV_CONST:
			case DECL_LVALUE:
			case RT_OX:
			case RT_OY:
			case RT_OZ:
			case OP_PLUS:
			case OP_MINUS:
			case OP_POW:
			case OP_MULT:
				p = new_param();
				switch(token)
					{
					case CV_CONST:
						p->V.x = atof(token_buffer);
						break;
					case DECL_LVALUE:
						p->data.lvalue = tk_var;
						break;
					}
				continue;
			default:
				break;
			}
		UngetToken(token);
		break;
		}

	if (NULL == plist)
		error("Polynomial: I need an expression.");
	if(paren_level)
		error1("Polynomial: expression syntax: ')' expected. Found '%s'."
		, token_buffer);

	/* Get optional stuff... */
	while(1)
		{
		while(NULL != cp->next)
			cp = cp->next;
		token = GetToken();
		switch(token)
			{
			case TK_BOUND:
				cp->next = new_param();
				cp = cp->next;
				cp->type = TK_BOUND;
				cp->next = compile_params("V,V");
				continue;

			case TK_RESOLUTION:
				cp->next = new_param();
				cp = cp->next;
				cp->type = TK_RESOLUTION;
				cp->next = compile_params("FO,FO,F");
				continue;

			default:
				UngetToken(token);
				break;
			}
		break;
		}
	return plist;
	} /* end of compile_poly_params() */

VOID make_poly(OBJECT *obj, PARAMS *par)
	{
	POLYNOMIAL *ply;
	int i;
	char *xpws, *ypws, *zpws;
	DBL **xcoefs, **ycoefs, **zcoefs;
	POWER *p;
	TERM *t, *dxt, *dyt, *dzt;

	ply = (POLYNOMIAL *) rmalloc(sizeof(POLYNOMIAL));
	ply->order = 0;
	ply->maxxp = 0;
	ply->maxyp = 0;
	ply->maxzp = 0;
	ply->dxlist = NULL;
	ply->dylist = NULL;
	ply->dzlist = NULL;
	ply->xpws = NULL;
	ply->ypws = NULL;
	ply->zpws = NULL;
	SetVec(&ply->bmin, -1.0, -1.0, -1.0);
	SetVec(&ply->bmax, 1.0, 1.0, 1.0);
	ply->xinc = 32.0;
	ply->yinc = 32.0;
	ply->zinc = 32.0;
	ply->bound = NULL;

	param = par;
	ply->trmlist = parse_poly_terms();
	par = param;

	while(NULL != par)
		{
		switch(par->type)
			{
			case TK_BOUND:
				par = par->next;
				eval_params(par);
				VCopy(&ply->bmin, &par->V);
				par = par->next;
				VCopy(&ply->bmax, &par->V);
				par = par->next;
				break;

			case TK_RESOLUTION:
				par = par->next;
				eval_params(par);
				ply->xinc = fabs(par->V.x);
				if(par->more)
					{
					par = par->next;
					ply->yinc = fabs(par->V.x);
					if(par->more)
						{
						par = par->next;
						ply->zinc = fabs(par->V.x);
						}
					}
				par = par->next;
				break;

			default:
      	fatal1("Bad param switched in make_poly(), %s.", __FILE__);
				break;
			}
		}

	if (NULL == ply->trmlist)
		error("Polynomial: I need an expression.");

	for (t = ply->trmlist; t != NULL; t = t->next)
		{
		if (ply->maxxp < t->xp)
			ply->maxxp = t->xp;
		if (ply->maxyp < t->yp)
			ply->maxyp = t->yp;
		if (ply->maxzp < t->zp)
			ply->maxzp = t->zp;
		if (ply->order < (t->xp + t->yp +t->zp))
			ply->order = t->xp + t->yp + t->zp;
		}

	if(ply->order < 1)
		{
		error(
		"Polynomial: Order = zero. Need at least one x, y, or z variable."
		);
		return;
		}

	xpws = (char *)mmalloc(sizeof(char) * ply->order);
	ypws = (char *)mmalloc(sizeof(char) * ply->order);
	zpws = (char *)mmalloc(sizeof(char) * ply->order);
	xcoefs = (DBL **)mmalloc(sizeof(DBL *) * ply->order);
	ycoefs = (DBL **)mmalloc(sizeof(DBL *) * ply->order);
	zcoefs = (DBL **)mmalloc(sizeof(DBL *) * ply->order);
	ply->xpow = (DBL *)rmalloc(sizeof(DBL) * (ply->order + 1));
	ply->ypow = (DBL *)rmalloc(sizeof(DBL) * (ply->order + 1));
	ply->zpow = (DBL *)rmalloc(sizeof(DBL) * (ply->order + 1));

	for (i = 0; i < ply->order; i++)
		{
		xpws[i] = FALSE;
		ypws[i] = FALSE;
		zpws[i] = FALSE;
		}

	for (t = ply->trmlist; t != NULL; t = t->next)
		{
		t->type = 0;

		if (t->xp != 0)
			{
			t->type |= _X;
			dxt = (TERM *)rmalloc(sizeof(TERM));
			*dxt = *t;
			dxt->xp = t->xp - 1;
			dxt->coef *= t->xp;
			dxt->next = ply->dxlist;
			ply->dxlist = dxt;
			if (!xpws[t->xp - 1])
				{
				p = (POWER *)rmalloc(sizeof(POWER));
				p->pw = t->xp;
				xcoefs[t->xp - 1] = p->coefs =
					(DBL *)rmalloc(sizeof(DBL) * (p->pw + 1));
				for(i = 0; i <= p->pw; i++)
					p->coefs[i] = 0.0;
				p->next = ply->xpws;
				ply->xpws = p;
				xpws[t->xp - 1] = TRUE;
				}
			t->xcoefs = xcoefs[t->xp - 1];
			}

		if (t->yp != 0)
			{
			t->type |= _Y;
			dyt = (TERM *)rmalloc(sizeof(TERM));
			*dyt = *t;
			dyt->yp = t->yp - 1;
			dyt->coef *= t->yp;
			dyt->next = ply->dylist;
			ply->dylist = dyt;
			if (!ypws[t->yp - 1])
				{
				p = (POWER *)rmalloc(sizeof(POWER));
				p->pw = t->yp;
				ycoefs[t->yp - 1] = p->coefs =
					(DBL *)rmalloc(sizeof(DBL) * (p->pw + 1));
				for(i = 0; i <= p->pw; i++)
					p->coefs[i] = 0.0;
				p->next = ply->ypws;
				ply->ypws = p;
				ypws[t->yp - 1] = TRUE;
				}
			t->ycoefs = ycoefs[t->yp - 1];
			}

		if (t->zp != 0)
			{
			t->type |= _Z;
			dzt = (TERM *)rmalloc(sizeof(TERM));
			*dzt = *t;
			dzt->zp = t->zp - 1;
			dzt->coef *= t->zp;
			dzt->next = ply->dzlist;
			ply->dzlist = dzt;
			if (!zpws[t->zp - 1])
				{
				p = (POWER *)rmalloc(sizeof(POWER));
				p->pw = t->zp;
				zcoefs[t->zp - 1] = p->coefs =
					(DBL *)rmalloc(sizeof(DBL) * (p->pw + 1));
				for(i = 0; i <= p->pw; i++)
					p->coefs[i] = 0.0;
				p->next = ply->zpws;
				ply->zpws = p;
				zpws[t->zp - 1] = TRUE;
				}
			t->zcoefs = zcoefs[t->zp - 1];
			}

		}

	if(ply->bmin.x > ply->bmax.x)
		SWAP(ply->bmin.x, ply->bmax.x);
	if(ply->bmin.y > ply->bmax.y)
		SWAP(ply->bmin.y, ply->bmax.y);
	if(ply->bmin.z > ply->bmax.z)
		SWAP(ply->bmin.z, ply->bmax.z);

	if(ply->xinc < 1.0) ply->xinc = 1.0;
	ply->xinc = (ply->bmax.x - ply->bmin.x) / ply->xinc;
	if(ply->yinc < 1.0) ply->yinc = 1.0;
	ply->yinc = (ply->bmax.y - ply->bmin.y) / ply->yinc;
	if(ply->zinc < 1.0) ply->zinc = 1.0;
	ply->zinc = (ply->bmax.z - ply->bmin.z) / ply->zinc;

	obj->data = ply;
	obj->type = OBJ_POLYNOMIAL;

	mfree(xpws, sizeof(char) * ply->order);
	mfree(ypws, sizeof(char) * ply->order);
	mfree(zpws, sizeof(char) * ply->order);
	mfree(xcoefs, sizeof(DBL *) * ply->order);
	mfree(ycoefs, sizeof(DBL *) * ply->order);
	mfree(zcoefs, sizeof(DBL *) * ply->order);

	} /* end of make_poly() */

/*
 * end of poly.c
 */
