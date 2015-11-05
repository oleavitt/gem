/*************************************************************************
*
*	viewport.c
*
*	Viewport functions.
*
*************************************************************************/

#include "ray.h"

/* The main viewports. */
/* If not a stereogram, just the left viewport is used. */
static Viewport left_viewport;
static Viewport right_viewport;

/* Projection mode. */
static int viewport_projection = VIEWPORT_PERSPECTIVE;

static void ViewportSetupViewport(Viewport *pvp,
	Vec3 *from, Vec3 *at, Vec3 *up, double FOVdegrees);

void Ray_SetupViewport(Vec3 *fromleft, Vec3 *fromright,
	Vec3 *at, Vec3 *up, double FOVdegrees, int projection)
{
	viewport_projection = projection;
	ViewportSetupViewport(&left_viewport, fromleft, at, up, FOVdegrees);
	if (viewport_projection == VIEWPORT_ANAGLYPH)
	{
		assert(fromright); /* We need the right eye 'from' vector. */
		ViewportSetupViewport(&right_viewport, fromright, at, up, FOVdegrees);
	}
}


void Ray_GetViewportInfo(Viewport *pvp, Vec3 *fromright, int *projection_mode)
{
	if (pvp != NULL)
		*pvp = left_viewport;
	if (fromright != NULL)
		*fromright = right_viewport.LookFrom;
	if (projection_mode != NULL)
		*projection_mode = viewport_projection;
}


int Ray_TraceRayFromViewport(double u, double v, Vec3 *color)
{
	static RayInitData raydata;
	int result;

	rt_uscreen = u;
	rt_vscreen = v;

	raydata.tmin = 0.001;
	raydata.tmax = HUGE;

	raydata.B = left_viewport.LookFrom;
	raydata.D.x = left_viewport.N.x + u * left_viewport.U.x +
		v * left_viewport.V.x;
	raydata.D.y = left_viewport.N.y + u * left_viewport.U.y +
		v * left_viewport.V.y;
	raydata.D.z = left_viewport.N.z + u * left_viewport.U.z +
		v * left_viewport.V.z;
	V3Normalize(&raydata.D);
	result = Ray_TraceRay(&raydata);

	if (viewport_projection == VIEWPORT_ANAGLYPH)
	{
		/* Render a 3D "funny glasses" stereogram. */
		/* Put the gray-scale value of the left eye color in the blue channel. */
		color->z = raydata.color.x * 0.33 + raydata.color.y * 0.56 + raydata.color.z * 0.11;
		color->y = 0.0;
		/* Trace a ray from the right eye viewport. */
		raydata.B = right_viewport.LookFrom;
		raydata.D.x = right_viewport.N.x + u * right_viewport.U.x +
			v * right_viewport.V.x;
		raydata.D.y = right_viewport.N.y + u * right_viewport.U.y +
			v * right_viewport.V.y;
		raydata.D.z = right_viewport.N.z + u * right_viewport.U.z +
			v * right_viewport.V.z;
		V3Normalize(&raydata.D);
		result = Ray_TraceRay(&raydata);
		/* Put the gray-scale value of the right eye color in the red channel. */
		color->x = raydata.color.x * 0.33 + raydata.color.y * 0.56 + raydata.color.z * 0.11;
	}
	else
		*color = raydata.color;

	return result;
}


/*
 * Initialize Viewport to default values.
 */
void ViewportInitialize(void)
{
	V3Set(&left_viewport.LookFrom, 0.0, -10.0, 10.0);
	V3Set(&left_viewport.LookAt, 0.0, 0.0, 0.0);
	V3Set(&left_viewport.LookUp, 0.0, 0.0, 1.0);
	left_viewport.ViewAngle = 30.0;
	ViewportSetupViewport(&left_viewport, &left_viewport.LookFrom,
		&left_viewport.LookAt, &left_viewport.LookUp, left_viewport.ViewAngle);
	V3Set(&right_viewport.LookFrom, 0.0, -10.0, 10.0);
	V3Set(&right_viewport.LookAt, 0.0, 0.0, 0.0);
	V3Set(&right_viewport.LookUp, 0.0, 0.0, 1.0);
	right_viewport.ViewAngle = 30.0;
	ViewportSetupViewport(&right_viewport, &right_viewport.LookFrom,
		&right_viewport.LookAt, &right_viewport.LookUp, right_viewport.ViewAngle);
}


/*
 * Initialize the constants for this viewport based on 
 * values given
 */
void ViewportSetupViewport(Viewport *pvp,
	Vec3 *from, Vec3 *at, Vec3 *up, double FOVdegrees)
{
	double a;

	assert(pvp);
	assert(from);
	assert(at);

	pvp->LookFrom = *from;
	pvp->LookAt = *at;
	if (up != NULL)
		pvp->LookUp = *up;
	else
		V3Set(&pvp->LookUp, 0.0, 0.0, 0.1);
	pvp->ViewAngle = FOVdegrees;

	V3Sub(&pvp->N, &pvp->LookAt, &pvp->LookFrom);
	V3Normalize(&pvp->N);
	a = V3Dot(&pvp->LookUp, &pvp->N);
	pvp->V.x = pvp->LookUp.x - a * pvp->N.x;
	pvp->V.y = pvp->LookUp.y - a * pvp->N.y;
	pvp->V.z = pvp->LookUp.z - a * pvp->N.z;
	V3Normalize(&pvp->V);
	V3Cross(&pvp->U, &pvp->N, &pvp->V);
	a = cos((pvp->ViewAngle * DTOR) / 2.0) /
		sin((pvp->ViewAngle * DTOR) / 2.0);
	pvp->N.x *= a;
	pvp->N.y *= a;
	pvp->N.z *= a;
}