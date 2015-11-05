/*************************************************************************
*
*  exparse.c - Parses and contructs an expression parse tree.
*
*************************************************************************/

#include "local.h"

static Expr *Term0(void);
static Expr *Term1(void);
static Expr *Term2(void);
static Expr *Term3(void);
static Expr *Term4(void);
static Expr *Term5(void);
static Expr *Term6(void);
static Expr *Term7(void);
static Expr *Term8(void);
static Expr *Term9(void);
static Expr *Term10(void);
static Expr *Term11(void);
static Expr *Term12(void);
static Expr *Term13(void);
static Expr *Term14(void);
static Expr *Term15(void);
static Expr *Atom(void);

static void CheckLValue(Expr *expr, const char *opname);
static void CheckRValue(Expr *expr, const char *opname);
static void CheckParamCount(Expr *expr, const char *fnname, int cnt);
static Expr *MakeFunc(void (*fn)(Expr *expr), int nparams, int returns_vec);
static Expr *MakeHandleFunc(void (*fn)(Expr *expr), int handle_token, 
	int nparams, int returns_vec);
static Expr *MakeImageMapFunc(void (*fn)(Expr *expr));
static Expr *MakeVec(void);

static int token;
static int vector_closing;

#define EXPR_NEW(e) if((e = ExprNew()) == NULL) return NULL

/*************************************************************************
*
*  ExprParse - Parses and contructs an expression parse tree.
*
*  Returns a pointer to top of expr tree or NULL if there's no
*    expr or an error occurred.
*
*************************************************************************/
Expr *ExprParse(void)
{
	Expr *expr;
	int errcnt = error_count;
	token = GetToken();
	vector_closing = 0;
	expr = Term1();
	UngetToken();
	if(error_count > errcnt)
	{
		ExprDelete(expr);
		return NULL;
	}
	return expr;
}

Expr *ExprParseLVInitializer(LValue *lv)
{
	Expr *expr = ExprNew();
	if((expr != NULL) && ((expr->l = ExprNew()) != NULL))
	{
		if((expr->r = ExprParse()) != NULL)
		{
			expr->l->data = LValueCopy(lv);
			expr->l->fn = eval_lvalue;
			expr->fn = eval_assign;
		}
		else
		{
			ExprDelete(expr);
			expr = NULL;
		}
	}
	else
		LogMemError("expr");
	return expr;
}

/*************************************************************************
*
*  Recursive-descent parser that defines level of precedence for
*    the various operators, where Term0() is the lowest precedence
*    operator, Term1() the next operator up the precedence ladder and
*    so on. Atom() parses operands (ie: constants, variables, etc.).
*
*************************************************************************/
/* , */
Expr *Term0(void)
{
	Expr *expr, *l;
	int old_vector_closing = vector_closing;
	vector_closing = 0;
	expr = Term1();
	while(token == OP_COMMA)
	{
		l = expr;
		expr = ExprNew();
		expr->fn = eval_comma;
		expr->l = l;
		token = GetToken();
		expr->r = Term1();
		CheckLValue(expr->l, ",");
		CheckRValue(expr->r, ",");
	}
	vector_closing = old_vector_closing;
	return expr;
}

/* =, +=, -=, *=, /=, %=, &=, ^=, |=, <<=, >>= */
Expr *Term1(void)
{
	int op;
	Expr *l, *expr;
	expr = Term2();
	while((op = token) == OP_ASSIGN)
	{
		if((expr == NULL) || (expr->fn != eval_lvalue))
		{
			LogError("expression syntax: Variable expected on left side of %s.",
				token_buffer);
			PrintFileAndLineNumber();
		}
		l = expr;
		expr = ExprNew();
		expr->l = l;
		token = GetToken();
		expr->r = Term1();
		switch(op)
		{
			case OP_ASSIGN:
				expr->fn = eval_assign;
				CheckRValue(expr->r, "=");
				break;
			default:
				expr->fn = eval_assign;
				CheckRValue(expr->r, "=");
				break;
		}
	}
	return expr;
}

