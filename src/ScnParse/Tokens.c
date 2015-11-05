/*************************************************************************
 *
 *  tokens.c - Tokenizer functions for the scene script parser.
 *
 ************************************************************************/

#include "scn.h"

char source_file[FILENAME_MAX];
int line_num;
int ntokens;
char token_buffer[MAX_TOKEN_BUF_SIZE];
TOKEN *cur_token;

static FILE *fp;
static long fpos;
static int last_line;

typedef struct tag_file_data
{
  char *name;
  int line_num;
  FILE *fp;
} FILEDATA;

#define MAX_INCLUDE_LEVELS  15
static int include_level;
static FILEDATA *include_stack[MAX_INCLUDE_LEVELS];

static TOKEN tokens[] =
{
  { "D", RT_D, TKFLAG_RTVAR },
  { "O", RT_O, TKFLAG_RTVAR },
  { "ON", RT_ON, TKFLAG_RTVAR },
  { "ONx", RT_ONx, TKFLAG_RTVAR },
  { "ONy", RT_ONy, TKFLAG_RTVAR },
  { "ONz", RT_ONz, TKFLAG_RTVAR },
  { "PI", CV_PI_CONST, 0 },
  { "Phong", TK_SPECULAR_SIZE, 0 },
  { "W", RT_W, TKFLAG_RTVAR },
  { "WN", RT_WN, TKFLAG_RTVAR },
  { "WNx", RT_WNx, TKFLAG_RTVAR },
  { "WNy", RT_WNy, TKFLAG_RTVAR },
  { "WNz", RT_WNz, TKFLAG_RTVAR },
  { "X", RT_X, TKFLAG_RTVAR },
  { "Y", RT_Y, TKFLAG_RTVAR },
  { "Z", RT_Z, TKFLAG_RTVAR },
  { "abs", FN_ABS, 0 },
  { "acos", FN_ACOS, 0 },
  { "add", TK_ADD, TKFLAG_OBJECT },
  { "ambient", TK_AMBIENT, 0 },
  { "angle", TK_ANGLE, 0 },
  { "asin", FN_ASIN, 0 },
  { "at", TK_AT, 0 },
  { "atan", FN_ATAN, 0 },
  { "atan2", FN_ATAN2, 0 },
  { "background", TK_BACKGROUND, 0 },
  { "blob", TK_BLOB, TKFLAG_OBJECT },
	{ "bound", TK_BOUND, 0 },
  { "box", TK_BOX, TKFLAG_OBJECT },
  { "break", TK_BREAK, TKFLAG_PROC },
  { "bump", TK_BUMP, 0 },
  { "camera", TK_VIEWPORT, 0 },
  { "caustics", TK_CAUSTICS, 0 },
  { "ceil", FN_CEIL, 0 },
  { "checker", FN_CHECKER, 0 },
  { "clamp", FN_CLAMP, 0 },
  { "clip", TK_CLIP, TKFLAG_OBJECT },
  { "closed_cone", TK_CLOSED_CONE, TKFLAG_OBJECT },
  { "closed_cylinder", TK_CLOSED_CYLINDER, TKFLAG_OBJECT },
  { "color", TK_COLOR, 0 },
  { "colored_triangle", TK_COLORED_TRIANGLE, TKFLAG_OBJECT },
  { "cone", TK_CONE, TKFLAG_OBJECT },
  { "cos", FN_COS, 0 },
  { "cosh", FN_COSH, 0 },
  { "cylinder", TK_CYLINDER, TKFLAG_OBJECT },
  { "cylinder_map", FN_CYLINDER_MAP, 0 },
  { "deg", FN_DEG, 0 },
  { "difference", TK_DIFFERENCE, TKFLAG_OBJECT },
  { "diffuse", TK_DIFFUSE, 0 },
  { "dir", TK_DIR, 0 },
  { "disc", TK_DISC, TKFLAG_OBJECT },
  { "disc_map", FN_DISC_MAP, 0 },
  { "do", TK_DO, TKFLAG_PROC },
  { "elif", TK_ELIF, 0 },
  { "else", TK_ELSE, 0 },
  { "environment_map", FN_ENVIRONMENT_MAP, 0 },
  { "exp", FN_EXP, 0 },
  { "extrude", TK_EXTRUDE, TKFLAG_OBJECT },
  { "float", TK_FLOAT, 0 },
  { "floor", FN_FLOOR, 0 },
  { "fn", TK_FUNCTION, 0 },
  { "fog", TK_FOG, 0 },
	{ "frame", FN_FRAME, 0 },
  { "from", TK_FROM, 0 },
  { "function", TK_FUNCTION, 0 },
  { "global_settings", TK_GLOBAL_SETTINGS, 0 },
  { "height_field", TK_HEIGHT_FIELD, TKFLAG_OBJECT },
  { "hexagon", FN_HEXAGON, 0 },
  { "if", TK_IF, TKFLAG_PROC },
  { "image_map", FN_IMAGE_MAP, 0 },
  { "image_map_file_paths", TK_IMAGE_MAP_FILE_PATHS, 0 },
  { "implicit", TK_IMPLICIT, TKFLAG_OBJECT },
  { "include_file_paths", TK_INCLUDE_FILE_PATHS, 0 },
  { "int", FN_INT, 0 },
  { "intersection", TK_INTERSECTION, TKFLAG_OBJECT },
  { "inverse", TK_INVERSE, 0 },
  { "ior", TK_IOR, 0 },
  { "ka", TK_AMBIENT, 0 },
  { "kd", TK_DIFFUSE, 0 },
  { "kr", TK_REFLECTION, 0 },
  { "ks", TK_SPECULAR, 0 },
  { "kt", TK_TRANSMISSION, 0 },
  { "light", TK_LIGHT, 0 },
  { "log", FN_LOG, 0 },
  { "log10", FN_LOG10, 0 },
  { "max_trace_depth", TK_MAX_TRACE_DEPTH, 0 },
  { "max_trace_dist", TK_MAX_TRACE_DIST, 0 },
  { "mesh", TK_MESH, TKFLAG_OBJECT },
  { "min_shadow_dist", TK_MIN_SHADOW_DIST, 0 },
  { "min_trace_dist", TK_MIN_TRACE_DIST, 0 },
  { "next", TK_NEXT, TKFLAG_PROC },
	{ "nframe", FN_NFRAME, 0 },
	{ "nframe2", FN_NFRAME2, 0 },
  { "no_shadow", TK_NO_SHADOW, 0 },
  { "no_specular", TK_NO_SPECULAR, 0 },
  { "no_textures", TK_NO_TEXTURES, 0 },
  { "noise", FN_NOISE, 0 },
  { "object", TK_OBJECT, TKFLAG_OBJECT },
  { "plane", TK_PLANE, TKFLAG_OBJECT },
  { "plane_map", FN_PLANE_MAP, 0 },
  { "polygon", TK_POLYGON, TKFLAG_OBJECT },
  { "polynomial", TK_POLYNOMIAL, TKFLAG_OBJECT },
  { "proc", TK_PROC, 0 },
  { "rad", FN_RAD, 0 },
  { "rand", FN_RAND, 0 },
  { "reflection", TK_REFLECTION, 0 },
  { "repeat", TK_REPEAT, TKFLAG_PROC },
  { "resolution", TK_RESOLUTION, 0 },
  { "return", TK_RETURN, TKFLAG_PROC },
  { "rotate", TK_ROTATE, 0 },
  { "scale", TK_SCALE, 0 },
  { "screenu", RT_screenu, TKFLAG_RTVAR },
  { "screenv", RT_screenv, TKFLAG_RTVAR },
  { "segment", TK_SEGMENT, 0 },
  { "shear", TK_SHEAR, 0 },
  { "sin", FN_SIN, 0 },
  { "sinh", FN_SINH, 0 },
  { "smooth", TK_SMOOTH, 0 },
  { "smooth_image_map", FN_SMOOTH_IMAGE_MAP, 0 },
  { "specular", TK_SPECULAR, 0 },
  { "specular_size", TK_SPECULAR_SIZE, 0 },
  { "sphere", TK_SPHERE, TKFLAG_OBJECT },
  { "sphere_map", FN_SPHERE_MAP, 0 },
  { "sqrt", FN_SQRT, 0 },
  { "tan", FN_TAN, 0 },
  { "tanh", FN_TANH, 0 },
  { "test", FN_TEST, 0 },
  { "texture", TK_TEXTURE, 0 },
  { "textures", TK_TEXTURES, 0 },
  { "torus", TK_TORUS, TKFLAG_OBJECT },
  { "torus_map", FN_TORUS_MAP, 0 },
  { "transform", TK_TRANSFORM, 0 },
  { "translate", TK_TRANSLATE, 0 },
  { "transmission", TK_TRANSMISSION, 0 },
  { "triangle", TK_TRIANGLE, TKFLAG_OBJECT },
  { "turb", FN_TURB, 0 },
  { "turb2", FN_TURB2, 0 },
  { "u", RT_u, TKFLAG_RTVAR },
  { "union", TK_UNION, TKFLAG_OBJECT },
  { "up", TK_UP, 0 },
  { "v", RT_v, TKFLAG_RTVAR },
  { "vcross", FN_VCROSS, 0 },
  { "vdot", FN_VDOT, 0 },
  { "vector", TK_VECTOR, 0 },
  { "vertex", TK_VERTEX, 0 },
  { "viewport", TK_VIEWPORT, 0 },
  { "vnoise", FN_VNOISE, 0 },
  { "vnorm", FN_VNORM, 0 },
  { "vrand", FN_VRAND, 0 },
  { "vturb", FN_VTURB, 0 },
  { "vturb2", FN_VTURB2, 0 },
  { "while", TK_WHILE, TKFLAG_PROC },
  { "x", RT_x, TKFLAG_RTVAR },
  { "y", RT_y, TKFLAG_RTVAR },
  { "z", RT_z, TKFLAG_RTVAR },
  { "", THE_END, 0 }
};

