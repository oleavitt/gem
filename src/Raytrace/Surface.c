/**
 *****************************************************************************
 * @file surface.c
 * Handles the Surface type.
 *
 *****************************************************************************
 */

#include "ray.h"

// Pointer to Surface that is used as the 'default' surface.
//
Surface *DefaultSurface;

/**
 * Initialize the surface stuff.
 * Called by Ray_Initialize() in raytrace.c when the renderer is initialized.
 *
 * @return int - 1 to indicate success, or 0 for failure.
 */
int InitializeSurface(void)
{
	DefaultSurface = Ray_NewSurface();
	if (DefaultSurface == NULL)
  		return 0;
	return 1;
}


/**
 * Close down the surface stuff.
 * Called by Ray_Close() in raytrace.c when the renderer is shut down.
 */
void CloseSurface(void)
{
	Ray_DeleteSurface(DefaultSurface);
	DefaultSurface = NULL;
}


/**
 * Allocates and initializes a new Surface.
 *
 * @return Surface* - Pointer to a new Surface, or NULL if failed.
 */
Surface *Ray_NewSurface(void)
{
	Surface *s = (Surface *) Malloc(sizeof(Surface));

	if (s != NULL)
	{
		V3Set(&s->color, 1.0, 1.0, 1.0);
		V3Set(&s->ka, 0.0, 0.0, 0.0);
		V3Set(&s->kd, 1.0, 1.0, 1.0);
		V3Set(&s->kr, 0.0, 0.0, 0.0);
		V3Set(&s->ks, 0.0, 0.0, 0.0);
		V3Set(&s->kt, 0.0, 0.0, 0.0);
		s->T = NULL;
		s->spec_power = 1.0;
		s->ior = 1.0;
		s->outior = ray_global_ior;
		s->nrefs = 1;
		s->shaders = NULL;
	}

	return s;
}


/**
 * Releases a share of a Surface.
 * If this is the last share, the Surface is freed from memory and
 * other resources used by the Surface are released.
 *
 * @param s - Surface* - Pointer to Surface to be released.
 */
void Ray_DeleteSurface(Surface *s)
{
	if (s != NULL)
	{
		if (--s->nrefs == 0)
		{
			Ray_DeleteShaderList(s->shaders);
			Ray_DeleteXform(s->T);
			Free(s, sizeof(Surface));
		}
	}
}


/**
 * Registers a share of a Surface.
 * Called whenever the same Surface used in more than one object,
 * or used from a library of pre-defined Surfaces.
 *
 * @param s - Surface* - Pointer to Surface to be shared.
 *
 * @return Surface* - Pointer to the same Surface instance.
 */
Surface *Ray_ShareSurface(Surface *s)
{
	if (s != NULL)
		s->nrefs++;

	return s;
}


/**
 * Creates an identical copy of a Surface from new resources.
 * Any transforms are deep copied.
 *
 * @param srcsurf - Surface* - Pointer to Surface to be copied.
 *
 * @return Surface* - Pointer to a new Surface instance, or NULL if alloc failed or src is NULL.
 */
Surface *Ray_CloneSurface(Surface *srcsurf)
{
	Surface	*newsurf = NULL;
	
	// If srcsurf is NULL, behavior is just like Ray_NewSurface()
	//
	if (srcsurf != NULL)
	{
		// Start with a new Surface.
		//
		newsurf = Ray_NewSurface();
		if (newsurf != NULL)
		{
			// Shallow copy all Surface struct members.
			//
			*newsurf = *srcsurf;
			
			// Reset the share counter.
			//
			newsurf->nrefs = 1;

			// Deep copy the transforms.
			//
			newsurf->T = Ray_CloneXform(srcsurf->T);

			// Deep copy the shader list.
			// The VMShaders pointed to in the list are shared by ref count.
			//
			newsurf->shaders = Ray_CloneShaderList(srcsurf->shaders);
		}
	}

	return newsurf;
}

/**
 * Add a transform to the transform matrix of a Surface.
 *
 * @param s - Surface* - The Surface to add transform to.
 * @param params - Vec3* - The transform parameters.
 * @param type - int - The type of transform. See enum XFORM_xxx in "raytrace.h".
 */
void Ray_Transform_Surface(Surface *s, Vec3 *params, int type)
{
	if (s->T == NULL)
		s->T = Ray_NewXform();
	XformXforms(s->T, params, type);
}