/* ?: */
Expr *Term2(void)
{
	Expr *expr, *l;
	expr = Term3();
	while(token == OP_QUESTION)
	{
		l = expr;
		expr = ExprNew();
		expr->fn = eval_ternary;
		expr->l = l;
		token = GetToken();
		expr->r = ExprNew();
		expr->r->l = Term2();
		if(token == OP_COLON)
		{
			token = GetToken();
			expr->r->r = Term2();
		}
		else
		{
			ErrUnknown(token, ":", "expression syntax");
		}
	}
	return expr;
}

/* || */
Expr *Term3(void)
{
	Expr *l, *expr;
	expr = Term4();
	while(token == OP_OROR)
	{
		l = expr;
		expr = ExprNew();
		expr->l = l;
		token = GetToken();
		expr->r = Term4();
		expr->fn = eval_logicor;
		CheckLValue(expr->l, "||");
		CheckRValue(expr->r, "||");
	}
	return expr;
}

/* && */
Expr *Term4(void)
{
	Expr *l, *expr;
	expr = Term5();
	while(token == OP_ANDAND)
	{
		l = expr;
		expr = ExprNew();
		expr->l = l;
		token = GetToken();
		expr->r = Term5();
		expr->fn = eval_logicand;
		CheckLValue(expr->l, "&&");
		CheckRValue(expr->r, "&&");
	}
	return expr;
}

/* | */
Expr *Term5(void)
{
	Expr *l, *expr;
	expr = Term6();
	while(token == OP_OR)
	{
		l = expr;
		expr = ExprNew();
		expr->l = l;
		token = GetToken();
		expr->r = Term6();
		expr->fn = eval_bitor;
		CheckLValue(expr->l, "|");
		CheckRValue(expr->r, "|");
	}
	return expr;
}

/* & */
Expr *Term6(void)
{
	Expr *l, *expr;
	expr = Term7();
	while(token == OP_AND)
	{
		l = expr;
		expr = ExprNew();
		expr->l = l;
		token = GetToken();
		expr->r = Term7();
		expr->fn = eval_bitand;
		CheckLValue(expr->l, "&");
		CheckRValue(expr->r, "&");
	}
	return expr;
}

/* ==, != */
Expr *Term7(void)
{
	int op;
	Expr *l, *expr;
	expr = Term8();
	while(((op = token) == OP_EQUAL) || (op == OP_NOTEQUAL))
	{
		l = expr;
		expr = ExprNew();
		expr->l = l;
		token = GetToken();
		expr->r = Term8();
		switch(op)
		{
			case OP_EQUAL:
				expr->fn = eval_isequal;
				CheckLValue(expr->l, "==");
				CheckRValue(expr->r, "==");
				break;
			default:
				expr->fn = eval_isnotequal;
				CheckLValue(expr->l, "!=");
				CheckRValue(expr->r, "!=");
				break;
		}
	}
	return expr;
}

/* <, <=, >, >= */
Expr *Term8(void)
{
	int op;
	Expr *l, *expr;
	expr = Term9();
	while(((op = token) == OP_LESSTHAN) ||
	      ((op == OP_GREATERTHAN) && (vector_closing == 0)) ||
	       (op == OP_LESSEQUAL) || (op == OP_GREATEQUAL))
	{
		l = expr;
		expr = ExprNew();
		expr->l = l;
		token = GetToken();
		expr->r = Term9();
		switch(op)
		{
			case OP_LESSTHAN:
				expr->fn = eval_lessthan;
				CheckLValue(expr->l, "<");
				CheckRValue(expr->r, "<");
				break;
			case OP_GREATERTHAN:
				expr->fn = eval_greaterthan;
				CheckLValue(expr->l, ">");
				CheckRValue(expr->r, ">");
				break;
			case OP_LESSEQUAL:
				expr->fn = eval_lessequal;
				CheckLValue(expr->l, "<=");
				CheckRValue(expr->r, "<=");
				break;
			default:
				expr->fn = eval_greatequal;
				CheckLValue(expr->l, ">=");
				CheckRValue(expr->r, ">=");
				break;
		}
	}
	return expr;
}

