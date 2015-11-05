/*************************************************************************
 *
 *   params.c - This module handles the parsing of parameters as
 *   specified by a format string.
 *
 ************************************************************************/

#include "scn.h"


PARAMS *New_Param(void)
	{
	PARAMS *p = (PARAMS *)mmalloc(sizeof(PARAMS));
	memset(p, 0, sizeof(PARAMS));
	p->next = NULL;
	return p;
	} /* end of New_Param() */


/*************************************************************************
 *
 *  Eval_Params() - Evaluate a pre-compiled list of parameters.
 *
 ************************************************************************/
void Eval_Params(PARAMS *p)
	{
	for(; p != NULL; p = p->next)
		{
		if(p->type == TK_VECTOR)
			{
			if(p->expr != NULL)
				VEval_Expr(&p->V, p->expr);
			else
				p->V = p->defval;
			}
		else
			{
			if(p->expr != NULL)
				p->V.x = FEval_Expr(p->expr);
			else
				p->V.x = p->defval.x;
			}
		if(! p->more)
			break;
		}
	} /* end of Eval_Params() */


/*************************************************************************
 *
 *  Compile_Params() - Get parameters from input stream as specified by
 *  a "format" string. Params are compiled into a list of compiled
 *  expressions. Returns pointer to parameter list.
 *  Format codes:
 *   F = Get expression and flag it as a float type.
 *   V = Get expression and flag it as a vector type.
 *   R = Get a run-time expression and flag it with the TK_FN token.
 *   , = Expect a comma and complain if not found.
 *   L = Next parameter is a comma-separated list of a type, keep
 *      getting parameters of the same type as long there's
 *      a comma ahead.
 *   O = Next parameter is optional. If no parameter is found, terminate
 *      parameter list but don't emit an error.
 *
 ************************************************************************/
PARAMS *Compile_Params(const char *format)
	{
	int opt, list, c, token, nparams, i;
	PARAMS *p, *plist, *par;
	EXPR *expr;

	assert(format != NULL);

	opt = 0;
	list = 0;
	plist = NULL;
	par = NULL;
	nparams = 0;

	while((c = *format++) != ASCII_NUL)
		{
		switch(c)
			{
			case ASCII_L:
				list = 1;
				continue;
			case ASCII_O:
				opt = 1;
				continue;
			case ASCII_COMMA:
				if((token = Get_Token()) != OP_COMMA)
					{
					if(opt == 0)
						{
						SCN_Message(SCN_MSG_ERROR, "Syntax: `,' expected. Found `%s'.",
							token_buffer);
						}
					else
						{
						Unget_Token();
						goto done;
						}
					}
				opt = 0;
				break;
			case ASCII_F:
			case ASCII_V:
				do
					{
					if((expr = Compile_Expr()) != NULL)
						{
						p = New_Param();
            p->type = (c == ASCII_V) ? TK_VECTOR : TK_FLOAT;
						if(plist == NULL)
							plist = p;
						else
							par->next = p;
						par = p;
						nparams++;

						p->expr = expr;
						if(list)
							{
							token = Get_Token();
							if(token != OP_COMMA)
								{
								Unget_Token();
								list = 0;
								}
							}
						}
					else if(opt == 0)
						{
						SCN_Message(SCN_MSG_ERROR, "Syntax: Numeric expression expected. Found `%s'.",
							token_buffer);
            list = 0;
						}
					else list = 0;  /* Reached end of list. */
					}
				while(list); /* Keep getting params if the list flag is set... */
				break;
			case ASCII_R:
				if((expr = Compile_Expr()) != NULL)
					{
					p = New_Param();
					p->type = TK_RT_EXPR;
					p->expr = expr;
					if(plist == NULL)
						plist = p;
					else
						par->next = p;
					par = p;
					nparams++;
					}
				else if(opt == 0)
					{
					SCN_Message(SCN_MSG_ERROR, "Syntax: Run-time expression expected. Found `%s'.",
						token_buffer);
					}
				break;
			default:
				SCN_Message(SCN_MSG_ERROR, "Unknown character switched in Compile_Params() "__FILE__);
				break;
			}
		}

	done:
	for(par = plist, i = nparams - 1; i >= 0; par = par->next, i--)
		par->more = i;

	return plist;
	} /* end of Compile_Params() */


