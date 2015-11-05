/*************************************************************************
 *
 *   expr.c - Expression parsing and evaluation functions.
 *
 *************************************************************************/

#include "scn.h"


/*************************************************************************
 * Built-in variables.
 */
Vec3 rt_D;  /* Direction of current ray. */
static RT_LVALUE lv_rt_D;
Vec3 rt_O;  /* Point hit in object coordinates. */
static RT_LVALUE lv_rt_O;
static RT_LVALUE lv_rt_Ox;
static RT_LVALUE lv_rt_Oy;
static RT_LVALUE lv_rt_Oz;
Vec3 rt_W;  /* Point hit in world coordinates. */
static RT_LVALUE lv_rt_W;
static RT_LVALUE lv_rt_Wx;
static RT_LVALUE lv_rt_Wy;
static RT_LVALUE lv_rt_Wz;
Vec3 rt_ON; /* Surface normal in object coordinates. */
static RT_LVALUE lv_rt_ON;
static RT_LVALUE lv_rt_ONx;
static RT_LVALUE lv_rt_ONy;
static RT_LVALUE lv_rt_ONz;
Vec3 rt_WN; /* Surface normal in world coordinates. */
static RT_LVALUE lv_rt_WN;
static RT_LVALUE lv_rt_WNx;
static RT_LVALUE lv_rt_WNy;
static RT_LVALUE lv_rt_WNz;
double rt_u;     /* U part of object UV coordinates. */
static RT_LVALUE lv_rt_u;
double rt_v;     /* V part of object UV coordinates. */
static RT_LVALUE lv_rt_v;
double rt_uscreen; /* U part of screen UV coordinates. */
static RT_LVALUE lv_rt_uscreen;
double rt_vscreen; /* V part of screen UV coordinates. */
static RT_LVALUE lv_rt_vscreen;

Surface *rt_surface; /* Current surface being processed. */

static EXPR *new_node(void);
static EXPR *expr0(void);
static EXPR *expr1(void);

static int re_entering;

/* Scratch pad vectors & floats. */
static Vec3 V1, V2;
static double x, y, z;

/* Current token being processed. */
static int token;

/* True while ">" is expected as the end of a vector triplet. */
static int end_of_vec;

/* Return point in case of parse error. */
static jmp_buf err_bailout;

/* Sequence value for Frand1(). */
static long frand1_seed;

/*************************************************************************
 *  Built-in constants.
 */
static const double const_PI = PI;

/*************************************************************************
 * Local functions...
 */

/*************************************************************************
 * Various actions for operators and functions...
 * These all must match the declaration for the "(* fn)()" pointer
 * in the node structure - see expr.h.
 */

/*
 * Constants.
 */
static void const_fn(EXPR *n)
{
}

/*
 * L-values.
 */
static void flvalue_fn(EXPR *n)
{
  LVALUE *lv = (LVALUE *)n->data;
  n->V.x = *lv->data.pf;
}

static void vlvalue_fn(EXPR *n)
{
  LVALUE *lv = (LVALUE *)n->data;
  n->V = *lv->data.pv;
}

/*
 * Reference a value in an array.
 */
static void subscript_fn(EXPR *n)
{
}

/*
 * User-defined proc that returns a value.
 */
static void user_fn(EXPR *n)
{
  STATEMENT *s;

  s = (STATEMENT *)n->data;
  Exec_Proc(s);
  n->is_vec = (s->data.proc->ret_type == EXP_VECTOR) ? 1 : 0;
  n->V = s->data.proc->ret_result;
}

/*
 * Unary minus.
 */
static void uminus_fn(EXPR *n)
{
  (n->r->fn)(n->r);
  n->V.x = - n->r->V.x;
  n->V.y = - n->r->V.y;
  n->V.z = - n->r->V.z;
  n->is_vec = n->r->is_vec;
}

/*
 * Logical negation.
 */
static void not_fn(EXPR *n)
{
  (n->r->fn)(n->r);
  n->V.x = (n->r->V.x == 0.0) ? 1 : 0;
}

/*
 * Addition.
 */
static void plus_fn(EXPR *n)
{
  EXPR *l = n->l  , *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x = l->V.x + r->V.x;
  if(l->is_vec)
  {
    n->is_vec = 1;
    if(r->is_vec)
    {
      n->V.y = l->V.y + r->V.y;
      n->V.z = l->V.z + r->V.z;
    }
    else
    {
      n->V.y = l->V.y + r->V.x;
      n->V.z = l->V.z + r->V.x;
    }
  }
  else if(r->is_vec)
  {
    n->is_vec = 1;
    n->V.y = l->V.x + r->V.y;
    n->V.z = l->V.x + r->V.z;
  }
  else
    n->is_vec = 0;
}

/*
 * Subtraction.
 */
static void minus_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x = l->V.x - r->V.x;
  if(l->is_vec)
  {
    n->is_vec = 1;
    if(r->is_vec)
    {
      n->V.y = l->V.y - r->V.y;
      n->V.z = l->V.z - r->V.z;
    }
    else
    {
      n->V.y = l->V.y - r->V.x;
      n->V.z = l->V.z - r->V.x;
    }
  }
  else if(r->is_vec)
  {
    n->is_vec = 1;
    n->V.y = l->V.x - r->V.y;
    n->V.z = l->V.x - r->V.z;
  }
  else
    n->is_vec = 0;
}

/*
 * Multiplication.
 */
static void mult_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x = l->V.x * r->V.x;
  if(l->is_vec)
  {
    n->is_vec = 1;
    if(r->is_vec)
    {
      n->V.y = l->V.y * r->V.y;
      n->V.z = l->V.z * r->V.z;
    }
    else
    {
      n->V.y = l->V.y * r->V.x;
      n->V.z = l->V.z * r->V.x;
    }
  }
  else if(r->is_vec)
  {
    n->is_vec = 1;
    n->V.y = l->V.x * r->V.y;
    n->V.z = l->V.x * r->V.z;
  }
  else
    n->is_vec = 0;
}

/*
 * Division.
 */
static void div_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x = l->V.x / r->V.x;
  if(l->is_vec)
  {
    n->is_vec = 1;
    if(r->is_vec)
    {
      n->V.z = l->V.y / r->V.y;
      n->V.z = l->V.z / r->V.z;
    }
    else
    {
      n->V.y = l->V.y / r->V.x;
      n->V.z = l->V.z / r->V.x;
    }
  }
  else if(r->is_vec)
  {
    n->is_vec = 1;
    n->V.y = l->V.x / r->V.y;
    n->V.z = l->V.x / r->V.z;
  }
  else
    n->is_vec = 0;
}

