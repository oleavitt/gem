/*************************************************************************
*
*  local.h - Common stuff needed by all modules within the Scn10
*            library.
*
*************************************************************************/

#ifndef LOCAL_H
#define LOCAL_H

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
#include "scn10.h"

/*************************************************************************
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
	TK_BLOB,
	TK_BOUND,
	TK_BOX,
	TK_BREAK,
	TK_BUMP,
	TK_CAUSTICS,
	TK_CLIP,
	TK_CLOSED_CONE,
	TK_CLOSED_CYLINDER,
	TK_COLOR,
	TK_CONE,
	TK_CONTINUE,
	TK_CONVERGE_TO,
	TK_CYLINDER,
	TK_DEFINE,
	TK_DIFFERENCE,
	TK_DIFFUSE,
	TK_DIR,
	TK_DISC,
	TK_DO,
	TK_ELSE,
	TK_EXTRUDE,
	TK_FLOAT,
	TK_FOR,
	TK_GOTO,
	TK_HEIGHT_FIELD,
	TK_IF,
	TK_IMPLICIT,
	TK_INCLUDE_FILE_PATHS,
	TK_INFINITE_LIGHT,
	TK_INTERSECTION,
	TK_INVERSE,
	TK_IOR,
	TK_JITTER,
	TK_LIGHT,
	TK_LOAD_COLOR_MAP,
	TK_MESSAGE,
	TK_NO_SHADOW,
	TK_NO_SPECULAR,
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
	TK_REFLECTION,
	TK_REL_STEP,
	TK_REPEAT,
	TK_RETURN,
	TK_ROT_STEP,
	TK_ROTATE,
	TK_SCALE,
	TK_SHADER,
	TK_SIZE,
	TK_SMOOTH_HEIGHT_FIELD,
	TK_SMOOTH_POLYMESH,
	TK_SPECULAR,
	TK_SPHERE,
	TK_SPOT,
	TK_STATIC,
	TK_SURFACE,
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
	TK_WHILE,

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

	FN_ABS,
	FN_ACOS,
	FN_ASIN,
	FN_ATAN,
	FN_ATAN2,
	FN_CEIL,
	FN_CHECKER,
	FN_CHECKER2,
	FN_CLAMP,
	FN_COLOR_MAP,
	FN_COS,
	FN_COSH,
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

	CV_INT_CONST,
	CV_FLOAT_CONST,
	CV_VECTOR_CONST,
	CV_PI_CONST,
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

	TK_LEFTBRACE,
	TK_RIGHTBRACE,

	TK_UNKNOWN_ID,
	TK_UNKNOWN_CHAR,
	TK_QUOTESTRING,

	DECL_FLOAT,
	DECL_VECTOR,
	DECL_COLOR_MAP,
	DECL_OBJECT,
	DECL_SURFACE,

	THE_END,

	NUM_TOKEN_CODES
};

/*************************************************************************
 * Token flags.
 */
#define TKFLAG_OBJECT      1

/*************************************************************************
 * Token info structure.
 */
typedef struct tag_token
{
	char *name;       /* Keyword associated with token. */
	int token, flags; /* Type code and flags. */
	void *data;       /* Data associated with token. */
} TOKEN;

/* Maximum token buffer size. */
#define TOKEN_SIZE_MAX   256

/*************************************************************************
 * File state info structure.
 */
typedef struct tag_file_data
{
	char name[FILENAME_MAX];
	int line_num;
	FILE *fp;
} FILEDATA;

/*************************************************************************
 * L-value data structure.
 */
typedef struct tag_lv
{
	int type;
	int nrefs;
	Vec3 v;
} LValue;

/*************************************************************************
 * Object stack data structure.
 */
typedef struct tag_objstack
{
	struct tag_objstack *next, *prev;
	Object *objlist, *lastobj, *curobj;
	int level;
} ObjStack;

/*************************************************************************
 * ColorMap data structure.
 */