/**
 * Apply surface shading to the current object that was hit.
 */
void ShadeSurface(void)
{
	Xform *T;
	Shader	*shader;

	assert(ct.objhit != NULL);
	Object_GetTextureInfo(ct.objhit, &ct.surface, &T);

	rt_surface = ct.surface;

	if (rt_surface != NULL)
	{
		if (ct.surface->shaders != NULL)
		{
			// Update run-time variables.
			//
			rt_O = rt_W = ct.Q;
			rt_ON = rt_WN = ct.N;
			rt_D = ct.D;
			if (ct.surface->T != NULL)
			{
				PointToObject(&rt_O, ct.surface->T);
				NormToObject(&rt_ON, ct.surface->T);
			}
			if (T != NULL)
			{
				PointToObject(&rt_O, T);
				NormToObject(&rt_ON, T);
			}

         // TODO: Check at compile time to see if "u" or "v" are used and set a flag if so.
         // Do this calculation only if "u" or "v" are used.
			ct.objhit->procs->CalcUVMap(ct.objhit, &rt_O, &rt_u, &rt_v);
			
			// Run the shader(s)
			//
			for (shader = ct.surface->shaders;
				shader != NULL;
				shader = shader->next)
			{
				Ray_RunShader(shader, ct.surface); 
			}

			if (ct.surface->T != NULL)
			{
				NormToWorld(&rt_ON, ct.surface->T);
			}
			if (T != NULL)
			{
				NormToWorld(&rt_ON, T);
			}
			V3Copy(&ct.N, &rt_ON);

			// Check for a changed transmission value and set the
			// object transmissive flag if necessary.
			//
			if (((ct.objhit->flags & OBJ_FLAG_TRANSMISSIVE) == 0) &&
					!V3IsZero(&ct.surface->kt))
				ct.objhit->flags |= OBJ_FLAG_TRANSMISSIVE;
		}
	}
	else
		ct.surface = rt_surface = DefaultSurface;

	// Save all of the lighting constants for this level that
	// might be changed on the surface during a recursive trace.
	//
	ct.color = rt_surface->color;
	ct.ka = rt_surface->ka;
	ct.kd = rt_surface->kd;
	ct.kr = rt_surface->kr;
	ct.ks = rt_surface->ks;
	ct.kt = rt_surface->kt;
	ct.ior = rt_surface->ior;
	ct.Phong = rt_surface->spec_power;
}

// TODO:
#if 0
void SMApplyShader( SurfaceModifier *sm, Surface *surf )
{
	VMSurfaceShader *sh = (VMSurfaceShader *) sm->data;

	/* Initialize the shader's lighting parameters with the
	 * current values in the surface.
	 */
	V3Copy(&sh->lv_color->v, &surf->color);
	V3Copy(&sh->lv_ka->v, &surf->ka);
	V3Copy(&sh->lv_kd->v, &surf->kd);
	V3Copy(&sh->lv_ks->v, &surf->ks);
	sh->lv_Phong->v.x = surf->spec_power;
	V3Copy(&sh->lv_kr->v, &surf->kr);
	V3Copy(&sh->lv_kt->v, &surf->kt);
	sh->lv_ior->v.x = surf->ior;
	sh->lv_outior->v.x = surf->outior;

	/* Run the shader. */
	sh->methods->fn( (VMStmt *) sh );

	/* Store the output results in the surface. */
	V3Copy(&surf->color, &sh->lv_color->v);
	V3Copy(&surf->ka, &sh->lv_ka->v);
	V3Copy(&surf->kd, &sh->lv_kd->v);
	V3Copy(&surf->ks, &sh->lv_ks->v);
	surf->spec_power = sh->lv_Phong->v.x;
	V3Copy(&surf->kr, &sh->lv_kr->v);
	V3Copy(&surf->kt, &sh->lv_kt->v);
	surf->ior = sh->lv_ior->v.x;
	surf->outior = sh->lv_outior->v.x;
}

void SMCopyShader( SurfaceModifier *smdst, SurfaceModifier *smsrc )
{
	VMSurfaceShader *sh = (VMSurfaceShader *) smsrc->data;
	sh->nrefs++;
	smdst->data = (void *) sh;
}

void SMDeleteShader( SurfaceModifier *sm )
{
	VMSurfaceShader *sh = (VMSurfaceShader *) sm->data;
	if ( --sh->nrefs == 0 )
		vm_delete( (VMStmt *) sh );
}
#endif