static TOKEN misc_token = { token_buffer, 0, 0 };

static TOKEN *last_token;
static SYMBOL *tk_symbol;

static int Close_Include_File(void);

void Init_Tokens(const char *fname)
{
  line_num = 0;
  include_level = 0;
  cur_token = &misc_token;
  misc_token.token = TK_NULL;

  /*
   * Find the main input file...
   */
  if((fp = SCN_FindFile(fname, "r", scn_source_paths,
    SCN_FINDFILE_CHK_CUR_FIRST)) == NULL)
  {
    SCN_Message(SCN_MSG_ERROR, "scene: Unable to find input file: %s", fname);
    return;
  }

  /*
   * Locate the last token in the list.
   */
  for(last_token = tokens, ntokens = 0;
      last_token->token != THE_END;
      last_token++, ntokens++)
      ;  /* Just inc through the list... */

  Init_Symbol();
  line_num = 1;
  strcpy(source_file, fname);
}

/*
 * Read everything between quotes in to the global buffer.
 */
static int Parse_Quoted_String(void)
{
  int size, ch;
  char *buf;

  buf = token_buffer;
  size = 0;

  /* Skip white space... */
  while (isspace(ch = fgetc(fp)))
    if(ch == ASCII_NEWLINE) line_num++;

  if(ch != ASCII_QUOTE)
  {
    SCN_Message(SCN_MSG_ERROR, "Double quote(\") expected.");
    return 0;
  }

  while((ch = fgetc(fp)) != ASCII_QUOTE && ch != ASCII_NEWLINE)
  {
    if(feof(fp)) break;
    *buf++ = (char)ch;
    if(size++ == MAX_TOKEN_BUF_SIZE) break;
  }
  *buf = ASCII_NUL;
  if(feof(fp))
  {
    SCN_Message(SCN_MSG_ERROR, "Expected \", but reached end of file.");
    return 0;
  }
  if(ch == ASCII_NEWLINE)
  {
    SCN_Message(SCN_MSG_ERROR, "Expected \", but reached end of line.");
    return 0;
  }
  if(size >= MAX_TOKEN_BUF_SIZE)
  {
    SCN_Message(SCN_MSG_ERROR, "Quoted string too big!");
    return 0;
  }
  return 1;
} /* end of Parse_Quoted_String() */