/* <<, >> */
Expr *Term9(void)
{
	return Term10();
}

/* +, - */
Expr *Term10(void)
{
	int op;
	Expr *l, *expr;
	expr = Term11();
	while(((op = token) == OP_PLUS) || (op == OP_MINUS))
	{
		l = expr;
		expr = ExprNew();
		expr->l = l;
		token = GetToken();
		expr->r = Term11();
		switch(op)
		{
			case OP_PLUS:
				expr->fn = eval_plus;
				CheckLValue(expr->l, "+");
				CheckRValue(expr->r, "+");
				break;
			default:
				expr->fn = eval_minus;
				CheckLValue(expr->l, "-");
				CheckRValue(expr->r, "-");
				break;
		}
	}
	return expr;
}

/* *, /, % */
Expr *Term11(void)
{
	int op;
	Expr *l, *expr;
	expr = Term12();
	while(((op = token) == OP_MULT) || (op == OP_DIVIDE) || (op == OP_MOD))
	{
		l = expr;
		expr = ExprNew();
		expr->l = l;
		token = GetToken();
		expr->r = Term12();
		switch(op)
		{
			case OP_MULT:
				expr->fn = eval_multiply;
				CheckLValue(expr->l, "*");
				CheckRValue(expr->r, "*");
				break;
			case OP_DIVIDE:
				expr->fn = eval_divide;
				CheckLValue(expr->l, "/");
				CheckRValue(expr->r, "/");
				break;
			default:
				expr->fn = eval_mod;
				CheckLValue(expr->l, "%");
				CheckRValue(expr->r, "%");
				break;
		}
	}
	return expr;
}

/* ^ */
Expr *Term12(void)
{
	Expr *l, *expr;
	expr = Term13();
	while(token == OP_POW)
	{
		l = expr;
		expr = ExprNew();
		expr->fn = eval_pow;
		expr->l = l;
		token = GetToken();
		expr->r = Term13();
		CheckLValue(expr->l, "^");
		CheckRValue(expr->r, "^");
	}
	return expr;
}

/* ! unary +, unary -, ~ */
Expr *Term13(void)
{
	int op = token;
	Expr *expr;
	switch(op)
	{
		case OP_PLUS:
			token = GetToken();
			expr = Term13();
			CheckRValue(expr, "+");
			break;
		case OP_MINUS:
			expr = ExprNew();
			expr->fn = eval_uminus;
			token = GetToken();
			expr->r = Term13();
			CheckRValue(expr->r, "-");
			break;
		case OP_NOT:
			expr = ExprNew();
			expr->fn = eval_logicnot;
			token = GetToken();
			expr->r = Term13();
			CheckRValue(expr->r, "!");
			break;
		default:
			return Term14();
	}
	return expr;
}

/* . */
Expr *Term14(void)
{
	Expr *l, *expr;
	expr = Term15();
	while(token == OP_DOT)
	{
		if(expr != NULL && expr->isvec) /* Vector member access. */
		{
			l = expr;
			expr = ExprNew();
			expr->l = l;
			token = GetToken();
			switch(token)
			{
				case RT_X:
					expr->fn = eval_dot_x;
					break;
				case RT_Y:
					expr->fn = eval_dot_y;
					break;
				case RT_Z:
					expr->fn = eval_dot_z;
					break;
				default:
					LogError("%s is not a vector member.", token_buffer);
					PrintFileAndLineNumber();
					break;
			}
		}
		else /* Nothing to access a member from. */
			ErrUnknown(token, ")", "expression syntax");
		token = GetToken();
	}
	return expr;
}

