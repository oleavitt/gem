/**
 *****************************************************************************
 *  @file local.h
 *   Common stuff needed by all modules within the Scn20 library.
 *
 *****************************************************************************
 */

#ifndef __LOCAL_H__
#define __LOCAL_H__

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include "raytrace.h"
#include "image.h"
#include "scn20.h"

/*
 * Token codes.
 */
enum
{
	TK_EOF = -1,
	TK_NULL = 0,

	TK_AMBIENT,
	TK_ANAGLYPH,
	TK_AT,
	TK_BACKGROUND,
	TK_BACKGROUND_SHADER,
	TK_BLOB,
	TK_BOUND,
	TK_BOX,
	TK_BREAK,
	TK_BUMP,
	TK_CAUSTICS,
	TK_CENTER,
	TK_CLIP,
	TK_CLOSED_CONE,
	TK_CLOSED_CYLINDER,
	TK_COLOR,
	TK_CONE,
	TK_CONTINUE,
	TK_CYLINDER,
	TK_DEFINE,
	TK_DIFFERENCE,
	TK_DIFFUSE,
	TK_DIR,
	TK_DISC,
	TK_DO,
	TK_ELSE,
	TK_EXTRUDE,
	TK_FALLOFF,
	TK_FLOAT,
	TK_FN_XYZ,
	TK_FOR,
	TK_FUNCTION,
	TK_GOTO,
	TK_HEIGHT_FIELD,
	TK_IF,
	TK_INCLUDE_FILE_PATHS,
	TK_INFINITE_LIGHT,
	TK_INTERSECTION,
	TK_INVERSE,
	TK_IOR,
	TK_JITTER,
	TK_LIGHT,
	TK_LOAD_COLOR_MAP,
	TK_LOCATION,
	TK_MAIN,
	TK_MESSAGE,
	TK_NO_SHADOW,
	TK_NO_SPECULAR,
	TK_NPOLYGON,
	TK_NURBS,
	TK_OBJECT,
	TK_OUTIOR,
	TK_PATH,
	TK_PHONG,
	TK_PLANE,
	TK_POINT,
	TK_POLYGON,
	TK_POLYMESH,
	TK_PRIVATE,
	TK_PUBLIC,
	TK_RADIUS,
	TK_REFLECTION,
	TK_REL_STEP,
	TK_REPEAT,
	TK_RETURN,
	TK_ROT_STEP,
	TK_ROTATE,
	TK_SCALE,
	TK_SIZE,
	TK_SMOOTH_HEIGHT_FIELD,
	TK_SMOOTH_POLYMESH,
	TK_SPECULAR,
	TK_SPHERE,
	TK_SPOT,
	TK_SURFACE,
	TK_SURFACE_SHADER,
	TK_TORUS,
	TK_TRANSLATE,
	TK_TRANSMISSION,
	TK_TRIANGLE,
	TK_TURN,
	TK_TWO_SIDES,
	TK_UNION,
	TK_VECTOR,
	TK_VERTEX,
	TK_VIEWPORT,
	TK_VISIBILITY,
	TK_WHILE,

	// Renderer environment variables. - To be replaced by get/set functions
	RT_RT_BEGIN,
	RT_D,
	RT_N,
	RT_O,
	RT_ON,
	RT_U,
	RT_USCREEN,
	RT_V,
	RT_VSCREEN,
	RT_X,
	RT_Y,
	RT_Z,
	RT_RT_END,

	// Functions that access the renderer environment.
/*	FN_GET_COLOR,
	FN_GET_OBJ_PT,
	FN_GET_OBJ_U,
	FN_GET_OBJ_V,
	FN_GET_RAY_DIR,
	FN_GET_RAY_DIST,
	FN_GET_RAY_ORG,
	FN_GET_WORLD_PT,
	FN_SET_COLOR, */

