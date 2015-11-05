/*************************************************************************
*
*  trace.c
*
*  Recursive ray trace functions.
*
*************************************************************************/

#include "ray.h"


/* If true, generate fake caustics in shadows. */
int ray_use_fake_caustics;

/* Minimum and maximum trace distances. */
double ray_min_trace_dist;
double ray_min_shadow_dist;
double ray_max_trace_dist;

/* Minimum color level for a ray to considered significant. */
double ray_min_color_weight;

/* Ray counters. */
unsigned long ray_eye_rays;
unsigned long ray_eye_rays_reflected;
unsigned long ray_eye_rays_transmitted;
unsigned long ray_shadow_rays;
unsigned long ray_shadow_rays_transmitted;

/* Scaling factor for faked caustics in shadow rays. */
static double caustics_scale;
/* True if any shadow rays get refracted during trace. */
static int rays_bent;
/* Copy of original light source direction vector to tweak. */
static Vec3 light_dir;
/* Current light source being tested for shadows. */
static Light *shadow_light;

/* Apply index of refraction to ray. */
static int Refract( Vec3 *dir, Vec3 *norm, double r );

static double a;

int Ray_TraceRay( RayInitData *raydata )
{
	ct.ray_flags = RAY_EYE;
	ray_eye_rays++;
	ct.B = raydata->B;
	ct.D = raydata->D;
	V3Set( &ct.weight, 1.0, 1.0, 1.0 );
	TraceRecursiveRay( );
	raydata->color = ct.total_color;

	return ( ct.objhit != NULL );
}


void TraceRecursiveRay( void )
{
	ct.tmax = ct.t = ray_max_trace_dist;
	ct.tmin = ray_min_trace_dist;
	V3Set( &ct.total_color, 0.0, 0.0, 0.0 );
	rt_D = ct.D;

	if ( FindClosestIntersection( ray_object_list, ct.hits ) )
	{
		ShadeSurface( );
		CalcLighting( );

		/*
		 * Test for reflecting rays...
		 */
		if ( ( V3Mag( &ct.weight ) > ray_min_color_weight ) &&
			( ct.trace_level < ray_max_trace_depth ) )
		{
			PushTraceStack( );
			ct.ray_flags = RAY_REFLECTED;
			V3Mul( &ct.weight, &pt.weight, &pt.kr );
			ray_eye_rays_reflected++;
			ct.B = pt.Q;
			a = - V3Dot( &pt.D, &pt.N ) * 2.0;
			ct.D.x = pt.D.x + pt.N.x * a;
			ct.D.y = pt.D.y + pt.N.y * a;
			ct.D.z = pt.D.z + pt.N.z * a;
			V3Normalize( &ct.D );

			TraceRecursiveRay( ); /* Do reflecting rays. */

			/*
			 * Restore this state and weigh in the color returned from
			 * this ray.
			 */
			PopTraceStack( );
			ct.total_color.x += pt.total_color.x * ct.kr.x;
			ct.total_color.y += pt.total_color.y * ct.kr.y;
			ct.total_color.z += pt.total_color.z * ct.kr.z;
		}

		/*
		 * Test for transmitting rays...
		 */
		if ( ( V3Mag( &ct.weight ) > ray_min_color_weight ) &&
			( ct.trace_level < ray_max_trace_depth ) )
		{
			PushTraceStack( );
			ct.ray_flags = RAY_TRANSMITTED;
			V3Mul( &ct.weight, &pt.weight, &pt.kt );
			ct.B = pt.Q;
			ct.D = pt.D;
			a = ( pt.entering ) ?
				pt.surface->outior / pt.surface->ior :
				pt.surface->ior / pt.surface->outior;
			if ( Refract( &ct.D, &pt.N, a ) )
			{
				/*
				 * This ray reflects internally within the rafractive object.
				 * Treat it as a reflecting ray.
				 */
				ct.ray_flags = RAY_INTREFLECTED;
				ray_eye_rays_reflected++;
			}
			else
				ray_eye_rays_transmitted++;

			TraceRecursiveRay( ); /* Do transmitting rays. */

			/*
			 * Restore this state and weigh in the color returned from
			 * this ray.
			 */
			PopTraceStack( );
			if ( ( pt.ray_flags & RAY_INTREFLECTED ) && ( ! ct.entering ) )
			{
				ct.total_color.x = pt.total_color.x;
				ct.total_color.y = pt.total_color.y;
				ct.total_color.z = pt.total_color.z;
			}
			else
			{
				ct.total_color.x += pt.total_color.x * ct.kt.x;
				ct.total_color.y += pt.total_color.y * ct.kt.y;
				ct.total_color.z += pt.total_color.z * ct.kt.z;
			}
		}
	}
	else
		Ray_DoBackground( );
   Ray_ApplyVisibility();
}


