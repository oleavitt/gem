/**
 *****************************************************************************
 *  @file raytrace.c
 *  Top-level implementation file for the Raytrace library.
 *
 *****************************************************************************
 */

#include "ray.h"

// Global variables used by shaders to get/set context-specific values
// in the renderer environment.
// These will be initialized appropriately before shaders that refer to
// them are called.
//
Vec3 ray_env_color;
// TODO: Move the rt_xxx vars to here.

/* Gets set to any of the error codes enum'd in raytrace.h. */
int ray_error;

/* "up" orientation vector for the world. */
Vec3 ray_up_vector;
/* Global index of refraction for the world. */
double ray_global_ior;

/* User supplied drawing functions. */
void (*Set_Pt)(int pt_ndx, double x, double y, double z);
void (*Move_To)(int pt_ndx);
void (*Line_To)(int pt_ndx);

/**
 * Initialize the renderer.
 * Called before the scene is built.
 * Initializes everything to safe default values.
 *
 * @return int 1 if successful or 0 if not.
 */
int Ray_Initialize(void)
{
	ray_error = RAY_ERROR_NONE;

	if (InitializeMem() &&
		InitializeShader() &&
		InitializeSurface() &&
		InitializeObject() &&
		InitializeLight() &&
		InitializeTraceStack())
	{
		ViewportInitialize();
		InitializeInter();
		InitializeBackground();
      InitializeVisibility();

		/* Set default background colors to black. */
		V3Set(&ray_background_color1, 0.0, 0.0, 0.0);
		V3Set(&ray_background_color2, 0.0, 0.0, 0.0);

      ray_visibility_distance = -1;
      V3Set(&ray_visibility_color, 1.0, 1.0, 1.0);

		/* Set default "up" orientation vector for the world. */
		V3Set(&ray_up_vector, 0.0, 0.0, 1.0);

		/* Set other default values. */
		ray_max_trace_depth = 20;
		ray_min_trace_dist = 0.001;
		ray_min_shadow_dist = 0.001;
		ray_min_color_weight = 0.002;
		ray_max_trace_dist = HUGE;
		ray_bound_threshold = 8;
		ray_max_cluster_size = 8;
		ray_global_ior = 1.0;
		ray_use_fake_caustics = 0;

		ray_object_list = NULL;
		ray_light_list = NULL;

		ray_eye_rays = 0;
		ray_eye_rays_reflected = 0;
		ray_eye_rays_transmitted = 0;
		ray_shadow_rays = 0;
		ray_shadow_rays_transmitted = 0;

      return 1;
	}

	Ray_Close();
	return 0;
}

/**
 * Sets up the renderer with the settings from a RaySetupData struct.
 *
 * @param rsd - RaySetupData* - Contains the renderer settings.
 *
 * @return int - non-zero if successful, 0 if not.
 */
int Ray_Setup(RaySetupData *rsd)
{
	/* Background colors. */
	ray_background_color1 = rsd->background_color1;
	ray_background_color2 = rsd->background_color2;

   // Visibility falloff
   ray_visibility_distance = rsd->visibility_distance;
   ray_visibility_color = rsd->visibility_color;

   /* Up orientation vector. */
	ray_up_vector = rsd->up_vector;

	/* Minimum and maximum trace distances. */
	ray_min_trace_dist = rsd->min_trace_dist;
	ray_max_trace_dist = rsd->max_trace_dist;
	ray_min_shadow_dist = rsd->min_shadow_dist;

	/* Maximum trace recursion depth. */
	ray_max_trace_depth = rsd->max_trace_depth;

	/* Minimum number of objects required for bounding tree to be built. */
	ray_bound_threshold = rsd->bound_threshold;

	/* Maximum number of objects per bounding box. */
	ray_max_cluster_size = rsd->max_cluster_size;

	/* If true, generate fake caustics in shadows. */
	ray_use_fake_caustics = rsd->use_fake_caustics;

	/* Global index of refraction for the world. */
	ray_global_ior = rsd->global_ior;

	/* The main viewport. */
	Ray_SetupViewport(&rsd->viewport.LookFrom, &rsd->right_eye_lookfrom,
		&rsd->viewport.LookAt, &rsd->viewport.LookUp, rsd->viewport.ViewAngle,
		rsd->projection_mode);

	/* Main object list. */
	ray_object_list = rsd->objects;

	/* Main light list. */
	ray_light_list = rsd->lights;
	SetupLight();

	Ray_BuildBounds(&ray_object_list);
	rsd->objects = ray_object_list;

	if (SetupTraceStack())
		return 1;

	Ray_Close();
	return 0;
}

