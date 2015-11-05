/**
 *****************************************************************************
 *  @file pcontext.c
 *  Maintain and track various state stacks here.
 *
 *****************************************************************************
 */

#include "local.h"

/*
 * Parse context state info structure.
 */
typedef struct tParseContext
{
	char		block_name[FILENAME_MAX];	/* Name of this block. */
	int			object_token;				/* TK_xxx value of object if */
											/* this is an object block. */
	SymbolTable	symbols;					/* Local symbols - anything */
											/* declared in this block. */
} ParseContext;

/* Reallocate stack memory when it reaches a multiple of this amount. */
#define PCONTEXT_STACK_ALLOC_INCR		16

/* Stack pointer tracking info. */
static ParseContext * s_pcontext_stack = NULL;
static int s_pcontext_level = -1;

/* Next level before we need to reallocate more memory. */
static int s_pcontext_maxlevel = 0;

/**
 *	Initialize this module.
 */
int pcontext_init(void)
{
	/* Did we cleanup after a previous session? */
	assert(s_pcontext_stack == NULL);
	assert(s_pcontext_maxlevel == 0);

	return 1;
}

/**
 *	Close down this module.
 *	Frees all resources used.
 */
void pcontext_close(void)
{
	if (s_pcontext_stack != NULL)
	{
		while (s_pcontext_level >= 0)
		{
			/* Free local symbols for this level. */
			symtab_close(&s_pcontext_stack[s_pcontext_level].symbols);
			s_pcontext_level--;
		}

		/* Free the stack array. */

		free(s_pcontext_stack);
		s_pcontext_stack = NULL;
		s_pcontext_level = -1;
		s_pcontext_maxlevel = 0;
	}
}

int pcontext_push(const char *block_name)
{
	s_pcontext_level++;
	if (s_pcontext_level == s_pcontext_maxlevel)
	{
		ParseContext *	newstack;
		int				old_maxlevel = s_pcontext_maxlevel;

		/* Allocate a bigger memory block for the stack. */
		s_pcontext_maxlevel += PCONTEXT_STACK_ALLOC_INCR;
		newstack = (ParseContext *) calloc(s_pcontext_maxlevel,
			sizeof(ParseContext));
		if (newstack == NULL)
		{
			logmemerror("Parse context stack");
			return 0;
		}

		/* Copy existing data to new memory block and free the old one. */
		if (s_pcontext_stack != NULL)
		{
			memcpy(newstack, s_pcontext_stack,
				sizeof(ParseContext) * old_maxlevel);
			free(s_pcontext_stack);
		}

		s_pcontext_stack = newstack;
	}

	/* Initialize the symbol table for this block. */
	symtab_init(&s_pcontext_stack[s_pcontext_level].symbols);

	if (s_pcontext_level > 0)
	{
		/* Copy the previous object token to this level. */
		s_pcontext_stack[s_pcontext_level].object_token =
			s_pcontext_stack[s_pcontext_level-1].object_token;
	}

	/* Set a new block name if one has been given. */
	if (block_name != NULL)
		strncpy(s_pcontext_stack[s_pcontext_level].block_name, block_name, FILENAME_MAX-1);

	return 1;
}

int pcontext_pop(void)
{
	if (s_pcontext_level < 0)
	{
		/* Too many pops. */
		assert(0);
		return 0;
	}

	/* Destroy local symbols for this level. */
	symtab_close(&s_pcontext_stack[s_pcontext_level].symbols);

	s_pcontext_level--;

	return 1;
}

char * pcontext_getname(void)
{
	if (s_pcontext_level < 0)
	{
		/* Nothing on the stack. */
		assert(0);
		return NULL;
	}

	return s_pcontext_stack[s_pcontext_level].block_name;
}

int pcontext_getobjtype(void)
{
	if (s_pcontext_level < 0)
	{
		/* Nothing on the stack. */
		assert(0);
		return TK_NULL;
	}

	return s_pcontext_stack[s_pcontext_level].object_token;
}

void pcontext_setobjtype(int objtoken)
{
	if (s_pcontext_level < 0)
	{
		/* Nothing on the stack. */
		assert(0);
		return;
	}

	s_pcontext_stack[s_pcontext_level].object_token = objtoken;
}

int pcontext_addsymbol(const char *name, int token, int level, void *data)
{
	if (s_pcontext_level < 0)
	{
		/* Nothing on the stack. */
		assert(0);
		return 0;
	}

	if (level > s_pcontext_level)
	{
		/* Offset level out of range! */
		assert(0);
		level = 0;
	}

	if (g_function_mode)
	{
		/* Add this to the current function's recursion stack list. */
		vm_function_add_to_statelist(token, data);
	}

	return symtab_add(&s_pcontext_stack[s_pcontext_level - level].symbols,
		name, token, 0, data);
}

TOKEN *pcontext_findsymbol(const char *name, int local_only)
{
	TOKEN *ptoken;

	if (s_pcontext_level < 0)
	{
		/* Nothing on the stack. */
		assert(0);
		return NULL;
	}

	/* Search local symbols in the current context... */
	ptoken = symtab_find(&s_pcontext_stack[s_pcontext_level].symbols, name);

	if ((ptoken == NULL) && (!local_only) && (s_pcontext_level > 0))
	{
		int level;
		/* Search for symbols outside of this context... */
		for (level = s_pcontext_level - 1; level >= 0; level--)
			if ((ptoken = symtab_find(&s_pcontext_stack[level].symbols, name)) != NULL)
				break;
	}

	return ptoken;
}

VMLValue **pcontext_build_lvalue_list(int *lv_list_len)
{
	if (s_pcontext_level < 0)
	{
		/* Nothing on the stack. */
		assert(0);
		return 0;
	}

	return symtab_build_lvalue_list(
		&s_pcontext_stack[s_pcontext_level].symbols, lv_list_len);
}

