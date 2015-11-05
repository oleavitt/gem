/*************************************************************************
*
*  object.c - Parses stuff specific to the object statement block.
*
*************************************************************************/

#include "local.h"

/* Global bounding object list for GSGs and other composite objects. */
Object *bound_objects = NULL;

typedef struct tag_declobjectstmtdata
{
	Object *declobj;
	Stmt *block;
} DeclObjectStmtData;

static void ExecTransformStmt(Stmt *stmt);
static void DeleteTransformStmt(Stmt *stmt);
static void ExecObjFlagStmt(Stmt *stmt);
static void DeleteObjFlagStmt(Stmt *stmt);
static void ExecDeclObjectStmt(Stmt *stmt);
static void DeleteDeclObjectStmt(Stmt *stmt);
static Stmt *ParseBoundObjectStmt(void);
static void ExecBoundObjectStmt(Stmt *stmt);
static void DeleteBoundObjectStmt(Stmt *stmt);

int cur_object_token = 0;

StmtProcs transform_stmt_procs =
{
	0,
	ExecTransformStmt,
	DeleteTransformStmt
};

StmtProcs objflag_stmt_procs =
{
	0,
	ExecObjFlagStmt,
	DeleteObjFlagStmt
};

StmtProcs declobject_stmt_procs =
{
	0,
	ExecDeclObjectStmt,
	DeleteDeclObjectStmt
};

StmtProcs boundobject_stmt_procs =
{
	0,
	ExecBoundObjectStmt,
	DeleteBoundObjectStmt
};

int ParseObjectDetails(Stmt **stmt)
{
	int token;
	token = GetToken();
	*stmt = NULL;
	switch (token)
	{
		/* transform statements */
		case TK_ROTATE:
		case TK_SCALE:
		case TK_TRANSLATE:
			if ((*stmt = NewStmt()) !=NULL)
			{
				(*stmt)->procs = &transform_stmt_procs;
				(*stmt)->int_data = 
					(token == TK_ROTATE) ? XFORM_ROTATE :
					(token == TK_SCALE) ? XFORM_SCALE :
					XFORM_TRANSLATE;
				if (((*stmt)->data = (void *)ExprParse()) != NULL)
					if ((token = GetToken()) != OP_SEMICOLON)
						ErrUnknown(token, ";", "transform");
			}
			break;
		
		/* flag setting statments */
		case TK_INVERSE:
		case TK_NO_SHADOW:
		case TK_TWO_SIDES:
			if ((*stmt = NewStmt()) !=NULL)
			{
				(*stmt)->procs = &objflag_stmt_procs;
				(*stmt)->int_data = 
					(token == TK_NO_SHADOW) ? OBJ_FLAG_NO_SHADOW :
					(token == TK_INVERSE) ? OBJ_FLAG_INVERSE :
					OBJ_FLAG_TWO_SIDES;
				if ((token = GetToken()) != OP_SEMICOLON)
					ErrUnknown(token, ";", "object");
			}
			break;

		default:
			/* Look for any object type specific statements. */
			switch (cur_object_token)
			{
				case TK_OBJECT:
				case TK_DIFFERENCE:
				case TK_INTERSECTION:
				case TK_UNION:
				case TK_CLIP:
					switch (token)
					{
						case TK_BOUND:
							*stmt = ParseBoundObjectStmt();
							return TK_NULL;
					}
					break;
				case TK_BLOB:
					switch (token)
					{
						case TK_SPHERE:
							*stmt = ParseBlobSphereStmt();
							return TK_NULL;
						case TK_CYLINDER:
							*stmt = ParseBlobCylinderStmt();
							return TK_NULL;
						case TK_PLANE:
							*stmt = ParseBlobPlaneStmt();
							return TK_NULL;
					}
					break;
				case TK_POLYGON:
					switch (token)
					{
						case TK_VERTEX:
							*stmt = ParsePolygonVertexStmt();
							return TK_NULL;
					}
					break;
				case TK_NURBS:
					switch (token)
					{
						case TK_POINT:
							*stmt = ParseNurbsPointStmt();
							return TK_NULL;
					}
					break;
				case TK_POLYMESH:
					switch (token)
					{
						case TK_VERTEX:
							*stmt = ParsePolymeshVertexStmt();
							return TK_NULL;
						case TK_EXTRUDE:
							*stmt = ParsePolymeshExtrudeStmt();
							return TK_NULL;
						case TK_PATH:
						// TODO: case TK_SMOOTH_PATH:
							*stmt = ParsePolymeshPathStmt(0);
							return TK_NULL;
					}
					break;
			}
			return token;
	}
	return TK_NULL;
}


void ExecTransformStmt(Stmt *stmt)
{
	Vec3 V;
	ExprEvalVector((Expr *)stmt->data, &V);
	if (objstack_ptr->curobj != NULL)
		Ray_Transform_Object(objstack_ptr->curobj, &V, stmt->int_data);
}

void DeleteTransformStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}


void ExecObjFlagStmt(Stmt *stmt)
{
	if (objstack_ptr->curobj != NULL)
		objstack_ptr->curobj->flags |= stmt->int_data;
}

void DeleteObjFlagStmt(Stmt *stmt)
{
	/* Not used: */
	stmt;
}


Stmt *ParseDeclObjectStmt(Object *declobj)
{
	int nparams, prev_object_token;
	Param params[1];
	DeclObjectStmtData *sd;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("user-defined object");
		return NULL;
	}
	sd = (DeclObjectStmtData *)calloc(1, sizeof(DeclObjectStmtData));
	if (sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("user-defined object");
		return NULL;
	}
	stmt->procs = &declobject_stmt_procs;
	stmt->data = (void *)sd;
	sd->declobj = declobj;
	sd->block = NULL;
	prev_object_token = cur_object_token;
	cur_object_token = DECL_OBJECT;
	nparams = ParseParams("OB", "user-defined object",
		ParseObjectDetails, params);
	cur_object_token = prev_object_token;
	if (nparams)
		sd->block = params[0].data.block;
	if (!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

void ExecDeclObjectStmt(Stmt *stmt)
{
	DeclObjectStmtData *sd = (DeclObjectStmtData *)stmt->data;
	Object *obj, *oldobj;
	if ((obj = Ray_CloneObject(sd->declobj)) != NULL)
	{
		ScnBuild_AddObject(obj);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, sd->block);
		objstack_ptr->curobj = oldobj;
	}
}

void DeleteDeclObjectStmt(Stmt *stmt)
{
	DeclObjectStmtData *sd = (DeclObjectStmtData *)stmt->data;
	if (sd != NULL)
	{
		/* Object is deleted when symbol table closes. */
		DeleteStatements(sd->block);
		free(sd);
	}
}


Stmt *ParseBoundObjectStmt(void)
{
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError(token_buffer);
		return NULL;
	}
	stmt->procs = &boundobject_stmt_procs;
	stmt->data = (void *)ParseBlock("bound", NULL);
	if (!error_count && (stmt->data != NULL))
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

void ExecBoundObjectStmt(Stmt *stmt)
{
	ScnBuild_PushObjStack();
	ExecBlock(stmt, (Stmt *)stmt->data);
	bound_objects = objstack_ptr->objlist;
	ScnBuild_PopObjStack();
}

void DeleteBoundObjectStmt(Stmt *stmt)
{
	DeleteStatements((Stmt *)stmt->data);
}