/* (), fn(), [], <x, y, z>, . */
Expr *Term15(void)
{
	switch(token) 
	{
		/* 0 parameter functions returning floats */
		case FN_FRAND:
			return MakeFunc(eval_frand, 0, 0);

		/* 0 parameter functions returning vectors */
		case FN_VRAND:
			return MakeFunc(eval_vrand, 0, 1);

		/* 1 parameter functions returning floats */
		case FN_ABS:
			return MakeFunc(eval_abs, 1, 0);
		case FN_ACOS:
			return MakeFunc(eval_acos, 1, 0);
		case FN_ASIN:
			return MakeFunc(eval_asin, 1, 0);
		case FN_ATAN:
			return MakeFunc(eval_atan, 1, 0);
		case FN_CEIL:
			return MakeFunc(eval_ceil, 1, 0);
		case FN_CHECKER:
			return MakeFunc(eval_checker, 1, 0);
		case FN_COS:
			return MakeFunc(eval_cos, 1, 0);
		case FN_COSH:
			return MakeFunc(eval_cosh, 1, 0);
		case FN_EXP:
			return MakeFunc(eval_exp, 1, 0);
		case FN_FLOOR:
			return MakeFunc(eval_floor, 1, 0);
		case FN_INT:
			return MakeFunc(eval_int, 1, 0);
		case FN_IRAND:
			return MakeFunc(eval_irand, 1, 0);
		case FN_LOG:
			return MakeFunc(eval_log, 1, 0);
		case FN_LOG10:
			return MakeFunc(eval_log10, 1, 0);
		case FN_NOISE:
			return MakeFunc(eval_noise, 1, 0);
		case FN_ROUND:
			return MakeFunc(eval_round, 1, 0);
		case FN_SIN:
			return MakeFunc(eval_sin, 1, 0);
		case FN_SINH:
			return MakeFunc(eval_sinh, 1, 0);
		case FN_SQRT:
			return MakeFunc(eval_sqrt, 1, 0);
		case FN_TAN:
			return MakeFunc(eval_tan, 1, 0);
		case FN_TANH:
			return MakeFunc(eval_tanh, 1, 0);
		case FN_VMAG:
			return MakeFunc(eval_vmag, 1, 0);

		/* 1 parameter functions returning vectors */
		case FN_VNOISE:
			return MakeFunc(eval_vnoise, 1, 1);
		case FN_VNORM:
			return MakeFunc(eval_vnorm, 1, 1);

		/* 2 parameter functions returning floats */
		case FN_ATAN2:
			return MakeFunc(eval_atan2, 2, 0);
		case FN_HEXAGON:
			return MakeFunc(eval_hexagon, 2, 0);
		case FN_TURB:
			return MakeFunc(eval_turb, 2, 0);
		case FN_VDOT:
			return MakeFunc(eval_vdot, 2, 0);

		/* 2 parameter functions returning vectors */
		case FN_COLOR_MAP:
			return MakeHandleFunc(eval_color_map,
				DECL_COLOR_MAP, 1, 1);
		case FN_IMAGE_MAP:
			return MakeImageMapFunc(eval_image_map);
		case FN_SMOOTH_IMAGE_MAP:
			return MakeImageMapFunc(eval_smooth_image_map);
		case FN_VCROSS:
			return MakeFunc(eval_vcross, 2, 1);
		case FN_VTURB:
			return MakeFunc(eval_vturb, 2, 1);
		case FN_WRINKLE:
			return MakeFunc(eval_wrinkle, 2, 1);

		/* 3 parameter functions returning floats */
		case FN_CLAMP:
			return MakeFunc(eval_clamp, 3, 0);
		case FN_LEGENDRE:
			return MakeFunc(eval_legendre, 3, 0);
		case FN_LERP:
			return MakeFunc(eval_lerp, 3, 0);

		/* 3 parameter functions with indeterminate return type */
		case FN_CHECKER2:
			return MakeFunc(eval_checker2, 3, 0);

		/* 3 parameter functions returning vectors */
		case FN_VROTATE:
			return MakeFunc(eval_vrotate, 3, 1);
		case FN_VLERP:
			return MakeFunc(eval_vlerp, 3, 1);

		/* 4 parameter functions returning floats */
		case FN_TURB2:
			return MakeFunc(eval_turb2, 4, 0);

		/* 4 parameter functions returning vectors */
		case FN_VTURB2:
			return MakeFunc(eval_vturb2, 4, 1);

		/* 5 parameter functions with indeterminate return type */
		case FN_HEXAGON2:
			return MakeFunc(eval_hexagon2, 5, 0);

		case OP_LPAREN:
		{
			Expr *expr;
			token = GetToken();
			expr = Term0();
			if(token == OP_RPAREN)
				token = GetToken();
			else
				ErrUnknown(token, ")", "expression syntax");
			return expr;
		}
		break;

		case OP_LESSTHAN:  /* starting a vector triplet */
			return MakeVec();

		default:
			break;
	}
	return Atom();
}

