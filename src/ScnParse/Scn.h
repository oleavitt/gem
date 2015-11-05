/*************************************************************************
*
*  scn.h - Common externals needed thoughout the scnparse library code.
*
*************************************************************************/

#ifndef SCN_H
#define SCN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include <stdarg.h>
#include "scnparse.h"
#include "tokens.h"
#include "mempool.h"
#include "findfile.h"
#include "lookup.h"
#include "image.h"


/*
 * findfile.c
 */
extern void FindFileInitialize(void);
extern void FindFileClose(void);


/*
 * params.c
 */
extern PARAMS *New_Param(void);


/*
 * objparse.c
 */
extern PARAMS *CompileBlobParams(void);
extern PARAMS *CompileHeightFieldParams(void);
extern PARAMS *CompileImplicitParams(void);
extern PARAMS *CompileMeshParams(void);
extern PARAMS *CompileTriangleParams(void);
extern PARAMS *CompileColoredTriangleParams(void);
extern PARAMS *CompileExtrudeParams(void);
extern Object *MakeExtrudeObject(PARAMS *par);


/*
 * parser.c
 */
extern STATEMENT *Parse_Texture_Stmt(void);
extern STATEMENT *Parse_Block(const char *block_name);


/*
 * proc.c
 */
extern void Parse_Proc_Decl(int type);
extern STATEMENT *Parse_Local_Decl(Proc *proc, int type,
	const char *block_name);
extern STATEMENT *Compile_Expr_User_Proc_Stmt(Proc *proc);
extern STATEMENT *Parse_Expr_Stmt(void);
extern STATEMENT *Parse_Proc_Stmt(int token);
extern void Proc_Add_Local(Proc *proc, LVALUE *lv);
extern Proc *New_User_Proc(void);
extern Proc *Copy_User_Proc(Proc *proc);
extern void Delete_User_Proc(Proc *proc);
extern STATEMENT *New_Statement(int token);
extern void Exec_Proc(STATEMENT *stmt);
extern void Proc_Initialize(void);
extern void Proc_Close(void);

extern int proc_nest_level;
extern Proc *cur_proc;

/*
 *  symbol.c.
 */

/* TRUE when a declaration name is expected. */
extern int local_only;

extern void Init_Symbol(void);
extern void Add_Symbol(char *name, void *data, int type);
extern SYMBOL *Fetch_Symbol(char *name);
extern void Push_Local_Symbols(void);
extern void Pop_Local_Symbols(void);
extern void Close_Symbol(void);

extern void Parse_Decl(int type);

#endif   /* SCN_H */
