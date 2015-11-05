/**
 *****************************************************************************
 *  @file vm.h
 *  Virtual Machine types and functions.
 *  Implementation is in the scn20 library.
 *
 *****************************************************************************
 */

#ifndef __VM_H__
#define __VM_H__



/**
 * Parameter type codes.
 */
#define PARAM_TOKEN          0
#define PARAM_EXPR           1
#define PARAM_STRING         2
#define PARAM_BLOCK          3



/*
 * Data container for a single element on an expression parse tree.
 */
typedef struct tVMExpr
{
	struct tVMExpr *l, *r;
	void	(*fn)(struct tVMExpr *expr);
	void	*data;
	int		isvec;
	Vec3	v;
} VMExpr;



/**
 *	Contains a value that is bound to a named variable.
 */
typedef struct tVMLValue
{
	int		type;
	int		nrefs;
	Vec3	v;
} VMLValue;



/* Common function prototype for all statement-specific functions that
 * is called when this this statement is executed.
 */
typedef struct tVMStmt VMStmt;
typedef void (*VMStmtFn)(VMStmt *stmt);



/**
 * Parameter list item.
 */
typedef struct tParamList
{
	int type;
	union
	{
		int token;
		VMExpr *expr;
		char *str;
		VMStmt *block;
		void *data;
	} data;
} ParamList;



/**
 * Statement class information that is common to all instances
 * of this type of statement.
 */
typedef struct tVMStmtMethods
{
	int			token;
	VMStmtFn	fn;
	VMStmtFn	cleanup;
} VMStmtMethods;



/**
 * Generic container for a single statement in the virtual machine.
 */
typedef struct tVMStmt
{
	VMStmt			*next;
	VMStmtMethods	*methods;
} VMStmt;

/**
 * Generic container for an Object statement in the virtual machine.
 */
typedef struct tVMStmtObj
{
	VMStmt			vmstmt;
	// VMStmt
	VMStmt			*block;
	VMLValue		*lv_no_shadow;		// no_shadow
} VMStmtObj;

/**
 * Argument list element.
 */
typedef struct tVMArgument
{
	struct tVMArgument	*next;
	VMLValue			*lv;
	VMExpr				*expr;
} VMArgument;



/****************************************************************************
*
*	Special statements that are used as runtime shaders in
*	the rendering engine.
*
****************************************************************************/

/**
 * Generic base container for a Shader in the virtual machine.
 */
typedef struct tVMShader
{
	VMStmt			vmstmt;
	VMStmt			*block;

	// Shader may be shared in more than one place.
	// This reference counter will be incremented for every place that
	// uses it. Therefore it will not be freed from memory until this
	// ref counter decrements
	// to zero.
	//
	int				nrefs;

	VMArgument		*tmp_arglist;
	void			*tmp_data;
} VMShader;

/**
 * Surface shader statement.
 */
typedef struct tVMSurfaceShader
{
	VMShader		vmshaderstmt;

	/* Input variables - these map to runtime environment variables. */
	VMLValue		*lv_W;
	VMLValue		*lv_D;
	VMLValue		*lv_N;
	VMLValue		*lv_O;
	VMLValue		*lv_ON;
	VMLValue		*lv_u;
	VMLValue		*lv_v;

	/* Output variables - these set the lighting parameters. */
	VMLValue		*lv_color;
	VMLValue		*lv_ka;
	VMLValue		*lv_kd;
	VMLValue		*lv_ks;
	VMLValue		*lv_Phong;
	VMLValue		*lv_kr;
	VMLValue		*lv_kt;
	VMLValue		*lv_ior;
	VMLValue		*lv_outior;
} VMSurfaceShader;


/**
 * Background shader statement.
 */
typedef struct tVMBackgroundShader
{
	VMShader		vmshaderstmt;

	/* Input variables - these map to runtime environment variables. */
	VMLValue		*lv_D;

	/* Output variable - this sets the background color. */
	VMLValue		*lv_color;
} VMBackgroundShader;

// Debug helpers
//
#ifndef NDEBUG
extern int num_stmts_allocd;
#endif // NDEBUG

/* Function protos */

extern int vm_init(void);
extern int vm_reset(void);
extern int vm_close(void);
extern void vm_delete(VMStmt *stmthead);
extern void vm_evalexpr(VMExpr *expr, void *result);
extern double vm_evaldouble(VMExpr *expr);
extern void vm_evalvector(VMExpr *expr, Vec3 *vec);
extern VMStmt * vm_alloc_stmt(size_t size, VMStmtMethods *methods);
extern VMShader * vm_alloc_shader(size_t size, VMStmtMethods *methods);
extern void vm_free_stmt(VMStmt *stmt);

#endif  /* __VM_H__ */
