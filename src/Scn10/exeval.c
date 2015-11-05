/*************************************************************************
*
*  exeval.c - Functions for the evaluation of expr parse trees.
*
*************************************************************************/

#include "local.h"

/*************************************************************************
 * Built-in variables.
 */
Vec3 rt_D;  /* Direction of current ray. */
Vec3 rt_O;  /* Point hit in object coordinates. */
Vec3 rt_W;  /* Point hit in world coordinates. */
Vec3 rt_ON; /* Surface normal in object coordinates. */
Vec3 rt_WN; /* Surface normal in world coordinates. */
double rt_u;       /* U part of object UV coordinates. */
double rt_v;       /* V part of object UV coordinates. */
double rt_uscreen; /* U part of screen UV coordinates. */
double rt_vscreen; /* V part of screen UV coordinates. */
Surface *rt_surface; /* Current surface being processed. */

static long frand_seed;

/*************************************************************************
*
*  ExprEval_Init - Initialize the expression evaluation code.
*
*************************************************************************/
void ExprEval_Init(void)
{
	frand_seed = -1; /* -1 initializes the Frand1() function. */
	Noise_Initialize(-1);
}

/*************************************************************************
*
*  ExprEval - Evaluates an expression parse tree.
*
*  Returns the type code of result and stores a type-specific result in
*    the data object pointed to by "result".
*
*************************************************************************/
int ExprEval(Expr *expr, void *result)
{
	expr->fn(expr);
	if(expr->isvec)
		V3Copy((Vec3 *)result, &expr->v);
	else
		*((double *)result) = expr->v.x;
	return expr->isvec ? EXPR_VECTOR : EXPR_FLOAT;
}

double ExprEvalDouble(Expr *expr)
{
	expr->fn(expr);
	return (expr->isvec) ? V3Mag(&expr->v) : expr->v.x;
}

void ExprEvalVector(Expr *expr, Vec3 *vec)
{
	expr->fn(expr);
	if(expr->isvec)
		V3Copy(vec, &expr->v);
	else
		vec->x = vec->y = vec->z = expr->v.x;
}

/*************************************************************************
*
*  Various action functions that get linked to the "fn" field in the
*    Expr struct as expr tree is parsed and built.
*
*************************************************************************/
void eval_const(Expr *expr)
{
}

void eval_rtfloat(Expr *expr)
{
	expr->v.x = *(double *)expr->data;
}

void eval_rtvec(Expr *expr)
{
	V3Copy(&expr->v, (Vec3 *)expr->data);
}

void eval_lvalue(Expr *expr)
{
	V3Copy(&expr->v, &((LValue *)expr->data)->v);
}

void eval_assign(Expr *expr)
{
	expr->r->fn(expr->r);
	V3Copy(&expr->v, &expr->r->v);
	V3Copy(&((LValue *)expr->l->data)->v, &expr->v);
	expr->isvec = expr->l->isvec;
}

void eval_comma(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	expr->isvec = expr->r->isvec;
	V3Copy(&expr->v, &expr->r->v);
}

void eval_vector(Expr *expr)
{
	expr->r->r->fn(expr->r->r);
	expr->r->l->fn(expr->r->l);
	expr->l->fn(expr->l);
	expr->v.x = expr->r->r->v.x;
	expr->v.y = expr->r->l->v.x;
	expr->v.z = expr->l->v.x;
}

void eval_dot_x(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = expr->l->v.x;
}

void eval_dot_y(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = expr->l->v.y;
}

void eval_dot_z(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = expr->l->v.z;
}

void eval_uminus(Expr *expr)
{
	expr->r->fn(expr->r);
	expr->v.x = -expr->r->v.x;
	expr->v.y = -expr->r->v.y;
	expr->v.z = -expr->r->v.z;
	expr->isvec = expr->r->isvec;
}

void eval_bitand(Expr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (unsigned int)expr->l->v.x & (unsigned int)expr->r->v.x;
}

void eval_bitor(Expr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (unsigned int)expr->l->v.x | (unsigned int)expr->r->v.x;
}

void eval_mod(Expr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (unsigned int)expr->l->v.x % (unsigned int)expr->r->v.x;
}

void eval_logicnot(Expr *expr)
{
	expr->r->fn(expr->r);
	if(expr->r->isvec)
		expr->v.x = (V3Mag(&expr->r->v) > EPSILON) ? 0.0 : 1.0;
	else
		expr->v.x = (fabs(expr->r->v.x) > EPSILON) ? 0.0 : 1.0;
}

void eval_logicand(Expr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x && expr->r->v.x) ? 1 : 0;
}

void eval_logicor(Expr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x || expr->r->v.x) ? 1 : 0;
}

void eval_lessthan(Expr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x < expr->r->v.x) ? 1 : 0;
}

void eval_greaterthan(Expr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x > expr->r->v.x) ? 1 : 0;
}

void eval_lessequal(Expr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x <= expr->r->v.x) ? 1 : 0;
}

void eval_greatequal(Expr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x >= expr->r->v.x) ? 1 : 0;
}

