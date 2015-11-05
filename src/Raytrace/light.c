/*************************************************************************
*
*  light.c
*
*  Light source fuctions.
*
*************************************************************************/

#include "ray.h"

/* Minimum tolerance for light source to be considered significant. */
double min_light_tol = 0.003;

Light *ray_light_list = NULL;

static long light_jitter_seed;

int InitializeLight(void)
{
  ray_light_list = NULL;
	light_jitter_seed = -1;
  return 1;
}


void CloseLight(void)
{
  Light *l;
  while(ray_light_list != NULL)
  {
    l = ray_light_list;
    ray_light_list = l->next;
    Ray_DeleteLight(l);
  }
}


void SetupLight(void)
{
	Light *l;
	int n_auto = 0;

	/*
	 * If any lights have the auto intensity flag set,
	 * normalize all of their color intesities so that the total for
	 * all auto-intensity lights is one.
	 */
	for(l = ray_light_list; l != NULL; l = l->next)
		if(l->flags & LIGHT_FLAG_AUTO_INTENSITY)
			n_auto++;

	if(n_auto)
	{
		for(l = ray_light_list; l != NULL; l = l->next)
			if(l->flags & LIGHT_FLAG_AUTO_INTENSITY)
			{
				l->color.x /= (double)n_auto;
				l->color.y /= (double)n_auto;
				l->color.z /= (double)n_auto;
			}
	}
}


/*************************************************************************
*
*  CalcLighting
*
*  Lighting calculations are done and a list of surfaces is attached
*  to the ray.
*
*************************************************************************/
void CalcLighting(void)
{
	Vec3		color, shadow_color, base_color;
	Light		*lite;
	double		NdotL, lite_scale, lite_dist;
	Vec3		lite_dir;
	int			has_diffuse, has_specular;

	V3Copy(&base_color, &ct.color);

	/* Start with the ambient component. */
	V3Mul(&color, &ct.ka, &ct.color);

	has_diffuse = (ct.kd.x != 0.0 || ct.kd.y != 0.0 || ct.kd.z != 0.0);
	has_specular = (ct.ks.x != 0.0 || ct.ks.y != 0.0 || ct.ks.z != 0.0);

	/* If current object is a colored triangle get its color component. */
	if (ct.objhit->procs->type == OBJ_COLORTRIANGLE)
	{
		ColorTriangleGetColor(ct.objhit, &shadow_color);
		base_color.x *= shadow_color.x;
		base_color.y *= shadow_color.y;
		base_color.z *= shadow_color.z;
	}

	/* If there are no diffuse and no specular weights, light sources
	 * will not contribute anything to this surface.
	 * We can finish up now.
	 */
	if (!has_diffuse && !has_specular)
		goto FinishCalcLighting;

	/* Calc falloff and add in the diffuse and specular contributions
	 * of each light source.
	 */
	for (lite = ray_light_list; lite != NULL; lite = lite->next)
	{
		lite_scale = 1.0;
		V3Set(&shadow_color, 1.0, 1.0, 1.0);

		if (lite->type != LIGHT_INFINITE)
		{
			Vec3 loc;
			V3Copy(&loc, &lite->loc);
			/* Add jitter to light's from point if any. */
			if (lite->flags & LIGHT_FLAG_JITTER)
			{
				loc.x += lite->jitter.x * (Frand1(&light_jitter_seed) - 0.5);
				loc.y += lite->jitter.y * (Frand1(&light_jitter_seed) - 0.5);
				loc.z += lite->jitter.z * (Frand1(&light_jitter_seed) - 0.5);
			}
			/* Get direction vector to light source... */
			V3Sub(&lite_dir, &loc, &ct.Q);
			lite_dist = V3Mag(&lite_dir);
			if (lite_dist > EPSILON)
			{
				lite_dir.x /= lite_dist;
				lite_dir.y /= lite_dist;
				lite_dir.z /= lite_dist;
			}
		}
		else
		{
			lite_dir = lite->dir;
			lite_dist = 0.0;
		}

		NdotL = V3Dot(&ct.N, &lite_dir);

		/* If light is behind object go on to next light. */
		if (NdotL < 0.0)
			continue;

		/* Determine effect of distance falloff... */
		if (lite->falloff > 0.0)
		{
			lite_scale *= 1.0 / (1.0 + lite->falloff * lite_dist * lite_dist);
			/* If light is completely diminished go on to next light. */
			if (lite_scale < min_light_tol)
				continue;
		}

		/* If this is a directional light, add in its direction angle falloff... */
		if (lite->type == LIGHT_DIRECTIONAL)
		{
			double cosa = V3Dot(&lite->dir, &lite_dir);
			if (cosa < EPSILON)
				continue;
			if (lite->angle_min > 0.0)
			{
				/* See if we are within cone aperture... */
				if (cosa < lite->angle_min)
					continue;
				if (cosa < lite->angle_max)
				{
					/* Cone has a soft edge, factor in its falloff... */
					lite_scale *= (cosa - lite->angle_min) / lite->angle_diff;
				}
			}
			lite_scale *= pow(cosa, lite->focus);
			/* If light is completely diminished goto next light. */
			if(lite_scale < min_light_tol)
				continue;
		}

		/* Calc shadow weight. */
		if ((lite->flags & LIGHT_FLAG_NO_SHADOW) == 0)
		{
			if (Ray_TraceShadowRay(&lite_dir, lite, &shadow_color))
				continue;
		}
		else
			V3Set(&shadow_color, 1.0, 1.0, 1.0);

		shadow_color.x *= lite_scale;
		shadow_color.y *= lite_scale;
		shadow_color.z *= lite_scale;

		/* Add in diffuse contribution... */
		if (has_diffuse)
		{
			color.x += lite->color.x * NdotL * ct.kd.x * shadow_color.x
				* base_color.x;
			color.y += lite->color.y * NdotL * ct.kd.y * shadow_color.y
				* base_color.y;
			color.z += lite->color.z * NdotL * ct.kd.z * shadow_color.z
				* base_color.z;
		}

		/* Add in specular contribution... */
		if (has_specular && !(lite->flags & LIGHT_FLAG_NO_SPECULAR))
		{
			double RdotV, spec;
			lite_dir.x -= ct.N.x * NdotL * 2.0;
			lite_dir.y -= ct.N.y * NdotL * 2.0;
			lite_dir.z -= ct.N.z * NdotL * 2.0;
			RdotV = V3Dot(&lite_dir, &ct.D);
			spec = pow(RdotV, ct.Phong) * lite_scale;
			color.x += lite->color.x * spec * ct.ks.x * shadow_color.x;
			color.y += lite->color.y * spec * ct.ks.y * shadow_color.y;
			color.z += lite->color.z * spec * ct.ks.z * shadow_color.z;
		}
	}

	FinishCalcLighting:
	ct.total_color.x += color.x;
	ct.total_color.y += color.y;
	ct.total_color.z += color.z;
}

