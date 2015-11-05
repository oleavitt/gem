/*************************************************************************
 *
 *  tokens.h - Token codes, types, and other externals for the
 *  tokenizer and symbol table modules.
 *
 ************************************************************************/

#ifndef TOKENS_H
#define TOKENS_H

/*************************************************************************
 * Token info structure.
 */
typedef struct tag_token
{
  char *name;       /* Keyword associated with  token. */
  int token, flags; /* Type code and flags. */
  void *data;       /* Data associated with token. */
} TOKEN;


/*************************************************************************
 *  Symbol table entry.
 */
typedef struct tag_symbol
{
  struct tag_symbol
    *l, *r,    /* Ptrs to left & right sub-trees in symbol table. */
    *next,     /* Different types under same name. */
    *pnext;    /* Link in local symbol re-use pool. */
  int type;    /* Token type code for data. */
  int level;   /* Non-zero if this is a local symbol. */
  char *name;  /* Identifier name. */
  void *data;  /* Pointer to the goods. */
} SYMBOL;


/*
 * Token flags.
 */
#define TKFLAG_OBJECT          1
#define TKFLAG_EXPR            2
#define TKFLAG_RTVAR           4
#define TKFLAG_PROC            8

/*
 * Token codes.
 */
typedef enum
{
  /*
   * the "nothing" token
   */
  TK_NULL = 0,
  /*
   * key words
   */
  TK_ADD,
  TK_AMBIENT,
  TK_ANGLE,
  TK_AT,
  TK_BACKGROUND,
	TK_BOUND,
  TK_BOX,
  TK_BLOB,
  TK_BREAK,
  TK_BUMP,
  TK_CAMERA,
  TK_CAUSTICS,
  TK_CLIP,
  TK_CLOSED_CONE,
  TK_CLOSED_CYLINDER,
  TK_COLOR,
  TK_COLORED_TRIANGLE,
  TK_CONE,
  TK_CYLINDER,
  TK_DIFFERENCE,
  TK_DIFFUSE,
  TK_DIR,
  TK_DISC,
  TK_DO,
  TK_ELIF,
  TK_ELSE,
	TK_EXTRUDE,
  TK_FLOAT,
  TK_FOG,
  TK_FROM,
  TK_FUNCTION,
  TK_GLOBAL_SETTINGS,
  TK_HEIGHT_FIELD,
  TK_IF,
	TK_IMAGE_MAP_FILE_PATHS,
  TK_IMPLICIT,
	TK_INCLUDE_FILE_PATHS,
  TK_INTERSECTION,
  TK_INVERSE,
  TK_IOR,
  TK_LIGHT,
  TK_MAX_TRACE_DEPTH,
  TK_MAX_TRACE_DIST,
  TK_MESH,
  TK_MIN_SHADOW_DIST,
  TK_MIN_TRACE_DIST,
  TK_NEXT,
  TK_NO_SHADOW,
  TK_NO_SPECULAR,
  TK_NO_TEXTURES,
  TK_OBJECT,
  TK_PLANE,
  TK_POLYGON,
  TK_POLYNOMIAL,
  TK_PROC,
  TK_REFLECTION,
  TK_REPEAT,
  TK_RESOLUTION,
  TK_RETURN,
  TK_ROTATE,
  TK_SCALE,
	TK_SEGMENT,
  TK_SHEAR,
	TK_SMOOTH,
  TK_SPECULAR,
  TK_SPECULAR_SIZE,
  TK_SPHERE,
  TK_STATEMENT,
  TK_TEXTURE,
  TK_TEXTURES,
  TK_TORUS,
  TK_TRANSFORM,
  TK_TRANSLATE,
  TK_TRANSMISSION,
  TK_TRIANGLE,
  TK_UNION,
  TK_UP,
  TK_VECTOR,
  TK_VERTEX,
  TK_VIEWPORT,
  TK_WHILE,
  /*
   * punctuator tokens
   */
  TK_LEFTBRACE,
  TK_RIGHTBRACE,
  /*
   * expression tokens
   */
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
  RT_D,
  RT_O,
  RT_x,
  RT_y,
  RT_z,
  RT_W,
  RT_X,
  RT_Y,
  RT_Z,
  RT_ON,
  RT_ONx,
  RT_ONy,
  RT_ONz,
  RT_WN,
  RT_WNx,
  RT_WNy,
  RT_WNz,
  RT_screenu,
  RT_screenv,
  RT_u,
  RT_v,
  FN_ABS,
  FN_ACOS,
  FN_ASIN,
  FN_ATAN,
  FN_ATAN2,
  FN_CEIL,
  FN_CHECKER,
  FN_CLAMP,
  FN_COS,
  FN_COSH,
  FN_CYLINDER_MAP,
  FN_DEG,
  FN_DISC_MAP,
  FN_ENVIRONMENT_MAP,
  FN_EXP,
  FN_FLOOR,
	FN_FRAME,
  FN_HEXAGON,
  FN_INT,
  FN_IMAGE_MAP,
  FN_LOG,
  FN_LOG10,
	FN_NFRAME,
	FN_NFRAME2,
  FN_NOISE,
  FN_PLANE_MAP,
  FN_RAD,
  FN_RAND,
  FN_SIN,
  FN_SINH,
  FN_SMOOTH_IMAGE_MAP,
  FN_SPHERE_MAP,
  FN_SQRT,
  FN_TAN,
  FN_TANH,
  FN_TEST,
  FN_TORUS_MAP,
  FN_TURB,
  FN_TURB2,
  FN_VDOT,
  FN_VCROSS,
  FN_VNOISE,
  FN_VNORM,
  FN_VRAND,
  FN_VTURB,
  FN_VTURB2,
  END_OF_EXPR,
  /*
   * declared stuff
   */
  DECL_LVALUE,
  DECL_PROC,
  DECL_LIGHT,
  DECL_OBJECT,
  DECL_TEXTURE,
  DECL_SYMBOL,
  /*
   * misc. data types
   */
  TK_UNKNOWN_ID,
  TK_UNKNOWN_CHAR,
  TK_QUOTESTRING,
  TK_RT_EXPR,
  TK_PROC_EXPR,
  THE_END
} TOKEN_CODES;