void eval_isequal(Expr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x == expr->r->v.x) ? 1 : 0;
}

void eval_isnotequal(Expr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x != expr->r->v.x) ? 1 : 0;
}

void eval_plus(Expr *expr)
{
	Expr *l = expr->l, *r = expr->r;
	l->fn(l); r->fn(r);
	expr->v.x = l->v.x + r->v.x;
	if(l->isvec)
	{
		if(r->isvec)
		{
			expr->v.y = l->v.y + r->v.y;
			expr->v.z = l->v.z + r->v.z;
		}
		else
		{
			expr->v.y = l->v.y + r->v.x;
			expr->v.z = l->v.z + r->v.x;
		}
		expr->isvec = 1;
	}
	else if(r->isvec)
	{
		expr->v.y = l->v.x + r->v.y;
		expr->v.z = l->v.x + r->v.z;
		expr->isvec = 1;
	}
	else
		expr->isvec = 0;
}

void eval_minus(Expr *expr)
{
	Expr *l = expr->l, *r = expr->r;
	l->fn(l); r->fn(r);
	expr->v.x = l->v.x - r->v.x;
	if(l->isvec)
	{
		if(r->isvec)
		{
			expr->v.y = l->v.y - r->v.y;
			expr->v.z = l->v.z - r->v.z;
		}
		else
		{
			expr->v.y = l->v.y - r->v.x;
			expr->v.z = l->v.z - r->v.x;
		}
		expr->isvec = 1;
	}
	else if(r->isvec)
	{
		expr->v.y = l->v.x - r->v.y;
		expr->v.z = l->v.x - r->v.z;
		expr->isvec = 1;
	}
	else
		expr->isvec = 0;
}

void eval_multiply(Expr *expr)
{
	Expr *l = expr->l, *r = expr->r;
	l->fn(l); r->fn(r);
	expr->v.x = l->v.x * r->v.x;
	if(l->isvec)
	{
		if(r->isvec)
		{
			expr->v.y = l->v.y * r->v.y;
			expr->v.z = l->v.z * r->v.z;
		}
		else
		{
			expr->v.y = l->v.y * r->v.x;
			expr->v.z = l->v.z * r->v.x;
		}
		expr->isvec = 1;
	}
	else if(r->isvec)
	{
		expr->v.y = l->v.x * r->v.y;
		expr->v.z = l->v.x * r->v.z;
		expr->isvec = 1;
	}
	else
		expr->isvec = 0;
}

void eval_divide(Expr *expr)
{
	Expr *l = expr->l, *r = expr->r;
	l->fn(l); r->fn(r);
	expr->v.x = l->v.x / r->v.x;
	if(l->isvec)
	{
		if(r->isvec)
		{
			expr->v.y = l->v.y / r->v.y;
			expr->v.z = l->v.z / r->v.z;
		}
		else
		{
			expr->v.y = l->v.y / r->v.x;
			expr->v.z = l->v.z / r->v.x;
		}
		expr->isvec = 1;
	}
	else if(r->isvec)
	{
		expr->v.y = l->v.x / r->v.y;
		expr->v.z = l->v.x / r->v.z;
		expr->isvec = 1;
	}
	else
		expr->isvec = 0;
}

void eval_ternary(Expr *expr)
{
	Expr *e;
	expr->l->fn(expr->l);
	e = (expr->l->v.x) ? expr->r->l : expr->r->r;
	e->fn(e);
	V3Copy(&expr->v, &e->v);
	expr->isvec = e->isvec;
}

void eval_pow(Expr *expr)
{
	Expr *l = expr->l, *r = expr->r;
	l->fn(l); r->fn(r);
	expr->v.x = pow(l->v.x, r->v.x);
}

void eval_abs(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = fabs(expr->l->v.x);
}

void eval_acos(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = acos(expr->l->v.x);
}

void eval_asin(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = asin(expr->l->v.x);
}

void eval_atan(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = atan(expr->l->v.x);
}

void eval_atan2(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	expr->v.x = atan2(expr->l->v.x, expr->r->v.x);
}

void eval_ceil(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = ceil(expr->l->v.x);
}

void eval_checker(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = (((int)floor(expr->l->v.x + EPSILON) +
             (int)floor(expr->l->v.y + EPSILON) +
             (int)floor(expr->l->v.z + EPSILON)) & 1) ? 1 : 0;
}

void eval_checker2(Expr *expr)
{
	Expr *e;
	expr->l->l->fn(expr->l->l);
	e = (((int)floor(expr->l->l->v.x + EPSILON) +
		(int)floor(expr->l->l->v.y + EPSILON) +
		(int)floor(expr->l->l->v.z + EPSILON)) & 1) ? expr->l->r : expr->r;
	e->fn(e);
	V3Copy(&expr->v, &e->v);
	expr->isvec = e->isvec;
}

void eval_clamp(Expr *expr)
{
	expr->l->l->fn(expr->l->l);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	expr->v.x = CLAMP(expr->l->l->v.x, expr->l->r->v.x, expr->r->v.x);
}