typedef struct tag_colormap
{
	int ncolors;
	int nrefs;
	float *colordata; /* Color rgb triple followed by value. */
} ColorMap;

/*************************************************************************
 * Symbol table data structure.
 */
typedef struct tag_symboltable
{
	TOKEN **symtab;
	int symcount;
	int symtabmax;
	int stuff_added;
} SymbolTable;

/*
 * backgrnd.c
 */
extern Stmt *ParseBackgroundStmt(void);

/*
 * blob.c
 */
extern Stmt *ParseBlobStmt(void);
extern Stmt *ParseBlobSphereStmt(void);
extern Stmt *ParseBlobCylinderStmt(void);
extern Stmt *ParseBlobPlaneStmt(void);

/*
 * box.c
 */
extern Stmt *ParseBoxStmt(void);

/*
 * branch.c
 */
extern Stmt *ParseIfStmt(int (*ParseDetails)(Stmt **stmt));
extern Stmt *ParseBreakStmt(void);
extern Stmt *ParseContinueStmt(void);

/*
 * colormap.c
 */
extern Stmt *ParseLoadColorMapStmt(void);
extern void ColorMap_LookupColor(ColorMap *cmap, double val, Vec3 *color);
extern ColorMap *ColorMap_Create(void);
extern int ColorMap_AddColor(ColorMap *cmap, Vec3 *color, double val);
extern ColorMap *ColorMap_Copy(ColorMap *cmap);
extern void ColorMap_Delete(ColorMap *cmap);

/*
 * cone.c
 */
extern Stmt *ParseConeStmt(int token);

/*
 * csg.c
 */
extern Stmt *ParseCSGStmt(int token);

/*
 * define.c
 */
/* Ptr to receive surface from ExecSurfaceStmt(). */
extern Surface *parse_declsurf;
extern int ParseDefine(void);

/*
 * disc.c
 */
extern Stmt *ParseDiscStmt(void);

/*
 * error.c
 */
extern MSGFN Message;
extern int warning_count;
extern int error_count;
extern int fatal_count;
extern int Error_Init(void);
extern void Error_Close(void);
extern void LogMessage(const char *fmt, ...);
extern void LogMessageNoFmt(const char *msg);
extern void LogWarning(const char *fmt, ...);
extern void LogError(const char *fmt, ...);
extern void LogFatal(const char *fmt, ...);
extern void LogMemError(const char *who);

/*
 * exeval.c
 */
extern void ExprEval_Init(void);
extern void eval_assign(Expr *expr);
extern void eval_const(Expr *expr);
extern void eval_lvalue(Expr *expr);
extern void eval_vector(Expr *expr);
extern void eval_comma(Expr *expr);
extern void eval_uminus(Expr *expr);
extern void eval_dot_x(Expr *expr);
extern void eval_dot_y(Expr *expr);
extern void eval_dot_z(Expr *expr);
extern void eval_bitand(Expr *expr);
extern void eval_bitor(Expr *expr);
extern void eval_logicnot(Expr *expr);
extern void eval_logicand(Expr *expr);
extern void eval_logicor(Expr *expr);
extern void eval_lessthan(Expr *expr);
extern void eval_greaterthan(Expr *expr);
extern void eval_lessequal(Expr *expr);
extern void eval_greatequal(Expr *expr);
extern void eval_isequal(Expr *expr);
extern void eval_isnotequal(Expr *expr);
extern void eval_plus(Expr *expr);
extern void eval_minus(Expr *expr);
extern void eval_multiply(Expr *expr);
extern void eval_divide(Expr *expr);
extern void eval_mod(Expr *expr);
extern void eval_ternary(Expr *expr);
extern void eval_pow(Expr *expr);

extern void eval_rtfloat(Expr *expr);
extern void eval_rtvec(Expr *expr);

