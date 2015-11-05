/**
 *****************************************************************************
 * @file vmblob.c
 *	Virtual machine functions for creating blob objects.
 *
 *****************************************************************************
 */

#include "local.h"

/*************************************************************************
*
*	Blob
*
*************************************************************************/

/*
 * Container for a 'blob' object statement.
 */
typedef struct tVMStmtBlob
{
	VMStmtObj	vmstmtobj;
	VMExpr		*expr_threshold;
} VMStmtBlob;



/*
 * Methods for the 'blob' stmt.
 */
static void vm_blob(VMStmt *thisstmt);
static void vm_blob_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_blob_stmt_methods =
{
	TK_BLOB,
	vm_blob,
	vm_blob_cleanup
};



/**
 *	Parse the blob object { } block.
 *
 *	The 'blob' keyword has just been parsed.
 *
 *	@return VMStmt *, ptr to a complete object stmt if successful. NULL otherwise.
 */
VMStmt * parse_vm_blob(void)
{
	VMStmtBlob *	newstmt;
	ParamList		params[2];
	int				nparams, i;
	char			name[] = "blob";

	newstmt = (VMStmtBlob *) begin_parse_object(
		sizeof(VMStmtBlob),
		name,
		TK_BLOB,
		&s_blob_stmt_methods);

	// Make sure alloc succeeded.
	//
	if (newstmt == NULL)
		return NULL;

	// Parse the blob's parameters and body.
	// The threshold parameter must be supplied.
	//
	nparams = parse_paramlist("EB", name, params);

	// Evaluate the parameters.
	//
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr_threshold == NULL) // The 'threshold' parameter.
					newstmt->expr_threshold = params[i].data.expr;
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
 *	VM blob <threshold> { block }
 */
void vm_blob(VMStmt *curstmt)
{
	VMStmtBlob *	stmtobj = (VMStmtBlob *) curstmt;
	double			threshold;
	int				success;
	Object *		newobj;
	
	vm_begin_object((VMStmtObj *) curstmt);
	
	// Evaulate the threshold parameter.
	threshold = 0.5;
	if (stmtobj->expr_threshold != NULL)
		threshold = vm_evaldouble(stmtobj->expr_threshold);

	// Create this object and store it in the VM stack.
	//
	newobj = Ray_MakeBlob(threshold);
	if (newobj == NULL)
	{
		logmemerror("blob");
		return;
	}

	vmstack_setcurobj(newobj);

	// Run the statements in the object's block.
	//
	vm_execute_object_block((VMStmtObj *) stmtobj);

	// Post process the object.
	//
	success = Ray_BlobFinish(newobj);

	vm_finish_object((VMStmtObj *) curstmt, newobj, success);
}

/**
 *	Cleanup function for VM 'blob' stmt.
 */
void vm_blob_cleanup(VMStmt *curstmt)
{
	VMStmtBlob *	stmtobj = (VMStmtBlob *) curstmt;

	// Free the parameter initializer expressions.
	//
	delete_exprtree(stmtobj->expr_threshold);
	stmtobj->expr_threshold = NULL;

	// Cleanup the base object statement.
	//
	vm_object_cleanup(curstmt);
}



/*************************************************************************
*
*	Blob elements - sphere, cylinder, plane
*
*************************************************************************/

/**
 * Polymorphic container for the three different blob element statements.
 * There can be upto four arguments, so four exprs are provided.
 */
typedef struct tVMStmtBlobElement
{
	VMStmt		vmstmt;
	VMExpr		*expr1;
	VMExpr		*expr2;
	VMExpr		*expr3;
	VMExpr		*expr4;
	int			blob_element_type;
} VMStmtBlobElement;


/*
 * Methods for the blob element statement.
 */
static void vm_blob_element(VMStmt *thisstmt);
static void vm_blob_element_cleanup(VMStmt *thisstmt);

static VMStmtMethods s_blob_element_stmt_methods =
{
	TK_NULL,
	vm_blob_element,
	vm_blob_element_cleanup
};

/**
 * Parses a VM blob element statement
 *
 *	sphere		center, radius, field strength
 *	cylinder	base point, end point, radius, field strength
 *	plane		center, normal, radius, field strength
 *
 * @param   blob_element_type_token - int - Token identifying blob element type.
 *
 * @return   VMStmt * - Pointer to the newly created blob element statement container.
 */