/*
 * Modulus.
 */
static void mod_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x = fmod(l->V.x, r->V.x);
  if(l->is_vec)
  {
    if(r->is_vec)
    {
      n->V.y = fmod(l->V.y, r->V.y);
      n->V.z = fmod(l->V.z, r->V.z);
    }
    else
    {
      n->V.y = fmod(l->V.y, r->V.x);
      n->V.z = fmod(l->V.z, r->V.x);
    }
    n->is_vec = 1;
  }
  else
    n->is_vec = 0;
}

/*
 * Exponentiation.
 */
static void pow_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x = pow(l->V.x, r->V.x);
}

/*
 * Assignment.
 */
static void assign_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  LVALUE *lv;

  /* (l->fn)(l); */
  (r->fn)(r);
  if(l->is_vec == r->is_vec)
  {
    l->V = r->V;
  }
  else if(l->is_vec)
  {
    l->V.x = l->V.y = l->V.z = r->V.x;
  }
  else
  {
    l->V.x = V3Mag(&r->V);
  }
  n->is_vec = l->is_vec;
  n->V = l->V;
  lv = (LVALUE *)l->data;
  if(lv->type == TK_FLOAT)
    *lv->data.pf = n->V.x;
  else
    *lv->data.pv = n->V;
}

static void plusassign_fn(EXPR *n)
{
}

static void minusassign_fn(EXPR *n)
{
}

static void multassign_fn(EXPR *n)
{
}

static void divassign_fn(EXPR *n)
{
}

/*
 * Ternary operator  ?:
 */
static void ternary_fn(EXPR *n)
{
  EXPR *colon, *e;

  (n->l->fn)(n->l);
  colon = n->r;
  e = (n->l->V.x != 0.0) ? colon->l : colon->r;
  (e->fn)(e);
  n->V = e->V;
  n->is_vec = e->is_vec;
}

/*
 * Logical equality operator  ==
 */
static void equal_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x =(l->V.x == r->V.x) ? 1 : 0;
}

/*
 * Logical inequality operator  !=
 */
static void notequal_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x =(l->V.x != r->V.x) ? 1 : 0;
}

/*
 * Logical less-than operator  <
 */
static void lessthan_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x =(l->V.x < r->V.x) ? 1 : 0;
}

/*
 * Logical less-than or equal-to operator  <=
 */
static void lessequal_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x =(l->V.x <= r->V.x) ? 1 : 0;
}

/*
 * Logical greater-than operator  >
 */
static void greaterthan_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x =(l->V.x > r->V.x) ? 1 : 0;
}

/*
 * Logical greater-than or equal-to operator  >=
 */
static void greatequal_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x =(l->V.x >= r->V.x) ? 1 : 0;
}

/*
 * Logical AND operator  &&
 */
static void andand_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x =(l->V.x && r->V.x) ? 1 : 0;
}

/*
 * Logical OR operator  ||
 */
static void oror_fn(EXPR *n)
{
  EXPR *l = n->l, *r = n->r;
  (l->fn)(l);
  (r->fn)(r);
  n->V.x =(l->V.x || r->V.x) ? 1 : 0;
}

/*
 * Comma operator  ,
 */
static void comma_fn(EXPR *n)
{
  EXPR *e;
  for(e = n; e != NULL; e = e->r)
  {
    (e->l->fn)(e->l);
    n->V = e->l->V;
    n->is_vec = e->l->is_vec;
  }
}

/*
 * Built-in functions.
 */
static void abs_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x =(n->l->is_vec) ? V3Mag(&n->l->V) : fabs(n->l->V.x);
}

static void acos_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = acos(n->l->V.x);
}

static void asin_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = asin(n->l->V.x);
}

static void atan_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = atan(n->l->V.x);
}

static void atan2_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  (n->r->fn)(n->r);
  n->V.x = atan2(n->l->V.x, n->r->V.x);
}

static void ceil_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = ceil(n->l->V.x);
  if(n->l->is_vec)
  {
    n->V.y = ceil(n->l->V.y);
    n->V.z = ceil(n->l->V.z);
    n->is_vec = 1;
  }
  else
    n->is_vec = 0;
}

static void checker_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  V1 = n->l->V;
  n->V.x = (((int)floor(V1.x + EPSILON) +
             (int)floor(V1.y + EPSILON) +
             (int)floor(V1.z + EPSILON)) & 1) ? 1 : 0;
}

static void clamp_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  (n->r->l->fn)(n->r->l);
  (n->r->r->fn)(n->r->r);
  x = n->l->V.x;
  y = n->r->l->V.x;
  z = n->r->r->V.x;
  n->V.x = (x > y) ? ((x < z) ? x : z) : ((y < z) ? y : z);
}

static void cos_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = cos(n->l->V.x);
}

static void cosh_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = cosh(n->l->V.x);
}

static void cylinder_map_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  Ray_CylinderMap(&n->l->V, &n->V.x, &n->V.y);
  n->V.z = 0.0;
}

static void deg_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = n->l->V.x * RTOD;
}

static void disc_map_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  Ray_DiscMap(&n->l->V, &n->V.x, &n->V.y);
  n->V.z = 0.0;
}

static void environment_map_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.z = Ray_EnvironmentMap(&n->l->V, &n->V.x, &n->V.y);
}

static void exp_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = exp(n->l->V.x);
}

static void floor_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = floor(n->l->V.x);
  if(n->l->is_vec)
  {
    n->V.y = floor(n->l->V.y);
    n->V.z = floor(n->l->V.z);
    n->is_vec = 1;
  }
  else
    n->is_vec = 0;
}

static void frame_fn(EXPR *n)
{
  n->V.x = (double)scn_cur_frame;
}

static void hexagon_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  (n->r->fn)(n->r);
  n->V.x = Lookup_Hexagon2D(n->l->V.x, n->r->V.x);
}

static void int_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = (long)(n->l->V.x);
  if(n->l->is_vec)
  {
    n->V.y = (long)(n->l->V.y);
    n->V.z = (long)(n->l->V.z);
    n->is_vec = 1;
  }
  else
    n->is_vec = 0;
}

