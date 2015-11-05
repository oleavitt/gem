/**
 *****************************************************************************
 *  @file gettoken.c
 *  Scans input from file stream and converts it to token codes.
 *
 *****************************************************************************
 */

#include "local.h"



static TOKEN tokens[] =
{
//	{ "D", RT_D, 0 },
//	{ "N", RT_N, 0 },
	{ "O", RT_O, 0 },
	{ "ON", RT_ON, 0 },
	{ "PI", CV_PI_CONST, 0 },
	{ "Phong", TK_PHONG, 0 },
	{ "abs", FN_ABS, 0 },
	{ "acos", FN_ACOS, 0 },
	{ "ambient", TK_AMBIENT, 0 },
	{ "anaglyph", TK_ANAGLYPH, 0 },
	{ "asin", FN_ASIN, 0 },
	{ "at", TK_AT, 0 },
	{ "atan", FN_ATAN, 0 },
	{ "atan2", FN_ATAN2, 0 },
	{ "background", TK_BACKGROUND, 0 },
	{ "background_shader", TK_BACKGROUND_SHADER, 0 },
	{ "blob", TK_BLOB, TKFLAG_OBJECT },
	{ "bound", TK_BOUND, 0 },
	{ "box", TK_BOX, TKFLAG_OBJECT },
	{ "break", TK_BREAK, 0 },
	{ "bump", FN_BUMP, 0 },
	{ "caustics", TK_CAUSTICS, 0 },
	{ "ceil", FN_CEIL, 0 },
	{ "center", TK_CENTER, 0 },
	{ "checker", FN_CHECKER, 0 },
	{ "checker2", FN_CHECKER2, 0 },
	{ "clamp", FN_CLAMP, 0 },
	{ "clip", TK_CLIP, TKFLAG_OBJECT },
	{ "closed_cone", TK_CLOSED_CONE, TKFLAG_OBJECT },
	{ "closed_cylinder", TK_CLOSED_CYLINDER, TKFLAG_OBJECT },
	{ "color", TK_COLOR, 0 },
	{ "color_map", FN_COLOR_MAP, 0 },
	{ "cone", TK_CONE, TKFLAG_OBJECT },
	{ "continue", TK_CONTINUE, 0 },
	{ "cos", FN_COS, 0 },
	{ "cosh", FN_COSH, 0 },
	{ "cylinder", TK_CYLINDER, TKFLAG_OBJECT },
	{ "cylinder_map", FN_CYLINDER_MAP, 0 },
	{ "define", TK_DEFINE, 0 },
	{ "difference", TK_DIFFERENCE, TKFLAG_OBJECT },
	{ "diffuse", TK_DIFFUSE, 0 },
	{ "dir", TK_DIR, 0 },
	{ "disc", TK_DISC, TKFLAG_OBJECT },
	{ "do", TK_DO, 0 },
	{ "else", TK_ELSE, 0 },
	{ "exp", FN_EXP, 0 },
	{ "extrude", TK_EXTRUDE, 0 },
	{ "falloff", TK_FALLOFF, 0 },
	{ "float", TK_FLOAT, 0 },
	{ "floor", FN_FLOOR, 0 },
	{ "fn_xyz", TK_FN_XYZ, TKFLAG_OBJECT },
	{ "for", TK_FOR, 0 },
	{ "frand", FN_FRAND, 0 },
	{ "function", TK_FUNCTION, 0 },
/*	{ "get_color", FN_GET_COLOR, TKFLAG_SHADER_FN },
	{ "get_obj_pt", FN_GET_OBJ_PT, TKFLAG_SHADER_FN },
	{ "get_obj_u", FN_GET_OBJ_U, TKFLAG_SHADER_FN },
	{ "get_obj_v", FN_GET_OBJ_V, TKFLAG_SHADER_FN },
	{ "get_ray_dir", FN_GET_RAY_DIR, TKFLAG_SHADER_FN },
	{ "get_ray_dist", FN_GET_RAY_DIST, TKFLAG_SHADER_FN },
	{ "get_ray_org", FN_GET_RAY_ORG, TKFLAG_SHADER_FN },
	{ "get_world_pt", FN_GET_WORLD_PT, TKFLAG_SHADER_FN }, */
	{ "goto", TK_GOTO, 0 },
	{ "height_field", TK_HEIGHT_FIELD, TKFLAG_OBJECT },
	{ "hexagon", FN_HEXAGON, 0 },
	{ "hexagon2", FN_HEXAGON2, 0 },
	{ "if", TK_IF, 0 },
	{ "image_map", FN_IMAGE_MAP, 0 },
	{ "include_file_paths", TK_INCLUDE_FILE_PATHS, 0 },
	{ "infinite_light", TK_INFINITE_LIGHT, 0 },
	{ "int", FN_INT, 0 },
	{ "intersection", TK_INTERSECTION, TKFLAG_OBJECT },
	{ "inverse", TK_INVERSE, 0 },
	{ "ior", TK_IOR, 0 },
	{ "irand", FN_IRAND, 0 },
	{ "jitter", TK_JITTER, 0 },
	{ "legendre", FN_LEGENDRE, 0 },
	{ "lerp", FN_LERP, 0 },
	{ "light", TK_LIGHT, 0 },
	{ "load_color_map", TK_LOAD_COLOR_MAP, 0 },
	{ "location", TK_LOCATION, 0 },
	{ "log", FN_LOG, 0 },
	{ "log10", FN_LOG10, 0 },
	{ "main", TK_MAIN, 0 },
	{ "message", TK_MESSAGE, 0 },
	{ "meta", TK_BLOB, TKFLAG_OBJECT },
	{ "no_shadow", TK_NO_SHADOW, 0 },
	{ "no_specular", TK_NO_SPECULAR, 0 },
	{ "noise", FN_NOISE, 0 },
	{ "npolygon", TK_NPOLYGON, TKFLAG_OBJECT },
	{ "nurbs", TK_NURBS, TKFLAG_OBJECT },
	{ "object", TK_OBJECT, TKFLAG_OBJECT },
	{ "outior", TK_OUTIOR, 0 },
	{ "path", TK_PATH, 0 },
	{ "plane", TK_PLANE, 0 },
	{ "point", TK_POINT, 0 },
	{ "polygon", TK_POLYGON, TKFLAG_OBJECT },
	{ "polymesh", TK_POLYMESH, TKFLAG_OBJECT },
	{ "private", TK_PRIVATE, 0 },
	{ "public", TK_PUBLIC, 0 },
	{ "radius", TK_RADIUS, 0 },
	{ "reflection", TK_REFLECTION, 0 },
	{ "rel_step", TK_REL_STEP, 0 },
	{ "repeat", TK_REPEAT, 0 },
	{ "return", TK_RETURN, 0 },
	{ "rot_step", TK_ROT_STEP, 0 },
	{ "rotate", TK_ROTATE, 0 },
	{ "round", FN_ROUND, 0 },
	{ "scale", TK_SCALE, 0 },
//	{ "set_color", FN_SET_COLOR, TKFLAG_SHADER_FN },
	{ "sin", FN_SIN, 0 },
	{ "sinh", FN_SINH, 0 },
	{ "size", TK_SIZE, 0 },
	{ "smooth_height_field", TK_SMOOTH_HEIGHT_FIELD, TKFLAG_OBJECT },
	{ "smooth_image_map", FN_SMOOTH_IMAGE_MAP, 0 },
	{ "smooth_polymesh", TK_SMOOTH_POLYMESH, TKFLAG_OBJECT },
	{ "specular", TK_SPECULAR, 0 },
	{ "sphere", TK_SPHERE, TKFLAG_OBJECT },
	{ "spot", TK_SPOT, 0 },
	{ "sqrt", FN_SQRT, 0 },
	{ "surface", TK_SURFACE, 0 },
	{ "surface_shader", TK_SURFACE_SHADER, 0 },
	{ "tan", FN_TAN, 0 },
	{ "tanh", FN_TANH, 0 },
	{ "torus", TK_TORUS, TKFLAG_OBJECT },
	{ "translate", TK_TRANSLATE, 0 },
	{ "transmission", TK_TRANSMISSION, 0 },
	{ "triangle", TK_TRIANGLE, TKFLAG_OBJECT },
	{ "turb", FN_TURB, 0 },
	{ "turb2", FN_TURB2, 0 },
	{ "turn", TK_TURN, 0 },
	{ "two_sides", TK_TWO_SIDES, 0 },
/*	{ "u", RT_U, 0 }, */
	{ "union", TK_UNION, TKFLAG_OBJECT },
/*	{ "uscreen", RT_USCREEN, 0 },
	{ "v", RT_V, 0 }, */
	{ "vcross", FN_VCROSS, 0 },
	{ "vdot", FN_VDOT, 0 },
	{ "vector", TK_VECTOR, 0 },
	{ "vertex", TK_VERTEX, 0 },
	{ "viewport", TK_VIEWPORT, 0 },
	{ "visibility", TK_VISIBILITY, 0 },
	{ "vlerp", FN_VLERP, 0 },
	{ "vmag", FN_VMAG, 0 },
	{ "vnoise", FN_VNOISE, 0 },
	{ "vnorm", FN_VNORM, 0 },
	{ "vrand", FN_VRAND, 0 },
	{ "vrotate", FN_VROTATE, 0 },
/*	{ "vscreen", RT_VSCREEN, 0 }, */
	{ "vturb", FN_VTURB, 0 },
	{ "vturb2", FN_VTURB2, 0 },
	{ "while", TK_WHILE, 0 },
	{ "wrinkle", FN_WRINKLE, 0 },
	{ "x", RT_X, 0 },
	{ "y", RT_Y, 0 },
	{ "z", RT_Z, 0 },
	{ "", THE_END, 0 }
};