extern void eval_abs(Expr *expr);
extern void eval_acos(Expr *expr);
extern void eval_asin(Expr *expr);
extern void eval_atan(Expr *expr);
extern void eval_atan2(Expr *expr);
extern void eval_ceil(Expr *expr);
extern void eval_checker(Expr *expr);
extern void eval_checker2(Expr *expr);
extern void eval_clamp(Expr *expr);
extern void eval_color_map(Expr *expr);
extern void eval_cos(Expr *expr);
extern void eval_cosh(Expr *expr);
extern void eval_exp(Expr *expr);
extern void eval_floor(Expr *expr);
extern void eval_frand(Expr *expr);
extern void eval_hexagon(Expr *expr);
extern void eval_hexagon2(Expr *expr);
extern void eval_image_map(Expr *expr);
extern void eval_int(Expr *expr);
extern void eval_irand(Expr *expr);
extern void eval_legendre(Expr *expr);
extern void eval_lerp(Expr *expr);
extern void eval_log(Expr *expr);
extern void eval_log10(Expr *expr);
extern void eval_noise(Expr *expr);
extern void eval_round(Expr *expr);
extern void eval_sin(Expr *expr);
extern void eval_sinh(Expr *expr);
extern void eval_smooth_image_map(Expr *expr);
extern void eval_sqrt(Expr *expr);
extern void eval_tan(Expr *expr);
extern void eval_tanh(Expr *expr);
extern void eval_turb(Expr *expr);
extern void eval_turb2(Expr *expr);
extern void eval_vcross(Expr *expr);
extern void eval_vdot(Expr *expr);
extern void eval_vlerp(Expr *expr);
extern void eval_vmag(Expr *expr);
extern void eval_vnoise(Expr *expr);
extern void eval_vnorm(Expr *expr);
extern void eval_vrand(Expr *expr);
extern void eval_vrotate(Expr *expr);
extern void eval_vturb(Expr *expr);
extern void eval_vturb2(Expr *expr);
extern void eval_wrinkle(Expr *expr);

/*
 * exparse.c
 */
extern Expr *ExprNew(void);
extern Expr *ExprParseLVInitializer(LValue *lv);

/*
 * file.c
 */
extern int File_Init(void);
extern void File_Close(void);
extern FILEDATA *File_OpenMain(const char *fname);
extern FILEDATA *File_OpenInclude(const char *fname);
extern FILEDATA *File_CloseInclude(void);
extern void PrintFileAndLineNumber(void);
extern void ErrUnknown(int token, const char *end,
	const char *block_name);

/*
 * gettoken.c
 */
extern char token_buffer[]; /* Token buffer. */
extern TOKEN *cur_token;    /* Ptr to current token details. */
extern int GetToken_Init(const char *fname);
extern void GetToken_Close(void);
extern int GetToken(void);
extern int GetNewIdentifier(void);
extern void UngetToken(void);
extern int ExpectToken(int expected_token, const char *token_name,
  const char *block_name);
extern void ErrUnknown(int token, const char *expected,
	const char *block_name);

/*
 * heightfield.c
 */
extern Stmt *ParseHeightFieldStmt(int smooth);

/*
 * implicit.c
 */
extern Stmt *ParseImplicitStmt(void);

/*
 * light.c
 */
extern Stmt *ParseLightStmt(void);
extern Stmt *ParseInfiniteLightStmt(void);

/*
 * loop.c
 */
extern Stmt *ParseWhileStmt(int (*ParseDetails)(Stmt **stmt));
extern Stmt *ParseDoWhileStmt(int (*ParseDetails)(Stmt **stmt));
extern Stmt *ParseForStmt(int (*ParseDetails)(Stmt **stmt));
extern Stmt *ParseRepeatStmt(int (*ParseDetails)(Stmt **stmt));

/*
 * lvalue.c
 */
extern LValue *LValueNew(int type_token);
extern void LValueDelete(LValue *lv);
extern LValue *LValueCopy(LValue *lv);
extern Stmt *ParseLVInitExprStmt(LValue *lv);
extern Stmt *ParseExprStmt(void);;

