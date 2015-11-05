/*************************************************************************
 *
 *  symbol.c - This module handles the symbol table details.
 *
 ************************************************************************/

#include "scn.h"

/* TRUE when a declaration name is expected. */
int local_only;

struct symstk
  {
  struct symstk *next, *prev;
  SYMBOL *sym;
  };

#define SYM_SIZE sizeof(SYMBOL)
#define SYM_STK_SIZE sizeof(struct symstk)

#define SYM_NAME_SIZE  MAX_TOKEN_BUF_SIZE

/* Non-zero while inside a procedure block. */
static int local_level;


/*
 * Master list of all symbol structs allocated.
 */
static SYMBOL *all_symbols;

/*
 * The symbol table tree stack.
 */
static struct symstk *sym_stk, *sym_stk_top;

/*
 * Receives a ptr to a newly entered symbol each
 * time Enter() is called.
 */
static SYMBOL *new_sym;

static char name[SYM_NAME_SIZE];

/*************************************************************************
 *
 * Parse_Decl() - Process user defined types and declarations.
 *   Assumes that a non-keyword identifier name has just been parsed
 *   and the global, "token_buffer", contains the name.
 *
 ************************************************************************/
void Parse_Decl(int type)
  {
  int token;

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

        /* Is there an initializer expr? */
        if(Get_Token() == OP_ASSIGN)
          {
          PARAMS *par;
          if(type == TK_FLOAT)
            {
            if((par = Compile_Params("F")) != NULL)
              {
              Eval_Params(par);
              lv->V.x = par->V.x;
              Delete_Params(par);
              }
            }
          else
            {
            if((par = Compile_Params("V")) != NULL)
              {
              Eval_Params(par);
              lv->V = par->V;
              Delete_Params(par);
              }
            }
          }
        else
          Unget_Token();

        /* Add new l-value to dictionary... */
        Add_Symbol(name, (void *)lv, DECL_LVALUE);

        if(Get_Token() != OP_COMMA)
          {
          Unget_Token();
          break;  /* Done, break from loop. */
          }

        /*
         * Gotta comma. Get next name, and recycle...
         */
        if((token = Get_Token()) != TK_STRING)
          Err_Unknown(token, "identifier", NULL);
        strcpy(name, token_buffer);
        } /* end of comma loop */
      }
      break;

    case TK_TEXTURE:
      {
      STATEMENT *s;

      if((s = Parse_Texture_Stmt()) != NULL)
        {
        STATEMENT *stmp;
        Execute_Program(s);
        Add_Symbol(name, s->data.surface, DECL_TEXTURE);
        while(s != NULL)
          {
          stmp = s;
          s = s->next;
          Delete_Statement(stmp);
          }
        }
      }
      break;

    default:
      SCN_Message(SCN_MSG_ERROR, "Invalid declaration type.");
      break;
    } /* end of "type" switch */

  } /* end of Parse_Decl() */


static SYMBOL *New_Symbol(char *name)
  {
  SYMBOL *sym;

  /*
   * Scan through symbol struct pool for a free slot...
   * sym->level < 0 means free for re-use.
   */
  for(sym = all_symbols; sym != NULL; sym = sym->pnext)
    if(sym->level < 0)
      break;

  if(sym == NULL)
    {
    sym = (SYMBOL *)mmalloc(SYM_SIZE);
    sym->name = mmalloc(sizeof(char)*SYM_NAME_SIZE);
    sym->pnext = all_symbols;   /* NULL if first */
    sym->data = NULL;
    sym->type = TK_NULL;
    all_symbols = sym;
    }
  strcpy(sym->name, name);
  sym->l = NULL;
  sym->r = NULL;
  sym->next = NULL;
  sym->level = local_level;
  return sym;
  }