static void image_map_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  (n->r->fn)(n->r);
  Image_Map((Image *)(n->data), &n->V, n->l->V.x, n->r->V.x, 1);
}

static void ln_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = log(n->l->V.x);
}

static void log10_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = log10(n->l->V.x);
}

static void nframe_fn(EXPR *n)
{
  n->V.x = scn_normalized_frame;
}

static void nframe2_fn(EXPR *n)
{
  n->V.x = scn_normalized_frame2;
}

static void noise_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  V1 = n->l->V;
  if(! n->l->is_vec)
  {
    V1.y = V1.x;
    V1.z = V1.x;
  }
  n->V.x = Lookup_Noise3D(&V1);
}

static void plane_map_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  Ray_PlaneMap(&n->l->V, &n->V.x, &n->V.y);
  n->V.z = 0.0;
}

static void rad_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = n->l->V.x * DTOR;
}

static void rand_fn(EXPR *n)
{
  n->V.x = Frand1(&frand1_seed);
}

static void rotate_fn(EXPR *n)
{
  double sin_theta, cos_theta;
  (n->l->fn)(n->l);
  (n->r->fn)(n->r);
  n->V = n->l->V;
  V2 = n->r->V;
  if(! n->l->is_vec)
    n->V.z = n->V.y = n->V.x;
  if(! n->r->is_vec)
    V2.z = V2.y = V2.x;
  /* rotate x axis */
  V1 = n->V;
  sin_theta = sin(V2.x * DTOR);
  cos_theta = cos(V2.x * DTOR);
  n->V.y =   V1.y * cos_theta + V1.z * sin_theta;
  n->V.z = - V1.y * sin_theta + V1.z * cos_theta;
  /* rotate y axis */
  V1 = n->V;
  sin_theta = sin(V2.y * DTOR);
  cos_theta = cos(V2.y * DTOR);
  n->V.x =   V1.x * cos_theta - V1.z * sin_theta;
  n->V.z =   V1.x * sin_theta + V1.z * cos_theta;
  /* rotate z axis */
  V1 = n->V;
  sin_theta = sin(V2.z * DTOR);
  cos_theta = cos(V2.z * DTOR);
  n->V.x =   V1.x * cos_theta + V1.y * sin_theta;
  n->V.y = - V1.x * sin_theta + V1.y * cos_theta;
}

static void scale_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  (n->r->fn)(n->r);
  V1 = n->l->V;
  V2 = n->r->V;
  if(! n->l->is_vec)
    V1.z = V1.y = V1.x;
  if(! n->r->is_vec)
    V2.z = V2.y = V2.x;
  n->V.x = V1.x * V2.x;
  n->V.y = V1.y * V2.y;
  n->V.z = V1.z * V2.z;
}

static void sin_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = sin(n->l->V.x);
}

static void sinh_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = sinh(n->l->V.x);
}

static void smooth_image_map_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  (n->r->fn)(n->r);
  Image_SmoothMap((Image *)(n->data), &n->V, n->l->V.x, n->r->V.x);
}

static void sphere_map_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  Ray_SphereMap(&n->l->V, &n->V.x, &n->V.y);
  n->V.z = 0.0;
}


static void sqrt_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = sqrt(n->l->V.x);
}

static void tan_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = tan(n->l->V.x);
}

static void tanh_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V.x = tanh(n->l->V.x);
}

static void torus_map_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  Ray_TorusMap(&n->l->V, &n->V.x, &n->V.y);
  n->V.z = 0.0;
}

static void translate_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  (n->r->fn)(n->r);
  V1 = n->l->V;
  V2 = n->r->V;
  if(! n->l->is_vec)
    V1.z = V1.y = V1.x;
  if(! n->r->is_vec)
    V2.z = V2.y = V2.x;
  n->V.x = V1.x + V2.x;
  n->V.y = V1.y + V2.y;
  n->V.z = V1.z + V2.z;
}

static void turb_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  (n->r->fn)(n->r);
  n->V.x = Lookup_Turb3D(&n->l->V, (int)n->r->V.x, 2.0, 0.5);
}

static void turb2_fn(EXPR *n)
{
  EXPR *n2;
  (n->l->fn)(n->l);
  n2 = n->r;
  (n2->l->fn)(n2->l);
  n2 = n2->r;
  (n2->l->fn)(n2->l);
  (n2->r->fn)(n2->r);
  V1 = n->l->V;  /* 3D point */
  n2 = n->r;
  x = n2->l->V.x;        /* octaves */
  n2 = n2->r;
  y = n2->l->V.x;        /* amplitude */
  z = n2->r->V.x;        /* frequency */

  n->V.x = Lookup_Turb3D(&V1, (int)x, y, z);
}


static void vdot_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  (n->r->fn)(n->r);
  n->V.x = V3Dot(&n->l->V, &n->r->V);
}

static void vcross_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  (n->r->fn)(n->r);
  V3Cross(&n->V, &n->l->V, &n->r->V);
}

static void vector_fn(EXPR *n)
{
  Vec3 *v = &n->V;
  (n->l->fn)(n->l);
  v->x = n->l->V.x;
  n = n->r;
  (n->l->fn)(n->l);
  v->y = n->l->V.x;
  (n->r->fn)(n->r);
  v->z = n->r->V.x;
}

static void vnorm_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  n->V = n->l->V;
  V3Normalize(&n->V);
}

static void vnoise_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  V1 = n->l->V;
  if(! n->l->is_vec)
  {
    V1.y = V1.x;
    V1.z = V1.x;
  }
  Lookup_VNoise3D(&V1, &n->V);
}

static void vrand_fn(EXPR *n)
{
  n->V.x = Frand1(&frand1_seed);
  n->V.y = Frand1(&frand1_seed);
  n->V.z = Frand1(&frand1_seed);
}

static void vturb_fn(EXPR *n)
{
  (n->l->fn)(n->l);
  (n->r->fn)(n->r);
  Lookup_VTurb3D(&n->l->V, (int)n->r->V.x, 2.0, 0.5, &n->V);
}

static void vturb2_fn(EXPR *n)
{
  EXPR *n2;
  (n->l->fn)(n->l);
  n2 = n->r;
  (n2->l->fn)(n2->l);
  n2 = n2->r;
  (n2->l->fn)(n2->l);
  (n2->r->fn)(n2->r);
  V1 = n->l->V;  /* 3D point */
  n2 = n->r;
  x = n2->l->V.x;        /* octaves */
  n2 = n2->r;
  y = n2->l->V.x;        /* amplitude */
  z = n2->r->V.x;        /* frequency */

  Lookup_VTurb3D(&V1, (int)x, y, z, &n->V);
}