	// Standard functions
	FN_ABS,
	FN_ACOS,
	FN_ASIN,
	FN_ATAN,
	FN_ATAN2,
	FN_BUMP,
	FN_CEIL,
	FN_CHECKER,
	FN_CHECKER2,
	FN_CLAMP,
	FN_COLOR_MAP,
	FN_COS,
	FN_COSH,
	FN_CYLINDER_MAP,
	FN_EXP,
	FN_FLOOR,
	FN_FRAND,
	FN_HEXAGON,
	FN_HEXAGON2,
	FN_IMAGE_MAP,
	FN_INT,
	FN_IRAND,
	FN_LEGENDRE,
	FN_LERP,
	FN_LOG,
	FN_LOG10,
	FN_NOISE,
	FN_ROUND,
	FN_SIN,
	FN_SINH,
	FN_SMOOTH_IMAGE_MAP,
	FN_SQRT,
	FN_TAN,
	FN_TANH,
	FN_TURB,
	FN_TURB2,
	FN_VCROSS,
	FN_VDOT,
	FN_VLERP,
	FN_VMAG,
	FN_VNOISE,
	FN_VNORM,
	FN_VRAND,
	FN_VROTATE,
	FN_VTURB,
	FN_VTURB2,
	FN_WRINKLE,

	// Built-in constants.
	CV_INT_CONST,
	CV_FLOAT_CONST,
	CV_VECTOR_CONST,
	CV_PI_CONST,

	// Operators
	OP_AND,
	OP_ANDAND,
	OP_ASSIGN,
	OP_COLON,
	OP_COMMA,
	OP_DECREMENT,
	OP_DIVASSIGN,
	OP_DIVIDE,
	OP_DOT,
	OP_EQUAL,
	OP_GREATEQUAL,
	OP_GREATERTHAN,
	OP_INCREMENT,
	OP_LESSEQUAL,
	OP_LESSTHAN,
	OP_LPAREN,
	OP_LSQUARE,
	OP_MINUS,
	OP_MINUSASSIGN,
	OP_MOD,
	OP_MULT,
	OP_MULTASSIGN,
	OP_NOT,
	OP_NOTEQUAL,
	OP_OR,
	OP_OROR,
	OP_POW,
	OP_PLUS,
	OP_PLUSASSIGN,
	OP_QUESTION,
	OP_RPAREN,
	OP_RSQUARE,
	OP_SEMICOLON,

	// Block delimiters
	TK_LEFTBRACE,
	TK_RIGHTBRACE,

	// Strings
	TK_UNKNOWN_ID,
	TK_UNKNOWN_CHAR,
	TK_QUOTESTRING,

	// Declaration types
	DECL_FLOAT,
	DECL_VECTOR,
	DECL_COLOR_MAP,
	DECL_OBJECT,
	DECL_SURFACE,
	DECL_FUNCTION,

	THE_END,

	NUM_TOKEN_CODES
};

/*
 * Token flags.
 */
#define TKFLAG_OBJECT		1
#define TKFLAG_SHADER_FN	2

/*
 * Token info structure.
 */
typedef struct tTOKEN
{
	char	*name;			/* Keyword associated with token. */
	int		token, flags;	/* Type code and flags. */
	void	*data;			/* Data associated with token. */
} TOKEN;

/* Maximum token buffer size. */
#define MAX_TOKEN_LEN   256

/*
 * File state info structure.
 */
typedef struct tFILEDATA
{
	char	name[FILENAME_MAX];
	int		line_num;
	FILE	*fp;
} FILEDATA;



/**
 *	Stores user defined variables.
 */
typedef struct tag_symboltable
{
	TOKEN **symtab;
	int symcount;
	int symtabmax;
	int stuff_added;
} SymbolTable;



/*
 * define.c
 */
extern int parse_define(void);



/*
 * error.c
 */
extern MSGFN Scn20Message;
extern int g_warning_count;
extern int g_error_count;
extern int g_fatal_count;
extern int error_initialize(void);
extern void error_close(void);
extern void logmsg(const char *fmt, ...);
extern void logmsg0(const char *msg);
extern void logwarn(const char *fmt, ...);
extern void logerror(const char *fmt, ...);
extern void logfatal(const char *fmt, ...);
extern void logmemerror(const char *who);



/*
 * file.c
 */
