/**
 *****************************************************************************
 *  @file symtab.c
 *  Symbol table operations.
 *
 *****************************************************************************
 */

#include "local.h"

#define SYMTAB_SIZE_INC  256

void symtab_init(SymbolTable *pst)
{
	pst->symtab = NULL;
	pst->symtabmax = pst->symcount = pst->stuff_added = 0;
}

void symtab_close(SymbolTable *pst)
{
	if (pst->symtab != NULL)
	{
		int i;
		for (i = 0; i < pst->symcount; i++)
			symtab_deletetoken(pst->symtab[i]);
		free(pst->symtab);
		pst->symtab = NULL;
	}
}

int symtab_add(SymbolTable *pst, const char *name, int token, int flags, void *data)
{
	TOKEN *s = (TOKEN *)calloc(1, sizeof(TOKEN));
	if (s == NULL)
		return 0;
	s->token = token;
	s->flags = flags;
	s->data = data;
	s->name = (char *)malloc(strlen(name)+1);
	if (s->name == NULL)
	{
		symtab_deletetoken(s);
		return 0;
	}
	strcpy(s->name, name);
	if (pst->symcount == pst->symtabmax)
	{
		TOKEN **newsymtab;
		pst->symtabmax += SYMTAB_SIZE_INC;
		newsymtab = calloc(pst->symtabmax, sizeof(TOKEN *));
		if (newsymtab == NULL)
		{
			symtab_deletetoken(s);
			return 0;
		}
		if (pst->symtab != NULL) /* will be NULL if this is the first entry. */
		{
			memcpy(newsymtab, pst->symtab, pst->symcount*sizeof(TOKEN *));
			free(pst->symtab);
		}
		pst->symtab = newsymtab;
	}
	pst->symtab[pst->symcount++] = s;
	pst->stuff_added = 1;

	return 1;
}

static int symcomp(const void *p1, const void *p2)
{
	TOKEN *s1 = *(TOKEN **)p1;
	TOKEN *s2 = *(TOKEN **)p2;
	return strcmp(s1->name, s2->name);
}

TOKEN *symtab_find(SymbolTable *pst, const char *name)
{
	TOKEN key;
	TOKEN *pkey = &key;
	TOKEN **result;
	key.name = (char *)name;
	if (pst->stuff_added)
	{
		qsort(pst->symtab, pst->symcount, sizeof(TOKEN *), symcomp);
		pst->stuff_added = 0;
	}
	result = (TOKEN **)bsearch((void *)&pkey, (void *)pst->symtab, pst->symcount,
		sizeof(TOKEN *), symcomp);
	if (result != NULL)
		return *result;
	return NULL;
}

void symtab_deletetoken(TOKEN *s)
{
	if (s != NULL)
	{
		switch (s->token)
		{
			case DECL_FLOAT:
			case DECL_VECTOR:
				vm_delete_lvalue((VMLValue *)s->data);
				s->data = NULL;
				break;

/* TODO:
			case DECL_COLOR_MAP:
				ColorMap_Delete((ColorMap *)s->data);
				s->data = NULL;
				break;
*/

			case DECL_OBJECT:
				Ray_DeleteObject((Object *)s->data);
				s->data = NULL;
				break;

			case DECL_SURFACE:
				Ray_DeleteSurface((Surface *)s->data);
				s->data = NULL;
				break;

			case DECL_FUNCTION:
				vm_delete((VMStmt *)s->data);
				s->data = NULL;
				break;
		}
		if (s->name != NULL)
			free(s->name);
		free(s);
	}
}

VMLValue **symtab_build_lvalue_list(SymbolTable *pst, int *lv_list_len)
{
	int			i, n;
	VMLValue	**lv_list = NULL;

	(*lv_list_len) = 0;
	n = 0;

	/* Get a count of all l-values in the symbol table. */
	for (i = 0; i < pst->symcount; i++)
	{
		switch (pst->symtab[i]->token)
		{
			case DECL_FLOAT:
			case DECL_VECTOR:
				n++;
				break;
		}
	}

	if (n == 0)
		return NULL;

	/* Allocate the array and fill it with pointers to all
	 * of the l-values.
	 */
	lv_list = (VMLValue **) calloc(n, sizeof(VMLValue *));
	if (lv_list != NULL)
	{
		n = 0;
		for (i = 0; i < pst->symcount; i++)
		{
			switch (pst->symtab[i]->token)
			{
				case DECL_FLOAT:
				case DECL_VECTOR:
					lv_list[n] = (VMLValue *)pst->symtab[i]->data;
					n++;
					break;
			}
		}
	}

	(*lv_list_len) = n;
	return lv_list;
}