Expr *Atom(void)
{
	Expr *expr = ExprNew();

	switch(token)
	{
		case CV_FLOAT_CONST:
		case CV_INT_CONST:
			expr->v.x = atof(token_buffer);
			break;
		case CV_PI_CONST:
			expr->v.x = PI;
			break;

		case DECL_FLOAT:
		case DECL_VECTOR:
			expr->data = (void *)LValueCopy((LValue *)cur_token->data);
			expr->fn = eval_lvalue;
			expr->isvec = (token == DECL_VECTOR) ? 1 : 0;
			break;

		case RT_D: /* Ray direction "D" variable. */
			expr->data = (void *)&rt_D;
			expr->fn = eval_rtvec;
			expr->isvec = 1;
			break;
		case RT_N: /* World normal "N" variable. */
			expr->data = (void *)&rt_WN;
			expr->fn = eval_rtvec;
			expr->isvec = 1;
			break;
		case RT_O: /* Object point hit "O" variable. */
			expr->data = (void *)&rt_O;
			expr->fn = eval_rtvec;
			expr->isvec = 1;
			break;
		case RT_ON: /* Object normal "N" variable. */
			expr->data = (void *)&rt_ON;
			expr->fn = eval_rtvec;
			expr->isvec = 1;
			break;
		case RT_USCREEN: /* "uscreen" variable. */
			expr->data = (void *)&rt_uscreen;
			expr->fn = eval_rtfloat;
			break;
		case RT_VSCREEN: /* "vscreen" variable. */
			expr->data = (void *)&rt_vscreen;
			expr->fn = eval_rtfloat;
			break;
		case RT_U: /* Object "u" variable. */
			expr->data = (void *)&rt_u;
			expr->fn = eval_rtfloat;
			break;
		case RT_V: /* Object "v" variable. */
			expr->data = (void *)&rt_v;
			expr->fn = eval_rtfloat;
			break;
		case RT_X: /* Object "x" variable. */
			expr->data = (void *)&rt_O.x;
			expr->fn = eval_rtfloat;
			break;
		case RT_Y: /* Object "y" variable. */
			expr->data = (void *)&rt_O.y;
			expr->fn = eval_rtfloat;
			break;
		case RT_Z: /* Object "z" variable. */
			expr->data = (void *)&rt_O.z;
			expr->fn = eval_rtfloat;
			break;

		default:
			ExprDelete(expr);
			return NULL;
	}

	token = GetToken();
	
	return expr;
}

/*************************************************************************
*
*  Utility functions.
*
*************************************************************************/

Expr *ExprNew(void)
{
	Expr *expr = (Expr *)calloc(1, sizeof(Expr));
	if(expr != NULL)
	{
		expr->l = expr->r = NULL;
		expr->fn = eval_const;
		expr->data = NULL;
		expr->isvec = 0;
	}
	else
		LogMemError("expression");
	return expr;
}

void ExprDelete(Expr *expr)
{
	if(expr != NULL)
	{
		ExprDelete(expr->l);
		ExprDelete(expr->r);
		if(expr->fn == eval_lvalue)
			LValueDelete((LValue *)expr->data);
		else if(expr->fn == eval_color_map)
			ColorMap_Delete((ColorMap *)expr->data);
		else if(expr->fn == eval_image_map ||
			expr->fn == eval_smooth_image_map)
			Delete_Image((Image *)expr->data);
		free(expr);
	}
}