/*
 * Get include file name, open it, verify it, and push a new stream
 * on to the tokenizer input stream stack.
 */
static void Open_Include_File(void)
{
  if(include_level >= MAX_INCLUDE_LEVELS)
    SCN_Message(SCN_MSG_ERROR, "Too many nested include files, possible self referential loop.");

  if(Parse_Quoted_String())
  {
    include_stack[include_level] = (FILEDATA *)mmalloc(sizeof(FILEDATA));
    include_stack[include_level]->fp = fp;
    include_stack[include_level]->line_num = line_num;
    include_stack[include_level]->name = str_dup(source_file);
    include_level++;

    if((fp = SCN_FindFile(token_buffer, "r", scn_include_paths,
      SCN_FINDFILE_CHK_CUR_FIRST)) == NULL)
    {
      SCN_Message(SCN_MSG_ERROR, "Can not find include file: %s.", token_buffer);
      Close_Include_File();
      return;
    }
    strcpy(source_file, token_buffer);
    line_num = 1;
  }
} /* end of Open_Include_File() */


static int Close_Include_File(void)
{
  if(include_level > 0)
  {
    include_level--;
    fclose(fp);
    fp = include_stack[include_level]->fp;
    line_num = include_stack[include_level]->line_num;
    strcpy(source_file, include_stack[include_level]->name);
    str_free(include_stack[include_level]->name);
    mfree(include_stack[include_level], sizeof(FILEDATA));
    return 1;
  }
  return 0;
}

