/*************************************************************************
*
*  mem.c
*
*  Memory managment functions.
*
*************************************************************************/

#include "ray.h"


size_t ray_mem_used = 0;


int InitializeMem(void)
{
	return 1;
}


void CloseMem(void)
{
}


void *Malloc(size_t size)
{
	void *ptr = malloc(size);
	if(ptr != NULL)
		ray_mem_used += size;
  else
    ray_error = RAY_ERROR_ALLOC;
	return ptr;
}


void *Calloc(size_t qty, size_t size)
{
	void *ptr = calloc(qty, size);
	if(ptr != NULL)
		ray_mem_used += qty * size;
  else
    ray_error = RAY_ERROR_ALLOC;
	return ptr;
}


void *Realloc(void *oldptr, size_t oldsize, size_t newsize)
{
	void *ptr;
	if(oldptr != NULL)
		ray_mem_used -= oldsize;
	ptr = realloc(oldptr, newsize);
	if(ptr != NULL)
		ray_mem_used += newsize;
  else
    ray_error = RAY_ERROR_ALLOC;
	return ptr;
}


void Free(void *ptr, size_t size)
{
	if(ptr != NULL)
	{
		free(ptr);
		ray_mem_used -= size;
	}
}