/*************************************************************************
*
*  CheckLValue - Checks for missing r-values. Outputs an error message
*    if r-value is NULL.
*
*************************************************************************/
void CheckLValue(Expr *expr, const char *opname)
{
	if(expr == NULL)
	{
		LogError("expression syntax: "
			"Expecting a value before `%s'.", opname);
		PrintFileAndLineNumber();
	}
}

/*************************************************************************
*
*  CheckRValue - Checks for missing r-values. Outputs an error message
*    if r-value is NULL.
*
*************************************************************************/
void CheckRValue(Expr *expr, const char *opname)
{
	if(expr == NULL)
	{
		LogError("expression syntax: "
			"Expecting a value after `%s'. Found `%s'",
			opname, token_buffer);
		PrintFileAndLineNumber();
	}
}

/*************************************************************************
*
*  CheckParamCount - Counts the number of comma-separated
*    sub-expressions in "expr" which is the comma operator if there is
*    two or more sub-expressions. If there is just one parameter, "expr"
*    is the expression. If no params, "expr" is NULL.
*    If count is not equal to "cnt" an error message is issued.
*
*************************************************************************/
void CheckParamCount(Expr *expr, const char *fnname, int cnt)
{
	int actualcnt, diffcnt;

	actualcnt = 0;
	if(expr != NULL)
	{
		actualcnt++;
		while(expr->fn == eval_comma)
		{
			actualcnt++;
			expr = expr->l;
		}
	}
	diffcnt = actualcnt - cnt;
	if(diffcnt < 0)
	{
		diffcnt = -diffcnt;
		LogError("%s: %d required parameter(s) missing.", fnname, diffcnt);
	}
	else if(diffcnt > 0)
	{
		LogError("%s: %d too many parameter(s).", fnname, diffcnt);
	}
}

Expr *MakeFunc(void (*fn)(Expr *), int nparams, int returns_vec)
{
	Expr *expr = NULL;
	TOKEN *fntoken = cur_token;
	token = GetToken();
	if(token == OP_LPAREN)
	{
		if(nparams < 2)
		{
			expr = ExprNew();
			expr->fn = fn;
			expr->isvec = returns_vec ? 1 : 0;
			token = GetToken();
			expr->l = Term0();
			if(token == OP_RPAREN)
			{
				CheckParamCount(expr->l, fntoken->name, nparams);
				token = GetToken();
			}
			else
				ErrUnknown(token, ")", "expression syntax");
		}
		else
		{
			token = GetToken();
			expr = Term0();
			if(token == OP_RPAREN)
			{
				CheckParamCount(expr, fntoken->name, nparams);
				token = GetToken();
				if(expr != NULL)
				{
					expr->fn = fn;
					expr->isvec = returns_vec ? 1 : 0;
				}
			}
			else
				ErrUnknown(token, ")", "expression syntax");
		}
	}
	else
		ErrUnknown(token, "(", "expression syntax");
	return expr;
}