static int comp_tokens(const void *name, const void *token)
{
	return strcmp((char *)name, ((TOKEN *)token)->name);
}

int Get_Token(void)
{
  int c, c2, start_line, nest_level, token;
  char *b;
  TOKEN *t;
#ifdef MYBSEARCH
	int result;
	TOKEN *lo, *hi;
#endif

  cur_token = &misc_token;
  misc_token.flags = 0;

  while(1)
  {
    /* Bookmark previous position for Unget_Token(). */
    fpos = ftell(fp);
    last_line = line_num;

    /* Gobble up white space and count lines... */
    do
    {
      c = fgetc(fp);
      if(c == '\n')
        line_num++;
    }
    while(isspace(c));

    /* Gobble up comments... */
    if(c == '/')
    {
      c2 = fgetc(fp);

      if(c2 == '/')      /* C++ style single-line comment. */
      {
        do
        {
          if((c = fgetc(fp)) == EOF)
          {
            token = EOF;
            goto done;
          }
        }
        while (c != '\n');
        line_num++;
        continue;
      }
      else if(c2 == '*') /* C style multi-line comment. */
      {
        /* Remember where we started in case of error. */
        start_line = line_num;
        nest_level = 0;
        while(1)
        {
          /* Entering nested comment? */
          if((c = fgetc(fp)) == '/')
            if((c = fgetc(fp)) == '*')
              nest_level++;
          /* End of outer most comment? */
          if(c == '*')
            if((c = fgetc(fp)) == '/')
              if(nest_level-- < 1)
                break;
          if(c == '\n')
            line_num++;
          if(feof(fp))
          {
            SCN_Message(SCN_MSG_ERROR,
"Unexpected end of file - Unterminated comment started on line: %d\
\n  File: %s, Line #: %d.",
              start_line, source_file, line_num);
            break;
          }
        }
        continue;
      }
      ungetc(c2, fp);
    }

    /* Stuff from this point on will be buffered... */
    b = token_buffer;

    /* See if we have an identifier name... */
    if(isalpha(c) || c == '_')
    {
      do
      {
        *b++ = (char)c;
        c = fgetc(fp);
      }
      while(isalnum(c) || c == '_');
      ungetc(c, fp);
      *b = '\0';
      /* See if it is a keyword... */
#ifdef MYBSEARCH
			lo = tokens;
			hi = last_token;
			while(1)
			{
				if(lo >= hi)
				{
					t = last_token;
					break;
				}
				t = lo + (unsigned)(hi - lo) / 2;
				if((result = strcmp(t->name, token_buffer)) > 0)
					hi = t;
				else if(result < 0)
					lo = t + 1;
				else if(result == 0)   /* match found */
        {
          cur_token = t;
          token = t->token;
          goto done;
        }
			}
#else
			t = (TOKEN *)bsearch((void *)token_buffer, (void *)tokens,
				ntokens, sizeof(TOKEN), comp_tokens);
			if(t != NULL)
      {
        cur_token = t;
        token = t->token;
        goto done;
      }
#endif
      /* See if it is the "include" directive... */
      if(strcmp(token_buffer, "include") == 0)
      {
        Open_Include_File();
        continue;
      }
      /* See if it is a user-defined symbol... */
      if((tk_symbol = Fetch_Symbol(token_buffer)) != NULL)
      {
        token = tk_symbol->type;
        misc_token.data = tk_symbol->data;
        if(token == DECL_OBJECT)
          misc_token.flags |= TKFLAG_OBJECT;
        if(token == DECL_PROC)
          misc_token.flags |= TKFLAG_PROC;
      }
      else
        token = TK_UNKNOWN_ID;
      goto done;
    }

    /* See if we have a number... */
    if(isdigit(c) || c == '.')
    {
      int dp_count, exp_count;

      dp_count = exp_count = 0;
      if(c == '.')
      {
        *b++ = (char)c;
        dp_count++;
        c = fgetc(fp);
      }
      while(isdigit(c))
      {
        *b++ = (char)c;
        c = fgetc(fp);
        if(c == '.' && dp_count == 0)
        {
          *b++ = (char)c;
          dp_count++;
          c = fgetc(fp);
        }
        if(toupper(c) == 'E' && exp_count == 0)
        {
          *b++ = (char)c;
          c = fgetc(fp);
          exp_count++;
          dp_count++;
          if(c == '-')
          {
            *b++ = (char)c;
            c = fgetc(fp);
          }
        }
      }
      *b = '\0';
      ungetc(c, fp);
      token = (dp_count) ? CV_FLOAT_CONST : CV_INT_CONST;
      goto done;
    }

    /* Place char in buffer... */
    *b++ = (char)c;
    *b = '\0';

    /* operator? */
    switch(c)
    {
      /* Check for duets... */
      case '+':
      case '-':
      case '*':
      case '/':
      case '&':
      case '|':
      case '<':
      case '>':
      case '!':
      case '=':
        c2 = fgetc(fp);
        *b++ = (char)c2;
        *b-- = '\0';
        switch(c)
        {
          case '+':
            token = (c2 == '=') ? OP_PLUSASSIGN :
                   (c2 == '+') ? OP_INCREMENT :
                   (ungetc(c2, fp), *b = '\0', OP_PLUS);
            goto done;
          case '-':
            token = (c2 == '=') ? OP_MINUSASSIGN :
                   (c2 == '-') ? OP_DECREMENT :
                   (ungetc(c2, fp), *b = '\0', OP_MINUS);
            goto done;
          case '*':
            token = (c2 == '=') ? OP_MULTASSIGN :
                   (ungetc(c2, fp), *b = '\0', OP_MULT);
            goto done;
          case '/':
            token = (c2 == '=') ? OP_DIVASSIGN :
                   (ungetc(c2, fp), *b = '\0', OP_DIVIDE);
            goto done;
          case '&':
            token = (c2 == '&') ? OP_ANDAND :
                   (ungetc(c2, fp), *b = '\0', OP_AND);
            goto done;
          case '|':
            token = (c2 == '|') ? OP_OROR :
                   (ungetc(c2, fp), *b = '\0', OP_OR);
            goto done;
          case '<':
            token = (c2 == '=') ? OP_LESSEQUAL :
                   (c2 == '>') ? OP_NOTEQUAL :
                   (ungetc(c2, fp), *b = '\0', OP_LESSTHAN);
            goto done;
          case '>':
            token = (c2 == '=') ? OP_GREATEQUAL :
                   (ungetc(c2, fp), *b = '\0', OP_GREATERTHAN);
            goto done;
          case '!':
            token = (c2 == '=') ? OP_NOTEQUAL :
                   (ungetc(c2, fp), *b = '\0', OP_NOT);
            goto done;
          case '=':
            token = (c2 == '=') ? OP_EQUAL :
                   (ungetc(c2, fp), *b = '\0', OP_ASSIGN);
            goto done;
        }
        /* Shouldn't get here. */

      case ',':
        token = OP_COMMA;
        goto done;
      case '{':
        token = TK_LEFTBRACE;
        goto done;
      case '}':
        token = TK_RIGHTBRACE;
        goto done;
      case '(':
        token = OP_LPAREN;
        goto done;
      case ')':
        token = OP_RPAREN;
        goto done;
      case '[':
        token = OP_LSQUARE;
        goto done;
      case ']':
        token = OP_RSQUARE;
        goto done;
      case '^':
        token = OP_POW;
        goto done;
      case '%':
        token = OP_MOD;
        goto done;
      case '?':
        token = OP_QUESTION;
        goto done;
      case ':':
        token = OP_COLON;
        goto done;
      case '"':
        ungetc(c, fp);
        Parse_Quoted_String();
        token = TK_QUOTESTRING;
        goto done;
    }


    /* end of file? */
    if(feof(fp))
    {
      if(Close_Include_File())
        continue;
      token = EOF;
      break;
    }

    /* Unknown non-alpha character. */
    token = TK_UNKNOWN_CHAR;
    break;
  }

  done:

  misc_token.token = token;
  if(token == EOF)
    strcpy(token_buffer, "End Of File");

  return token;
}

