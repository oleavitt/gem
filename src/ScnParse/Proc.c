/*************************************************************************
 *
 *  proc.c - Functions for managing and executing STATEMENTs.
 *
 ************************************************************************/

#include "scn.h"

#define PROC_NEST_MAX 100

int proc_nest_level;
Proc *cur_proc = NULL;

static Vec3 ret_result;
static int ret_flag = 0, ret_type;

static jmp_buf execute_program_jmpbuf;

/*************************************************************************
 *
 *      Local variable stack operations.
 *
 ************************************************************************/
/*
 * Stack for local variables.
 */
static LVALUE **local_lv_stack = NULL, **local_lv_stack_ptr = NULL;
static int nlocals;

static void Push_Local_Stack(LVALUE *lv)
{
  if(*local_lv_stack_ptr == NULL)
 	{
		LVALUE **new_local_stack, **src, **dest;

	  nlocals++;
  	new_local_stack = (LVALUE **)mmalloc(sizeof(LVALUE *) * (nlocals + 1));
  	src = local_lv_stack;
  	dest = new_local_stack;
	  while(*src != NULL)
      *dest++ = *src++;
  	*dest++ = New_Lvalue(TK_FLOAT);
  	*dest = NULL;
  	mfree(local_lv_stack, sizeof(LVALUE *) * nlocals);
	  local_lv_stack = new_local_stack;
    local_lv_stack_ptr = local_lv_stack + (nlocals - 1);
  }
  (*local_lv_stack_ptr)->V = lv->V;
/*  (*local_lv_stack_ptr)->type = lv->type; */
  local_lv_stack_ptr++;
}


static void Pop_Local_Stack(LVALUE *lv)
{
  local_lv_stack_ptr--;
  lv->V = (*local_lv_stack_ptr)->V;
/*  lv->type = (*local_lv_stack_ptr)->type; */
}


/*************************************************************************
 *
 *  Exec_Expr() - Evaluate an expression. Vector "ret_result" is a plug
 *  since expr is evaluated for the "side effect" of performing an
 *  assignment.
 *
 ************************************************************************/
static void Exec_Expr(STATEMENT *stmt)
{
	(void)Eval_Expr(&ret_result, stmt->expr);
}


/*************************************************************************
 *
 *  Exec_Break() - Break from the current iteration loop(s).
 *  Optional expression specifies number of loops to break from.
 *  If no expression is given, the default number of loops is 1.
 *
 ************************************************************************/
static void Exec_Break(STATEMENT *stmt)
{
	if(stmt->expr != NULL)
	{
		stmt->break_flag = (int)FEval_Expr(stmt->expr);
		if(stmt->break_flag < 0) stmt->break_flag = 0;
	}
	else
		stmt->break_flag = 1;
}


/*************************************************************************
 *
 *  Exec_Next() - Recycle the current iteration loop.
 *
 ************************************************************************/
static void Exec_Next(STATEMENT *stmt)
{
	stmt->next_flag = 1;
}


/*************************************************************************
 *
 *  Exec_Return() - Return form current procedure. Evaluate optional
 *    return expression, if any, and set "ret_type" appropriately.
 *
 ************************************************************************/
static void Exec_Return(STATEMENT *stmt)
{
	if(stmt->expr != NULL)
	{
		ret_type = Eval_Expr(&ret_result, stmt->expr);
	}
	else
	{
		V3Set(&ret_result, 0.0, 0.0, 0.0);
		ret_type = 0;
	}
	ret_flag = 1;
} /* end of Exec_Return() */


/*************************************************************************
 *
 *  Exec_Repeat() - Iterate a repeat loop.
 *
 ************************************************************************/
static void Exec_Repeat(STATEMENT *stmt)
{
	STATEMENT *s;
	unsigned int i;
	static double ftmp;

	ftmp = FEval_Expr(stmt->expr);
  stmt->break_flag = 0;
	for(i = (ftmp > 0.0) ? (unsigned int)ftmp : 0L; i != 0L; i--)
	{
		for(s = stmt->block; s != NULL; s = s->next)
		{
			(s->exec)(s);
			if(ret_flag)
				return;
			if(s->break_flag)
			{
				stmt->break_flag = s->break_flag - 1;
				s->break_flag = 0;
				return;             /* process "break" */
			}
			if(s->next_flag)
			{
				s->next_flag = 0;
				break;              /* process "next" */
			}
		}
	}
}