static void Enter(SYMBOL **node, char *name)
  {
  if(*node == NULL)
    {
    new_sym = New_Symbol(name);
    *node = new_sym;
    }
  else
    {
    int result;

    result = strcmp((*node)->name, name);

    if(result > 0)
      Enter(&(*node)->l, name);
    else if(result < 0)
      Enter(&(*node)->r, name);
    else  /* Name exists, stack new entry atop existing one. */
      {
      new_sym = New_Symbol(name);
      new_sym->next = (*node);
      new_sym->l = (*node)->l;
      new_sym->r = (*node)->r;
      *node = new_sym;
      }
    }
  }

static SYMBOL *Lookup(SYMBOL *root, char *name)
  {
  if(root != NULL)
    {
    int result;

    result = strcmp(root->name, name);

    if(result > 0)
      return Lookup(root->l, name);
    if(result < 0)
      return Lookup(root->r, name);
    }
  return root;
  }


void Add_Symbol(char *name, void *data, int type)
  {
  Enter(&sym_stk_top->sym, name);
  new_sym->type = type;
  new_sym->data = data;
  }

SYMBOL *Fetch_Symbol(char *name)
  {
  struct symstk *stk;

  for(stk = sym_stk_top; stk != NULL; stk = stk->prev)
    {
    if((new_sym = Lookup(stk->sym, name)) != NULL)
      break;
    if(local_only)  /* Just checking local symbols. */
      break;
    }
  return new_sym;
  }


void Push_Local_Symbols(void)
  {
  local_level++;
  if(sym_stk_top->next == NULL)
    {
    sym_stk_top->next = (struct symstk *)mmalloc(SYM_STK_SIZE);
    sym_stk_top->next->prev = sym_stk_top;
    sym_stk_top = sym_stk_top->next;
    sym_stk_top->next = NULL;
    }
  else
    sym_stk_top = sym_stk_top->next;
  sym_stk_top->sym = NULL;
  } /* end of Push_Local_Symbols() */

void Pop_Local_Symbols(void)
  {
  SYMBOL *sym;

  assert(local_level > 0);

  local_level--;
  sym_stk_top = sym_stk_top->prev;

  /*
   * Free up local symbols for re-use.
   */
  for(sym = all_symbols; sym != NULL; sym = sym->pnext)
    if(sym->level > local_level)
      {
      if(sym->type == DECL_LVALUE)
        Delete_Lvalue((LVALUE *)sym->data);
      sym->data = NULL;
      sym->type = TK_NULL;
      sym->level = -1;
      }

  } /* end of Pop_Local_Symbols() */


void Init_Symbol(void)
  {
  sym_stk = (struct symstk *)mmalloc(SYM_STK_SIZE);
  sym_stk->prev = NULL;
  sym_stk->next = NULL;
  sym_stk->sym = NULL;
  sym_stk_top = sym_stk;
  all_symbols = NULL;
  local_level = 0;
  local_only  = 0;
  }  /* end of Init_Symbol() */

void Close_Symbol(void)
{
  while(sym_stk != NULL)
  {
    sym_stk_top = sym_stk;
    sym_stk = sym_stk->next;
    mfree(sym_stk_top, SYM_STK_SIZE);
  }

  while(all_symbols != NULL)
  {
    new_sym = all_symbols;
    all_symbols = all_symbols->pnext;
    switch(new_sym->type)
    {
      case DECL_LVALUE:
        new_sym->data = Delete_Lvalue((LVALUE *)new_sym->data);
        break;
      case DECL_OBJECT:
        Ray_DeleteObject((Object *)new_sym->data);
        break;
      case DECL_PROC:
        Delete_User_Proc((Proc *)new_sym->data);
        break;
      case DECL_TEXTURE:
        Ray_DeleteSurface((Surface *)new_sym->data);
        break;
    }
    mfree(new_sym->name, sizeof(char)*SYM_NAME_SIZE);
    mfree(new_sym, SYM_SIZE);
  }
}  /* end of Close_Symbol() */

