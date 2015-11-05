/**
 *****************************************************************************
 *  @file math3d.h
 *  The MATH3D library API
 *
 *****************************************************************************
 */

#ifndef MATH3D_H
#define MATH3D_H

#include <math.h>  /* sqrt(), floor() */

#ifdef  __cplusplus
extern "C" {
#endif

/*************************************************************************
*
*	types
*
*************************************************************************/

typedef struct tag_vec2
{
	double x, y;
} Vec2;

typedef struct tag_vec3
{
	double x, y, z;
} Vec3;

typedef struct tag_vec4
{
	double x, y, z, w;
} Vec4;

typedef struct tag_mat3x3
{
	double e[3][3];
} Mat3x3;

typedef struct tag_mat4x4
{
	double e[4][4];
} Mat4x4;

/*************************************************************************
*
*	Useful constants and macros
*
*************************************************************************/

/* Ridiculously big. */
#ifdef HUGE
#undef HUGE
#endif
#define HUGE		1e10

/* Ridiculously small. */
#ifdef EPSILON
#undef EPSILON
#endif
#define EPSILON		1e-10

#ifdef PI
#undef PI
#endif
#define PI			3.14159265358979323846

#define TWOPI		(2.0 * PI)
#define HALFPI		(0.5 * PI)
#define SQRT2		1.41421356237309504880
#define SQRT3		1.73205080756887729353
#define GOLDEN		0.6180339887

/* Convert degrees to radians. */
#define DTOR		(PI / 180.0)
#define RAD(a)		((a) * DTOR)
/* Convert radians to degrees. */
#define RTOD		(180.0 / PI)
#define DEG(a)		((a) * RTOD)

#define ABS(a)		((a) < 0 ? -(a) : (a))
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

/* Square a value. */
#define SQR(a)		((a)*(a))
/* Round a up or down to the nearest int. */
#define ROUND(a)	floor((a) + 0.5)
/* Linear interpolate from lo (a == 0) to hi (a == 1). */
#define LERP(a, lo, hi) ((lo) + (a)*((hi)-(lo)))
/* Clamp a to the range specified by lo and hi. */
#define CLAMP(a, lo, hi) ((a) < (lo) ? (lo) : (a) > (hi) ? (hi) : (a))
/* True if number is smaller than EPSILON. */
#define ISZERO(a)	(ABS(a) < EPSILON)

/*************************************************************************
*
*	2D vector macros
*
*************************************************************************/

/*************************************************************************
*
*	3D vector macros
*
*************************************************************************/

/* Init a vector with three floats. */
#define V3Set(V, a, b, c) ((V)->x=(a), (V)->y=(b), (V)->z=(c))

#define V3Add(vo, v1, v2) \
	( \
	(vo)->x = (v1)->x + (v2)->x, \
	(vo)->y = (v1)->y + (v2)->y, \
	(vo)->z = (v1)->z + (v2)->z \
	)

#define V3Zero(v) ((v)->x = (v)->y = (v)->z = 0.0)

#define V3Copy(v1, v2) (*(v1) = *(v2))

#define V3Sub(vo, v1, v2) \
	( \
	(vo)->x = (v1)->x - (v2)->x, \
	(vo)->y = (v1)->y - (v2)->y, \
	(vo)->z = (v1)->z - (v2)->z \
	)

#define V3Mul(vo, v1, v2) \
	( \
	(vo)->x = (v1)->x * (v2)->x, \
	(vo)->y = (v1)->y * (v2)->y, \
	(vo)->z = (v1)->z * (v2)->z \
	)

#define V3Div(vo, v1, v2) \
	( \
	(vo)->x = (v1)->x / (v2)->x, \
	(vo)->y = (v1)->y / (v2)->y, \
	(vo)->z = (v1)->z / (v2)->z \
	)

#define V3ScalMul(vo, v, s) \
	( \
	(vo)->x = (v)->x * (s), \
	(vo)->y = (v)->y * (s), \
	(vo)->z = (v)->z * (s) \
	)

#define V3ScalDiv(vo, v, s) \
	( \
	(vo)->x = (v)->x / (s), \
	(vo)->y = (v)->y / (s), \
	(vo)->z = (v)->z / (s) \
	)

#define V3Combine(vo, v1, s1, v2, s2) \
	( \
	(vo)->x = (v1)->x * (s1) + (v2)->x * (s2), \
	(vo)->y = (v1)->y * (s1) + (v2)->y * (s2), \
	(vo)->z = (v1)->z * (s1) + (v2)->z * (s2) \
	)

#define V3Dot(v1, v2) \
	((v1)->x*(v2)->x + (v1)->y*(v2)->y + (v1)->z*(v2)->z)

#define V3Mag(v) \
	(sqrt((v)->x*(v)->x + (v)->y*(v)->y + (v)->z*(v)->z))

#define V3Cross(vc, v1, v2) \
	( \
	(vc)->x = (v1)->y*(v2)->z - (v2)->y*(v1)->z, \
	(vc)->y = (v1)->z*(v2)->x - (v2)->z*(v1)->x, \
	(vc)->z = (v1)->x*(v2)->y - (v2)->x*(v1)->y \
  )

#define V3Normalize(v) \
	{ \
	double d = sqrt((v)->x*(v)->x + (v)->y*(v)->y + (v)->z*(v)->z); \
	if (d != 0.0) { (v)->x /= d; (v)->y /= d; (v)->z /= d; } \
	}


/*
 * Clamp the elements in vector V to certain range.
 */
#define V3Clamp(V, lo, hi) ( \
	(V)->x = MIN( MAX( (V)->x, lo ), hi), \
	(V)->y = MIN( MAX( (V)->y, lo ), hi), \
	(V)->z = MIN( MAX( (V)->z, lo ), hi))