/*************************************************************************
*
*  Exec_Do() - Iterate a do-while loop.
*
*************************************************************************/
static void Exec_Do(STATEMENT *stmt)
{
	STATEMENT *s;

  stmt->break_flag = 0;
	do
	{
		for(s = stmt->block; s != NULL; s = s->next)
		{
			(s->exec)(s);
			if(ret_flag)
				return;
			if(s->break_flag)
			{
				stmt->break_flag = s->break_flag - 1;
				s->break_flag = 0;
				return;             /* process "break" */
			}
			if(s->next_flag)
			{
				s->next_flag = 0;
				break;              /* process "next" */
			}
		}
	}
	while(FEval_Expr(stmt->expr));
}


/*************************************************************************
*
*  Exec_While() - Iterate a while loop.
*
*************************************************************************/
static void Exec_While(STATEMENT *stmt)
{
	STATEMENT *s;

  stmt->break_flag = 0;
	while(FEval_Expr(stmt->expr))
	{
		for(s = stmt->block; NULL != s; s = s->next)
		{
			(s->exec)(s);
			if(ret_flag)
				return;
			if(s->break_flag)
			{
				stmt->break_flag = s->break_flag - 1;
				s->break_flag = 0;
				return;             /* process "break" */
			}
			if(s->next_flag)
			{
	  		s->next_flag = 0;
  			break;              /* process "next" */
			}
		}
	}
}


/*************************************************************************
 *
 *  Exec_If() - Evaluate an if...else if...else statement.
 *
 ************************************************************************/
static void Exec_If(STATEMENT *stmt)
{
  STATEMENT *s, *st;

	 /*
		* If these are remaining else if's or else's from a previous
		* if statement, just skip on...
		*/
	if(stmt->token == TK_ELSE)
		return;

	st = stmt;
	do
	{
    st->break_flag = 0;
	  if(st->expr == NULL || FEval_Expr(st->expr))
 		{
	    for(s = st->block; s != NULL; s = s->next)
			{
				(s->exec)(s);
				if(ret_flag || s->break_flag || s->next_flag)
				{
					stmt->break_flag = s->break_flag;
					stmt->next_flag = s->next_flag;
					s->break_flag = 0;
					s->next_flag = 0;
					break;    /* process "break" and "next" */
				}
			}
			break;   /* from do loop */
   	}
    st = st->next;
  }
  while(st != NULL && st->token == TK_ELSE);
}


/*************************************************************************
 *
 *  Exec_Proc() - Execute a parameterized pre-compiled procedure.
 *
 *************************************************************************/
void Exec_Proc(STATEMENT *stmt)
{
	STATEMENT *s;
	Proc *proc;
	LVALUE *lv, **lvp;
	PARAMS *p, *pp;
  int i;

  stmt->break_flag = 0;

  if(proc_nest_level >= PROC_NEST_MAX)
  {
    SCN_Message(SCN_MSG_WARNING,
      "Maximum recursion depth for Exec_Proc() reached!");
    longjmp(execute_program_jmpbuf, 1);
  }
  proc_nest_level++;
	proc = stmt->data.proc;

  /* Push data in local vars on to stack. */
  for(lvp = proc->locals; *lvp != NULL; lvp++)
  	Push_Local_Stack(*lvp);

	/* Evaluate parameters, if any... */
	for(pp = proc->params, p = stmt->params; pp != NULL; pp = pp->next)
	{
		if(p != NULL)
		{
			if(pp->type == TK_VECTOR)
				VEval_Expr(&pp->V, p->expr);
			else
				pp->V.x = FEval_Expr(p->expr);
			p = p->next;
		}
		else     /* Use default value. */
			pp->V = pp->defval;
	}

	/* Copy parameter results, if any, to local l-values... */
  lvp = proc->locals;
	for(pp = proc->params; NULL != pp; pp = pp->next)
	{
		lv = *lvp++;
		assert(lv != NULL);
		lv->V = pp->V;
	}

	/* Execute statements in proc's body. */
	ret_type = 0;
	for(s = proc->block; s != NULL; s = s->next)
	{
		(s->exec)(s);
		if(ret_flag)  /* "return" statement was processed */
			break;
	}

	proc->ret_type = ret_type;
	if(ret_type == 0)
		V3Set(&proc->ret_result, 0.0, 0.0, 0.0);
	else
		proc->ret_result = ret_result;
	ret_flag = 0;

  /* Pop previous data from stack back to local vars. */
  for(i = proc->nlocals - 1, lvp = proc->locals + i; i >= 0; lvp--, i--)
  	Pop_Local_Stack(*lvp);
  proc_nest_level--;
} /* end of Exec_Proc() */