/*************************************************************************
 *
 *  Compile_Proc_Params() - Compile a list of parameters for a user-
 *    defined procedure. Returns parameter list or NULL if just an empty
 *    pair of parenthesis was processed.
 *
 *************************************************************************/
PARAMS *Compile_Proc_Params(PARAMS *proc_params)
	{
	int token, extras;
	PARAMS *plist, *par, *p, *pp;
	EXPR *expr;

	plist = par = NULL;
	extras = 0;
	token = TK_NULL;
	pp = proc_params;
	Expect(OP_LPAREN, "(", "proc");
	while(1)
		{
		if((expr = Compile_Expr()) != NULL)
			{
			if(pp != NULL)
				{
				p = New_Param();
				p->type = TK_RT_EXPR;
				p->expr = expr;
				if(plist == NULL)
					plist = p;
				else
					par->next = p;
				par = p;
				pp = pp->next;
				}
			else
				extras++;
			if((token = Get_Token()) != OP_COMMA)
				{
				Unget_Token();
				break; /* from loop */
				}
			}
		else
			{
			if(token == OP_COMMA)
				SCN_Message(SCN_MSG_ERROR, "Syntax: Expression expected. Found `%s'.",
					token_buffer);
			break; /* from loop */
			}
		}

	if(extras)
		SCN_Message(SCN_MSG_WARNING, "%d extra parameter%signored.",
    	extras, (extras > 1) ? "s " : " " );

	Expect(OP_RPAREN, ")", "proc");

	return plist;
	} /* end of Compile_Proc_Params() */


/*************************************************************************
 *
 *  Compile_Proc_Params_Decl() - Compile a list of parameters for a
 *    user-defined procedure declaration. Adds parameter list, or
 *    NULL if just an empty pair of parenthesis was processed, to
 *    proc's "params" field. Parameter l-values are also added to
 *    proc's "locals" list.
 *
 ************************************************************************/
void Compile_Proc_Params_Decl(Proc *proc)
	{
	int token, type;
	PARAMS *par, *p, *params_list;
	LVALUE *lv;

	proc->params = par = NULL;
	Expect(OP_LPAREN, "(", "proc");
	while((token = Get_Token()) != OP_RPAREN)
		{
		switch(token)
			{
			case OP_COMMA:
				break;
			case TK_FLOAT:
			case TK_VECTOR:
				type = token;
				local_only = 1;
				if(Get_Token() == TK_STRING)
					{
					local_only = 0;
					p = New_Param();
					if(proc->params == NULL)
						proc->params = p;
					else
						par->next = p;
					par = p;
          lv = New_Lvalue(type);
 					Add_Symbol(token_buffer, lv, DECL_LVALUE);
					p->data.lvalue = lv;
					p->type = type;
					Proc_Add_Local(proc, lv);
					if((token = Get_Token()) == OP_ASSIGN)
						{
						if(type == TK_FLOAT)
							{
							if((params_list = Compile_Params("F")) != NULL)
              	{
                Eval_Params(params_list);
								p->defval.x = params_list->V.x;
                }
							}
						else if((params_list = Compile_Params("V")) != NULL)
             	{
              Eval_Params(params_list);
							p->defval = params_list->V;
              }
            Delete_Params(params_list);
						}
					else
						Unget_Token();
					break;
					}
        local_only = 0;
			/* Else, fall through and report error. */
			default:
				Err_Unknown(token, NULL, "proc");
				break;
			}
		}
	} /* end of Compile_Proc_Params_Decl() */


/*************************************************************************
 *
 *  Delete_Params() - Delete a parameter list.
 *
 ************************************************************************/
void Delete_Params(PARAMS *params)
	{
	PARAMS *p;

	while(params != NULL)
		{
		p = params;
		params = params->next;
		switch(p->type)
			{
			case TK_QUOTESTRING:
			case TK_STRING:
				str_free(p->data.name);
        break;
      case HFIELD_IMAGE:
        Delete_Image((Image *)p->data.data);
        break;
			}
    Delete_Expr(p->expr);
		mfree(p, sizeof(PARAMS));
		}
	} /* end of Delete_Params() */