Light *NewLight(void)
{
  Light *lite = (Light *)Malloc(sizeof(Light));
  if(lite != NULL)
  {
    lite->type = LIGHT_POINT;
    lite->flags = 0;
    lite->falloff = 0.0;
    lite->focus = 1.0;
    lite->angle_min = 0.0;
    lite->angle_max = 0.0;
    lite->angle_diff = 0.0;
    V3Set(&lite->loc, 0.0, 0.0, 0.0);
    V3Set(&lite->at, 0.0, 0.0, -1.0);
    V3Set(&lite->color, 1.0, 1.0, 1.0);
    V3Set(&lite->dir, 0.0, 0.0, 1.0);
    V3Set(&lite->jitter, 0.0, 0.0, 0.0);
    lite->block_obj_cached = NULL;
  }
  return lite;
}


Light *Ray_DeleteLight(Light *lite)
{
  Free(lite, sizeof(Light));
  return NULL;
}


Light *Ray_MakePointLight(Vec3 *loc, Vec3 *color, double falloff)
{
  Light *lite = NewLight();
  if(lite != NULL)
  {
    lite->type = LIGHT_POINT;
    lite->loc = *loc;
    lite->color = *color;
    lite->falloff = falloff;
  }
  return lite;
}


Light *Ray_MakeInfiniteLight(Vec3 *dir, Vec3 *color)
{
  Light *lite = NewLight();
  if(lite != NULL)
  {
    lite->type = LIGHT_INFINITE;
    lite->dir = *dir;
    V3Normalize(&lite->dir);
    lite->color = *color;
  }
  return lite;
}


void Ray_AddLight(Light **llist, Light *lite)
{
  if(lite != NULL)
  {
		/* Do any needed post-processing. */
		if(lite->type != LIGHT_INFINITE)
		{
			V3Sub(&lite->dir, &lite->loc, &lite->at);
			V3Normalize(&lite->dir);
		}
    lite->next = *llist;
    *llist = lite;
  }
}
