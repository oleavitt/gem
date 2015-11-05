/*************************************************************************
*
*  csg.c - CSG statements.
*
*  syntax: object { object & modifier stmts }
*  syntax: union { object & modifier stmts }
*  syntax: difference { object & modifier stmts }
*  syntax: intersection { object & modifier stmts }
*  syntax: clip { object & modifier stmts }
*
*************************************************************************/

#include "local.h"

static void ExecCSGStmt(Stmt *stmt);
static void DeleteCSGStmt(Stmt *stmt);

StmtProcs csg_stmt_procs =
{
	0,
	ExecCSGStmt,
	DeleteCSGStmt
};

Stmt *ParseCSGStmt(int token)
{
	int prev_object_token;

	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError(token_buffer);
		return NULL;
	}
	stmt->procs = &csg_stmt_procs;
	stmt->int_data = (token == TK_CLIP) ? OBJ_CSGCLIP :
		(token == TK_DIFFERENCE) ? OBJ_CSGDIFFERENCE :
		(token == TK_INTERSECTION) ? OBJ_CSGINTERSECTION :
		(token == TK_UNION) ? OBJ_CSGUNION :
		OBJ_CSGGROUP;
	prev_object_token = cur_object_token;
	cur_object_token = token;
	stmt->data = (void *)ParseBlock("csg", ParseObjectDetails);
	cur_object_token = prev_object_token;
	if(!error_count && (stmt->data != NULL))
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

void ExecCSGStmt(Stmt *stmt)
{
	Object *obj, *prev_bound_objects;

	if((obj = Ray_MakeCSG(stmt->int_data)) != NULL)
	{
		obj->surface = Ray_ShareSurface(default_surface);
		ScnBuild_PushObjStack();
		objstack_ptr->curobj = obj;
		prev_bound_objects = bound_objects;
		bound_objects = NULL;
		ExecBlock(stmt, (Stmt *)stmt->data);
		if(Ray_FinishCSG(obj, objstack_ptr->objlist, bound_objects))
		{
			ScnBuild_PopObjStack();
			ScnBuild_AddObject(obj);
		}
		else
		{
			Ray_DeleteObject(obj);
			ScnBuild_PopObjStack();
		}
		bound_objects = prev_bound_objects;
	}
}

void DeleteCSGStmt(Stmt *stmt)
{
	DeleteStatements((Stmt *)stmt->data);
}
