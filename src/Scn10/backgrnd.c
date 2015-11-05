/*************************************************************************
*
*  backgrnd.c - Background statements.
*
*  syntax: background vexpr_color [, vexpr_color2 ];
*  syntax: background shader statement; | { statements }
*
*************************************************************************/

#include "local.h"

typedef struct tag_bgstmtdata
{
	Expr *color1;
	Expr *color2;
	Stmt *shader;
} BGStmtData;

static void ExecBackgroundStmt(Stmt *stmt);
static void DeleteBackgroundStmt(Stmt *stmt);
static void ExecSetBackgroundStmt(Stmt *stmt);
static void DeleteSetBackgroundStmt(Stmt *stmt);
static int ParseBackgroundDetails(Stmt **stmt);

StmtProcs bg_stmt_procs =
{
	TK_BACKGROUND,
	ExecBackgroundStmt,
	DeleteBackgroundStmt
};

StmtProcs setbg_stmt_procs =
{
	TK_BACKGROUND,
	ExecSetBackgroundStmt,
	DeleteSetBackgroundStmt
};

Stmt *ParseBackgroundStmt(void)
{
	int nparams, token;
	Param params[2];
	BGStmtData *sd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("background");
		return NULL;
	}
	sd = (BGStmtData *)calloc(1, sizeof(BGStmtData));
	if(sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("background");
		return NULL;
	}
	stmt->procs = &bg_stmt_procs;
	stmt->data = (void *)sd;
	sd->color1 = sd->color2 = NULL;
	sd->shader = NULL;
	nparams = ParseParams("OEOE", "background", NULL, params);
	if(nparams > 0)
	{
		sd->color1 = params[0].data.expr;
		if(nparams > 1)
			sd->color2 = params[1].data.expr;
		if((token = GetToken()) != OP_SEMICOLON)
			ErrUnknown(token, ";", "background");
	}
	else if(GetToken() == TK_SHADER)
	{
		sd->shader = ParseBlock("background", ParseBackgroundDetails);
	}
	else
	{
		LogError("background: Expecting color values or a shader. Found: %s",
			token_buffer);
		PrintFileAndLineNumber();
		UngetToken();
	}
	if(!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

int ParseBackgroundDetails(Stmt **stmt)
{
	int token;
	token = GetToken();
	*stmt = NULL;
	switch(token)
	{
		case TK_BACKGROUND:
			if((*stmt = NewStmt()) !=NULL)
			{
				(*stmt)->procs = &setbg_stmt_procs;
				if(((*stmt)->data = (void *)ExprParse()) != NULL)
				{
					if((token = GetToken()) != OP_SEMICOLON)
						ErrUnknown(token, ";", "background");
				}
				else
				{
					ErrUnknown(GetToken(), "numeric expression", "background");
					UngetToken();
				}
			}
			break;
		default:
			return token;
	}
	return TK_NULL;
}


void ExecBackgroundStmt(Stmt *stmt)
{
	BGStmtData *sd = (BGStmtData *)stmt->data;
	if(sd->color1 != NULL)
	{
		ExprEvalVector(sd->color1, &ray_setup_data->background_color1);
		if(sd->color2 != NULL)
			ExprEvalVector(sd->color2, &ray_setup_data->background_color2);
		else
			V3Copy(&ray_setup_data->background_color2,
				&ray_setup_data->background_color1);
	}
	if(sd->shader != NULL)
	{
		DeleteStatements(Ray_SetBackgroundShader(sd->shader));
		/* So that DeleteBackgroundStmt() doesn't delete shader. */
		/* Ray trace will delete it when done. */
		sd->shader = NULL;
	}
}

void ExecSetBackgroundStmt(Stmt *stmt)
{
	ExprEvalVector((Expr *)stmt->data, &ray_background_color1);
}

void DeleteBackgroundStmt(Stmt *stmt)
{
	BGStmtData *sd = (BGStmtData *)stmt->data;
	if(sd != NULL)
	{
		ExprDelete(sd->color1);
		ExprDelete(sd->color2);
		DeleteStatements(sd->shader);
		free(sd);
	}
}

void DeleteSetBackgroundStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}