/*
 * message.c
 */
extern Stmt *ParseMessageStmt(void);

/*
 * misc.c
 */
extern Stmt *ParseCausticsStmt(void);

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
 * nurbs.c
 */
extern Stmt *ParseNurbsStmt(void);
extern Stmt *ParseNurbsPointStmt(void);

/*
 * object.c
 */
/* Global bounding object list for GSGs and other composite objects. */
extern Object *bound_objects;
extern int cur_object_token;
extern int ParseObjectDetails(Stmt **stmt);
extern Stmt *ParseDeclObjectStmt(Object *declobj);

/*
 * param.c
 */
extern int ParseParams(const char *format, const char *forwhom,
	int (*ParseDetails)(Stmt **stmt), Param *paramlist);

/*
 * parse.c
 */
/* Flag to tell everybody that we're in a declaration. */
extern int parse_declsurfflag;
extern int Parse_Init(void);
extern void Parse_Close(void);
extern void Parse(void);
extern Stmt *ParseBlock(const char *block_name,
	int (*ParseDetails)(Stmt **stmt));
extern Stmt *ParseObject(int token, Object *baseobj);

/*
 * polygon.c
 */
extern Stmt *ParsePolygonStmt(void);
extern Stmt *ParsePolygonVertexStmt(void);

/*
 * polymesh.c
 */
extern Stmt *ParsePolymeshStmt(int smooth);
extern Stmt *ParsePolymeshVertexStmt(void);
extern Stmt *ParsePolymeshExtrudeStmt(void);
extern Stmt *ParsePolymeshPathStmt(int smooth);

/*
 * scn10.c
 */

/*
 * scnbuild.c
 */
extern Surface *default_surface;
extern ObjStack *objstack_ptr;
extern RaySetupData *ray_setup_data;
extern int ScnBuild_Init(RaySetupData *rsd);
extern void ScnBuild_Close(void);
extern void ScnBuild_AddObject(Object *obj);
extern Object *ScnBuild_RemoveLastObject(void);
extern int ScnBuild_PushObjStack(void);
extern void ScnBuild_PopObjStack(void);
extern void ScnBuild_CommitScene(void);

/*
 * sphere.c
 */
extern Stmt *ParseSphereStmt(void);

/*
 * stmt.c
 */
extern Stmt *NewStmt(void);
extern void DeleteStmt(Stmt *stmt);
extern void ExecBlock(Stmt *stmt, Stmt *block);

/*
 * surface.c
 */
extern Stmt *ParseSurfaceStmt(void);
extern Stmt *ParseDeclSurfaceStmt(Surface *declsurf);

/*
 * symbol.c
 */
extern int Symbol_Init(void);
extern void Symbol_Close(void);
extern int Symbol_AddPublic(const char *name, int token, int flags, void *data);
extern int Symbol_AddLocal(const char *name, int token, int flags, void *data);
extern int Symbol_AddPrivate(const char *name, int token, int flags, void *data);
extern TOKEN *Symbol_Find(const char *name);
extern TOKEN *Symbol_FindLocal(const char *name);
extern void Symbol_Push(void);
extern void Symbol_Pop(void);
extern void Symbol_Delete(TOKEN *s);

/*
 * symtab.c
 */
extern void Symtab_Init(SymbolTable *pst);
extern void Symtab_Close(SymbolTable *pst);
extern int Symtab_Add(SymbolTable *pst, const char *name, int token, int flags, void *data);
extern TOKEN *Symtab_Find(SymbolTable *pst, const char *name);
extern void Symtab_DeleteToken(TOKEN *s);

/*
 * torus.c
 */
extern Stmt *ParseTorusStmt(void);

/*
 * triangle.c
 */
extern Stmt *ParseTriangleStmt(void);

/*
 * viewport.c
 */
extern Stmt *ParseAnaglyphStmt(void);
extern Stmt *ParseViewportStmt(void);

#endif  /* LOCAL_H */
