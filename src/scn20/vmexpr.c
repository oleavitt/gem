/**
 *****************************************************************************
 *  @file vmexpr.c
 *  Functions for the evaluation of expression parse trees.
 *
 *****************************************************************************
 */

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
*  vmexpr_init - Initialize the expression evaluation code.
*
*************************************************************************/
int vmexpr_init(void)
{
	frand_seed = -1; /* -1 initializes the Frand1() function. */
	Noise_Initialize(-1);

	return 1;
}

/*************************************************************************
*
*  vmeval_expr - Evaluates an expression parse tree.
*
*************************************************************************/
void vm_evalexpr(VMExpr *expr, void *result)
{
	expr->fn(expr);
	if(expr->isvec)
		V3Copy((Vec3 *)result, &expr->v);
	else
		*((double *)result) = expr->v.x;
}

double vm_evaldouble(VMExpr *expr)
{
	expr->fn(expr);
	return (expr->isvec) ? V3Mag(&expr->v) : expr->v.x;
}

void vm_evalvector(VMExpr *expr, Vec3 *vec)
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
*    VMExpr struct as expr tree is parsed and built.
*
*************************************************************************/
void vmeval_const(VMExpr *expr)
{
	/* No action here - constant value is already stored. */
	expr;
}

void vmeval_rtfloat(VMExpr *expr)
{
	expr->v.x = *(double *)expr->data;
}

void vmeval_rtvec(VMExpr *expr)
{
	V3Copy(&expr->v, (Vec3 *)expr->data);
}

void vmeval_lvalue(VMExpr *expr)
{
	V3Copy(&expr->v, &((VMLValue *)expr->data)->v);
}

void vmeval_assign(VMExpr *expr)
{
	expr->r->fn(expr->r);
	if ((!expr->r->isvec) && expr->l->isvec)
	{
		expr->v.x = expr->r->v.x;
		expr->v.y = expr->r->v.x;
		expr->v.z = expr->r->v.x;
	}
	else
		V3Copy(&expr->v, &expr->r->v);
	V3Copy(&((VMLValue *)expr->l->data)->v, &expr->v);
	expr->isvec = expr->l->isvec;
}

void vmeval_comma(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	expr->isvec = expr->r->isvec;
	V3Copy(&expr->v, &expr->r->v);
}

void vmeval_vector(VMExpr *expr)
{
	expr->r->r->fn(expr->r->r);
	expr->r->l->fn(expr->r->l);
	expr->l->fn(expr->l);
	expr->v.x = expr->r->r->v.x;
	expr->v.y = expr->r->l->v.x;
	expr->v.z = expr->l->v.x;
}

void vmeval_dot_x(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = expr->l->v.x;
}

void vmeval_dot_y(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = expr->l->v.y;
}

void vmeval_dot_z(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = expr->l->v.z;
}

void vmeval_uminus(VMExpr *expr)
{
	expr->r->fn(expr->r);
	expr->v.x = -expr->r->v.x;
	expr->v.y = -expr->r->v.y;
	expr->v.z = -expr->r->v.z;
	expr->isvec = expr->r->isvec;
}

void vmeval_bitand(VMExpr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (unsigned int)expr->l->v.x & (unsigned int)expr->r->v.x;
}

void vmeval_bitor(VMExpr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (unsigned int)expr->l->v.x | (unsigned int)expr->r->v.x;
}

void vmeval_mod(VMExpr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (unsigned int)expr->l->v.x % (unsigned int)expr->r->v.x;
}

void vmeval_logicnot(VMExpr *expr)
{
	expr->r->fn(expr->r);
	if(expr->r->isvec)
		expr->v.x = (V3Mag(&expr->r->v) > EPSILON) ? 0.0 : 1.0;
	else
		expr->v.x = (fabs(expr->r->v.x) > EPSILON) ? 0.0 : 1.0;
}

void vmeval_logicand(VMExpr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x && expr->r->v.x) ? 1 : 0;
}

void vmeval_logicor(VMExpr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x || expr->r->v.x) ? 1 : 0;
}

void vmeval_lessthan(VMExpr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x < expr->r->v.x) ? 1 : 0;
}

void vmeval_greaterthan(VMExpr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x > expr->r->v.x) ? 1 : 0;
}

void vmeval_lessequal(VMExpr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x <= expr->r->v.x) ? 1 : 0;
}

void vmeval_greatequal(VMExpr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x >= expr->r->v.x) ? 1 : 0;
}

void vmeval_isequal(VMExpr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x == expr->r->v.x) ? 1 : 0;
}

void vmeval_isnotequal(VMExpr *expr)
{
	expr->l->fn(expr->l); expr->r->fn(expr->r);
	expr->v.x = (expr->l->v.x != expr->r->v.x) ? 1 : 0;
}