VMStmt *parse_vm_blob_element(int blob_element_type_token)
{
	ParamList		params[4];
	int				nparams, i;
	VMStmtBlobElement	*newstmt =
		(VMStmtBlobElement *) vm_alloc_stmt(sizeof(VMStmtBlobElement), &s_blob_element_stmt_methods);
	if (newstmt == NULL)
	{
		logmemerror("vertex");
		return NULL;
	}

	newstmt->blob_element_type = blob_element_type_token;

	nparams = 0;
	switch (newstmt->blob_element_type)
	{
		case TK_CYLINDER: // basept, endpt, radius, field_stregth
			// Parse the cylinder element's parameters.
			//
			nparams = parse_paramlist("EEOEOE;", "blob: cylinder", params);
			break;
			break;

		case TK_PLANE: // center, normal, radius, field_stregth
			// Parse the plane element's parameters.
			//
			nparams = parse_paramlist("EEOEOE;", "blob: plane", params);
			break;

		case TK_SPHERE: // center, radius, field_stregth
			// Parse the sphere element's parameters.
			//
			nparams = parse_paramlist("EOEOE;", "blob: sphere", params);
			break;

		default:
			assert(0); // Unknown blob element type.
			break;
	}

	// Get the expressions from the parameter list.
	//
	for (i = 0; i < nparams; i++)
	{
		switch (params[i].type)
		{
			case PARAM_EXPR:
				if (newstmt->expr1 == NULL)
					newstmt->expr1 = params[i].data.expr;
				else if (newstmt->expr2 == NULL)
					newstmt->expr2 = params[i].data.expr;
				else if (newstmt->expr3 == NULL)
					newstmt->expr3 = params[i].data.expr;
				else
					newstmt->expr4 = params[i].data.expr;
				break;
		}
	}

	return (VMStmt *)newstmt;
}

/*************************************************************************/

/**
 * VM blob element statement
 *
 *  Inserts a blob element into the parent blob object.
 *
 * @param   *curstmt - VMStmt - Pointer to the blob element statement container.
 */
void vm_blob_element(VMStmt *curstmt)
{
	VMStmtBlobElement *	stmt = (VMStmtBlobElement *) curstmt;
	Vec3			center, pt2;
	double			radius, field_strength;
	Object			*curobj;

	curobj = vmstack_getcurobj();
	if (curobj == NULL)
	{
		assert(0);
		return;
	}

	radius = 1.0;
	field_strength = 1.0;
	V3Set(&center, 0.0, 0.0, 0.0);
	V3Set(&pt2, 0.0, 0.0, 1.0);

	// All elements parameters begin with a center point.
	// For the cylinder type, this is the base point.
	//
	if (stmt->expr1 != NULL)
		vm_evalvector(stmt->expr1, &center);

	switch (stmt->blob_element_type)
	{
		case TK_CYLINDER: // basept, endpt, radius, field_stregth
			// 2nd param is the end point.
			if (stmt->expr2 != NULL)
				vm_evalvector(stmt->expr2, &pt2);
			// 3rd param is the radius.
			if (stmt->expr3 != NULL)
				radius = vm_evaldouble(stmt->expr3);
			// 4th param is the field strength.
			if (stmt->expr4 != NULL)
				field_strength = vm_evaldouble(stmt->expr4);
			Ray_BlobAddCylinder(curobj, &center, &pt2, radius, field_strength);
			break;

		case TK_PLANE: // center, normal, radius, field_stregth
			// 2nd param is the normal.
			if (stmt->expr2 != NULL)
				vm_evalvector(stmt->expr2, &pt2);
			// 3rd param is the radius.
			if (stmt->expr3 != NULL)
				radius = vm_evaldouble(stmt->expr3);
			// 4th param is the field strength.
			if (stmt->expr4 != NULL)
				field_strength = vm_evaldouble(stmt->expr4);
			Ray_BlobAddPlane(curobj, &center, &pt2, radius, field_strength);
			break;

		case TK_SPHERE: // center, radius, field_stregth
			// 2nd param is the radius.
			if (stmt->expr2 != NULL)
				radius = vm_evaldouble(stmt->expr2);
			// 3rd param is the field strength.
			if (stmt->expr3 != NULL)
				field_strength = vm_evaldouble(stmt->expr3);
			Ray_BlobAddSphere(curobj, &center, radius, field_strength);
			break;

		default:
			assert(0); // Unknown blob element type.
			break;
	}
}

/**
 *	Cleanup function for VM blob element stmt.
 *
 *  Free up resources used by the parameter expression trees.
 *
 * @param   *curstmt - VMStmt - Pointer to the blob element statement container.
 */
void vm_blob_element_cleanup(VMStmt *curstmt)
{
	VMStmtBlobElement *	stmt = (VMStmtBlobElement *) curstmt;

	// Delete the expression trees.
	//
	delete_exprtree(stmt->expr1);
	stmt->expr1 = NULL;
	delete_exprtree(stmt->expr2);
	stmt->expr2 = NULL;
	delete_exprtree(stmt->expr3);
	stmt->expr3 = NULL;
	delete_exprtree(stmt->expr4);
	stmt->expr4 = NULL;
}




