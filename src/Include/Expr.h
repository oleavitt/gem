/*************************************************************************
 *
 *  expr.h - Externals for the expression library.
 *
 ************************************************************************/

#ifndef EXPR_H
#define EXPR_H

#include "math3d.h"

typedef struct tag_surface Surface;
typedef struct tag_object Object;
typedef struct tag_proc Proc;

/*************************************************************************
 *    Return type codes.
 */
#define EXP_INT             100
#define EXP_FLOAT           101
#define EXP_VECTOR          102

/*************************************************************************
 *    Parameter type codes.
 */
#define PARAM_TOKEN          0
#define PARAM_EXPR           1
#define PARAM_STRING         2
#define PARAM_BLOCK          3

typedef struct tag_exprtree Expr;
typedef struct tag_statement Stmt;
typedef struct tag_param Param;

/*
 * Data container for a single element on an expression parse tree.
 */
typedef struct tag_exprtree
{
	Expr *l, *r;
	void (*fn)(Expr *expr);
	void *data;
	int isvec;
	Vec3 v;
} Expr;

/*
 * Parameter array item.
 */
typedef struct tag_param
{
	int type;
	union
	{
		int token;
		Expr *expr;
		char *str;
		Stmt *block;
		void *data;
	} data;
} Param;

/*
 * Statement-specific function pointers.
 */
typedef struct tag_stmtprocs
{
	int token;
	void (*Exec)(Stmt *stmt);
	void (*Delete)(Stmt *stmt);
} StmtProcs;

/*
 * Data container for a single program statement.
 */
typedef struct tag_statement
{
	Stmt *next;
	StmtProcs *procs;
	void *data;
	int int_data;
	int break_count;
	short continue_flag;
	short return_flag;
} Stmt;

/*************************************************************************
 * Built-in variables.
 */
extern Vec3 rt_D;  /* Direction of current ray. */
extern Vec3 rt_O;  /* Point hit in object coordinates. */
extern Vec3 rt_W;  /* Point hit in world coordinates. */
extern Vec3 rt_ON; /* Surface normal in object coordinates. */
extern Vec3 rt_WN; /* Surface normal in world coordinates. */
extern double rt_u;       /* U part of object UV coordinates. */
extern double rt_v;       /* V part of object UV coordinates. */
extern double rt_uscreen; /* U part of screen UV coordinates. */
extern double rt_vscreen; /* V part of screen UV coordinates. */
extern Surface *rt_surface; /* Current surface being processed. */


/*************************************************************************
 * Function protos.
 */
extern void ExecStatements(Stmt *stmts);
extern void DeleteStatements(Stmt *stmts);

extern Expr *ExprParse(void);
extern int ExprEval(Expr *expr, void *result);
extern double ExprEvalDouble(Expr *expr);
extern void ExprEvalVector(Expr *expr, Vec3 *vec);
extern void ExprDelete(Expr *expr);

#endif    /* EXPR_H */