extern int file_Init(void);
extern void file_Close(void);
extern FILEDATA *file_OpenMain(const char *fname);
extern FILEDATA *file_OpenInclude(const char *fname);
extern FILEDATA *file_CloseInclude(void);
extern void file_PrintFileAndLineNumber(void);



/*
 * gettoken.c
 */
extern int gettoken_Init(const char *fname);
extern void gettoken_Close(void);
extern int gettoken(void);
extern int gettoken_GetNewIdentifier(void);
extern void gettoken_Unget(void);
extern int gettoken_Expect(int expected_token, const char *token_name);
extern void gettoken_ErrUnknown(int token, const char *expected);
extern void gettoken_Error(const char *who, const char *what);



/*
 * global.c
 */
extern int g_scn20error;
extern int g_compile_mode;
extern int g_define_mode;
extern int g_function_mode;
extern char g_define_name[];
extern RaySetupData * g_rsd;
extern char g_token_buffer[];
extern TOKEN *g_cur_token;



/*
 * noise.c
 */
extern void Noise_Initialize(long seed);
extern double Noise3D(Vec3 *pt);
extern void VNoise3D(Vec3 *pt, Vec3 *noise_vec);
extern double Turb3D(Vec3 *pt, int octaves,
	double freq_factor, double amp_factor);
extern void VTurb3D(Vec3 *pt, int octaves, double freq_factor,
	double amp_factor, Vec3 *turb_vec);
extern void Wrinkles3D(Vec3 *N, Vec3 *P, int oct);
extern int Hexagon2D(double u, double v);
extern double Legendre(int l, int m, double x);



/*
 * param.c
 */
extern int parse_paramlist(const char *format, const char *forwhom,
	ParamList *paramlist);



/*
 * parse.c
 */
extern void parse(RaySetupData * rsd);
extern void parse_main(void);
extern VMStmt * parse_vm_block(void);
extern int parse_vm_vm_token(int token, VMStmt **stmtlist, int no_declare);
extern int parse_vm_object_token(int token, VMStmt **stmtlist);
extern int parse_vm_global_token(int token, VMStmt **stmtlist);
extern int parse_vm_decl(int token, VMStmt **stmtlist, int no_declare);
extern VMStmt * parse_vm_float_decl(void);
extern VMStmt * parse_vm_vector_decl(void);



/*
 * pcontext.c
 */
extern int pcontext_init(void);
extern void pcontext_close(void);
extern int pcontext_push(const char *block_name);
extern int pcontext_pop(void);
extern char * pcontext_getname(void);
extern int pcontext_getobjtype(void);
extern void pcontext_setobjtype(int objtoken);
extern int pcontext_addsymbol(const char *name,
	int token, int level, void *data);
extern TOKEN *pcontext_findsymbol(const char *name, int local_only);
extern VMLValue **pcontext_build_lvalue_list(int *lv_list_len);

/* Shortcut macro for our name. */
#define ME	pcontext_getname()

	

/*
 * pexpr.c
 */
extern double parse_fexpr(void);
extern void parse_vexpr(Vec3 * vresult);
extern VMExpr *parse_exprtree(void);
extern VMExpr *parse_vm_lvalue_expr(VMLValue *lv);



/*
 * shadebg.c
 */
extern VMBackgroundShader * parse_vm_background_shader(void);



/*
 * shadesrf.c
 */
extern VMSurfaceShader * parse_vm_surface_shader(void);
extern int parse_vm_surface_shader_token(int token, VMStmt **stmtlist);



/*
 * symtab.c
 */
extern void symtab_init(SymbolTable *pst);
extern void symtab_close(SymbolTable *pst);
extern int symtab_add(SymbolTable *pst, const char *name, int token, int flags, void *data);
extern TOKEN *symtab_find(SymbolTable *pst, const char *name);
extern void symtab_deletetoken(TOKEN *s);
extern VMLValue **symtab_build_lvalue_list(SymbolTable *pst, int *lv_list_len);



/*
 * vm.c
 */



/*
 * vmbkgrnd.c
 */
extern VMStmt *parse_vm_background(void);
extern int parse_vm_background_token(int token, VMStmt **stmtlist);