/*************************************************************************
 *
 *  Execute_Program() - Execute a list of statements.
 *
 ************************************************************************/
void Execute_Program(STATEMENT *stmts)
{
  STATEMENT *s;
  jmp_buf old_jmpbuf;

  /* Save current jmp_buf, just in case this is a re-entrant call. */
  *old_jmpbuf = *execute_program_jmpbuf;

  /* Set up a safe place to jump back to in case of trouble. */
  if( ! setjmp(execute_program_jmpbuf))
  {
    /* Execute the statements... */
    proc_nest_level = 0;
  	for(s = stmts; s != NULL; s = s->next)
	  {
		  (s->exec)(s);
  		if(ret_flag || s->break_flag)
	  		break;      /* process "return" and "break" */
	  }
  }

  *execute_program_jmpbuf = *old_jmpbuf;

} /* end of Execute_Program() */


/*************************************************************************
 *
 *  Parse_Proc_Decl() - The "proc" declaration keyword has just
 *  been parsed. Create Proc structure and add name to symbol table.
 *  Get parameter LVALUES and add them to proc's local symbol table.
 *  Get the block of text that makes up the procedure body and save
 *  a pointer to it in the Proc struct.
 *
 ************************************************************************/
void Parse_Proc_Decl(int type)
{
	Proc *proc;

	if(Get_Token() != TK_STRING)
		Err_Unknown(type, "identifier", NULL);

	/* Allocate and initialize a Proc struct... */
	proc = New_User_Proc();

	/* Enter proc's name in symbol table. */
	Add_Symbol(token_buffer, proc, DECL_PROC);

	cur_proc = proc;
	Push_Local_Symbols();
	proc_nest_level++;

	/* Get parameter declarations... */
	Compile_Proc_Params_Decl(proc);

	/* Get body... */
	proc->block = Parse_Block("proc");

	Pop_Local_Symbols();
	proc_nest_level--;
	cur_proc = NULL;

} /* end of Parse_Proc_Decl() */


STATEMENT *Compile_Expr_User_Proc_Stmt(Proc *proc)
{
	STATEMENT *s = New_Statement(DECL_PROC);
	s->exec = Exec_Proc;
	s->data.proc = Copy_User_Proc(proc);
	s->params = Compile_Proc_Params(proc->params);
  return s;
} /* end of Compile_Expr_User_Proc_Stmt() */


/*************************************************************************
 *
 *  Parse_Expr_Stmt() - Parse an assignment expression and return it
 *  as a statement structure.
 *
 ************************************************************************/
STATEMENT *Parse_Expr_Stmt(void)
{
 	STATEMENT *s = New_Statement(TK_PROC_EXPR);
  s->exec = Exec_Expr;
	Unget_Token();
	if((s->expr = Compile_Expr()) == NULL)
 	{
    Delete_Statement(s);
    s = NULL;
  }
  return s;
}


/*************************************************************************
 *
 * Parse_Local_Decl() - Process user defined types and declarations
 *   that are local to the body of a user-defined procedure.
 *   Assumes that a non-keyword identifier name has just been parsed
 *   and the global, "token_buffer", contains the name. If
 *   initialization expressions are processed, a STATEMENT list will
 *   be constructed and returned.
 *
 ************************************************************************/
STATEMENT *Parse_Local_Decl(Proc *proc, int type, const char *block_name)
{
	int token;
  STATEMENT *stmt = NULL, *s = NULL;
  EXPR *expr;
  static char name[MAX_TOKEN_BUF_SIZE];

  assert(proc != NULL);

  local_only = 1;
	if((token = Get_Token()) != TK_STRING)
		Err_Unknown(token, "identifier", block_name);
  local_only = 0;

  if(scn_error_cnt)
  	return NULL;

	strcpy(name, token_buffer);
	switch(type)
	{
		case TK_FLOAT:
		case TK_VECTOR:
		{
			LVALUE *lv;

			while(1)
			{
				/* Initialize l-value data structure... */
				lv = New_Lvalue(type);

				/* If there is an initializer expr? */
        if((expr = Compile_Lvalue_Expr(lv)) != NULL)
       	{
          if(s != NULL)
         	{
            s->next = New_Statement(TK_PROC_EXPR);
            s = s->next;
          }
          else
	          s = stmt = New_Statement(TK_PROC_EXPR);
				  s->exec = Exec_Expr;
          s->expr = expr;
				}

				/* Add new l-value to dictionary and proc... */
				Add_Symbol(name, (void *)lv, DECL_LVALUE);
        Proc_Add_Local(proc, lv);

				if(Get_Token() != OP_COMMA)
       	{
          Unget_Token();
					break;  /* Done, break from loop. */
        }

				/*
				 * Gotta comma. Get next name, and recycle...
				 */
        local_only = 1;
				if((token = Get_Token()) != TK_STRING)
					Err_Unknown(token, "identifier", block_name);
        local_only = 0;
				strcpy(name, token_buffer);
			} /* end of comma loop */
		}
		  break;

		default:
			SCN_Message(SCN_MSG_ERROR, "%s: Invalid declaration type.", block_name);
			break;
	} /* end of "type" switch */

  return stmt;
} /* end of Parse_Local_Decl() */