/*************************************************************************
 * Support functions for the expression parser...
 */

static EXPR *new_node(void)
{
  EXPR *n;

  if((n = (EXPR *)mmalloc(sizeof(EXPR))) != NULL)
  {
    memset(n, 0, sizeof(EXPR));
    n->l = n->r = NULL;
    n->fn = const_fn;
    n->nrefs = 1;
  }
/*  else
  {
    SCN_Message(SCN_MSG_ERROR, "expr: Memory allocation error.");
    longjmp(err_bailout, 1);
  }  */
  return n;
}

static void chk_token(int expected, char *keyword)
{
  if(token != expected)
  {
    SCN_Message(SCN_MSG_ERROR, 
      "Expression syntax: `%s' expected. Found `%s'.",
      keyword, token_buffer);
    longjmp(err_bailout, 1);
  }
}

static void chk_param(EXPR *p)
{
  if(p == NULL)
  {
    SCN_Message(SCN_MSG_ERROR, 
      "Expression: Parameter expected in function. Found `%s'."
      , token_buffer);
    longjmp(err_bailout, 1);
  }
}

static void err_val_expected(void)
{
  SCN_Message(SCN_MSG_ERROR, 
    "Expression syntax: Numeric value expected. Found `%s'.",
    token_buffer);
  longjmp(err_bailout, 1);
}

static void chk_ptr(void *ptr)
{
  if(ptr == NULL)
  {
    SCN_Message(SCN_MSG_ERROR, 
      "Expression: Unknown or invalid identifier name: `%s'.",
      token_buffer);
    longjmp(err_bailout, 1);
  }
}


static EXPR *make_lvalue(LVALUE *lv)
{
  EXPR *n = new_node();
  if(lv->type == TK_VECTOR)
  {
    n->fn = vlvalue_fn;
    n->is_vec = 1;
  }
  else
  {
    n->fn = flvalue_fn;
    n->is_vec = 0;
  }
  n->data = lv;
  return n;
}


static EXPR *make_func0(void *fn, int nargs, int is_vec)
{
  EXPR *node, *n;
  n = new_node();
  n->fn = fn;
  n->is_vec = is_vec;
  node = n;
  token = Get_Token();
  switch(nargs)
  {
    case 0:
      break;
    case 1:
      n->l = expr1();
      chk_param(n->l);
      break;
    default:
      nargs -= 2;
      n->l = expr1();
      chk_param(n->l);
      chk_token(OP_COMMA, ",");
      token = Get_Token();
      while(nargs--)
      {
        n->r = new_node();
        n = n->r;
        n->l = expr1();
        chk_param(n->l);
        chk_token(OP_COMMA, ",");
        token = Get_Token();
      }
      n->r = expr1();
      chk_param(n->r);
      break;
  }
  return node;
}


static EXPR *make_func(void *fn, int nargs, int is_vec)
{
  EXPR *node;
  token = Get_Token();
  chk_token(OP_LPAREN, "(");
  node = make_func0(fn, nargs, is_vec);
  chk_token(OP_RPAREN, ")");
  token = Get_Token();
  return node;
}


static EXPR *make_func2(void *fn, int nargs, int is_vec)
{
  EXPR *node;
  token = Get_Token();
  chk_token(OP_COMMA, ",");
  node = make_func0(fn, nargs, is_vec);
  chk_token(OP_RPAREN, ")");
  token = Get_Token();
  return node;
}


/*
 * Constants, lvalues, built-in variables, array/vector index,
 * parenthesis, and functions.
 * 123.45, my_var, built_in_var, my_array[r], (expr), func(expr, expr)
 */