/*
 * vmblob.c
 */
extern VMStmt *parse_vm_blob(void);
extern VMStmt *parse_vm_blob_element(int blob_element_type_token);



/*
 * vmcsg.c
 */
extern VMStmt *parse_vm_csg(int csg_type_token);



/*
 * vmexpr.c
 */
extern int vmexpr_init(void);
extern void vmeval_assign(VMExpr *expr);
extern void vmeval_const(VMExpr *expr);
extern void vmeval_lvalue(VMExpr *expr);
extern void vmeval_vector(VMExpr *expr);
extern void vmeval_comma(VMExpr *expr);
extern void vmeval_uminus(VMExpr *expr);
extern void vmeval_dot_x(VMExpr *expr);
extern void vmeval_dot_y(VMExpr *expr);
extern void vmeval_dot_z(VMExpr *expr);
extern void vmeval_bitand(VMExpr *expr);
extern void vmeval_bitor(VMExpr *expr);
extern void vmeval_logicnot(VMExpr *expr);
extern void vmeval_logicand(VMExpr *expr);
extern void vmeval_logicor(VMExpr *expr);
extern void vmeval_lessthan(VMExpr *expr);
extern void vmeval_greaterthan(VMExpr *expr);
extern void vmeval_lessequal(VMExpr *expr);
extern void vmeval_greatequal(VMExpr *expr);
extern void vmeval_isequal(VMExpr *expr);
extern void vmeval_isnotequal(VMExpr *expr);
extern void vmeval_plus(VMExpr *expr);
extern void vmeval_minus(VMExpr *expr);
extern void vmeval_multiply(VMExpr *expr);
extern void vmeval_divide(VMExpr *expr);
extern void vmeval_mod(VMExpr *expr);
extern void vmeval_ternary(VMExpr *expr);
extern void vmeval_pow(VMExpr *expr);

extern void vmeval_rtfloat(VMExpr *expr);
extern void vmeval_rtvec(VMExpr *expr);

extern void vmeval_abs(VMExpr *expr);
extern void vmeval_acos(VMExpr *expr);
extern void vmeval_asin(VMExpr *expr);
extern void vmeval_atan(VMExpr *expr);
extern void vmeval_atan2(VMExpr *expr);
extern void vmeval_bump(VMExpr *expr);
extern void vmeval_ceil(VMExpr *expr);
extern void vmeval_checker(VMExpr *expr);
extern void vmeval_checker2(VMExpr *expr);
extern void vmeval_clamp(VMExpr *expr);
extern void vmeval_color_map(VMExpr *expr);
extern void vmeval_cos(VMExpr *expr);
extern void vmeval_cosh(VMExpr *expr);
extern void vmeval_cylinder_map(VMExpr *expr);
extern void vmeval_exp(VMExpr *expr);
extern void vmeval_floor(VMExpr *expr);
extern void vmeval_frand(VMExpr *expr);
//extern void vmeval_get_color(VMExpr *expr);
extern void vmeval_hexagon(VMExpr *expr);
extern void vmeval_hexagon2(VMExpr *expr);
extern void vmeval_image_map(VMExpr *expr);
extern void vmeval_int(VMExpr *expr);
extern void vmeval_irand(VMExpr *expr);
extern void vmeval_legendre(VMExpr *expr);
extern void vmeval_lerp(VMExpr *expr);
extern void vmeval_log(VMExpr *expr);
extern void vmeval_log10(VMExpr *expr);
extern void vmeval_noise(VMExpr *expr);
extern void vmeval_round(VMExpr *expr);
//extern void vmeval_set_color(VMExpr *expr);
extern void vmeval_sin(VMExpr *expr);
extern void vmeval_sinh(VMExpr *expr);
extern void vmeval_smooth_image_map(VMExpr *expr);
extern void vmeval_sqrt(VMExpr *expr);
extern void vmeval_tan(VMExpr *expr);
extern void vmeval_tanh(VMExpr *expr);
extern void vmeval_turb(VMExpr *expr);
extern void vmeval_turb2(VMExpr *expr);
extern void vmeval_vcross(VMExpr *expr);
extern void vmeval_vdot(VMExpr *expr);
extern void vmeval_vlerp(VMExpr *expr);
extern void vmeval_vmag(VMExpr *expr);
extern void vmeval_vnoise(VMExpr *expr);
extern void vmeval_vnorm(VMExpr *expr);
extern void vmeval_vrand(VMExpr *expr);
extern void vmeval_vrotate(VMExpr *expr);
extern void vmeval_vturb(VMExpr *expr);
extern void vmeval_vturb2(VMExpr *expr);
extern void vmeval_wrinkle(VMExpr *expr);