void eval_color_map(Expr *expr)
{
	expr->l->fn(expr->l);
	ColorMap_LookupColor((ColorMap *)expr->data, expr->l->v.x, &expr->v);
}

void eval_cos(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = cos(expr->l->v.x);
}

void eval_cosh(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = cosh(expr->l->v.x);
}

void eval_exp(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = exp(expr->l->v.x);
}

void eval_floor(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = floor(expr->l->v.x);
}

void eval_frand(Expr *expr)
{
	expr->v.x = Frand1(&frand_seed);
}

void eval_hexagon(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	expr->v.x = Hexagon2D(expr->l->v.x, expr->r->v.x);
}

void eval_hexagon2(Expr *expr)
{
	int result;
	Expr *e = expr->l->l->l;
	e->l->fn(e->l);
	e->r->fn(e->r);
	result = Hexagon2D(e->l->v.x, e->r->v.x);
	e = (result < 0) ? expr->l->l->r :
			(result > 0) ? expr->r : expr->l->r;
	e->fn(e);
	V3Copy(&expr->v, &e->v);
	expr->isvec = e->isvec;
}

void eval_image_map(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	Image_Map((Image *)expr->data, &expr->v, expr->l->v.x, expr->r->v.x, 1);
}

void eval_int(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = (double)((int)expr->l->v.x);
}

void eval_irand(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = (double)((int)(Frand1(&frand_seed) * floor(expr->l->v.x)));
}

void eval_legendre(Expr *expr)
{
	expr->l->l->fn(expr->l->l);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	expr->v.x = Legendre((int)expr->l->l->v.x, (int)expr->l->r->v.x, expr->r->v.x);
}

void eval_lerp(Expr *expr)
{
	expr->l->l->fn(expr->l->l);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	expr->v.x = LERP(expr->l->l->v.x, expr->l->r->v.x, expr->r->v.x);
}

void eval_log(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = log(expr->l->v.x);
}

void eval_log10(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = log10(expr->l->v.x);
}

void eval_noise(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = Noise3D(&expr->l->v);
}

void eval_round(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = ROUND(expr->l->v.x);
}

void eval_sin(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = sin(expr->l->v.x);
}

void eval_sinh(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = sinh(expr->l->v.x);
}

void eval_smooth_image_map(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	Image_SmoothMap((Image *)expr->data, &expr->v, expr->l->v.x,
		expr->r->v.x);
}

void eval_sqrt(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = sqrt(expr->l->v.x);
}

void eval_tan(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = tan(expr->l->v.x);
}

void eval_tanh(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = tanh(expr->l->v.x);
}

void eval_turb(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	expr->v.x = Turb3D(&expr->l->v, (int)expr->r->v.x, 2.0, 0.5);
}

void eval_turb2(Expr *expr)
{
	expr->l->l->l->fn(expr->l->l->l);
	expr->l->l->r->fn(expr->l->l->r);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	expr->v.x = Turb3D(&expr->l->l->l->v, (int)expr->l->l->r->v.x,
		expr->l->r->v.x, expr->r->v.x);
}

void eval_vcross(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	V3Cross(&expr->v, &expr->l->v, &expr->r->v);
}

void eval_vdot(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	expr->v.x = V3Dot(&expr->l->v, &expr->r->v);
}

void eval_vlerp(Expr *expr)
{
	expr->l->l->fn(expr->l->l);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	expr->v.x = LERP(expr->l->l->v.x, expr->l->r->v.x, expr->r->v.x);
	expr->v.y = LERP(expr->l->l->v.x, expr->l->r->v.y, expr->r->v.y);
	expr->v.z = LERP(expr->l->l->v.x, expr->l->r->v.z, expr->r->v.z);
}

void eval_vmag(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = V3Mag(&expr->l->v);
}

void eval_vnoise(Expr *expr)
{
	expr->l->fn(expr->l);
	VNoise3D(&expr->l->v, &expr->v);
}

void eval_vnorm(Expr *expr)
{
	expr->l->fn(expr->l);
	V3Copy(&expr->v, &expr->l->v);
	V3Normalize(&expr->v);
}

void eval_vrotate(Expr *expr)
{
	expr->l->l->fn(expr->l->l);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	V3Copy(&expr->v, &expr->l->l->v);
	RotatePoint3D(&expr->v, expr->r->v.x, &expr->l->r->v);
}

void eval_vrand(Expr *expr)
{
	expr->v.x = Frand1(&frand_seed);
	expr->v.y = Frand1(&frand_seed);
	expr->v.z = Frand1(&frand_seed);
}

void eval_vturb(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	VTurb3D(&expr->l->v, (int)expr->r->v.x, 2.0, 0.5, &expr->v);
}

void eval_vturb2(Expr *expr)
{
	expr->l->l->l->fn(expr->l->l->l);
	expr->l->l->r->fn(expr->l->l->r);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	VTurb3D(&expr->l->l->l->v, (int)expr->l->l->r->v.x,
		expr->l->r->v.x, expr->r->v.x, &expr->v);
}

void eval_wrinkle(Expr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	Wrinkles3D(&expr->v, &expr->l->v, (int)expr->r->v.x);
}