TOKEN misc_token;

/*
 * Conditional modes for special cases, such as getting
 * a new id for a declaration.
 */
enum
{
	GETTOKEN_NORMAL = 0,
	GETTOKEN_NEWID
};
static int gettoken_mode = GETTOKEN_NORMAL;

static FILEDATA *fdp;
static int last_line;
static long fpos;
static TOKEN *last_token;

int gettoken_Init(const char *fname)
{
	if (!file_Init())
		return 0;
	if ((fdp = file_OpenMain(fname)) == NULL)
		return 0;

	g_cur_token = &misc_token;
	misc_token.token = TK_EOF;

	/* Locate the last token in the list. */
	for (last_token = tokens;
		last_token->token != THE_END;
		last_token++)
			;  /* Just inc through the list... */

	return 1;
}

void gettoken_Close(void)
{
	file_Close();
}

static int Parse_Quoted_String(FILEDATA *fdp)
{
	char *b;
	int i, c, lastc, lastl;
	long fps;
	FILE *fp = fdp->fp;

	do
	{
		c = fgetc(fp);
		if (c == '\n')
			fdp->line_num++; /* update the line counter */
	}
	while (isspace(c));

	if (c != '"')
	{
		logerror("Expecting string literal - \"string\"");
		file_PrintFileAndLineNumber();
		return 0;
	}
	
	b = g_token_buffer;
	for (i = 0; i < MAX_TOKEN_LEN; i++)
	{
		lastc = c;
		c = fgetc(fp);
		if ((c == '\n')&&(lastc == '\\')) /* Backslash contiuation char. */
		{
			i--;
			b--; /* spit out the backslash */
			fdp->line_num++; /* update the line counter */
			c = fgetc(fp); /* get next char after newline */
		}
		if (c == '"') /* Closing '"' reached... */
		{ /* If there are any more quoted strings following... */
		  /* ...concatenate them. */
			fps = ftell(fp);
			lastl = fdp->line_num;
			do /* gobble up white space */
			{
				c = fgetc(fp);
				if (c == '\n')
					fdp->line_num++; /* update the line counter */
			}
			while (isspace(c));
			if (c != '"')   /* if no opening quote, push back and return */
			{
				fseek(fp, fps, SEEK_SET);
				fdp->line_num = lastl;
				*b = '\0';
				return 1;
			}
			/* else, segue into next string */
			c = fgetc(fp);
		}
		if ((c == '\n')||(c == EOF))
			break;
		*b++ = (char)c;
	}
	if (c != '"')
	{
		if (i == MAX_TOKEN_LEN)
		{
			g_token_buffer[MAX_TOKEN_LEN-1] = '\0';
			logerror("String literal too long - %d characters max",
				MAX_TOKEN_LEN);
			file_PrintFileAndLineNumber();
			return 0;
		}
		*b = '\0';
		switch (c)
		{
			case '\n':
				logerror("Unterminated string literal, end of line reached.");
				logmsg("  Expecting closing  \"");
				file_PrintFileAndLineNumber();
				fdp->line_num++; /* update the line counter */
				return 0;
			case EOF:
				logerror("Unterminated string literal, end of file reached.");
				logmsg("  Expecting closing  \"");
				file_PrintFileAndLineNumber();
				return 0;
		}
	}
	*b = '\0';
	return 1;
}

