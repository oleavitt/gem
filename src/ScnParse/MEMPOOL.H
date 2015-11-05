/*************************************************************************
*
*  mempool.h - Memory pool managment functions.
*
*************************************************************************/

#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stdlib.h>       /* for size_t type */

/*
 * Maintains a pointer to a block of memory obtained by a call to mpalloc().
 */
typedef struct tag_mem
	{
	struct tag_mem *next;  /* Next block in list. */
	void *block;           /* The memory block*/
	char *ptr;             /* Next free slot in memory block*/
	size_t used; /* Total bytes used from this block, including list elem. */
	size_t avail;          /* Memory available from this block. */
	size_t block_size;     /* Total block increment size. */
	} mempool;


extern mempool *new_mem_pool(size_t block_size);
extern void *mpalloc(size_t size, mempool *pool);
extern mempool *delete_mem_pool(mempool *pool);

extern void *mmalloc(size_t size);
extern void *mrealloc(void *old_ptr, size_t old_size, size_t new_size);
extern char *str_dup(const char *src);
extern void mfree(void *ptr, size_t size);
extern void str_free(char *str);

/* Total memory used. */
/* extern size_t scn_mem_used; */


#endif   /* MEMPOOL_H defined */