#define TK_STRING     TK_UNKNOWN_ID

#define FN_ROTATE     TK_ROTATE
#define FN_SCALE      TK_SCALE
#define FN_TRANSLATE  TK_TRANSLATE

/*
 * ASCII character codes.
 */
#define ASCII_NUL        '\0'

#define ASCII_NEWLINE    '\n'
#define ASCII_RETURN     '\r'
#define ASCII_BACKSP     '\b'
#define ASCII_TAB        '\t'

#define ASCII_LBRACE     '{'
#define ASCII_RBRACE     '}'
#define ASCII_LPAREN     '('
#define ASCII_RPAREN     ')'
#define ASCII_LSQUARE    '['
#define ASCII_RSQUARE    ']'
#define ASCII_PERIOD     '.'
#define ASCII_COMMA      ','
#define ASCII_COLON      ':'
#define ASCII_SEMICOLON  ';'
#define ASCII_QUESTION   '?'
#define ASCII_QUOTE      '\"'
#define ASCII_SQUOTE     '\''
#define ASCII_TILDE      '~'
#define ASCII_EXCLAM     '!'
#define ASCII_BSLASH     '\\'
#define ASCII_PLUS       '+'
#define ASCII_MINUS      '-'
#define ASCII_UNDER      '_'
#define ASCII_ASTERISK   '*'
#define ASCII_SLASH      '/'
#define ASCII_EQUAL      '='
#define ASCII_PERCENT    '%'
#define ASCII_UPCARET    '^'
#define ASCII_LCARET     '<'
#define ASCII_RCARET     '>'
#define ASCII_PIPE       '|'
#define ASCII_AMPERSAND  '&'
#define ASCII_POUND      '#'
#define ASCII_DOLLAR     '$'
#define ASCII_AT         '@'
#define ASCII_APOSTROPHE '`'
#define ASCII_SPACE      ' '

#define ASCII_A          'A'
#define ASCII_B          'B'
#define ASCII_C          'C'
#define ASCII_D          'D'
#define ASCII_E          'E'
#define ASCII_F          'F'
#define ASCII_G          'G'
#define ASCII_H          'H'
#define ASCII_I          'I'
#define ASCII_J          'J'
#define ASCII_K          'K'
#define ASCII_L          'L'
#define ASCII_M          'M'
#define ASCII_N          'N'
#define ASCII_O          'O'
#define ASCII_P          'P'
#define ASCII_Q          'Q'
#define ASCII_R          'R'
#define ASCII_S          'S'
#define ASCII_T          'T'
#define ASCII_U          'U'
#define ASCII_V          'V'
#define ASCII_W          'W'
#define ASCII_X          'X'
#define ASCII_Y          'Y'
#define ASCII_Z          'Z'

#define ASCII_1          '1'
#define ASCII_2          '2'
#define ASCII_3          '3'
#define ASCII_4          '4'
#define ASCII_5          '5'
#define ASCII_6          '6'
#define ASCII_7          '7'
#define ASCII_8          '8'
#define ASCII_9          '9'
#define ASCII_0          '0'

#define ASCII_a          'a'
#define ASCII_b          'b'
#define ASCII_c          'c'
#define ASCII_d          'd'
#define ASCII_e          'e'
#define ASCII_f          'f'
#define ASCII_g          'g'
#define ASCII_h          'h'
#define ASCII_i          'i'
#define ASCII_j          'j'
#define ASCII_k          'k'
#define ASCII_l          'l'
#define ASCII_m          'm'
#define ASCII_n          'n'
#define ASCII_o          'o'
#define ASCII_p          'p'
#define ASCII_q          'q'
#define ASCII_r          'r'
#define ASCII_s          's'
#define ASCII_t          't'
#define ASCII_u          'u'
#define ASCII_v          'v'
#define ASCII_w          'w'
#define ASCII_x          'x'
#define ASCII_y          'y'
#define ASCII_z          'z'

/*************************************************************************
 *  Tokenizer stuff.
 */

extern char source_file[];
extern int line_num;
extern char token_buffer[];
extern TOKEN *cur_token;

extern void Init_Tokens(const char *fname);
extern int Get_Token(void);
extern void Unget_Token(void);
extern int Expect(int expected_token, const char *token_name,
	const char *block_name);
extern void Check_EOF(int token, const char *block_name);
extern void Err_Unknown(int token, const char *end,
	const char *block_name);
extern void Close_Tokens(void);


#endif    /* TOKENS_H */
