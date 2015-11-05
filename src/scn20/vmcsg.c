/**
 *****************************************************************************
 *  @file vmcsg.c
 *  Virtual machine functions for creating CSG objects.
 *
 *****************************************************************************
 */

#include "local.h"

/*************************************************************************
*
*	Object
*
*************************************************************************/

/*
 * Container for the CSG object statement.
 */
typedef struct tVMStmtCSG
{
	VMStmtObj	vmstmtobj;
	int			csg_type;
	Object		*src_object;
} VMStmtCSG;



/*
 * Methods for the CSG object stmt.
 */
static void vm_csg(VMStmt *thisstmt);
static void vm_csg_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_csg_stmt_methods =
{
	0,
	vm_csg,
	vm_csg_cleanup
};



/**
 *	Parse the CSG object { } block.
 *
 *	A CSG object keyword has just been parsed.
 *
 *	@return VMStmt *, ptr to a CSG object stmt.
 */
VMStmt * parse_vm_csg(int csg_type_token)
{
	VMStmtCSG	*newstmt;
	int			csg_type;
	Object		*src_object = NULL;
	char		name[MAX_TOKEN_LEN];

	switch (csg_type_token)
	{
		case TK_OBJECT:
			strcpy(name, "object");
			csg_type = OBJ_CSGGROUP;
			break;
		case TK_UNION:
			strcpy(name, "union");
			csg_type = OBJ_CSGUNION;
			break;
		case TK_DIFFERENCE:
			strcpy(name, "difference");
			csg_type = OBJ_CSGDIFFERENCE;
			break;
		case TK_INTERSECTION:
			strcpy(name, "intersection");
			csg_type = OBJ_CSGINTERSECTION;
			break;
		case TK_CLIP:
			strcpy(name, "clip");
			csg_type = OBJ_CSGCLIP;
			break;
		case DECL_OBJECT:
			src_object = (Object *) g_cur_token->data;
			strcpy(name, g_token_buffer);
			if (src_object != NULL)
			{
				csg_type = src_object->procs->type;
				switch (csg_type)
				{
					case OBJ_CSGGROUP:
						strcat(name, " (object)");
						break;
					case OBJ_CSGUNION:
						strcat(name, " (union)");
						break;
					case OBJ_CSGDIFFERENCE:
						strcat(name, " (difference)");
						break;
					case OBJ_CSGINTERSECTION:
						strcat(name, " (intersection)");
						break;
					case OBJ_CSGCLIP:
						strcat(name, " (clip)");
						break;
					default:
						assert(0);
						return NULL;
				}
			}
			else
			{
				assert(0);
				return NULL;
			}
			break;
		default:
			assert(0);
			return NULL;
	}

	newstmt = (VMStmtCSG *) begin_parse_object(
		sizeof(VMStmtCSG),
		name,
		csg_type_token,
		&s_csg_stmt_methods);

	// Make sure alloc succeeded.
	//
	if (newstmt == NULL)
		return NULL;

	// Build a CSG object statement and add it to the VM.
	//
	newstmt->csg_type = csg_type;
	newstmt->src_object = src_object;

	// Parse the CSG object's body which will contain child objects.
	//
	newstmt->vmstmtobj.block = parse_vm_block();

	return (VMStmt *) finish_parse_object( (VMStmtObj *) newstmt);
}




/*************************************************************************/

/**
 *	VM object|union|difference|intersection|clip|<defined name> { block }
 */
void vm_csg(VMStmt *curstmt)
{
	VMStmtCSG *	stmtobj = (VMStmtCSG *) curstmt;
	Object *		newobj;
	int				success;

	vm_begin_object((VMStmtObj *) curstmt);
	
	/* Create this object and store it in the VM stack. */
	if (stmtobj->src_object != NULL)
		newobj = Ray_CloneObject(stmtobj->src_object);
	else
		newobj = Ray_MakeCSG(stmtobj->csg_type);
	if (newobj == NULL)
	{
		logmemerror("CSG");
		return;
	}

	vmstack_setcurobj(newobj);

	// Run the statements in the object's block.
	//
	vm_execute_object_block((VMStmtObj *) stmtobj);

	success = Ray_FinishCSG(newobj); 

	vm_finish_object((VMStmtObj *) curstmt, newobj, success);
}

/**
 *	Cleanup function for VM CSG object stmt.
 */
void vm_csg_cleanup(VMStmt *curstmt)
{
	// VMStmtCSG *	stmtobj = (VMStmtCSG *) curstmt;

	// Cleanup the base object statement.
	//
	vm_object_cleanup(curstmt);
}