static EXPR *atom(void)
{
  EXPR *n;

  n = NULL;
  switch(token)
  {
    /*
     * Constants.
     */
    case CV_INT_CONST:
    case CV_FLOAT_CONST:
      n = new_node();
      n->fn = const_fn;
      n->is_vec = 0;
      n->V.x = atof(token_buffer);
      token = Get_Token();
      break;
    case CV_PI_CONST:
      n = new_node();
      n->fn = const_fn;
      n->V.x = const_PI;
      n->is_vec = 0;
      token = Get_Token();
      break;

    case OP_LESSTHAN:
      n = new_node();
      n->fn = vector_fn;
      n->is_vec = 1;
      token = Get_Token();
      n->l = expr1();
      if(n->l == NULL)
        err_val_expected();
      chk_token(OP_COMMA, ",");
      n->r = new_node();
      token = Get_Token();
      n->r->l = expr1();
      if(n->r->l == NULL)
        err_val_expected();
      chk_token(OP_COMMA, ",");
      token = Get_Token();
      end_of_vec++;
      n->r->r = expr1();
      if(n->r->r == NULL)
        err_val_expected();
      end_of_vec--;
      chk_token(OP_GREATERTHAN, ">");
      token = Get_Token();
      break;
    case OP_LPAREN:
      token = Get_Token();
      n = expr0();
      chk_token(OP_RPAREN, ")");
      token = Get_Token();
      break;

    /*
     * Built-in variables.
     */
    case RT_D:
      n = make_lvalue((LVALUE *)&lv_rt_D);
      token = Get_Token();
      break;
    case RT_O:
      n = make_lvalue((LVALUE *)&lv_rt_O);
      token = Get_Token();
      break;
    case RT_x:
      n = make_lvalue((LVALUE *)&lv_rt_Ox);
      token = Get_Token();
      break;
    case RT_y:
      n = make_lvalue((LVALUE *)&lv_rt_Oy);
      token = Get_Token();
      break;
    case RT_z:
      n = make_lvalue((LVALUE *)&lv_rt_Oz);
      token = Get_Token();
      break;
    case RT_W:
      n = make_lvalue((LVALUE *)&lv_rt_W);
      token = Get_Token();
      break;
    case RT_X:
      n = make_lvalue((LVALUE *)&lv_rt_Wx);
      token = Get_Token();
      break;
    case RT_Y:
      n = make_lvalue((LVALUE *)&lv_rt_Wy);
      token = Get_Token();
      break;
    case RT_Z:
      n = make_lvalue((LVALUE *)&lv_rt_Wz);
      token = Get_Token();
      break;
    case RT_ON:
      n = make_lvalue((LVALUE *)&lv_rt_ON);
      token = Get_Token();
      break;
    case RT_ONx:
      n = make_lvalue((LVALUE *)&lv_rt_ONx);
      token = Get_Token();
      break;
    case RT_ONy:
      n = make_lvalue((LVALUE *)&lv_rt_ONy);
      token = Get_Token();
      break;
    case RT_ONz:
      n = make_lvalue((LVALUE *)&lv_rt_ONz);
      token = Get_Token();
      break;
    case RT_WN:
      n = make_lvalue((LVALUE *)&lv_rt_WN);
      token = Get_Token();
      break;
    case RT_WNx:
      n = make_lvalue((LVALUE *)&lv_rt_WNx);
      token = Get_Token();
      break;
    case RT_WNy:
      n = make_lvalue((LVALUE *)&lv_rt_WNy);
      token = Get_Token();
      break;
    case RT_WNz:
      n = make_lvalue((LVALUE *)&lv_rt_WNz);
      token = Get_Token();
      break;
    case RT_u:
      n = make_lvalue((LVALUE *)&lv_rt_u);
      token = Get_Token();
      break;
    case RT_v:
      n = make_lvalue((LVALUE *)&lv_rt_v);
      token = Get_Token();
      break;
    case RT_screenu:
      n = make_lvalue((LVALUE *)&lv_rt_uscreen);
      token = Get_Token();
      break;
    case RT_screenv:
      n = make_lvalue((LVALUE *)&lv_rt_vscreen);
      token = Get_Token();
      break;

    /*
     * User defined variables.
     */
    case DECL_LVALUE:
      n = make_lvalue(Copy_Lvalue((LVALUE *)cur_token->data));
      token = Get_Token();
      break;

    /*
     * User defined functions.
     */
    case DECL_PROC:
      n = new_node();
      n->fn = user_fn;
      n->data = Compile_Expr_User_Proc_Stmt((Proc *)cur_token->data);
      token = Get_Token();
      break;

    /*
     * No arg functions.
     */
    case FN_FRAME:
      n = new_node();
      n->fn = frame_fn;
      n->is_vec = 0;
      token = Get_Token();
      break;
    case FN_NFRAME:
      n = new_node();
      n->fn = nframe_fn;
      n->is_vec = 0;
      token = Get_Token();
      break;
    case FN_NFRAME2:
      n = new_node();
      n->fn = nframe2_fn;
      n->is_vec = 0;
      token = Get_Token();
      break;
    case FN_RAND:
      n = new_node();
      n->fn = rand_fn;
      n->is_vec = 0;
      token = Get_Token();
      break;
    case FN_VRAND:
      n = new_node();
      n->fn = vrand_fn;
      n->is_vec = 1;
      token = Get_Token();
      break;

    /*
     * One arg functions.
     */
    case FN_ABS:
      n = make_func(abs_fn, 1, 0);
      break;
    case OP_OR: /* The |expr| version of abs(). */
      n = new_node();
      n->fn = abs_fn;
      n->is_vec = 0;
      token = Get_Token();
      n->l = expr1();
      if(n->l == NULL)
        err_val_expected();
      chk_token(OP_OR, "|");
      token = Get_Token();
      break;
    case FN_ACOS:
      n = make_func(acos_fn, 1, 0);
      break;
    case FN_ASIN:
      n = make_func(asin_fn, 1, 0);
      break;
    case FN_ATAN:
      n = make_func(atan_fn, 1, 0);
      break;
    case FN_CEIL:
      n = make_func(ceil_fn, 1, 0);
      break;
    case FN_CHECKER:
      n = make_func(checker_fn, 1, 0);
      break;
    case FN_COS:
      n = make_func(cos_fn, 1, 0);
      break;
    case FN_COSH:
      n = make_func(cosh_fn, 1, 0);
      break;
    case FN_CYLINDER_MAP:
      n = make_func(cylinder_map_fn, 1, 1);
      break;
    case FN_DEG:
      n = make_func(deg_fn, 1, 0);
      break;
    case FN_DISC_MAP:
      n = make_func(disc_map_fn, 1, 1);
      break;
    case FN_ENVIRONMENT_MAP:
      n = make_func(environment_map_fn, 1, 1);
      break;
    case FN_EXP:
      n = make_func(exp_fn, 1, 0);
      break;
    case FN_FLOOR:
      n = make_func(floor_fn, 1, 0);
      break;
    case FN_INT:
      n = make_func(int_fn, 1, 0);
      break;
    case FN_LOG:
      n = make_func(ln_fn, 1, 0);
      break;
    case FN_LOG10:
      n = make_func(log10_fn, 1, 0);
      break;
    case FN_NOISE:
      n = make_func(noise_fn, 1, 0);
      break;
    case FN_PLANE_MAP:
      n = make_func(plane_map_fn, 1, 1);
      break;
    case FN_RAD:
      n = make_func(rad_fn, 1, 0);
      break;
    case FN_SIN:
      n = make_func(sin_fn, 1, 0);
      break;
    case FN_SINH:
      n = make_func(sinh_fn, 1, 0);
      break;
    case FN_SPHERE_MAP:
      n = make_func(sphere_map_fn, 1, 1);
      break;
    case FN_SQRT:
      n = make_func(sqrt_fn, 1, 0);
      break;
    case FN_TAN:
      n = make_func(tan_fn, 1, 0);
      break;
    case FN_TANH:
      n = make_func(tanh_fn, 1, 0);
      break;
    case FN_TORUS_MAP:
      n = make_func(torus_map_fn, 1, 1);
      break;
    case FN_VNOISE:
      n = make_func(vnoise_fn, 1, 1);
      break;
    case FN_VNORM:
      n = make_func(vnorm_fn, 1, 1);
      break;


    /*
     * Two arg functions.
     */
    case FN_ATAN2:
      n = make_func(atan2_fn, 2, 0);
      break;
    case FN_IMAGE_MAP:
    case FN_SMOOTH_IMAGE_MAP:
      {
        int type = token;
        void *ptr;
        FILE *fp;

        token = Get_Token();
        chk_token(OP_LPAREN, "(");
        if(Get_Token() == TK_QUOTESTRING)
        {
          if((fp = SCN_FindFile(token_buffer, "rb", scn_bitmap_paths,
            SCN_FINDFILE_CHK_CUR_FIRST)) == NULL)
          {
            SCN_Message(SCN_MSG_ERROR,
              "Image: Unable to find image file: `%s'.", token_buffer);
          }
          else if((ptr = Image_Load(fp, token_buffer)) != NULL)
          {
            fclose(fp);
            if(type == FN_IMAGE_MAP)
              n = make_func2(image_map_fn , 2, 1);
            else
              n = make_func2(smooth_image_map_fn , 2, 1);
            n->data = ptr;
          }
          else
          {
            fclose(fp);
            SCN_Message(SCN_MSG_ERROR,
              "Image: Invalid image file: `%s'.", token_buffer);
          }
        }
        else
        {
          SCN_Message(SCN_MSG_ERROR,
            "Image: File name expected. Found `%s'.", token_buffer);
        }
      }
      break;
    case FN_HEXAGON:
      n = make_func(hexagon_fn, 2, 0);
      break;
    case FN_ROTATE:
      n = make_func(rotate_fn, 2, 1);
      break;
    case FN_SCALE:
      n = make_func(scale_fn, 2, 1);
      break;
    case FN_TRANSLATE:
      n = make_func(translate_fn, 2, 1);
      break;
    case FN_TURB:
      n = make_func(turb_fn, 2, 0);
      break;
    case FN_VCROSS:
      n = make_func(vcross_fn, 2, 1);
      break;
    case FN_VDOT:
      n = make_func(vdot_fn, 2, 0);
      break;
    case FN_VTURB:
      n = make_func(vturb_fn, 2, 1);
      break;

    /*
     * Three arg functions.
     */
    case FN_CLAMP:
      n = make_func(clamp_fn, 3, 0);
      break;
    case FN_TEST:
      n = make_func(ternary_fn, 3, 0);
      break;

    /*
     * Four arg functions.
     */
    case FN_TURB2:
      n = make_func(turb2_fn, 4, 0);
      break;
    case FN_VTURB2:
      n = make_func(vturb2_fn, 4, 1);
      break;
  }

  return n;
}

