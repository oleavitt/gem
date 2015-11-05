/**
 *****************************************************************************
 * @file object.c
 *	Top-level object functions.
 *
 *****************************************************************************
 */

#include "ray.h"

unsigned long ray_num_objects = 0;
Object* ray_object_list = NULL;


int InitializeObject(void)
{
	ray_object_list = NULL;
	ray_num_objects = 0;

	/* Reset the statistics counters. */
	ray_blob_tests = 0;
	ray_blob_hits = 0;
	ray_box_tests = 0;
	ray_box_hits = 0;
	ray_colortri_tests = 0;
	ray_colortri_hits = 0;
	ray_cone_tests = 0;
	ray_cone_hits = 0;
	ray_disc_tests = 0;
	ray_disc_hits = 0;
	ray_fnxyz_tests = 0;
	ray_fnxyz_hits = 0;
	ray_mesh_tests = 0;
	ray_mesh_hits = 0;
	ray_polygon_tests = 0;
	ray_polygon_hits = 0;
	ray_sphere_tests = 0;
	ray_sphere_hits = 0;
	ray_torus_tests = 0;
	ray_torus_hits = 0;
	ray_triangle_tests = 0;
	ray_triangle_hits = 0;

	ray_num_bounds = 0;

	return 1;
}


void CloseObject(void)
{
	Object *o;
	while(ray_object_list != NULL)
	{
		o = ray_object_list;
		ray_object_list = o->next;
		Ray_DeleteObject(o);
	}
}


Object *NewObject(void)
{
	Object *obj = (Object *)Malloc(sizeof(Object));
	if (obj != NULL)
	{
		memset(obj, 0, sizeof(Object));
		obj->next = NULL;
		obj->surface = NULL;
		obj->T = NULL;
		ray_num_objects++;
	}
	return obj;
}


void CalcBoundObjectExtents(Object *boundobjs, Vec3 *omin, Vec3 *omax)
{
	Vec3 bmin, bmax;
	Object *o = boundobjs;
	o->procs->CalcExtents(o, omin, omax);
	for (o = o->next; o != NULL; o = o->next)
	{
		o->procs->CalcExtents(o, &bmin, &bmax);
		if (bmin.x < omin->x) omin->x = bmin.x;
		if (bmax.x > omax->x) omax->x = bmax.x;
		if (bmin.y < omin->y) omin->y = bmin.y;
		if (bmax.y > omax->y) omax->y = bmax.y;
		if (bmin.z < omin->z) omin->z = bmin.z;
		if (bmax.z > omax->z) omax->z = bmax.z;
	}
}


void Ray_PostProcessObject(Object *obj)
{
	if (obj != NULL)
	{
		switch (obj->procs->type)
		{
			case OBJ_CSGGROUP:
			case OBJ_CSGUNION:
			case OBJ_CSGDIFFERENCE:
			case OBJ_CSGINTERSECTION:
			case OBJ_CSGCLIP:
				PostProcessCSG(obj);
				break;
			case OBJ_BBOX:
				PostProcessBBox(obj);
				break;
		}
	}
}


Object *Ray_DeleteObject(Object *obj)
{
	if (obj != NULL)
	{
		if (obj->procs != NULL)
			obj->procs->Delete(obj);
		Ray_DeleteXform(obj->T);
		Ray_DeleteSurface(obj->surface);
		Free(obj, sizeof(Object));
		ray_num_objects--;
	}
	return NULL;
}


void Ray_AddObject(Object **olist, Object *obj)
{
	if (obj != NULL)
	{
		csg_nest_level = 0;
		Ray_PostProcessObject(obj);
		obj->next = *olist;
		*olist = obj;
	}
}


void Ray_Transform_Object(Object *obj, Vec3 *params, int type)
{
	/*
	 * If we have any surfaces with run-time modifiers, we
	 * need a transform matrix - create an identity matrix now.
	 * Not all object-specific transforms create matrices.
	 */
	if (obj->T == NULL && obj->surface != NULL &&
		 obj->surface->shaders != NULL)
		obj->T = Ray_NewXform();
	obj->procs->Transform(obj, params, type);
}


Object *Ray_CloneObject(Object *srcobj)
{
	Object *destobj = NULL;
	if (srcobj != NULL)
	{
		if ((destobj = NewObject()) != NULL)
		{
			destobj->procs = srcobj->procs;
			destobj->surface = Ray_CloneSurface(srcobj->surface);
			destobj->flags = srcobj->flags;
			destobj->T = Ray_CloneXform(srcobj->T);
			srcobj->procs->Copy(destobj, srcobj);
		}
	}
	return destobj;
}


void Object_GetTextureInfo(Object *obj, Surface **surf, Xform **T)
{
	switch (obj->procs->type)
	{
		case OBJ_CSGGROUP:
		case OBJ_CSGUNION:
		case OBJ_CSGDIFFERENCE:
		case OBJ_CSGINTERSECTION:
		case OBJ_CSGCLIP:
			CSG_GetTextureInfo(obj, surf, T);
			return;
		case OBJ_BBOX:
			BBox_GetTextureInfo(obj, surf, T);
			return;
		default:
			if (obj->surface != NULL)
			{
				*surf = obj->surface;
				/* TODO: Check for local Xform on LocalSurfaceData. */
				*T = obj->T;
			}
			else
			{
				*surf = NULL;
				*T = NULL;
			}
			return;
	}
}
