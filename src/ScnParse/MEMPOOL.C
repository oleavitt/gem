/*************************************************************************
 *
 *  mempool.c
 *
 *  Memory pool and allocation management functions.
 *
 ************************************************************************/

#include "scn.h"


/* Total memory used. */
size_t scn_mem_used = 0;

/*
 * Use this block size if another size
 * isn't explicitly given.
 */
#define BLOCK_SIZE 16384

/*
 * Allocate blocks in increments of this size.
 */
static size_t block_size_inc;

/*
 * local functions and data
 */
/* Pointer to any of the pools below. */
static mempool *mem_pool = NULL;

/* Low memory warning flag. */
static int low_mem = 0;

static void *pool_malloc(size_t size);
static mempool *new_mem_block(size_t size);
static void free_pool(void);


/*
 *  Create a new user-owned pool.
 */	
mempool *new_mem_pool(size_t block_size)
	{
	block_size_inc = block_size;
	return new_mem_block(block_size);
	}

/*
 *  Allocate memory from a user-owned pool.
 */
void *mpalloc(size_t size, mempool *pool)
	{
	mem_pool = pool;
	return pool_malloc(size);
	}

/*
 *  De-allocate a user-owned pool.
 */
mempool *delete_mem_pool(mempool *pool)
	{
	mem_pool = pool;
	free_pool();
	return NULL;
	}


/*
 * Allocate memory from currently selected pool.
 */
static void *pool_malloc(size_t size)
	{
	void *ptr;
	mempool *cur_pool;

	if(mem_pool == NULL) /* First mem request, start the list. */
		mem_pool = new_mem_block(size);

	cur_pool = mem_pool;
	/* Scan all blocks for any slots big enough for "size". */
	while(cur_pool != NULL)
		{
		if(size <= cur_pool->avail)
			{
			cur_pool->avail -= size;
			ptr = cur_pool->ptr;
			cur_pool->ptr += size;
			break;
			}

		if(cur_pool->next == NULL) /* Add a new block, to the list. */
			cur_pool->next = new_mem_block(size);

		cur_pool = cur_pool->next;
		}

	return ptr;
  }

/*
 * Make a new memory pool block.
 */
mempool *new_mem_block(size_t size)
	{
	size_t block_size;
	mempool *new_pool;

	block_size = max(size, block_size_inc);

	new_pool = (mempool *)malloc(sizeof(mempool));
	if(new_pool == NULL)
		{
		/* If this failed, we're out. */
		SCN_Message(SCN_MSG_ERROR, 
      "Out of memory! %u bytes requested. %lu bytes used.",
      block_size, scn_mem_used);
		}

	new_pool->next = NULL;
	new_pool->block_size = block_size_inc;
	new_pool->used = sizeof(mempool);
	scn_mem_used += sizeof(mempool);

	new_pool->block = malloc(block_size);
	if(new_pool->block == NULL) /* Not enough memory for block... */
		{
		if(size < block_size) /* Try a smaller request... */
			{
			block_size = size;
			new_pool->block = malloc(block_size);
			if(new_pool->block != NULL) /* Got it, but we're running out...*/
				{
				new_pool->avail = 0;
				new_pool->used += block_size;
				scn_mem_used += block_size;
				if(low_mem == 0)
					{
					SCN_Message(SCN_MSG_WARNING, "Low memory! Less than %u bytes left. %lu bytes used.",
						block_size_inc, scn_mem_used);
					low_mem = 1;    /* Warn only once! */
					}
				return new_pool;
				}
			}
		SCN_Message(SCN_MSG_ERROR, "Out of memory! %u bytes requested. %lu bytes used.",
			block_size, scn_mem_used);
		}  /* end of if(block is NULL) */

	new_pool->ptr = new_pool->block;
	new_pool->avail = block_size;
	new_pool->used += block_size;
	scn_mem_used += block_size;

	return new_pool;
	}

/*
 * Free selected memory pool.
 */
static void free_pool(void)
	{
	mempool *tmp;

	while(mem_pool != NULL)
		{
		free(mem_pool->block);
		scn_mem_used -= mem_pool->used; /* Includes size of the "mempool" struct. */
		tmp = mem_pool;
		mem_pool = mem_pool->next;
		free(tmp);
		}
	}


/*************************************************************************
 * Request memory from system and check for valid pointer.
 */
void *mmalloc(size_t size)
	{
  void *ptr;

	ptr = malloc(size);

	if(ptr == NULL)
		{
		SCN_Message(SCN_MSG_ERROR, "Memory allocation error. %u bytes requested. %lu bytes used.",
			size, scn_mem_used);
		}
	scn_mem_used += (unsigned long)size;

	return ptr;
	}

/*************************************************************************
 * Reallocate memory and check for valid pointer.
 */
void *mrealloc(void *old_ptr, size_t old_size, size_t new_size)
	{
	void *ptr;

	ptr = realloc(old_ptr, new_size);

	if(ptr == NULL)
		{
		SCN_Message(SCN_MSG_ERROR, "Memory allocation error. %u bytes requested. %lu bytes used.",
			new_size, scn_mem_used);
		}
	scn_mem_used += (unsigned long)new_size - (unsigned long)old_size;

	return ptr;
	}

/*************************************************************************
 * Allocate storage for, and duplicate, a zero terminated string.
 */
char *str_dup(const char *src)
	{
	char * dest;
	dest = (char *)mmalloc(sizeof(char) * (strlen(src) + 1));
	strcpy(dest, src);
	return dest;
	}

/*************************************************************************
 * Corresponding mfree() for mmalloc(), above.
 */
void mfree(void *ptr, size_t size)
	{
	if(NULL != ptr)
		{
		free(ptr);
		scn_mem_used -= (unsigned long)size;
		}
	}


/*
 * Free a string obtained from str_dup().
 */
void str_free(char *str)
	{
	if(str != NULL)
		mfree(str, sizeof(char) * (strlen(str) + 1));
	}