/*
 * Array subscript operator (foo_array[expr]).
 */
static EXPR *expr11(void)
{
  EXPR *n, *n2;
  n = atom();
  if(token == OP_LSQUARE && n != NULL)
  {
    n2 = n;
    n = new_node();
    n->fn = subscript_fn;
    n->l = n2;
    token = Get_Token();
    n->r = expr0();
    if(NULL == n->r)
      err_val_expected();
    chk_token(OP_RSQUARE, "]");
    token = Get_Token();
  }
  return n;
}

/*
 * Unary plus, minus, and logical negation (+r, -r, and !r).
 */
static EXPR *expr10(void)
{
  EXPR *n;

  if(token == OP_MINUS)
  {
    token = Get_Token();
    n = new_node();
    n->fn = uminus_fn;
    n->r = expr10();
    if(n->r == NULL)
      err_val_expected();
  }
  else if(token == OP_PLUS)
  {
    token = Get_Token();
    n = expr10();
    if(n == NULL)
      err_val_expected();
  }
  else if(token == OP_NOT)
  {
    token = Get_Token();
    n = new_node();
    n->is_vec = 0;
    n->fn = not_fn;
    n->r = expr10();
    if(n->r == NULL)
      err_val_expected();
  }
  else
    n = expr11();
  return n;
}

/*
 * Exponentiation (l ^ r).
 */
static EXPR *expr9(void)
{
  EXPR *n, *n2;

  n = expr10();
  while(token == OP_POW)
  {
    token = Get_Token();
    n2 = n;
    n = new_node();
    n->fn = pow_fn;
    n->l = n2;
    n->r = expr10();
    if(n->r == NULL)
      err_val_expected();
  }
  return n;
}

/*
 * Multiplication, division and modulus (l * r, l / r and l % r).
 */
static EXPR *expr8(void)
{
  EXPR *n, *n2;
  int op;

  n = expr9();
  while(token == OP_MULT || token == OP_DIVIDE || token == OP_MOD)
  {
    op = token;
    token = Get_Token();
    n2 = n;
    n = new_node();
    n->fn = (op == OP_MULT) ? mult_fn :
            (op == OP_DIVIDE) ? div_fn : mod_fn;
    n->l = n2;
    n->r = expr9();
    if(n->r == NULL)
      err_val_expected();
  }
  return n;
}

/*
 * Addition and subtraction (l + r and l - r).
 */
static EXPR *expr7(void)
{
  EXPR *n, *n2;
  int op;

  n = expr8();
  while(token == OP_PLUS || token == OP_MINUS)
  {
    op = token;
    token = Get_Token();
    n2 = n;
    n = new_node();
    n->fn =(op == OP_PLUS) ? plus_fn : minus_fn;
    n->l = n2;
    n->r = expr8();
    if(n->r == NULL)
      err_val_expected();
  }
  return n;
}

/*
 * Less-than(equal) and greater-than(equal) (l < r, l <= r, l > r and l >= r).
 */
static EXPR *expr6(void)
{
  EXPR *n, *n2;
  int op;

  n = expr7();
  while(token == OP_LESSTHAN || token == OP_LESSEQUAL ||
    (token == OP_GREATERTHAN && (! end_of_vec)) || token == OP_GREATEQUAL)
  {
    op = token;
    token = Get_Token();
    n2 = n;
    n = new_node();
    n->is_vec = 0;
    n->fn = (op == OP_LESSTHAN)    ? lessthan_fn    :
            (op == OP_LESSEQUAL)   ? lessequal_fn   :
            (op == OP_GREATERTHAN) ? greaterthan_fn : greatequal_fn;
    n->l = n2;
    n->r = expr7();
    if(NULL == n->r)
      err_val_expected();
  }
  return n;
}

/*
 * Equality and inequality (l == r and l != r).
 */
static EXPR *expr5(void)
{
  EXPR *n, *n2;
  int op;

  n = expr6();
  while(OP_EQUAL == token || OP_NOTEQUAL == token)
  {
    op = token;
    token = Get_Token();
    n2 = n;
    n = new_node();
    n->is_vec = 0;
    n->fn = (op == OP_EQUAL) ? equal_fn : notequal_fn;
    n->l = n2;
    n->r = expr6();
    if(NULL == n->r)
      err_val_expected();
  }
  return n;
}

/*
 * Logical AND (l && r).
 */
