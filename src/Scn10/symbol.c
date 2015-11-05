/*************************************************************************
*
*	symbol.c - Symbol table operations.
*		Maintains context stack for public, local, and private
*		declarations.
*
*************************************************************************/

#include "local.h"

/* Fixed size for context name fields. */
#define NAME_MAX 40
/* Allocate memory for stack 16 elements at a time. */
#define STACK_ALLOC_INC 16

typedef struct tag_ContextStack
{
	struct tag_ContextStack *next, *prev;
	char contextname[NAME_MAX];
	SymbolTable localsymbols;
	SymbolTable privatesymbols;
} ContextStack;

static ContextStack *rootcontext = NULL;
static ContextStack *curcontext = NULL;

static int contextlevel = 0;

static int NewContext(void);
static void PopContext(void);

int Symbol_Init(void)
{
	assert(curcontext == NULL); /* Did you close last session? */
	contextlevel = 0;
	NewContext();
	rootcontext = curcontext;
	strcpy(rootcontext->contextname, "main");
	return 1;
}

void Symbol_Close(void)
{
	if (curcontext != NULL)
	{
		do
		{
			Symtab_Close(&curcontext->privatesymbols);
			Symtab_Close(&curcontext->localsymbols);
			curcontext = curcontext->prev;
		}
		while (curcontext != NULL);
		while (rootcontext != NULL)
		{
			curcontext = rootcontext->next;
			free(rootcontext);
			rootcontext = curcontext;
		}
	}
}

int Symbol_AddPublic(const char *name, int token, int flags, void *data)
{
	assert(curcontext != NULL); /* Did you initialize? */
	return Symtab_Add(&rootcontext->localsymbols, name, token, flags, data);
}

int Symbol_AddLocal(const char *name, int token, int flags, void *data)
{
	assert(curcontext != NULL); /* Did you initialize? */
	return Symtab_Add(&curcontext->localsymbols, name, token, flags, data);
}

int Symbol_AddPrivate(const char *name, int token, int flags, void *data)
{
	assert(curcontext != NULL); /* Did you initialize? */
	return Symtab_Add(&curcontext->privatesymbols, name, token, flags, data);
}

TOKEN *Symbol_FindLocal(const char *name)
{
	TOKEN *ptoken;
	assert(curcontext != NULL); /* Did you initialize? */
	/* Search private symbols... */
	/* ...and then local symbols on this level only... */
	if ((ptoken = Symtab_Find(&curcontext->privatesymbols, name)) == NULL)
		ptoken = Symtab_Find(&curcontext->localsymbols, name);
	return ptoken;
}

TOKEN *Symbol_Find(const char *name)
{
	TOKEN *ptoken;
	assert(curcontext != NULL); /* Did you initialize? */
	/* Search private symbols... */
	if ((ptoken = Symtab_Find(&curcontext->privatesymbols, name)) == NULL)
	{
		ContextStack *pcontext;
		/* Search local symbols in this and in all contexts above this one... */
		for (pcontext = curcontext; pcontext != NULL; pcontext = pcontext->prev)
			if ((ptoken = Symtab_Find(&pcontext->localsymbols, name)) != NULL)
				return ptoken;
	}
	return ptoken;
}

void Symbol_Push(void)
{
	assert(curcontext != NULL); /* Did you initialize? */
	/* Initialize a new context on the stack. */
	NewContext();
}

void Symbol_Pop(void)
{
	assert(curcontext != NULL); /* Did you initialize? */
	/* Cleanup this context and return to previous one. */
	/* Private and local symbols are now out of scope. */
	PopContext();
}

void Symbol_Delete(TOKEN *s)
{
	assert(curcontext != NULL); /* Did you initialize? */
	Symtab_DeleteToken(s);
}

/***************************************************************
*
*	Local helper functions.
*
***************************************************************/

int NewContext(void)
{
	ContextStack *newcontext = (curcontext != NULL) ? curcontext->next : NULL;
	if (newcontext == NULL)
	{
		if ((newcontext = (ContextStack *)malloc(sizeof(ContextStack))) == NULL)
			return 0;
		memset(newcontext, 0, sizeof(ContextStack));
		newcontext->next = NULL;
		newcontext->prev = curcontext;
		if (curcontext != NULL)
			curcontext->next = newcontext;
	}
	curcontext = newcontext;
	contextlevel++;
	Symtab_Init(&curcontext->localsymbols);
	Symtab_Init(&curcontext->privatesymbols);
	return 1;
}

void PopContext(void)
{
	assert(curcontext->prev != NULL);
	Symtab_Close(&curcontext->localsymbols);
	Symtab_Close(&curcontext->privatesymbols);
	curcontext = curcontext->prev;
	contextlevel--;
}