/*
 * vmfnxyz.c
 */
extern VMStmt *parse_vm_fnxyz(void);



/*
 * vmfunc.c
 */
extern VMStmt *parse_vm_function(int function_type_token, const char *function_name);
extern VMStmt *parse_vm_function_call(VMStmt *stmtfunc);
extern VMArgument *parse_vm_argument_declarations(void);
extern void vm_delete_arglist(VMArgument *arglist);
extern void vm_function_add_to_statelist(int type_token, void *lv);
/*extern void vm_function_addref(VMStmt *stmtfunc);*/



/*
 * vmlight.c
 */
extern VMStmt *parse_vm_light(int light_type_token);



/*
 * vmlval.c
 */
extern VMStmt *parse_vm_lvalue_init(VMLValue *lv);
extern VMStmt *parse_vm_expr(void);
extern VMLValue *vm_new_lvalue(int type_token);
extern void vm_delete_lvalue(VMLValue *lv);
extern VMLValue *vm_copy_lvalue(VMLValue *lv);

 
 
/*
 * vmobject.c
 */
extern void vm_begin_object(VMStmtObj *stmt);
extern void vm_execute_object_block(VMStmtObj *stmtobj);
extern int vm_finish_object(VMStmtObj *stmt, Object *obj, int success);
extern void vm_object_cleanup(VMStmt *stmt);
extern VMStmt *parse_vm_sphere(void);
extern VMStmt *parse_vm_disc(void);
extern VMStmt *parse_vm_box(void);
extern VMStmt *parse_vm_torus(void);
extern VMStmt *parse_vm_cone_or_cylinder(int token);
extern int parse_vm_object_modifier_token(int token, VMStmt **stmtlist);
extern VMStmtObj *begin_parse_object(
	size_t			size,
	const char		*name,
	int				token,
	VMStmtMethods	*methods);
extern VMStmtObj *finish_parse_object(
	VMStmtObj *stmtobj);

/*
 * vmpoly.c
 */
extern VMStmt *parse_vm_polygon(int polygon_type_token);
extern VMStmt *parse_vm_polygon_vertex(void);

/*
 * vmproc.c
 */
extern VMStmt * parse_vm_repeat(void);
extern VMStmt * parse_vm_while(void);
extern VMStmt * parse_vm_for(void);
extern VMStmt * parse_vm_if(void);
extern VMStmt * parse_vm_break(void);



/*
 * vmstack.c
 */
extern int vmstack_init(void);
extern void vmstack_close(void);
extern int vmstack_push(VMStmt * stmt);
extern int vmstack_pop(void);
extern Object *vmstack_getcurobj(void);
extern int vmstack_setcurobj(Object * obj);
extern void vmstack_save_lvalues(VMLValue **lv_list, int lv_list_len);
extern void vmstack_restore_lvalues(VMLValue **lv_list, int lv_list_len);



/*
 * vmsrface.c
 */
extern VMStmt *parse_vm_surface(Surface *src_surface);
extern int parse_vm_surface_token(int token, VMStmt **stmtlist);



/*
 * vmviewpt.c
 */
extern VMStmt *parse_vm_viewport(void);



/*
 * vmvsblty.c
 */
extern VMStmt *parse_vm_visibility(void);
extern int parse_vm_visibility_token(int token, VMStmt **stmtlist);



/*
 * vmxform.c
 */
extern VMStmt *parse_vm_transform(int transform_type_token);



#endif  /* __LOCAL_H__ */