static EXPR *expr4(void)
{
  EXPR *n, *n2;

  n = expr5();
  while(token == OP_ANDAND)
  {
    token = Get_Token();
    n2 = n;
    n = new_node();
    n->is_vec = 0;
    n->fn = andand_fn;
    n->l = n2;
    n->r = expr5();
    if(n->r == NULL)
      err_val_expected();
  }
  return n;
}

/*
 * Logical OR (l || r).
 */
static EXPR *expr3(void)
{
  EXPR *n, *n2;

  n = expr4();
  while(token == OP_OROR)
  {
    token = Get_Token();
    n2 = n;
    n = new_node();
    n->is_vec = 0;
    n->fn = oror_fn;
    n->l = n2;
    n->r = expr4();
    if(NULL == n->r)
      err_val_expected();
  }
  return n;
}

/*
 * Ternary conditional operator (l ? r->l : r->r).
 */
static EXPR *expr2(void)
{
  EXPR *n, *n2;
  n = expr3();
  if(token == OP_QUESTION)
  {
    n2 = n;
    n = new_node();
    n->fn = ternary_fn;
    n->l = n2;
    n2 = new_node();
    n->r = n2;
    token = Get_Token();
    n2->l = expr2();
    if(n2->l == NULL)
      err_val_expected();
    chk_token(OP_COLON, ":");
    token = Get_Token();
    n2->r = expr2();
    if(n2->r == NULL)
      err_val_expected();
  }
  return n;
}

/*
 * Entry into the recursive expression parsing functions.
 * Enter here to stop at comma.
 * Returns pointer to top node of parse-tree.
 * Assignment operators (l = r, l += r, l -= r, l *= r and l /= r).
 */
static EXPR *expr1(void)
{
  EXPR *n, *lv;
  int last_token, op, run_time_var;

  last_token = token;
  run_time_var = cur_token->flags & TKFLAG_RTVAR;

  n = expr2();
  op = token;
  if(op == OP_ASSIGN ||
     op == OP_PLUSASSIGN || op == OP_MINUSASSIGN ||
     op == OP_MULTASSIGN || op == OP_DIVASSIGN)
  {
    if( ! (last_token == DECL_LVALUE || run_time_var))
    {
      SCN_Message(SCN_MSG_ERROR,
        "Expession syntax: Variable required on the left side of `='.");
      longjmp(err_bailout, 1);
    }
    token = Get_Token();
    lv = n;
    n = new_node();
    n->fn = (OP_ASSIGN == op)      ? assign_fn      :
            (OP_PLUSASSIGN == op)  ? plusassign_fn  :
            (OP_MINUSASSIGN == op) ? minusassign_fn :
            (OP_MULTASSIGN == op)  ? multassign_fn  : divassign_fn;
    n->l = lv;
    n->r = expr1();    /* right to left associativity */
    if(NULL == n->r)
      err_val_expected();
  }
  return n;
}

/*
 * Entry into the recursive expression parsing functions.
 * Enter here to include commas as the sequence operator.
 * Returns pointer to top node of parse-tree.
 * Comma sequence operator (l, r->l, r->r->l, r->r->r->l, ...).
 */
static EXPR *expr0(void)
{
  EXPR *node, *n;

  node = expr1();
  n = NULL;
  while(OP_COMMA == token)
  {
    token = Get_Token();
    if(NULL == n)
    {
      n = new_node();
      n->l = node;
      node = n;
      n->fn = comma_fn;
    }
    n->r = new_node();
    n = n->r;
    n->fn = comma_fn;
    n->l = expr1();
    if(n->l == NULL)
      err_val_expected();
  }
  return node;
}

/*************************************************************************
 * External functions...
 */

/*************************************************************************
 *
 *  Parse_Expr() - Parse expression from ASCII input stream,
 *    evaluating result directly. Returns result in Vec3 "result".
 *    Returns non-zero result-type if an expression was processed.
 *    0 if error or no expression.
 *
 ************************************************************************/
int Parse_Expr(Vec3 *result)
{
  EXPR *n;
  int type;

  end_of_vec = 0;
  n = NULL;

  if((re_entering ? 0 : setjmp(err_bailout)) == 0)
  {
    re_entering++;
    token = Get_Token();
    n = expr1();
    Unget_Token();
    if(n != NULL)
    {
      (n->fn)(n);
       *result = n->V;
      type = n->is_vec ? EXP_VECTOR : EXP_FLOAT;
    }
    else
      type = 0;
    re_entering--;
  }
  else
    type = re_entering = 0;

  Delete_Expr(n);

  return type;
}


/*************************************************************************
 *
 *    Compile_Expr() - Parse expression from ASCII input stream,
 *  compiling it into a parse-tree. Returns a pointer to compiled
 *  expression tree if an expression was processed. NULL if error
 *  or no expression.
 *
 ************************************************************************/
EXPR *Compile_Expr(void)
{
  EXPR *expr;

  if((re_entering ? 0 : setjmp(err_bailout)) == 0)
  {
    re_entering++;
    end_of_vec = 0;
    token = Get_Token();
    expr = expr1();
    Unget_Token();
    re_entering--;
  }
  else
{
    re_entering = 0;
    expr = NULL;
  }

  return expr;
}


/*************************************************************************
 *
 *  Compile_Lvalue_Expr() - Get next token from ASCII input stream, if it
 *    is the assignment operator, "=", compile the expression to the
 *    right of the equals sign and place the tree on the right side
 *    of an "l-value =" node. Returns 1 and an EXPR struct in "expr"
 *    if an assignment op was found. 0 if error or no assignment.
 *
 ************************************************************************/
EXPR *Compile_Lvalue_Expr(LVALUE *lv)
{
  EXPR *rv, *expr = NULL;

  end_of_vec = 0;

  if((re_entering ? 0 : setjmp(err_bailout)) == 0)
  {
    re_entering++;
    /* See if there's an equals sign ahead... */
    if((token = Get_Token()) == OP_ASSIGN)
    {
      /* Get the right-side expression tree... */
      token = Get_Token();
      rv = expr1();
      Unget_Token();
      if(rv != NULL)
      {
        /* Make the assignment tree... */
        expr = new_node();
        expr->fn = assign_fn;
        expr->l = new_node();
        expr->l->data = Copy_Lvalue(lv);
        expr->l->fn = (lv->type == TK_VECTOR) ? vlvalue_fn : flvalue_fn;
        expr->l->is_vec = (lv->type == TK_VECTOR) ? 1 : 0;
        expr->r = rv;
      }
      else  /* No expr to the right, flag an error. */
        err_val_expected();
    }
    else /* No assignment expression. */
      Unget_Token();
    re_entering--;
  }
  else
    re_entering = 0;

  return expr;
} /* end of Compile_Lvalue_Expr() */