/*************************************************************************
 *
 *  Unget_Token() - Undo the last call to Get_Token(). Restores the stream
 *    position and line number to the values saved by Get_Token() before
 *    it reads from the stream.
 *
 ************************************************************************/
void Unget_Token(void)
{
  fseek(fp, fpos, SEEK_SET);
  line_num = last_line;
}


/*************************************************************************
 *
 *  Expect() - Get a token. If it doesn't match "expected_token" report
 *    an error.
 *
 ************************************************************************/
int Expect(int expected_token, const char *token_name,
  const char *block_name)
{
  int token;

  token = Get_Token();
  if(token != expected_token)
  {
    if(block_name != NULL && *block_name != '\0')
      SCN_Message(SCN_MSG_ERROR, "%s: `%s' expected. Found `%s'.\n  File: %s, Line: %d.",
        block_name, token_name, token_buffer, source_file, line_num);
    else
      SCN_Message(SCN_MSG_ERROR, "`%s' expected. Found `%s'.\n  File: %s, Line: %d.",
        token_name, token_buffer, source_file, line_num);
  }
  return token;
}


/*************************************************************************
 *
 *  Check_EOF() - See if end of file has been reached and report error
 *    if EOF has been reached. This is used within block statements to
 *    check for unexpected EOFs.
 *
 ************************************************************************/