/*
 * Get a token as usual. If it is not a regular keyword or directive
 * check only in current context for user-declarations before returning
 * TK_UNKNOWN_ID
 */
int gettoken_GetNewIdentifier(void)
{
	int token, prevmode = gettoken_mode;
	gettoken_mode = GETTOKEN_NEWID;
	token = gettoken();
	gettoken_mode = prevmode;
	return token;
}

int gettoken(void)
{
	int c, c2, start_line, nest_level, result;
	int token = TK_EOF;
	char *b;
	TOKEN *t, *lo, *hi;
	FILE *fp;

	g_cur_token = &misc_token;
	misc_token.flags = 0;

	if (fdp == NULL)
	{
		token = TK_EOF;
		goto done;
	}

	fp = fdp->fp;

	for (;;)
	{
		/* Bookmark previous position for gettoken_Unget(). */
		fpos = ftell(fp);
		last_line = fdp->line_num;

		/* Gobble up white space and count lines... */
		do
		{
			c = fgetc(fp);
			if (c == '\n')
				fdp->line_num++;
		}
		while (isspace(c));

		/* Gobble up comments... */
		if (c == '/')
		{
			c2 = fgetc(fp);

			if (c2 == '/')      /* ANSI C++ style single-line comment. */
			{
				do
				{
					if ((c = fgetc(fp)) == EOF)
					{
						token = TK_EOF;
						goto done;
					}
				}
				while (c != '\n');
					fdp->line_num++;
				continue;
			}
			else if (c2 == '*') /* C style multi-line comment. */
			{
				/* Remember where we started in case of error. */
				start_line = fdp->line_num;
				nest_level = 0;
				for (;;)
				{
					/* Entering nested comment? */
					if ((c = fgetc(fp)) == '/')
						if ((c = fgetc(fp)) == '*')
							nest_level++;
					/* End of outer most comment? */
					if (c == '*')
						if ((c = fgetc(fp)) == '/')
							if (nest_level-- < 1)
								break;
					if (c == '\n')
						fdp->line_num++;
					if (feof(fp))
					{
						logerror("Unexpected end of file.");
						logmsg("  Unterminated comment started on line: %d",
						start_line);
						file_PrintFileAndLineNumber();
						break;
					}
				}
				continue;
			}
			ungetc(c2, fp);
		}

		/* Stuff from this point on will be buffered... */
		b = g_token_buffer;

		/* See if we have an identifier name... */
		if (isalpha(c) || c == '_')
		{
			do
			{
				*b++ = (char)c;
				c = fgetc(fp);
			}
			while (isalnum(c) || c == '_');
			ungetc(c, fp);
			*b = '\0';

			/* See if it is a user-defined symbol... */
			g_cur_token = pcontext_findsymbol(
				g_token_buffer,
				(gettoken_mode == GETTOKEN_NEWID));
			if (g_cur_token != NULL)
			{
				token = g_cur_token->token;
				goto done;
			}

			/* See if it is the "include" directive... */
			if (strcmp(g_token_buffer, "include") == 0)
			{
				if (Parse_Quoted_String(fdp))
				{
					fdp = file_OpenInclude(g_token_buffer);
					fp = fdp->fp;
					continue;
				}
				else
				{
					logerror("include: Quoted file name expected.");
					file_PrintFileAndLineNumber();
				}
			}

			/* See if it is a keyword... */
			lo = tokens;
			hi = last_token;
			for (;;)
			{
				if (lo >= hi)
				{
					t = last_token;
					break;
				}
				t = lo + (unsigned)(hi - lo) / 2;
				if ((result = strcmp(t->name, g_token_buffer)) > 0)
					hi = t;
				else if (result < 0)
					lo = t + 1;
				else if (result == 0)   /* match found */
				{
					// These are recognized only in the fn_xyz object.
					//
					if ((t->token > RT_RT_BEGIN) &&
						(t->token < RT_RT_END) &&
						(pcontext_getobjtype() != TK_FN_XYZ))
						break;

					g_cur_token = t;
					token = t->token;
					goto done;
				}
			}

			g_cur_token = &misc_token;

			token = TK_UNKNOWN_ID;
			goto done;
		}

		/* See if we have a number... */
		if (isdigit(c) || c == '.')
		{
			int dp_count, exp_count;

			dp_count = exp_count = 0;
			if (c == '.')
			{
				*b++ = (char)c;
				dp_count++;
				c = fgetc(fp);
			}
			while (isdigit(c))
			{
				*b++ = (char)c;
				c = fgetc(fp);
				if (c == '.' && dp_count == 0)
				{
					*b++ = (char)c;
					dp_count++;
					c = fgetc(fp);
				}
				if (toupper(c) == 'E' && exp_count == 0)
				{
					*b++ = (char)c;
					c = fgetc(fp);
					exp_count++;
					dp_count++;
					if (c == '-')
					{
						*b++ = (char)c;
						c = fgetc(fp);
					}
				}
			}
			*b = '\0';
			ungetc(c, fp);
			token = (dp_count == 0) ? CV_INT_CONST :
				(strlen(g_token_buffer) > 1) ? CV_FLOAT_CONST : OP_DOT;
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
						/*	(c2 == '+') ? OP_INCREMENT : */
						(ungetc(c2, fp), *b = '\0', OP_PLUS);
						goto done;
					case '-':
						token = (c2 == '=') ? OP_MINUSASSIGN :
						/*	(c2 == '-') ? OP_DECREMENT : */
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
			case ';':
				token = OP_SEMICOLON;
				goto done;
			case '"':
				ungetc(c, fp);
				Parse_Quoted_String(fdp);
				token = TK_QUOTESTRING;
				goto done;
		}


		/* end of file? */
		if (feof(fp))
		{
			if ((fdp = file_CloseInclude()) != NULL)
			{
				fp = fdp->fp;
				continue;
			}
			fdp = NULL;
			token = TK_EOF;
			break;
		}

		/* Unknown non-alpha character. */
		token = TK_UNKNOWN_CHAR;
		break;
	}

	done:

	misc_token.token = token;
	if (token == TK_EOF)
		strcpy(g_token_buffer, "End Of File");

	return token;
}