/**
 * Store the current settings of the renderer in a RaySetupData struct.
 *
 * @param rsd - RaySetupData* - Receives the renderer settings.
 */
void Ray_GetSetup(RaySetupData *rsd)
{
	/* Background colors. */
	rsd->background_color1 = ray_background_color1;
	rsd->background_color2 = ray_background_color2;

   // Visibility falloff
   rsd->visibility_distance = ray_visibility_distance;
   rsd->visibility_color = ray_visibility_color;

	/* Up orientation vector. */
	rsd->up_vector = ray_up_vector;

	/* Minimum and maximum trace distances. */
	rsd->min_trace_dist = ray_min_trace_dist;
	rsd->max_trace_dist = ray_max_trace_dist;
	rsd->min_shadow_dist = ray_min_shadow_dist;

	/* Maximum trace recursion depth. */
	rsd->max_trace_depth = ray_max_trace_depth;

	/* Minimum number of objects required for bounding tree to be built. */
	rsd->bound_threshold = ray_bound_threshold;

	/* Maximum number of objects per bounding box. */
	rsd->max_cluster_size = ray_max_cluster_size;

	/* If true, generate fake caustics in shadows. */
	rsd->use_fake_caustics = ray_use_fake_caustics;

	/* Global index of refraction for the world. */
	rsd->global_ior = ray_global_ior;

	/* The main viewport. */
	Ray_GetViewportInfo(&rsd->viewport, &rsd->right_eye_lookfrom, &rsd->projection_mode);

	/* Main object list. */
	rsd->objects = ray_object_list;

	/* Main light list. */
	rsd->lights = ray_light_list;
}

/**
 * Close down the renderer, releasing all resources used.
 */
void Ray_Close(void)
{
	CloseBackground();
   CloseVisibility();
	CloseInter();
	CloseTraceStack();
	CloseLight();
	CloseObject();
	CloseSurface();
	CloseShader();
	CloseMem();
}


/*************************************************************************
*
*	Draw the whole scene.
*
*************************************************************************/

void Ray_DrawScene(
	void (*set_pt)(int pt_ndx, double x, double y, double z),
	void (*move_to)(int pt_ndx),
	void (*line_to)(int pt_ndx))
{
	Object *o;
	assert(set_pt != NULL);
	assert(move_to != NULL);
	assert(line_to != NULL);
	Set_Pt = set_pt;
	Move_To = move_to;
	Line_To = line_to;
	/* Draw the objects. */
	for	(o = ray_object_list; o != NULL; o = o->next)
		o->procs->Draw(o);
	/* Draw the lights. */
}


/*************************************************************************
*
*	Get the world bounds of a list of objects.
*
*************************************************************************/

void Ray_GetBounds(Object *root, Vec3 *bmin, Vec3 *bmax)
{
	if (root != NULL)
	{
		Vec3 omin, omax;
		root->procs->CalcExtents(root, bmin, bmax);
		for (root = root->next; root != NULL; root = root->next)
		{
			root->procs->CalcExtents(root, &omin, &omax);
			if (bmin->x > omin.x) bmin->x = omin.x; 
			if (bmin->y > omin.y) bmin->y = omin.y; 
			if (bmin->z > omin.z) bmin->z = omin.z; 
			if (bmax->x < omax.x) bmax->x = omax.x; 
			if (bmax->y < omax.y) bmax->y = omax.y; 
			if (bmax->z < omax.z) bmax->z = omax.z; 
		}
	}
	else
	{
		V3Zero(bmin);
		V3Zero(bmax);
	}
}
