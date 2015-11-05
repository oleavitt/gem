/**
 *****************************************************************************
 *  @file vmstack.c
 *  Maintain and track the VM state stack here.
 *
 *****************************************************************************
 */

#include "local.h"

/*
 * Virtual machine state info structure.
 */
typedef struct tVMStackElem
{
	VMStmt	*stmt;
	Object	*cur_object;
	Vec3	*lv_values_list;
	int		lv_values_list_len;
} VMStackElem;

/* Reallocate stack memory when it reaches a multiple of this amount. */
#define VMSTACK_STACK_ALLOC_INCR		16

/* Stack pointer tracking info. */
static VMStackElem * s_vmstack_stack = NULL;
static int s_vmstack_level = -1;

/* Next level before we need to reallocate more memory. */
static int s_vmstack_maxlevel = 0;

/**
 *	Initialize this module.
 */
int vmstack_init(void)
{
	/* Did we cleanup after a previous session? */
	assert(s_vmstack_stack == NULL);
	assert(s_vmstack_maxlevel == 0);

	return 1;
}

/**
 *	Close down this module.
 *	Frees all resources used.
 */
void vmstack_close(void)
{
	if (s_vmstack_stack != NULL)
	{
		while (s_vmstack_level >= 0)
		{
			if (s_vmstack_stack[s_vmstack_level].lv_values_list != NULL)
				free(s_vmstack_stack[s_vmstack_level].lv_values_list);
			s_vmstack_level--;
		}
		free(s_vmstack_stack);
		s_vmstack_stack = NULL;
		s_vmstack_maxlevel = 0;
	}
}

int vmstack_push(VMStmt * stmt)
{
	s_vmstack_level++;
	if (s_vmstack_level == s_vmstack_maxlevel)
	{
		VMStackElem *	newstack;
		int				old_maxlevel = s_vmstack_maxlevel;

		/* Allocate a bigger memory block for the stack. */
		s_vmstack_maxlevel += VMSTACK_STACK_ALLOC_INCR;
		newstack = (VMStackElem *) calloc(s_vmstack_maxlevel,
			sizeof(VMStackElem));
		if (newstack == NULL)
		{
			logmemerror("VM stack");
			return 0;
		}

		/* Copy existing data to new memory block and free the old one. */
		if (s_vmstack_stack != NULL)
		{
			memcpy(newstack, s_vmstack_stack,
				sizeof(VMStackElem) * old_maxlevel);
			free(s_vmstack_stack);
		}

		s_vmstack_stack = newstack;

		s_vmstack_stack[s_vmstack_level].cur_object = NULL;
		s_vmstack_stack[s_vmstack_level].lv_values_list = NULL;
		s_vmstack_stack[s_vmstack_level].lv_values_list_len = 0;
	}

	s_vmstack_stack[s_vmstack_level].stmt = stmt;
	if (s_vmstack_level > 0)
	{
		/* Copy a any saved object context to this level. */
		s_vmstack_stack[s_vmstack_level].cur_object =
			s_vmstack_stack[s_vmstack_level-1].cur_object;
	}
	else
	{
		s_vmstack_stack[s_vmstack_level].cur_object = NULL;
	}

	return 1;
}

int vmstack_pop(void)
{
	if (s_vmstack_level < 0)
	{
		/* Too many pops. */
		assert(0);
		return 0;
	}
	s_vmstack_level--;

	return 1;
}

/*************************************************************************
*
*	Object context
*
*************************************************************************/
/**
 * Get a ptr to the current object that we are processing.
 */
Object * vmstack_getcurobj(void)
{
	if (s_vmstack_level < 0)
	{
		/* No stack! */
		/* assert(0); */
		return NULL;
	}

	return s_vmstack_stack[s_vmstack_level].cur_object;
}

/**
 * Store a ptr to an object that we are processing.
 * This object will e the current object for this level and any levels
 * pushed onto the stack.
 */
int vmstack_setcurobj(Object * obj)
{
	if (s_vmstack_level < 0)
	{
		/* No stack! */
		assert(0);
		return 0;
	}

	s_vmstack_stack[s_vmstack_level].cur_object = obj;
	return 1;
}

/*************************************************************************
*
*	Function call context
*
*************************************************************************/

void vmstack_save_lvalues(VMLValue **lv_list, int lv_list_len)
{
	VMStackElem	*cur;
	int			i;

	if (s_vmstack_level < 0)
	{
		/* No stack. */
		return;
	}

	cur = &s_vmstack_stack[s_vmstack_level];
	if (cur->lv_values_list_len < lv_list_len)
	{
		if (cur->lv_values_list != NULL)
			free(cur->lv_values_list);

		cur->lv_values_list = (Vec3 *) calloc(lv_list_len, sizeof(Vec3));
		cur->lv_values_list_len = lv_list_len;
	}

	if (cur->lv_values_list != NULL)
	{
		for (i = 0; i < lv_list_len; i++)
			V3Copy(&cur->lv_values_list[i], &lv_list[i]->v);
	}
}

void vmstack_restore_lvalues(VMLValue **lv_list, int lv_list_len)
{
	VMStackElem	*cur;
	int			i;

	if (s_vmstack_level < 0)
	{
		/* No stack. */
		return;
	}

	cur = &s_vmstack_stack[s_vmstack_level];
	if (lv_list_len > cur->lv_values_list_len)
	{
		/* Did we call vmstack_save_lvalues() for this level? */
		assert(0);
		return;
	}

	if (cur->lv_values_list != NULL)
	{
		for (i = 0; i < lv_list_len; i++)
			V3Copy(&lv_list[i]->v, &cur->lv_values_list[i]);
	}
}
