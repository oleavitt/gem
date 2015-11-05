/*************************************************************************
*
*  symtab.c - Symbol table operations.
*
*************************************************************************/

#include "local.h"

#define SYMTAB_SIZE_INC  256

void Symtab_Init(SymbolTable *pst)
{
	pst->symtab = NULL;
	pst->symtabmax = pst->symcount = pst->stuff_added = 0;
}

void Symtab_Close(SymbolTable *pst)
{
	if (pst->symtab != NULL)
	{
		int i;
		for (i = 0; i < pst->symcount; i++)
			Symtab_DeleteToken(pst->symtab[i]);
		free(pst->symtab);
		pst->symtab = NULL;
	}
}

int Symtab_Add(SymbolTable *pst, const char *name, int token, int flags, void *data)
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
		Symtab_DeleteToken(s);
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
			Symtab_DeleteToken(s);
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

static int SymComp(const void *p1, const void *p2)
{
	TOKEN *s1 = *(TOKEN **)p1;
	TOKEN *s2 = *(TOKEN **)p2;
	return strcmp(s1->name, s2->name);
}

TOKEN *Symtab_Find(SymbolTable *pst, const char *name)
{
	TOKEN key;
	TOKEN *pkey = &key;
	TOKEN **result;
	key.name = (char *)name;
	if (pst->stuff_added)
	{
		qsort(pst->symtab, pst->symcount, sizeof(TOKEN *), SymComp);
		pst->stuff_added = 0;
	}
	result = (TOKEN **)bsearch((void *)&pkey, (void *)pst->symtab, pst->symcount,
		sizeof(TOKEN *), SymComp);
	if (result != NULL)
		return *result;
	return NULL;
}

void Symtab_DeleteToken(TOKEN *s)
{
	if (s != NULL)
	{
		switch (s->token)
		{
			case DECL_FLOAT:
			case DECL_VECTOR:
				LValueDelete((LValue *)s->data);
				s->data = NULL;
				break;
			case DECL_COLOR_MAP:
				ColorMap_Delete((ColorMap *)s->data);
				s->data = NULL;
				break;
			case DECL_OBJECT:
				Ray_DeleteObject((Object *)s->data);
				s->data = NULL;
				break;
			case DECL_SURFACE:
				Ray_DeleteSurface((Surface *)s->data);
				s->data = NULL;
				break;
		}
		if (s->name != NULL)
			free(s->name);
		free(s);
	}
}