/*************************************************************************
 *
 *  Parse_Proc_Stmt() - See if "token" is the start of a procedural
 *  statement, and, if so, build the statement and return a pointer
 *  to it, otherwise, return NULL.
 *
 ************************************************************************/
STATEMENT *Parse_Proc_Stmt(int token)

{
  STATEMENT *stmt, *st;

  if((cur_token->flags & TKFLAG_PROC) == 0)
  	return NULL;

  stmt = New_Statement(token);

  switch(token)
	{
		case TK_BREAK:
			stmt->exec = Exec_Break;
      stmt->expr = Compile_Expr();
			break;

		case TK_DO:
			stmt->exec = Exec_Do;
			stmt->block = Parse_Block("do");
			Expect(TK_WHILE, "while", "do");
      if((stmt->expr = Compile_Expr()) == NULL)
				SCN_Message(SCN_MSG_ERROR,
          "do: Numeric expression expected after `while'. Found `%s'.",
				token_buffer);
			break;

    case TK_IF:
      stmt->exec = Exec_If;
      if((stmt->expr = Compile_Expr()) == NULL)
				SCN_Message(SCN_MSG_ERROR, "if: Numeric expression expected. Found `%s'.",
				token_buffer);
		  stmt->block = Parse_Block("if");
			st = stmt;
			while(1)
			{
				if((token = Get_Token()) == TK_ELSE)
				{
					st->next = New_Statement(TK_ELSE);
					st = st->next;
					st->exec = Exec_If;
					if(Get_Token() == TK_IF)
					{
			      if((st->expr = Compile_Expr()) == NULL)
           	{
							SCN_Message(SCN_MSG_ERROR, "else if: Numeric expression expected. Found `%s'.",
							token_buffer);
              break;
            }
						st->block = Parse_Block("else if");
					}
					else
					{
						Unget_Token();
						st->block = Parse_Block("else");
						break;   /* from loop */
					}
				}
				else if(token == TK_ELIF)
				{
					st->next = New_Statement(TK_ELSE);
					st = st->next;
					st->exec = Exec_If;
		      if((st->expr = Compile_Expr()) == NULL)
         	{
						SCN_Message(SCN_MSG_ERROR, "elif: Numeric expression expected. Found `%s'.",
						token_buffer);
            break;
          }
					st->block = Parse_Block("elif");
				}
				else
				{
					Unget_Token();
					break;   /* from loop */
				}
			}
    	break;

		case TK_NEXT:
			stmt->exec = Exec_Next;
			break;

		case TK_REPEAT:
			stmt->exec = Exec_Repeat;
      if((stmt->expr = Compile_Expr()) == NULL)
				SCN_Message(SCN_MSG_ERROR,
          "repeat: Numeric expression expected. Found `%s'.",
				token_buffer);
		  stmt->block = Parse_Block("repeat");
			break;

		case TK_RETURN:
			stmt->exec = Exec_Return;
      stmt->expr = Compile_Expr();
			break;

		case TK_WHILE:
			stmt->exec = Exec_While;
      if((stmt->expr = Compile_Expr()) == NULL)
				SCN_Message(SCN_MSG_ERROR,
          "while: Numeric expression expected. Found `%s'.",
				token_buffer);
			stmt->block = Parse_Block("while");
			break;

		case DECL_PROC:
			stmt->exec = Exec_Proc;
      stmt->data.proc = Copy_User_Proc((Proc *)cur_token->data);
			stmt->params = Compile_Proc_Params(stmt->data.proc->params);
			break;

    default:
      SCN_Message(SCN_MSG_ERROR, "Unknown statement type in Parse_Proc_Stmt()");
    	break;
  }

  if(scn_error_cnt)
	{
  	Delete_Statement(stmt);
    stmt = NULL;
  }

  return stmt;
}