/*************************************************************************
 *
 *  Eval_Expr() - Evaluate a compiled parse-tree. Return the result
 *   in "result" and return a result type code - int, float or vector.
 *
 ************************************************************************/
int Eval_Expr(Vec3 *result, EXPR *expr)
{
  (expr->fn)(expr);
   *result = expr->V;
  return expr->is_vec ? EXP_VECTOR : EXP_FLOAT;
}


/*************************************************************************
 *
 *  FEval_Expr() - Evaluate a compiled parse-tree. If result is a
 *    vector, promote it to a float equal to the magnitude of the
 *    vector result.
 *
 ************************************************************************/
double FEval_Expr(EXPR *expr)
{
  (expr->fn)(expr);
  return (expr->is_vec) ? V3Mag(&expr->V) : expr->V.x;
}


/*************************************************************************
 *
 *  VEval_Expr() - Evaluate a compiled parse-tree. If result is a
 *    float, promote it to a vector with x, y, and z equal to the
 *    float result.
 *
 ************************************************************************/
void VEval_Expr(Vec3 *result, EXPR *expr)
{
  (expr->fn)(expr);
  if(expr->is_vec)
    *result = expr->V;
  else
    result->x = result->y = result->z = expr->V.x;
}


EXPR *Copy_Expr(EXPR *expr)
{
  if(expr != NULL)
    expr->nrefs++;
  return expr;
}


LVALUE *New_Lvalue(int type)
{
  LVALUE *lv = (LVALUE *)mmalloc(sizeof(LVALUE));
  lv->nrefs = 1;
  lv->type = type;
  if(type == TK_FLOAT)
    lv->data.pf = &lv->V.x;
  else
    lv->data.pv = &lv->V;
  V3Set(&lv->V, 0.0, 0.0, 0.0);
  return lv;
}


LVALUE *Copy_Lvalue(LVALUE *lv)
{
  if(lv != NULL)
    lv->nrefs++;
  return lv;
}


EXPR *Delete_Expr(EXPR *expr)
{
  if((expr != NULL) && (--expr->nrefs == 0))
  {
    if(expr->l != NULL)
      expr->l = Delete_Expr(expr->l);
    if(expr->r != NULL)
      expr->r = Delete_Expr(expr->r);
    if(expr->fn == flvalue_fn || expr->fn == vlvalue_fn)
      Delete_Lvalue((LVALUE *)expr->data);
    else if(expr->fn == user_fn)
    {
      STATEMENT *s = (STATEMENT *)expr->data;
      Delete_Statement(s);
    }
    else if(expr->fn == image_map_fn || expr->fn == smooth_image_map_fn)
    {
      Image *img = (Image *)expr->data;
      Delete_Image(img);
    }
    mfree(expr, sizeof(EXPR));
  }
  return NULL;
}


LVALUE *Delete_Lvalue(LVALUE *lv)
{
  if((lv != NULL) && (lv->nrefs > 0))
    if(--lv->nrefs == 0)
      mfree(lv, sizeof(LVALUE));
  return NULL;
}


/*************************************************************************
 *
 *  Init_Expr() - Initialize the expression parser. Call before using
 *    expression parser.
 *
 ************************************************************************/
void Init_Expr(void)
{
  re_entering = 0;
  frand1_seed = -1;  /* Negative value initializes Frand1(). */

  /*
   * Set up the static RT_LVALUE references to the run-time variables.
   */
  lv_rt_D.type = TK_VECTOR;
  lv_rt_D.data.pv = &rt_D;

  lv_rt_O.type = TK_VECTOR;
  lv_rt_O.data.pv = &rt_O;
  lv_rt_Ox.type = TK_FLOAT;
  lv_rt_Ox.data.pf = &rt_O.x;
  lv_rt_Oy.type = TK_FLOAT;
  lv_rt_Oy.data.pf = &rt_O.y;
  lv_rt_Oz.type = TK_FLOAT;
  lv_rt_Oz.data.pf = &rt_O.z;

  lv_rt_W.type = TK_VECTOR;
  lv_rt_W.data.pv = &rt_W;
  lv_rt_Wx.type = TK_FLOAT;
  lv_rt_Wx.data.pf = &rt_W.x;
  lv_rt_Wy.type = TK_FLOAT;
  lv_rt_Wy.data.pf = &rt_W.y;
  lv_rt_Wz.type = TK_FLOAT;
  lv_rt_Wz.data.pf = &rt_W.z;

  lv_rt_ON.type = TK_VECTOR;
  lv_rt_ON.data.pv = &rt_ON;
  lv_rt_ONx.type = TK_FLOAT;
  lv_rt_ONx.data.pf = &rt_ON.x;
  lv_rt_ONy.type = TK_FLOAT;
  lv_rt_ONy.data.pf = &rt_ON.y;
  lv_rt_ONz.type = TK_FLOAT;
  lv_rt_ONz.data.pf = &rt_ON.z;

  lv_rt_WN.type = TK_VECTOR;
  lv_rt_WN.data.pv = &rt_WN;
  lv_rt_WNx.type = TK_FLOAT;
  lv_rt_WNx.data.pf = &rt_WN.x;
  lv_rt_WNy.type = TK_FLOAT;
  lv_rt_WNy.data.pf = &rt_WN.y;
  lv_rt_WNz.type = TK_FLOAT;
  lv_rt_WNz.data.pf = &rt_WN.z;

  lv_rt_u.type = TK_FLOAT;
  lv_rt_u.data.pf = &rt_u;

  lv_rt_v.type = TK_FLOAT;
  lv_rt_v.data.pf = &rt_v;

  lv_rt_uscreen.type = TK_FLOAT;
  lv_rt_uscreen.data.pf = &rt_uscreen;

  lv_rt_vscreen.type = TK_FLOAT;
  lv_rt_vscreen.data.pf = &rt_vscreen;

  Proc_Initialize();
  Lookup_Initialize();
}


/*************************************************************************
 *
 *  Close_Expr() - Clean up the expression parser. Call after using
 *    expression parser.
 *
 ************************************************************************/
void Close_Expr(void)
{
  Proc_Close();
}