void Check_EOF(int token, const char *block_name)
{
  if(token == EOF)
  {
    if(block_name != NULL && *block_name != '\0')
      SCN_Message(SCN_MSG_ERROR, "%s: Unexpected end of file.\n  File: %s, Line: %d.",
        block_name, source_file, line_num);
    else
      SCN_Message(SCN_MSG_ERROR, "Unexpected end of file.\n  File: %s, Line: %d.",
        source_file, line_num);
  }
}


/*************************************************************************
 *
 * Err_Unknown - Error message for unknown, or out of context, token.
 *   If "block_name" is not NULL, "block_name: " is inserted in message.
 *   If "end" is not NULL, "`end' expected" is appended to message.
 *
 ************************************************************************/
void Err_Unknown(int token, const char *end, const char *block_name)
{
  static char s[MAX_MESSAGE_SIZE];

  if(token == EOF)
  {
    if(block_name != NULL)
      sprintf(s, "%s: Unexpected End Of File.", block_name);
    else
      sprintf(s, "Unexpected End Of File.");
  }
  else
  {
    if(block_name != NULL)
      sprintf(s, "%s: Unknown or misplaced token `%s'.",
        block_name, token_buffer);
    else
      sprintf(s, "Unknown or misplaced token `%s'.", token_buffer);
  }

  if(end != NULL)
    SCN_Message(SCN_MSG_ERROR, "%s `%s' expected.\n  File: %s, Line #: %d.",
      s, end, source_file, line_num);
  else
    SCN_Message(SCN_MSG_ERROR, "%s\n  File: %s, Line #: %d.", s, source_file, line_num);
}


void Close_Tokens(void)
{
  line_num = 0;
  if(fp != NULL)
    fclose(fp);
  fp = NULL;
  while(include_level-- > 0)
  {
    str_free(include_stack[include_level]->name);
    mfree(include_stack[include_level], sizeof(FILEDATA));
    include_stack[include_level] = NULL;
  }
  Close_Symbol();
}