Expr *MakeImageMapFunc(void (*fn)(Expr *))
{
	Expr *expr = NULL;
	TOKEN *fntoken = cur_token;
	Image *img = NULL;

	token = GetToken();
	if(token == OP_LPAREN)
	{
		/* Get the quoted file name and first comma... */
		if((token = GetToken()) == TK_QUOTESTRING)
		{
			FILE *fp;
			/* Open the file... */
			if((fp = SCN_FindFile(token_buffer, READBIN,
				scn_include_paths, SCN_FINDFILE_CHK_CUR_FIRST)) != NULL)
			{
				/* Load the image... */
				img = Image_Load(fp, token_buffer); 
				fclose(fp);
				if(img == NULL)
				{
					LogError("Unable to load image file: %s", token_buffer);
					PrintFileAndLineNumber();
				}
			}
			else
			{
				LogError("Unable to open image file: %s", token_buffer);
				PrintFileAndLineNumber();
			}
		}
		else
			ErrUnknown(token, "expecting image file name",
				"expression syntax");
		
		/* Eat the first comma... */
		if((token = GetToken()) != OP_COMMA)
			ErrUnknown(token, ",", "expression syntax");
		/* Get the U, V expressions and finish. */
		token = GetToken();
		expr = Term0();
		if(token == OP_RPAREN)
		{
			CheckParamCount(expr, fntoken->name, 2);
			token = GetToken();
			if(expr != NULL)
			{
				expr->data = img;
				expr->fn = fn;
				expr->isvec = 1;
			}
		}
		else
			ErrUnknown(token, ")", "expression syntax");
	}
	else
		ErrUnknown(token, "(", "expression syntax");
	return expr;
}

Expr *MakeHandleFunc(void (*fn)(Expr *), int handle_token,
	int nparams, int returns_vec)
{
	Expr *expr = NULL;
	void *data = NULL;
	TOKEN *fntoken = cur_token;
	token = GetToken();
	if(token == OP_LPAREN)
	{
		/*
		 * Look for a handle to a declared type that matches
		 * the type specified by handle_token.
		 */
		token = GetToken();
		if(token == handle_token)
		{
			switch(token)
			{
				case DECL_COLOR_MAP:
					data = (void *)ColorMap_Copy((ColorMap *)cur_token->data);
					break;
			}
		}
		else
		{
			switch(handle_token)
			{
				case DECL_COLOR_MAP:
					ErrUnknown(token, "color map handle id", "expression syntax");
					break;
				default:
					ErrUnknown(token, "handle to a declared item",
						"expression syntax");
					break;
			}
		}
		/* Get the first comma if more params follow. */
		if(nparams > 0)
		{
			token = GetToken();
			if(token != OP_COMMA)
				ErrUnknown(token, ",", "expression syntax");
		}
		if(nparams < 2)
		{
			expr = ExprNew();
			expr->fn = fn;
			expr->data = data;
			expr->isvec = returns_vec ? 1 : 0;
			token = GetToken();
			expr->l = Term0();
			if(token == OP_RPAREN)
			{
				CheckParamCount(expr->l, fntoken->name, nparams);
				token = GetToken();
			}
			else
				ErrUnknown(token, ")", "expression syntax");
		}
		else
		{
			token = GetToken();
			expr = Term0();
			if(token == OP_RPAREN)
			{
				CheckParamCount(expr, fntoken->name, nparams);
				token = GetToken();
				if(expr != NULL)
				{
					expr->fn = fn;
					expr->data = data;
					expr->isvec = returns_vec ? 1 : 0;
				}
				else if(data != NULL)
				{
					switch(token)
					{
						case DECL_COLOR_MAP:
							ColorMap_Delete((ColorMap *)data);
							break;
					}
				}
			}
			else
				ErrUnknown(token, ")", "expression syntax");
		}
	}
	else
		ErrUnknown(token, "(", "expression syntax");
	return expr;
}

Expr *MakeVec(void)
{
	Expr *expr;
	expr = ExprNew();
	expr->r = ExprNew();
	expr->fn = eval_vector;
	expr->isvec = 1;
	token = GetToken();
	if((expr->r->r = Term1()) != NULL)
	{
		if(token == OP_COMMA)
		{
			token = GetToken();
			if((expr->r->l = Term1()) != NULL)
			{
				if(token == OP_COMMA)
				{
					token = GetToken();
					vector_closing++;
					if((expr->l = Term1()) != NULL)
					{
						if(token == OP_GREATERTHAN)
						{
							token = GetToken();
							vector_closing--;
							return expr;
						}
						else
							ErrUnknown(token, ">", "expression syntax");
					}
				}
				else
					ErrUnknown(token, ",", "expression syntax");
			}
		}
		else
			ErrUnknown(token, ",", "expression syntax");
	}
	return expr;
}