int Ray_TraceShadowRay( Vec3 *D, Light *light, Vec3 *color )
{
	Object *obj;

	if ( ct.trace_level >= ray_max_trace_depth )
		return 1;

	PushTraceStack( );
	ct.ray_flags = RAY_SHADOW;
	ray_shadow_rays++;
	ct.B = pt.Q;
	ct.D = *D;
	ct.baseobj = pt.objhit;
	rays_bent = 0;
	caustics_scale = 1.0;
	shadow_light = light;
	ct.tmin = ray_min_shadow_dist;
	if ( shadow_light->type != LIGHT_INFINITE )
	{
		V3Sub( &light_dir, &shadow_light->loc, &ct.B );
		ct.tmax = ct.t = V3Mag( &light_dir );
	}
	else
	{
		ct.tmax = ct.t = ray_max_trace_dist;
		light_dir = shadow_light->dir;
	}

	/*
	 * First, check our cached object, if present.
	 */
	obj = light->block_obj_cached;
	if ( ( obj != NULL ) && ( ! ray_use_fake_caustics ) )
	{
		if ( ( obj->procs->Intersect )( obj, ct.hits ) )
		{
			/* shadow_ray_hits++; */
			PopTraceStack( );
			return 1;
		}
		light->block_obj_cached = NULL;
	}

	TraceRecursiveShadowRay( );
	PopTraceStack( );
	if ( caustics_scale > 0.0 )
	{
		color->x = pt.total_color.x * caustics_scale;
		color->y = pt.total_color.y * caustics_scale;
		color->z = pt.total_color.z * caustics_scale;
	}
	else
	{
		return 1;
	}

	return 0;
}


void TraceRecursiveShadowRay( void )
{
	ct.tmin = ray_min_shadow_dist;
	if ( shadow_light->type != LIGHT_INFINITE )
	{
		V3Sub( &light_dir, &shadow_light->loc, &ct.B );
		ct.tmax = ct.t = V3Mag( &light_dir );
	}
	else
	{
		ct.tmax = ct.t = ray_max_trace_dist;
	}
	UpdateTraceStack( );

	if ( FindClosestIntersection( ray_object_list, ct.hits ) )
	{
		ShadeSurface( );
		if ( ( V3Mag( &ct.kt ) > ray_min_color_weight ) &&
			( ct.trace_level < ray_max_trace_depth ) )
		{
			ct.total_color = ct.kt;
			PushTraceStack( );
			ct.ray_flags = RAY_SHADOW | RAY_TRANSMITTED;
			ct.B = pt.Q;
			ct.D = pt.D;
			ct.baseobj = pt.objhit;
			if ( ray_use_fake_caustics )
			{
				rays_bent = 1;
				a = ( pt.entering ) ?
					pt.surface->outior / pt.surface->ior :
					pt.surface->ior / pt.surface->outior;
				if ( ! Refract( &ct.D, &pt.N, a ) )
				{
					V3Normalize( &light_dir );
					caustics_scale = V3Dot( &light_dir, &ct.D );
					ray_shadow_rays_transmitted++;
					TraceRecursiveShadowRay( );   /* Trace transmitting ray. */
				}
				else   /* Terminate on internal reflections. */
					caustics_scale = 0.0;
			}
			else     /* Not using fake caustics. */
			{
				TraceRecursiveShadowRay( );   /* Trace transmitting ray. */
			}

			PopTraceStack( );

			/* Weigh in transmitted ray's shadow level. */
			ct.total_color.x *= pt.total_color.x;
			ct.total_color.y *= pt.total_color.y;
			ct.total_color.z *= pt.total_color.z;
		}
		else  /* Shadow ray is completely blocked. */
		{
			if ( rays_bent == 0 )
			{
				if ( ( ct.objhit->flags & OBJ_FLAG_TRANSMISSIVE ) == 0 )
					shadow_light->block_obj_cached = ct.objhit;
			}
			caustics_scale = 0.0;
		}
	}
	else    /* No blocking objects hit. */
	{
		V3Set( &ct.total_color, 1.0, 1.0, 1.0 );
	}
}


/*************************************************************************
 *
 *  Refract() - Given a directional vector, pointed to by "dir",
 * 	 bends it based on the surface normal "norm" and the ratio, "r",
 *	 of the refraction indices of the inside medium & outside
 *	 medium (usually air - 1.0). If the ray is reflecting internally, a
 *   reflecting direction vector is computed and 1 is returned. Returns
 *   zero, otherwise.
 *
 ************************************************************************/
int Refract( Vec3 *dir, Vec3 *norm, double r )
{
	double theta1, theta2;

	if ( r != 1.0 )
	{
		theta1 = - V3Dot(dir, norm);
		theta2 = 1.0 - r * r * ( 1.0 - theta1 * theta1 );
		if ( theta2 > 0.0 )
		{
			theta2 = r * theta1 - sqrt(theta2);
			dir->x = dir->x * r + theta2 * norm->x;
			dir->y = dir->y * r + theta2 * norm->y;
			dir->z = dir->z * r + theta2 * norm->z;
			V3Normalize( dir );

			return 0;
		}
		else  /* Ray is reflecting internally. */
		{
			theta1 *= 2.0;
			dir->x += norm->x * theta1;
			dir->y += norm->y * theta1;
			dir->z += norm->z * theta1;
			V3Normalize( dir );

			return 1;
		}
	}

	return 0;
}