void vmeval_plus(VMExpr *expr)
{
	VMExpr *l = expr->l, *r = expr->r;
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

void vmeval_minus(VMExpr *expr)
{
	VMExpr *l = expr->l, *r = expr->r;
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

void vmeval_multiply(VMExpr *expr)
{
	VMExpr *l = expr->l, *r = expr->r;
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

void vmeval_divide(VMExpr *expr)
{
	VMExpr *l = expr->l, *r = expr->r;
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

void vmeval_ternary(VMExpr *expr)
{
	VMExpr *e;
	e = (vm_evaldouble(expr->l)) ? expr->r->l : expr->r->r;
	e->fn(e);
	V3Copy(&expr->v, &e->v);
	expr->isvec = e->isvec;
}

void vmeval_pow(VMExpr *expr)
{
	expr->v.x = pow(vm_evaldouble(expr->l), vm_evaldouble(expr->r));
}

void vmeval_abs(VMExpr *expr)
{
	expr->v.x = fabs(vm_evaldouble(expr->l));
}

void vmeval_acos(VMExpr *expr)
{
	expr->v.x = acos(vm_evaldouble(expr->l));
}

void vmeval_asin(VMExpr *expr)
{
	expr->v.x = asin(vm_evaldouble(expr->l));
}

void vmeval_atan(VMExpr *expr)
{
	expr->v.x = atan(vm_evaldouble(expr->l));
}

void vmeval_atan2(VMExpr *expr)
{
	expr->v.x = atan2(vm_evaldouble(expr->l), vm_evaldouble(expr->r));
}

void vmeval_bump(VMExpr *expr)
{
	expr->l->fn(expr->l);
	rt_ON.x += expr->l->v.x;
	rt_ON.y += expr->l->v.y;
	rt_ON.z += expr->l->v.z;
  	V3Normalize(&rt_ON);
}

void vmeval_ceil(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = ceil(expr->l->v.x);
}

void vmeval_checker(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = (((int)floor(expr->l->v.x + EPSILON) +
             (int)floor(expr->l->v.y + EPSILON) +
             (int)floor(expr->l->v.z + EPSILON)) & 1) ? 1 : 0;
}

void vmeval_checker2(VMExpr *expr)
{
	VMExpr *e;
	expr->l->l->fn(expr->l->l);
	e = (((int)floor(expr->l->l->v.x + EPSILON) +
		(int)floor(expr->l->l->v.y + EPSILON) +
		(int)floor(expr->l->l->v.z + EPSILON)) & 1) ? expr->l->r : expr->r;
	e->fn(e);
	V3Copy(&expr->v, &e->v);
	expr->isvec = e->isvec;
}

void vmeval_clamp(VMExpr *expr)
{
	expr->l->l->fn(expr->l->l);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	expr->v.x = CLAMP(expr->l->l->v.x, expr->l->r->v.x, expr->r->v.x);
}

void vmeval_color_map(VMExpr *expr)
{
	expr->l->fn(expr->l);
/* TODO:	ColorMap_LookupColor((ColorMap *)expr->data, expr->l->v.x, &expr->v); */
}

void vmeval_cos(VMExpr *expr)
{
	expr->v.x = cos(vm_evaldouble(expr->l));
}

void vmeval_cosh(VMExpr *expr)
{
	expr->v.x = cosh(vm_evaldouble(expr->l));
}

void vmeval_cylinder_map(VMExpr *expr)
{
	expr->l->fn(expr->l);
	Ray_CylinderMap(&expr->l->v, &expr->v.x, &expr->v.y);
	expr->v.z = 0.0;
}

void vmeval_exp(VMExpr *expr)
{
	expr->v.x = exp(vm_evaldouble(expr->l));
}

void vmeval_floor(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = floor(expr->l->v.x);
}

void vmeval_frand(VMExpr *expr)
{
	expr->v.x = Frand1(&frand_seed);
}

/*
void vmeval_get_color(VMExpr *expr)
{
	V3Copy(&expr->v, &ray_env_color);
}
*/

void vmeval_hexagon(VMExpr *expr)
{
	expr->v.x = Hexagon2D(vm_evaldouble(expr->l), vm_evaldouble(expr->r));
}

void vmeval_hexagon2(VMExpr *expr)
{
	int result;
	VMExpr *e = expr->l->l->l;
	e->l->fn(e->l);
	e->r->fn(e->r);
	result = Hexagon2D(e->l->v.x, e->r->v.x);
	e = (result < 0) ? expr->l->l->r :
			(result > 0) ? expr->r : expr->l->r;
	e->fn(e);
	V3Copy(&expr->v, &e->v);
	expr->isvec = e->isvec;
}

void vmeval_image_map(VMExpr *expr)
{
	Image_Map((Image *)expr->data, &expr->v,
		vm_evaldouble(expr->l), vm_evaldouble(expr->r), 1);
}

void vmeval_int(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = (double)((int)expr->l->v.x);
}

void vmeval_irand(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = (double)((int)(Frand1(&frand_seed) * floor(expr->l->v.x)));
}

void vmeval_legendre(VMExpr *expr)
{
	expr->l->l->fn(expr->l->l);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	expr->v.x = Legendre((int)expr->l->l->v.x, (int)expr->l->r->v.x, expr->r->v.x);
}

void vmeval_lerp(VMExpr *expr)
{
	expr->l->l->fn(expr->l->l);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	expr->v.x = LERP(expr->l->l->v.x, expr->l->r->v.x, expr->r->v.x);
}

void vmeval_log(VMExpr *expr)
{
	expr->v.x = log(vm_evaldouble(expr->l));
}

void vmeval_log10(VMExpr *expr)
{
	expr->v.x = log10(vm_evaldouble(expr->l));
}

void vmeval_noise(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = Noise3D(&expr->l->v);
}

void vmeval_round(VMExpr *expr)
{
	double n = vm_evaldouble(expr->l);
	expr->v.x = ROUND(n);
}

/*
void vmeval_set_color(VMExpr *expr)
{
	expr->l->fn(expr->l);

	// Return the old color.
	V3Copy(&expr->v, &ray_env_color);

	// Set the env color to the new value.
	V3Copy(&ray_env_color, &expr->l->v);
}
*/
void vmeval_sin(VMExpr *expr)
{
	expr->v.x = sin(vm_evaldouble(expr->l));
}

void vmeval_sinh(VMExpr *expr)
{
	expr->v.x = sinh(vm_evaldouble(expr->l));
}

void vmeval_smooth_image_map(VMExpr *expr)
{
	Image_SmoothMap((Image *)expr->data, &expr->v,
		vm_evaldouble(expr->l), vm_evaldouble(expr->r));
}

void vmeval_sqrt(VMExpr *expr)
{
	expr->v.x = sqrt(vm_evaldouble(expr->l));
}

void vmeval_tan(VMExpr *expr)
{
	expr->v.x = tan(vm_evaldouble(expr->l));
}

void vmeval_tanh(VMExpr *expr)
{
	expr->v.x = tanh(vm_evaldouble(expr->l));
}

void vmeval_turb(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	expr->v.x = Turb3D(&expr->l->v, (int)expr->r->v.x, 2.0, 0.5);
}

void vmeval_turb2(VMExpr *expr)
{
	expr->l->l->l->fn(expr->l->l->l);
	expr->l->l->r->fn(expr->l->l->r);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	expr->v.x = Turb3D(&expr->l->l->l->v, (int)expr->l->l->r->v.x,
		expr->l->r->v.x, expr->r->v.x);
}

void vmeval_vcross(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	V3Cross(&expr->v, &expr->l->v, &expr->r->v);
}

void vmeval_vdot(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	expr->v.x = V3Dot(&expr->l->v, &expr->r->v);
}

void vmeval_vlerp(VMExpr *expr)
{
	expr->l->l->fn(expr->l->l);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	expr->v.x = LERP(expr->l->l->v.x, expr->l->r->v.x, expr->r->v.x);
	expr->v.y = LERP(expr->l->l->v.x, expr->l->r->v.y, expr->r->v.y);
	expr->v.z = LERP(expr->l->l->v.x, expr->l->r->v.z, expr->r->v.z);
}

void vmeval_vmag(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->v.x = V3Mag(&expr->l->v);
}

void vmeval_vnoise(VMExpr *expr)
{
	expr->l->fn(expr->l);
	VNoise3D(&expr->l->v, &expr->v);
}

void vmeval_vnorm(VMExpr *expr)
{
	expr->l->fn(expr->l);
	V3Copy(&expr->v, &expr->l->v);
	V3Normalize(&expr->v);
}

void vmeval_vrotate(VMExpr *expr)
{
	expr->l->l->fn(expr->l->l);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	V3Copy(&expr->v, &expr->l->l->v);
	RotatePoint3D(&expr->v, expr->r->v.x, &expr->l->r->v);
}

void vmeval_vrand(VMExpr *expr)
{
	expr->v.x = Frand1(&frand_seed);
	expr->v.y = Frand1(&frand_seed);
	expr->v.z = Frand1(&frand_seed);
}

void vmeval_vturb(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	VTurb3D(&expr->l->v, (int)expr->r->v.x, 2.0, 0.5, &expr->v);
}

void vmeval_vturb2(VMExpr *expr)
{
	expr->l->l->l->fn(expr->l->l->l);
	expr->l->l->r->fn(expr->l->l->r);
	expr->l->r->fn(expr->l->r);
	expr->r->fn(expr->r);
	VTurb3D(&expr->l->l->l->v, (int)expr->l->l->r->v.x,
		expr->l->r->v.x, expr->r->v.x, &expr->v);
}

void vmeval_wrinkle(VMExpr *expr)
{
	expr->l->fn(expr->l);
	expr->r->fn(expr->r);
	Wrinkles3D(&expr->v, &expr->l->v, (int)expr->r->v.x);
}