/*************************************************************************
*
*	gettoken_Unget() - Undo the last call to gettoken().
*	Restores the stream
*	position and line number to the values saved by gettoken() before
*	it reads from the stream.
*
*************************************************************************/
void gettoken_Unget(void)
{
	if (fdp != NULL)
	{
		fseek(fdp->fp, fpos, SEEK_SET);
		fdp->line_num = last_line;
	}
}


/*************************************************************************
*
*	gettoken_Expect() - Get a token.
*	If it doesn't match "expected_token" report an error.
*
*************************************************************************/
int gettoken_Expect(
	int				expected_token,
	const char *	token_name
	)
{
	int		token;

	token = gettoken();
	if (token != expected_token)
	{
		logerror("%s: '%s' expected. Found '%s'",
			ME, token_name, g_token_buffer);
		file_PrintFileAndLineNumber();
	}

	return token;
}


/*************************************************************************
*
*	gettoken_ErrUnknown - Error message for unknown, or out of context, token.
*	The name of the current block on the context stack is used to
*	identify the origin of the message.
*	If "expected" is not NULL, "'expected' expected" is appended
*	to message.
*
*************************************************************************/
void gettoken_ErrUnknown(int token, const char *expected)
{
	if (token == TK_EOF)
	{
		logerror("%s: Unexpected End Of File.",
			ME);
	}
	else
	{
		logerror("%s: Unknown or misplaced token '%s'.",
			ME, g_token_buffer);
	}

	if (expected != NULL)
		logmsg("  '%s' expected.", expected);

	file_PrintFileAndLineNumber();
}


/*************************************************************************
*
*	gettoken_Error - General purpose error message.
*
*************************************************************************/
void gettoken_Error(const char *who, const char *what)
{
	logerror("%s: %s", who, what);
	file_PrintFileAndLineNumber();
}
