/**
 *****************************************************************************
 * @file vmpoly.c
 *	Virtual machine functions for creating polygon objects.
 *
 *****************************************************************************
 */

#include "local.h"

/*************************************************************************
*
*	Polygon
*
*************************************************************************/

/*
 * Container for a 'polygon' object statement.
 */
typedef struct tVMStmtPolygon
{
	VMStmtObj	vmstmtobj;
	VMExpr		*expr_num_sides;
	int			polygon_type;
} VMStmtPolygon;



/*
 * Methods for the 'polygon' stmt.
 */
static void vm_polygon(VMStmt *thisstmt);
static void vm_polygon_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_polygon_stmt_methods =
{
	TK_POLYGON,
	vm_polygon,
	vm_polygon_cleanup
};



/**
 *	Parse the polygon object { } block.
 *
 *	The 'polygon' keyword has just been parsed.
 *
 *	@return VMStmt *, ptr to a complete object stmt if successful. NULL otherwise.
 */
VMStmt * parse_vm_polygon(int polygon_type_token)
{
	VMStmtPolygon *	newstmt;
	ParamList		params[3];
	int				nparams, i;
	char			name[MAX_TOKEN_LEN];

	switch (polygon_type_token)
	{
		case TK_NPOLYGON:
			strcpy(name, "npolygon");
			break;
		case TK_POLYGON:
			strcpy(name, "polygon");
			break;
		default:
			assert(0);
			return NULL;
	}

	newstmt = (VMStmtPolygon *) begin_parse_object(
		sizeof(VMStmtPolygon),
		name,
		polygon_type_token,
		&s_polygon_stmt_methods);

	// Make sure alloc succeeded.
	//
	if (newstmt == NULL)
		return NULL;

	// Remember what variant of the of the polygon primitive we are.
	newstmt->polygon_type = polygon_type_token;

	/*
	 * For 'npolygon' add 'num_sides' and 'radius' to the local namespace.
	 * These will be bound at runtime to the actual fields in the object.
	 */
	if (polygon_type_token == TK_NPOLYGON)
	{
		/* Parse the npolygon's parameters and body. */
		nparams = parse_paramlist("OEOB", name, params);
	}
	else
	{
		/* Parse the polygon's parameters and body. */
		nparams = parse_paramlist("OB", name, params);
	}

	// Evaluate the parameters.
	//
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr_num_sides == NULL) /* The 'num_sides' parameter. */
					newstmt->expr_num_sides = params[i].data.expr;
				break;
			case PARAM_BLOCK:
				newstmt->vmstmtobj.block = params[i].data.block;
				break;
		}
	}

	return (VMStmt *) finish_parse_object( (VMStmtObj *) newstmt);
}



/*************************************************************************/

/**
 *	VM polygon { block }
 */
void vm_polygon(VMStmt *curstmt)
{
	VMStmtPolygon *	stmtobj = (VMStmtPolygon *) curstmt;
	int				num_sides;
	int				success;
	Object *		newobj;
	
	vm_begin_object((VMStmtObj *) curstmt);
	
	// Create this object and store it in the VM stack.
	//
	num_sides = 3;
	if (stmtobj->polygon_type == TK_NPOLYGON)
	{
		// For the 'npolygon' type, evaluate the number of sides
		// parameter and generate an n-sided polygon of unit radius on
		// the XY plane at point <0, 0, 0>.
		// Location, rotation, and size of the object are set by using
		// transforms.
		if (stmtobj->expr_num_sides != NULL)
			num_sides = (int) vm_evaldouble(stmtobj->expr_num_sides);
		newobj = Ray_MakeNPolygon(num_sides);
		if (newobj == NULL)
		{
			logmemerror("npolygon");
			return;
		}
	}
	else
	{
		newobj = Ray_MakePolygon(NULL, 0);
		if (newobj == NULL)
		{
			logmemerror("polygon");
			return;
		}
	}

	vmstack_setcurobj(newobj);

	// Run the statements in the object's block.
	//
	vm_execute_object_block((VMStmtObj *) stmtobj);

	success = Ray_PolygonFinish(newobj);

	vm_finish_object((VMStmtObj *) curstmt, newobj, success);
}

/**
 *	Cleanup function for VM 'polygon' stmt.
 */
void vm_polygon_cleanup(VMStmt *curstmt)
{
	VMStmtPolygon *	stmtobj = (VMStmtPolygon *) curstmt;

	// Free the parameter initializer expressions.
	//
	delete_exprtree(stmtobj->expr_num_sides);
	stmtobj->expr_num_sides = NULL;

	// Cleanup the base object statement.
	//
	vm_object_cleanup(curstmt);
}



/*************************************************************************
*
*	Polygon vertex
*
*************************************************************************/

/**
 * Container for a vertex statement.
 */
typedef struct tVMStmtVertex
{
	VMStmt		vmstmt;
	VMExpr		*expr;
} VMStmtVertex;


/*
 * Methods for the transform statement.
 */
static void vm_polygon_vertex(VMStmt *thisstmt);
static void vm_polygon_vertex_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_polygon_vertex_stmt_methods =
{
	TK_VERTEX,
	vm_polygon_vertex,
	vm_polygon_vertex_cleanup
};

/**
 *	vertex <x, y, z>
 */
VMStmt *parse_vm_polygon_vertex(void)
{
	int				token;
	VMStmtVertex	*newstmt =
		(VMStmtVertex *) vm_alloc_stmt(sizeof(VMStmtVertex), &s_polygon_vertex_stmt_methods);
	if (newstmt == NULL)
	{
		logmemerror("vertex");
		return NULL;
	}
	
	/* Parse the vertex's vector expression. */
	if ((newstmt->expr = parse_exprtree()) != NULL)
		if ((token = gettoken()) != OP_SEMICOLON)
			gettoken_ErrUnknown(token, ";");

	return (VMStmt *)newstmt;
}

/*************************************************************************/

/**
 *	VM vertex statement
 */
void vm_polygon_vertex(VMStmt *curstmt)
{
	VMStmtVertex *	stmt = (VMStmtVertex *) curstmt;
	Vec3			v;
	Object			*curobj;

	/* Evaluate the expression. */
	vm_evalvector(stmt->expr, &v);

	/* Insert the vertex into the polygon object that we are in. */
	curobj = vmstack_getcurobj();
	if (curobj != NULL)
		Ray_PolygonAddVertex(curobj, &v);
}

/**
 *	Cleanup function for VM vertex stmt.
 */
void vm_polygon_vertex_cleanup(VMStmt *curstmt)
{
	VMStmtVertex *	stmt = (VMStmtVertex *) curstmt;

	/* Delete the expression. */
	delete_exprtree(stmt->expr);
	stmt->expr = NULL;
}




