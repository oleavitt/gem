/*************************************************************************
*
*  scnbuild.c - Interface function that connect the scene building
*    language to the renderer database.
*
*************************************************************************/

#include "local.h"

ObjStack *objstack_ptr = NULL;
Surface *default_surface = NULL;
static ObjStack objstack_base;

RaySetupData *ray_setup_data;

int ScnBuild_Init(RaySetupData *rsd)
{
	assert(objstack_ptr == NULL); /* Make sure that we cleaned up properly. */
	objstack_base.next = objstack_base.prev = NULL;
	objstack_base.objlist = objstack_base.lastobj =
		objstack_base.curobj = NULL;
	objstack_ptr = &objstack_base;
	ray_setup_data = rsd;
	return 1;
}

void ScnBuild_Close(void)
{
	ObjStack *os;
	objstack_ptr = objstack_base.next;
	while(objstack_ptr != NULL)
	{
		os = objstack_ptr;
		objstack_ptr = objstack_ptr->next;
		free(os);
	}
	Ray_DeleteSurface(default_surface);
	default_surface = NULL;
}

int ScnBuild_PushObjStack(void)
{
	if(objstack_ptr->next == NULL)
	{
		ObjStack *os = (ObjStack *)calloc(1, sizeof(ObjStack));
		if(os == NULL)
			return 0;
		objstack_ptr->next = os;
		os->prev = objstack_ptr;
		os->level = objstack_ptr->level + 1;
	}
	objstack_ptr = objstack_ptr->next;
	objstack_ptr->objlist = objstack_ptr->lastobj =
		objstack_ptr->curobj = NULL;
	return 1;
}

void ScnBuild_PopObjStack(void)
{
	objstack_ptr = objstack_ptr->prev;
	assert(objstack_ptr != NULL);
}

void ScnBuild_AddObject(Object *obj)
{
	assert(obj != NULL);
	if(objstack_ptr->lastobj != NULL)
		objstack_ptr->lastobj->next = obj;
	else
		objstack_ptr->objlist = obj;
	objstack_ptr->lastobj = obj;
}

Object *ScnBuild_RemoveLastObject(void)
{
	Object *prevobj, *obj;
	prevobj = NULL;
	for(obj = objstack_ptr->objlist;
		obj != objstack_ptr->lastobj;
		obj = obj->next)
		prevobj = obj;
	objstack_ptr->lastobj = prevobj;
	if(prevobj != NULL)
		prevobj->next = NULL;
	assert(obj != NULL);
	return obj;
}

void ScnBuild_CommitScene(void)
{
	Object *obj;
	assert(objstack_ptr->level == 0);
	ray_setup_data->objects = NULL;
	while(objstack_ptr->objlist != NULL)
	{
		obj = objstack_ptr->objlist;
		objstack_ptr->objlist = obj->next;
		Ray_AddObject(&ray_setup_data->objects, obj);
	}
}
