/*************************************************************************
*
*  loop.c - Looping statements - while, do-while, for, repeat.
*
*************************************************************************/

#include "local.h"

typedef struct tag_whiledata
{
	Expr *expr;
	Stmt *block;
} WhileData;

typedef struct tag_fordata
{
	Expr *initexpr;
	Expr *condexpr;
	Expr *increxpr;
	Stmt *block;
} ForData;

static void ExecWhileStmt(Stmt *stmt);
static void ExecDoWhileStmt(Stmt *stmt);
static void ExecForStmt(Stmt *stmt);
static void ExecRepeatStmt(Stmt *stmt);
static void DeleteWhileStmt(Stmt *stmt);
static void DeleteForStmt(Stmt *stmt);

StmtProcs while_stmt_procs =
{
	TK_WHILE,
	ExecWhileStmt,
	DeleteWhileStmt
};

StmtProcs dowhile_stmt_procs =
{
	TK_DO,
	ExecDoWhileStmt,
	DeleteWhileStmt
};

StmtProcs for_stmt_procs =
{
	TK_FOR,
	ExecForStmt,
	DeleteForStmt
};

StmtProcs repeat_stmt_procs =
{
	TK_REPEAT,
	ExecRepeatStmt,
	DeleteWhileStmt
};

Stmt *ParseWhileStmt(int (*ParseDetails)(Stmt **stmt))
{
	Stmt *stmt;
	WhileData *wd;
	if((stmt = NewStmt()) != NULL)
	{
		if((wd = (WhileData *)calloc(1, sizeof(WhileData))) != NULL)
		{
			if((wd->expr = ExprParse()) != NULL)
			{
				wd->block = ParseBlock("while", ParseDetails);
				stmt->procs = &while_stmt_procs;
				stmt->data = (void *)wd;
				return stmt;
			}
			free(wd);
		}
		else
			LogMemError("while");
	}
	else
		LogMemError("while");
	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseDoWhileStmt(int (*ParseDetails)(Stmt **stmt))
{
	int token;
	Stmt *stmt;
	WhileData *wd;
	if((stmt = NewStmt()) != NULL)
	{
		if((wd = (WhileData *)calloc(1, sizeof(WhileData))) != NULL)
		{
			wd->block = ParseBlock("do-while", ParseDetails);
			stmt->procs = &dowhile_stmt_procs;
			stmt->data = (void *)wd;
			if((token = GetToken()) == TK_WHILE)
			{
				if((wd->expr = ExprParse()) != NULL)
				{
					if((token = GetToken()) == OP_SEMICOLON)
						return stmt;
					else
						ErrUnknown(token, ";", "do-while");
				}
			}
			else
				LogError("do-while: Expecting `while(expr)' after do { ... } block.");
		}
		else
			LogMemError("do-while");
	}
	else
		LogMemError("do-while");
	PrintFileAndLineNumber();
	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseForStmt(int (*ParseDetails)(Stmt **stmt))
{
	Stmt *stmt;
	ForData *fd;
	int token;
	if((stmt = NewStmt()) != NULL)
	{
		if((fd = (ForData *)calloc(1, sizeof(ForData))) != NULL)
		{
			stmt->data = (void *)fd;
			stmt->procs = &for_stmt_procs;
			if((token = GetToken()) == OP_LPAREN)
			{
				fd->initexpr = ExprParse();
				if((token = GetToken()) == OP_SEMICOLON)
				{
					fd->condexpr = ExprParse();
					if((token = GetToken()) == OP_SEMICOLON)
					{
						fd->increxpr = ExprParse();
						if((token = GetToken()) == OP_RPAREN)
						{
							fd->block = ParseBlock("for", ParseDetails);
							return stmt;
						}
						else
							ErrUnknown(token, ")", "for");
					}
					else
						ErrUnknown(token, ";", "for");
				}
				else
					ErrUnknown(token, ";", "for");
			}
			else
				ErrUnknown(token, "(", "for");
		}
		else
			LogMemError("for");
	}
	else
		LogMemError("for");
	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseRepeatStmt(int (*ParseDetails)(Stmt **stmt))
{
	Stmt *stmt;
	WhileData *wd;
	if((stmt = NewStmt()) != NULL)
	{
		if((wd = (WhileData *)calloc(1, sizeof(WhileData))) != NULL)
		{
			stmt->procs = &repeat_stmt_procs;
			stmt->data = (void *)wd;
			if((wd->expr = ExprParse()) != NULL)
			{
				wd->block = ParseBlock("repeat", ParseDetails);
				return stmt;
			}
		}
		else
			LogMemError("repeat");
	}
	else
		LogMemError("repeat");
	DeleteStmt(stmt);
	return NULL;
}

void ExecWhileStmt(Stmt *stmt)
{
	WhileData *wd = (WhileData *)stmt->data;
	Stmt *s;
	while(fabs(ExprEvalDouble(wd->expr)) > EPSILON)
	{
		/* TODO: Put system cooperation call here. */
		for(s = wd->block; s != NULL; s = s->next)
		{
			s->procs->Exec(s);
			if(s->break_count)
			{
				stmt->break_count = s->break_count - 1;
				s->break_count = 0;
				return;
			}
			if(s->continue_flag)
			{
				s->continue_flag = 0;
				break;
			}
		}
	}
}

void ExecDoWhileStmt(Stmt *stmt)
{
	WhileData *wd = (WhileData *)stmt->data;
	Stmt *s;
	do
	{
		/* TODO: Put system cooperation call here. */
		for(s = wd->block; s != NULL; s = s->next)
		{
			s->procs->Exec(s);
			if(s->break_count)
			{
				stmt->break_count = s->break_count - 1;
				s->break_count = 0;
				return;
			}
			if(s->continue_flag)
			{
				s->continue_flag = 0;
				break;
			}
		}
	}
	while(fabs(ExprEvalDouble(wd->expr)) > EPSILON);
}

void ExecForStmt(Stmt *stmt)
{
	ForData *fd = (ForData *)stmt->data;
	Stmt *s;
	for(ExprEvalDouble(fd->initexpr);
	    fabs(ExprEvalDouble(fd->condexpr)) > EPSILON;
	    ExprEvalDouble(fd->increxpr))
	{
		/* TODO: Put system cooperation call here. */
		for(s = fd->block; s != NULL; s = s->next)
		{
			s->procs->Exec(s);
			if(s->break_count)
			{
				stmt->break_count = s->break_count - 1;
				s->break_count = 0;
				return;
			}
			if(s->continue_flag)
			{
				s->continue_flag = 0;
				break;
			}
		}
	}
}

void ExecRepeatStmt(Stmt *stmt)
{
	WhileData *wd = (WhileData *)stmt->data;
	Stmt *s;
	int count = (int)ExprEvalDouble(wd->expr);
	while(count-- > 0)
	{
		/* TODO: Put system cooperation call here. */
		for(s = wd->block; s != NULL; s = s->next)
		{
			s->break_count = 0;
			s->procs->Exec(s);
			if(s->break_count)
			{
				stmt->break_count = s->break_count - 1;
				return;
			}
			if(s->continue_flag)
			{
				s->continue_flag = 0;
				break;
			}
		}
	}
}

void DeleteWhileStmt(Stmt *stmt)
{
	WhileData *wd = (WhileData *)stmt->data;
	ExprDelete(wd->expr);
	DeleteStatements(wd->block);
	free(wd);
}

void DeleteForStmt(Stmt *stmt)
{
	ForData *fd = (ForData *)stmt->data;
	ExprDelete(fd->initexpr);
	ExprDelete(fd->condexpr);
	ExprDelete(fd->increxpr);
	DeleteStatements(fd->block);
	free(fd);
}