/*
 * Linear interpolate between two vectors.
 */
#define V3Interpolate(Vout, V1, n, V2) ( \
	(Vout)->x = (V1)->x + (n) * ((V2)->x - (V1)->x), \
	(Vout)->y = (V1)->y + (n) * ((V2)->y - (V1)->y), \
	(Vout)->z = (V1)->z + (n) * ((V2)->z - (V1)->z))

/*
 * Returns true if all elements of vector are equal.
 */
#define V3IsIsotropic(v) ((v)->x == (v)->y)&&((v)->x == (v)->z)

/*
 * Returns true if all elements of vector are zero.
 */
#define V3IsZero(v) (ISZERO((v)->x) && ISZERO((v)->y) && ISZERO((v)->z))

/*************************************************************************
*
*	2D vector functions
*
*************************************************************************/

/*************************************************************************
*
*	3D vector functions
*
*************************************************************************/

/*************************************************************************
*
*	3x3 matrix functions
*
*************************************************************************/
extern void M3x3Copy(Mat3x3 *Mdest, Mat3x3 *Msrc);
extern void M3x3Identity(Mat3x3 *M);
extern void M3x3Transpose(Mat3x3 *M);
extern void M3x3Mul(Mat3x3 *Mout, Mat3x3 *M1, Mat3x3 *M2);
extern void M3x3Inverse(Mat3x3 *I);

/*************************************************************************
*
*	4x4 matrix functions
*
*************************************************************************/
extern void M4x4Copy(Mat4x4 *Mdest, Mat4x4 *Msrc);
extern void M4x4Identity(Mat4x4 *M);
extern void M4x4Transpose(Mat4x4 *M);
extern void M4x4Mul(Mat4x4 *Mout, Mat4x4 *M1, Mat4x4 *M2);
extern void M4x4Inverse(Mat4x4 *I);
extern void M4x4Translate(Mat4x4 *M, Vec3 *V);
extern void M4x4Scale(Mat4x4 *M, Vec3 *V);
extern void M4x4RotateX(Mat4x4 *M, double angle);
extern void M4x4RotateY(Mat4x4 *M, double angle);
extern void M4x4RotateZ(Mat4x4 *M, double angle);
extern void M4x4Rotate(Mat4x4 *M, Vec3 *axis, double angle);
extern void M4x4Perspective(Mat4x4 *M, double height,
	double zmin, double zmax);
extern void M4x4XformPt(Vec3 *V, Mat4x4 *M);
extern void M4x4ProjectPt(Vec3 *V, Mat4x4 *M);
extern void M4x4XformDir(Vec3 *V, Mat4x4 *M);
extern void M4x4XformNormal(Vec3 *V, Mat4x4 *M);

/*************************************************************************
*
*	2D transform functions
*
*************************************************************************/

/*************************************************************************
*
*	3D transform functions
*
*************************************************************************/
extern void RotatePoint3D(Vec3 *P, double angle, Vec3 *axis);

/*************************************************************************
*
*	Root solving functions
*
*************************************************************************/
extern int SolvePoly(double *c, double *s, int order, double lo, double hi);
extern int SolveLinear(double *c, double *s);
extern int SolveQuadric(double *c, double *s);
extern int SolveCubic(double *c, double *s);
extern int SolveQuartic(double *c, double *s);

/*************************************************************************
*
*	Random number functions
*
*************************************************************************/
extern double Frand1(long *seed);

#ifdef  __cplusplus
}
#endif

#endif /* MATH3D_H */