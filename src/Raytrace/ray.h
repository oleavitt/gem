/**
 *****************************************************************************
 * @file ray.h
 *  Local stuff common to all Raytrace library implementation files.
 *
 *****************************************************************************
 */

#ifndef RAY_H
#define RAY_H

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "raytrace.h"


/* Ray type flags. */
/* No flags means ray is the originating ray (the eye ray) */
#define RAY_EYE           0
#define RAY_REFLECTED     1
#define RAY_TRANSMITTED   2
#define RAY_INTREFLECTED  4
#define RAY_SHADOW        8
#define RAY_USER          16

/**
 *	Ray-trace recursion stack element.
 */
typedef struct tag_tracestack
{
	int ray_flags;		/* Ray type ID flags. (see above) */
	int trace_level;	/* Depth of this ray on trace stack. */
	Vec3 B, D, Q, N;	/* B=base pt, D=ray direction, Q=pt hit, N=normal. */
	double tmin, tmax;	/* Intersection interval range. */
	double t;			/* Closest intersection distance. */
	HitData *hits;		/* Ray/Object intersection list. */
	Object *objhit;		/* Closest object hit by ray. */
	Object *baseobj;	/* Object ray is originating from. */
	int entering;		/* True if ray is entering object. */
	int calc_all;		/* If true, test all ray/object intersections. */
	Vec3 color, ka, kd, kr, ks, kt;	/* Saved lighting constants for this level. */
	double ior, Phong;	/* Saved ior & Phong power for this level. */
	Surface *surface;	/* The surface to use for shading. */
	Vec3 weight;		/* Contribution significance for this ray. */
	Vec3 total_color;	/* Cummulative color total for this ray. */
} TraceStack;



/*
 * background.c
 */
extern void InitializeBackground(void);
extern void CloseBackground(void);
extern void Ray_DoBackground(void);

/*
 * bound.c
 */
extern void PostProcessBBox(Object *obj);
extern void BBox_GetTextureInfo(Object *obj, Surface **surf, Xform **T);

/*
 * box.c
 */
extern int Intersect_Box(Vec3 *B, Vec3 *D, Vec3 *bmin, Vec3 *bmax,
  double *T1, double *T2);

/*
 * colortri.c
 */
extern void ColorTriangleGetColor(Object *obj, Vec3 *color);

/*
 * csg.c
 */
extern int csg_nest_level;
extern void PostProcessCSG(Object *obj);
extern void CSG_GetTextureInfo(Object *obj, Surface **surf, Xform **T);

/*
 * inter.c
 */
extern void InitializeInter(void);
extern void CloseInter(void);
extern HitData *NewHit(void);
extern HitData *DeleteHits(HitData *hits);
extern HitData *GetNextHit(HitData *hit);
extern int FindClosestIntersection(Object *first_obj, HitData *hits);
extern int FindAllIntersections(Object *first_obj, HitData *hits);
extern void SortHits(HitData *hits, int nhits);

/*
 * light.c
 */
extern int InitializeLight(void);
extern void SetupLight(void);
extern void CloseLight(void);
extern void CalcLighting(void);
extern Light *NewLight(void);
extern Light *DeleteLight(Light *lite);
/* Minimum tolerance for light source to be considered significant. */
extern double min_light_tol;

/*
 * mem.c
 */
extern int InitializeMem(void);
extern void CloseMem(void);
extern void *Malloc(size_t size);
extern void *Calloc(size_t qty, size_t size);
extern void *Realloc(void *oldptr, size_t oldsize, size_t newsize);
extern void Free(void *ptr, size_t size);

/*
 * object.c
 */
extern int InitializeObject(void);
extern void CloseObject(void);
extern Object *NewObject(void);
extern void CalcBoundObjectExtents(Object *boundobjs,
	Vec3 *omin, Vec3 *omax);
extern void Ray_PostProcessObject(Object *obj);
extern void Object_GetTextureInfo(Object *obj, Surface **surf, Xform **T);
extern Object *ray_object_list;

/*
 * polygon.c
 */
extern void MatrixTransformPolygon(PolygonData *p, Xform *T);

/*
 * raytrace.c
 */
/* "up" orientation vector for the world. */
extern Vec3 WorldUpVector;
/* User supplied drawing functions. */
extern void (*Set_Pt)(int pt_ndx, double x, double y, double z);
extern void (*Move_To)(int pt_ndx);
extern void (*Line_To)(int pt_ndx);

/*
 * sphere.c
 */

/*
 * shader.c
 */
extern int InitializeShader(void);
extern void CloseShader(void);

/*
 * surface.c
 */
extern int InitializeSurface(void);
extern void CloseSurface(void);
extern void ShadeSurface(void);
extern Surface *DefaultSurface;

/*
 * trace.c
 */
extern int Ray_TraceRay(RayInitData *raydata);
extern int Ray_TraceShadowRay(Vec3 *D, Light *light, Vec3 *color);
extern void TraceRecursiveRay(void);
extern void TraceRecursiveShadowRay(void);

/*
 * tracestk.c
 */
extern int InitializeTraceStack(void);
extern int SetupTraceStack(void);
extern void CloseTraceStack(void);
extern void PushTraceStack(void);
extern void PopTraceStack(void);
extern void UpdateTraceStack(void);
/* Current and previous trace levels. */
extern TraceStack ct, pt;

/*
 * viewport.c
 */
extern void ViewportInitialize(void);

/*
 * visiblty.c
 */
extern void InitializeVisibility(void);
extern void CloseVisibility(void);
extern void Ray_ApplyVisibility(void);

/*
 * xform.c
 */

#endif  /* RAY_H */