void Proc_Add_Local(Proc *proc, LVALUE *lv)
{
	LVALUE **new_locals, **src, **dest;

  proc->nlocals++;
  new_locals = (LVALUE **)mmalloc(sizeof(LVALUE *) * (proc->nlocals + 1));
  src = proc->locals;
  dest = new_locals;
  while(*src != NULL)
    *dest++ = *src++;
  *dest++ = Copy_Lvalue(lv);
  *dest = NULL;
  mfree(proc->locals, sizeof(LVALUE *) * proc->nlocals);
  proc->locals = new_locals;
}


/*************************************************************************
 *
 *  Initialization and cleanup functions.
 *
 ************************************************************************/

Proc *New_User_Proc(void)
{
  Proc *proc = (Proc *)mmalloc(sizeof(Proc));
  memset(proc, 0, sizeof(Proc));
	proc->params = NULL;
	proc->block = NULL;
  /* Locals array is terminated with a NULL ptr. */
	proc->locals = (LVALUE **)mmalloc(sizeof(LVALUE *));
	*proc->locals = NULL;
	proc->nlocals = 0;
  proc->nrefs = 1;
  return proc;
}

Proc *Copy_User_Proc(Proc *proc)
{
  if(proc != NULL)
  	proc->nrefs++;
  return proc;
}

void Delete_User_Proc(Proc *proc)
{
  if((proc != NULL) && (--proc->nrefs == 0))
 	{
    STATEMENT *stmp;
    LVALUE **lvp;

    Delete_Params(proc->params);
    while(proc->block != NULL)
   	{
      stmp = proc->block;
      proc->block = stmp->next;
      Delete_Statement(stmp);
    }
    for(lvp = proc->locals; *lvp != NULL; lvp++)
      Delete_Lvalue(*lvp);
    mfree(proc->locals, sizeof(LVALUE *) * (proc->nlocals + 1));
    mfree(proc, sizeof(Proc));
  }
}

STATEMENT *New_Statement(int token)
{
  STATEMENT *s = (STATEMENT *)mmalloc(sizeof(STATEMENT));
  memset(s, 0, sizeof(STATEMENT));
  s->token = token;
  s->next = NULL;
  s->block = NULL;
  s->expr = NULL;
  s->params = NULL;
  s->decl.object = NULL;
  s->data.misc = NULL;
	s->nrefs = 1;
  return s;
}


STATEMENT *Copy_Statement(STATEMENT *stmt)
{
  if(stmt != NULL)
	  stmt->nrefs++;
	return stmt;
}


void Delete_Statement(STATEMENT *stmt)
{
  STATEMENT *s, *t;

  if(stmt != NULL && --stmt->nrefs == 0)
 	{
    switch(stmt->token)
    {
      case DECL_PROC:
    	  Delete_User_Proc(stmt->data.proc);
        break;
      case TK_STATEMENT:   /* STATEMENT containers for run-time Procs. */
			  s = stmt->data.stmt;
		    while(s != NULL)
   			{
		      t = s;
		      s = s->next;
					Delete_Statement(t);
		    }
        break;
    }
    s = stmt->block;
    while(s != NULL)
   	{
      t = s;
      s = s->next;
    	Delete_Statement(t);
    }
    Delete_Expr(stmt->expr);
    Delete_Params(stmt->params);
    mfree(stmt, sizeof(STATEMENT));
  }
}


void Init_Proc_Stack(void)
{
  Close_Proc_Stack();
  local_lv_stack = local_lv_stack_ptr =
  	(LVALUE **)mmalloc(sizeof(LVALUE *));
  *local_lv_stack = NULL;
  nlocals = 0;
}


void Close_Proc_Stack(void)
{
  if(local_lv_stack != NULL)
 	{
	  for(local_lv_stack_ptr = local_lv_stack;
  	  *local_lv_stack_ptr != NULL;
    	local_lv_stack_ptr++)
	    Delete_Lvalue(*local_lv_stack_ptr);
  	mfree(local_lv_stack, sizeof(LVALUE *) * (nlocals + 1));
	  local_lv_stack = local_lv_stack_ptr = NULL;
  }
  local_lv_stack = local_lv_stack_ptr = NULL;
}


void Proc_Initialize(void)
{
  Init_Proc_Stack();
}


void Proc_Close(void)
{
  Close_Proc_Stack();
}

