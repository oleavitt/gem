/**
 *****************************************************************************
 *  @file pexpr.c
 *  Expression parsing stuff.
 *
 *****************************************************************************
 */

#include "local.h"



static VMExpr *new_exprtree(void);

double parse_fexpr(void)
{
	double		result;
	VMExpr *	expr;
	
	pcontext_push("expr");
	expr = parse_exprtree();

	if (expr != NULL)
	{
		result = vm_evaldouble(expr);
		delete_exprtree(expr);
	}
	else
		result = 0.0;

	pcontext_pop();

	return result;
}

void parse_vexpr(Vec3 * vresult)
{
	VMExpr *	expr;
	
	pcontext_push("expr");
	expr = parse_exprtree();

	if (expr != NULL)
	{
		vm_evalvector(expr, vresult);
		delete_exprtree(expr);
	}
	else
		V3Zero(vresult);

	pcontext_pop();
}

/*************************************************************************/

static VMExpr *term0(void);
static VMExpr *term1(void);
static VMExpr *term2(void);
static VMExpr *term3(void);
static VMExpr *term4(void);
static VMExpr *term5(void);
static VMExpr *term6(void);
static VMExpr *term7(void);
static VMExpr *term8(void);
static VMExpr *term9(void);
static VMExpr *term10(void);
static VMExpr *term11(void);
static VMExpr *term12(void);
static VMExpr *term13(void);
static VMExpr *term14(void);
static VMExpr *term15(void);
static VMExpr *Atom(void);

static void CheckLValue(VMExpr *expr, const char *opname);
static void CheckRValue(VMExpr *expr, const char *opname);
static void CheckParamCount(VMExpr *expr, const char *fnname, int cnt);
static VMExpr *MakeFunc(void (*fn)(VMExpr *expr), int nparams, int returns_vec);
static VMExpr *MakeHandleFunc(void (*fn)(VMExpr *expr), int handle_token, 
	int nparams, int returns_vec);
static VMExpr *MakeImageMapFunc(void (*fn)(VMExpr *expr));
static VMExpr *MakeVec(void);

static int token;
static int vector_closing;

#define EXPR_NEW(e) if ((e = new_exprtree()) == NULL) return NULL

/*************************************************************************
*
*  parse_exprtree - Parses and contructs an expression parse tree.
*
*  Returns a pointer to top of expr tree or NULL if there's no
*    expr or an error occurred.
*
*************************************************************************/
VMExpr *parse_exprtree(void)
{
	VMExpr *	expr;
	int			errcnt = g_error_count;

	token = gettoken();
	vector_closing = 0;
	expr = term1();
	gettoken_Unget();
	if (g_error_count > errcnt)
	{
		delete_exprtree(expr);
		return NULL;
	}
	return expr;
}



VMExpr *parse_vm_lvalue_expr(VMLValue *lv)
{
	VMExpr *expr = new_exprtree();
	if ((expr != NULL) && ((expr->l = new_exprtree()) != NULL))
	{
		if ((expr->r = parse_exprtree()) != NULL)
		{
			expr->l->data = vm_copy_lvalue(lv);
			expr->l->fn = vmeval_lvalue;
			expr->fn = vmeval_assign;
		}
		else
		{
			delete_exprtree(expr);
			expr = NULL;
		}
	}
	else
		logmemerror("expr");

	return expr;
}



/*************************************************************************
*
*  Recursive-descent parser that defines level of precedence for
*    the various operators, where term0() is the lowest precedence
*    operator, term1() the next operator up the precedence ladder and
*    so on. Atom() parses operands (ie: constants, variables, etc.).
*
*************************************************************************/
/* , */
VMExpr *term0(void)
{
	VMExpr *expr, *l;
	int old_vector_closing = vector_closing;
	vector_closing = 0;
	expr = term1();
	while(token == OP_COMMA)
	{
		l = expr;
		expr = new_exprtree();
		expr->fn = vmeval_comma;
		expr->l = l;
		token = gettoken();
		expr->r = term1();
		CheckLValue(expr->l, ",");
		CheckRValue(expr->r, ",");
	}
	vector_closing = old_vector_closing;
	return expr;
}

/* =, +=, -=, *=, /=, %=, &=, ^=, |=, <<=, >>= */
VMExpr *term1(void)
{
	int op;
	VMExpr *l, *expr;
	expr = term2();
	while((op = token) == OP_ASSIGN)
	{
		if ((expr == NULL) || (expr->fn != vmeval_lvalue))
		{
			logerror("expression syntax: Variable expected on left side of %s.",
				g_token_buffer);
			file_PrintFileAndLineNumber();
		}
		l = expr;
		expr = new_exprtree();
		expr->l = l;
		token = gettoken();
		expr->r = term1();
		switch(op)
		{
			case OP_ASSIGN:
				expr->fn = vmeval_assign;
				CheckRValue(expr->r, "=");
				break;
			default:
				expr->fn = vmeval_assign;
				CheckRValue(expr->r, "=");
				break;
		}
	}
	return expr;
}

/* ?: */
VMExpr *term2(void)
{
	VMExpr *expr, *l;
	expr = term3();
	while(token == OP_QUESTION)
	{
		l = expr;
		expr = new_exprtree();
		expr->fn = vmeval_ternary;
		expr->l = l;
		token = gettoken();
		expr->r = new_exprtree();
		expr->r->l = term2();
		if (token == OP_COLON)
		{
			token = gettoken();
			expr->r->r = term2();
		}
		else
		{
			gettoken_ErrUnknown(token, ":");
		}
	}
	return expr;
}

/* || */
VMExpr *term3(void)
{
	VMExpr *l, *expr;
	expr = term4();
	while(token == OP_OROR)
	{
		l = expr;
		expr = new_exprtree();
		expr->l = l;
		token = gettoken();
		expr->r = term4();
		expr->fn = vmeval_logicor;
		CheckLValue(expr->l, "||");
		CheckRValue(expr->r, "||");
	}
	return expr;
}

/* && */
VMExpr *term4(void)
{
	VMExpr *l, *expr;
	expr = term5();
	while(token == OP_ANDAND)
	{
		l = expr;
		expr = new_exprtree();
		expr->l = l;
		token = gettoken();
		expr->r = term5();
		expr->fn = vmeval_logicand;
		CheckLValue(expr->l, "&&");
		CheckRValue(expr->r, "&&");
	}
	return expr;
}

/* | */
VMExpr *term5(void)
{
	VMExpr *l, *expr;
	expr = term6();
	while(token == OP_OR)
	{
		l = expr;
		expr = new_exprtree();
		expr->l = l;
		token = gettoken();
		expr->r = term6();
		expr->fn = vmeval_bitor;
		CheckLValue(expr->l, "|");
		CheckRValue(expr->r, "|");
	}
	return expr;
}

/* & */
VMExpr *term6(void)
{
	VMExpr *l, *expr;
	expr = term7();
	while(token == OP_AND)
	{
		l = expr;
		expr = new_exprtree();
		expr->l = l;
		token = gettoken();
		expr->r = term7();
		expr->fn = vmeval_bitand;
		CheckLValue(expr->l, "&");
		CheckRValue(expr->r, "&");
	}
	return expr;
}

/* ==, != */
VMExpr *term7(void)
{
	int op;
	VMExpr *l, *expr;
	expr = term8();
	while(((op = token) == OP_EQUAL) || (op == OP_NOTEQUAL))
	{
		l = expr;
		expr = new_exprtree();
		expr->l = l;
		token = gettoken();
		expr->r = term8();
		switch(op)
		{
			case OP_EQUAL:
				expr->fn = vmeval_isequal;
				CheckLValue(expr->l, "==");
				CheckRValue(expr->r, "==");
				break;
			default:
				expr->fn = vmeval_isnotequal;
				CheckLValue(expr->l, "!=");
				CheckRValue(expr->r, "!=");
				break;
		}
	}
	return expr;
}

/* <, <=, >, >= */
VMExpr *term8(void)
{
	int op;
	VMExpr *l, *expr;
	expr = term9();
	while(((op = token) == OP_LESSTHAN) ||
	      ((op == OP_GREATERTHAN) && (vector_closing == 0)) ||
	       (op == OP_LESSEQUAL) || (op == OP_GREATEQUAL))
	{
		l = expr;
		expr = new_exprtree();
		expr->l = l;
		token = gettoken();
		expr->r = term9();
		switch(op)
		{
			case OP_LESSTHAN:
				expr->fn = vmeval_lessthan;
				CheckLValue(expr->l, "<");
				CheckRValue(expr->r, "<");
				break;
			case OP_GREATERTHAN:
				expr->fn = vmeval_greaterthan;
				CheckLValue(expr->l, ">");
				CheckRValue(expr->r, ">");
				break;
			case OP_LESSEQUAL:
				expr->fn = vmeval_lessequal;
				CheckLValue(expr->l, "<=");
				CheckRValue(expr->r, "<=");
				break;
			default:
				expr->fn = vmeval_greatequal;
				CheckLValue(expr->l, ">=");
				CheckRValue(expr->r, ">=");
				break;
		}
	}
	return expr;
}

/* <<, >> */
VMExpr *term9(void)
{
	return term10();
}

/* +, - */
VMExpr *term10(void)
{
	int op;
	VMExpr *l, *expr;
	expr = term11();
	while(((op = token) == OP_PLUS) || (op == OP_MINUS))
	{
		l = expr;
		expr = new_exprtree();
		expr->l = l;
		token = gettoken();
		expr->r = term11();
		switch(op)
		{
			case OP_PLUS:
				expr->fn = vmeval_plus;
				CheckLValue(expr->l, "+");
				CheckRValue(expr->r, "+");
				break;
			default:
				expr->fn = vmeval_minus;
				CheckLValue(expr->l, "-");
				CheckRValue(expr->r, "-");
				break;
		}
	}
	return expr;
}

/* *, /, % */
VMExpr *term11(void)
{
	int op;
	VMExpr *l, *expr;
	expr = term12();
	while(((op = token) == OP_MULT) || (op == OP_DIVIDE) || (op == OP_MOD))
	{
		l = expr;
		expr = new_exprtree();
		expr->l = l;
		token = gettoken();
		expr->r = term12();
		switch(op)
		{
			case OP_MULT:
				expr->fn = vmeval_multiply;
				CheckLValue(expr->l, "*");
				CheckRValue(expr->r, "*");
				break;
			case OP_DIVIDE:
				expr->fn = vmeval_divide;
				CheckLValue(expr->l, "/");
				CheckRValue(expr->r, "/");
				break;
			default:
				expr->fn = vmeval_mod;
				CheckLValue(expr->l, "%");
				CheckRValue(expr->r, "%");
				break;
		}
	}
	return expr;
}

/* ^ */
VMExpr *term12(void)
{
	VMExpr *l, *expr;
	expr = term13();
	while(token == OP_POW)
	{
		l = expr;
		expr = new_exprtree();
		expr->fn = vmeval_pow;
		expr->l = l;
		token = gettoken();
		expr->r = term13();
		CheckLValue(expr->l, "^");
		CheckRValue(expr->r, "^");
	}
	return expr;
}

/* ! unary +, unary -, ~ */
VMExpr *term13(void)
{
	int op = token;
	VMExpr *expr;
	switch(op)
	{
		case OP_PLUS:
			token = gettoken();
			expr = term13();
			CheckRValue(expr, "+");
			break;
		case OP_MINUS:
			expr = new_exprtree();
			expr->fn = vmeval_uminus;
			token = gettoken();
			expr->r = term13();
			CheckRValue(expr->r, "-");
			break;
		case OP_NOT:
			expr = new_exprtree();
			expr->fn = vmeval_logicnot;
			token = gettoken();
			expr->r = term13();
			CheckRValue(expr->r, "!");
			break;
		default:
			return term14();
	}
	return expr;
}

/* . */
VMExpr *term14(void)
{
	VMExpr *l, *expr;
	expr = term15();
	while (token == OP_DOT)
	{
		if (expr != NULL && expr->isvec) /* Vector member access. */
		{
			l = expr;
			expr = new_exprtree();
			expr->l = l;
			token = gettoken();
			if (strcmp(g_token_buffer, "x") == 0)
				expr->fn = vmeval_dot_x;
			else if (strcmp(g_token_buffer, "y") == 0)
				expr->fn = vmeval_dot_y;
			else if (strcmp(g_token_buffer, "z") == 0)
				expr->fn = vmeval_dot_z;
			else
			{
				logerror("%s is not a vector member.", g_token_buffer);
				file_PrintFileAndLineNumber();				
			}
		}
		else /* Nothing to access a member from. */
			gettoken_ErrUnknown(token, ")");
		token = gettoken();
	}
	return expr;
}

/* (), fn(), [], <x, y, z>, . */
VMExpr *term15(void)
{
	switch(token) 
	{
		/* 0 parameter functions returning floats */
		case FN_FRAND:
			return MakeFunc(vmeval_frand, 0, 0);

		/* 0 parameter functions returning vectors */
		case FN_VRAND:
			return MakeFunc(vmeval_vrand, 0, 1);
//		case FN_GET_COLOR:
//			return MakeFunc(vmeval_get_color, 0, 1);

		/* 1 parameter functions returning floats */
		case FN_ABS:
			return MakeFunc(vmeval_abs, 1, 0);
		case FN_ACOS:
			return MakeFunc(vmeval_acos, 1, 0);
		case FN_ASIN:
			return MakeFunc(vmeval_asin, 1, 0);
		case FN_ATAN:
			return MakeFunc(vmeval_atan, 1, 0);
		case FN_CEIL:
			return MakeFunc(vmeval_ceil, 1, 0);
		case FN_CHECKER:
			return MakeFunc(vmeval_checker, 1, 0);
		case FN_COS:
			return MakeFunc(vmeval_cos, 1, 0);
		case FN_COSH:
			return MakeFunc(vmeval_cosh, 1, 0);
		case FN_EXP:
			return MakeFunc(vmeval_exp, 1, 0);
		case FN_FLOOR:
			return MakeFunc(vmeval_floor, 1, 0);
		case FN_INT:
			return MakeFunc(vmeval_int, 1, 0);
		case FN_IRAND:
			return MakeFunc(vmeval_irand, 1, 0);
		case FN_LOG:
			return MakeFunc(vmeval_log, 1, 0);
		case FN_LOG10:
			return MakeFunc(vmeval_log10, 1, 0);
		case FN_NOISE:
			return MakeFunc(vmeval_noise, 1, 0);
		case FN_ROUND:
			return MakeFunc(vmeval_round, 1, 0);
		case FN_SIN:
			return MakeFunc(vmeval_sin, 1, 0);
		case FN_SINH:
			return MakeFunc(vmeval_sinh, 1, 0);
		case FN_SQRT:
			return MakeFunc(vmeval_sqrt, 1, 0);
		case FN_TAN:
			return MakeFunc(vmeval_tan, 1, 0);
		case FN_TANH:
			return MakeFunc(vmeval_tanh, 1, 0);
		case FN_VMAG:
			return MakeFunc(vmeval_vmag, 1, 0);

		/* 1 parameter functions returning vectors */
		case FN_BUMP:
			return MakeFunc(vmeval_bump, 1, 1);
		case FN_CYLINDER_MAP:
			return MakeFunc(vmeval_cylinder_map, 1, 1);
//		case FN_SET_COLOR:
//			return MakeFunc(vmeval_set_color, 1, 1);
		case FN_VNOISE:
			return MakeFunc(vmeval_vnoise, 1, 1);
		case FN_VNORM:
			return MakeFunc(vmeval_vnorm, 1, 1);

		/* 2 parameter functions returning floats */
		case FN_ATAN2:
			return MakeFunc(vmeval_atan2, 2, 0);
		case FN_HEXAGON:
			return MakeFunc(vmeval_hexagon, 2, 0);
		case FN_TURB:
			return MakeFunc(vmeval_turb, 2, 0);
		case FN_VDOT:
			return MakeFunc(vmeval_vdot, 2, 0);

		/* 2 parameter functions returning vectors */
		case FN_COLOR_MAP:
			return MakeHandleFunc(vmeval_color_map,
				DECL_COLOR_MAP, 1, 1);
		case FN_IMAGE_MAP:
			return MakeImageMapFunc(vmeval_image_map);
		case FN_SMOOTH_IMAGE_MAP:
			return MakeImageMapFunc(vmeval_smooth_image_map);
		case FN_VCROSS:
			return MakeFunc(vmeval_vcross, 2, 1);
		case FN_VTURB:
			return MakeFunc(vmeval_vturb, 2, 1);
		case FN_WRINKLE:
			return MakeFunc(vmeval_wrinkle, 2, 1);

		/* 3 parameter functions returning floats */
		case FN_CLAMP:
			return MakeFunc(vmeval_clamp, 3, 0);
		case FN_LEGENDRE:
			return MakeFunc(vmeval_legendre, 3, 0);
		case FN_LERP:
			return MakeFunc(vmeval_lerp, 3, 0);

		/* 3 parameter functions with indeterminate return type */
		case FN_CHECKER2:
			return MakeFunc(vmeval_checker2, 3, 0);

		/* 3 parameter functions returning vectors */
		case FN_VROTATE:
			return MakeFunc(vmeval_vrotate, 3, 1);
		case FN_VLERP:
			return MakeFunc(vmeval_vlerp, 3, 1);

		/* 4 parameter functions returning floats */
		case FN_TURB2:
			return MakeFunc(vmeval_turb2, 4, 0);

		/* 4 parameter functions returning vectors */
		case FN_VTURB2:
			return MakeFunc(vmeval_vturb2, 4, 1);

		/* 5 parameter functions with indeterminate return type */
		case FN_HEXAGON2:
			return MakeFunc(vmeval_hexagon2, 5, 0);

		case OP_LPAREN:
		{
			VMExpr *expr;
			token = gettoken();
			expr = term0();
			if (token == OP_RPAREN)
				token = gettoken();
			else
				gettoken_ErrUnknown(token, ")");
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

VMExpr *Atom(void)
{
	VMExpr *expr = new_exprtree();

	switch(token)
	{
		case CV_FLOAT_CONST:
		case CV_INT_CONST:
			expr->v.x = atof(g_token_buffer);
			break;
		case CV_PI_CONST:
			expr->v.x = PI;
			break;

		case DECL_FLOAT:
		case DECL_VECTOR:
			expr->data = (void *)vm_copy_lvalue((VMLValue *)g_cur_token->data);
			expr->fn = vmeval_lvalue;
			expr->isvec = (token == DECL_VECTOR) ? 1 : 0;
			break;

/*		case RT_D: /* Ray direction "D" variable. */
/*			expr->data = (void *)&rt_D;
			expr->fn = vmeval_rtvec;
			expr->isvec = 1;
			break;
		case RT_N: /* World normal "N" variable. */
/*			expr->data = (void *)&rt_WN;
			expr->fn = vmeval_rtvec;
			expr->isvec = 1;
			break; */
		case RT_O: /* Object point hit "O" variable. */
			expr->data = (void *)&rt_O;
			expr->fn = vmeval_rtvec;
			expr->isvec = 1;
			break;
		case RT_ON: /* Object normal "N" variable. */
			expr->data = (void *)&rt_ON;
			expr->fn = vmeval_rtvec;
			expr->isvec = 1;
			break;
/*		case RT_USCREEN: /* "uscreen" variable. */
/*			expr->data = (void *)&rt_uscreen;
			expr->fn = vmeval_rtfloat;
			break;
		case RT_VSCREEN: /* "vscreen" variable. */
/*			expr->data = (void *)&rt_vscreen;
			expr->fn = vmeval_rtfloat;
			break;
		case RT_U: /* Object "u" variable. */
/*			expr->data = (void *)&rt_u;
			expr->fn = vmeval_rtfloat;
			break;
		case RT_V: /* Object "v" variable. */
/*			expr->data = (void *)&rt_v;
			expr->fn = vmeval_rtfloat;
			break; */
		case RT_X: /* Object "x" variable. */
			expr->data = (void *)&rt_O.x;
			expr->fn = vmeval_rtfloat;
			break;
		case RT_Y: /* Object "y" variable. */
			expr->data = (void *)&rt_O.y;
			expr->fn = vmeval_rtfloat;
			break;
		case RT_Z: /* Object "z" variable. */
			expr->data = (void *)&rt_O.z;
			expr->fn = vmeval_rtfloat;
			break;

		default:
			delete_exprtree(expr);
			return NULL;
	}

	token = gettoken();
	
	return expr;
}

/*************************************************************************
*
*  Utility functions.
*
*************************************************************************/

/**
 *	Allocate and initialize a new expression tree element.
 */
VMExpr *new_exprtree(void)
{
	VMExpr *expr = (VMExpr *)calloc(1, sizeof(VMExpr));
	if (expr != NULL)
	{
		expr->l = expr->r = NULL;
		expr->fn = vmeval_const;
		expr->data = NULL;
		expr->isvec = 0;
	}
	else
		logmemerror("expression");
	return expr;
}

/**
 *	Recursively delete an expression tree.
 */
void delete_exprtree(VMExpr *expr)
{
	if (expr != NULL)
	{
		delete_exprtree(expr->l);
		delete_exprtree(expr->r);
		if (expr->fn == vmeval_lvalue)
			vm_delete_lvalue((VMLValue *)expr->data);
/* TODO:
		else if (expr->fn == vmeval_color_map)
			ColorMap_Delete((ColorMap *)expr->data);
*/
		else if (expr->fn == vmeval_image_map ||
			expr->fn == vmeval_smooth_image_map)
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
void CheckLValue(VMExpr *expr, const char *opname)
{
	if (expr == NULL)
	{
		logerror("expression syntax: "
			"Expecting a value before `%s'.", opname);
		file_PrintFileAndLineNumber();
	}
}

/*************************************************************************
*
*  CheckRValue - Checks for missing r-values. Outputs an error message
*    if r-value is NULL.
*
*************************************************************************/
void CheckRValue(VMExpr *expr, const char *opname)
{
	if (expr == NULL)
	{
		logerror("expression syntax: "
			"Expecting a value after `%s'. Found `%s'",
			opname, g_token_buffer);
		file_PrintFileAndLineNumber();
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
void CheckParamCount(VMExpr *expr, const char *fnname, int cnt)
{
	int actualcnt, diffcnt;

	actualcnt = 0;
	if (expr != NULL)
	{
		actualcnt++;
		while (expr->fn == vmeval_comma)
		{
			actualcnt++;
			expr = expr->l;
		}
	}
	diffcnt = actualcnt - cnt;
	if (diffcnt < 0)
	{
		diffcnt = -diffcnt;
		logerror("%s: %d required parameter(s) missing.", fnname, diffcnt);
	}
	else if (diffcnt > 0)
	{
		logerror("%s: %d too many parameter(s).", fnname, diffcnt);
	}
}

VMExpr *MakeFunc(void (*fn)(VMExpr *), int nparams, int returns_vec)
{
	VMExpr *expr = NULL;
	TOKEN *fntoken = g_cur_token;
	token = gettoken();
	if (token == OP_LPAREN)
	{
		if (nparams < 2)
		{
			expr = new_exprtree();
			expr->fn = fn;
			expr->isvec = returns_vec ? 1 : 0;
			token = gettoken();
			expr->l = term0();
			if (token == OP_RPAREN)
			{
				CheckParamCount(expr->l, fntoken->name, nparams);
				token = gettoken();
			}
			else
				gettoken_ErrUnknown(token, ")");
		}
		else
		{
			token = gettoken();
			expr = term0();
			if (token == OP_RPAREN)
			{
				CheckParamCount(expr, fntoken->name, nparams);
				token = gettoken();
				if (expr != NULL)
				{
					expr->fn = fn;
					expr->isvec = returns_vec ? 1 : 0;
				}
			}
			else
				gettoken_ErrUnknown(token, ")");
		}
	}
	else
		gettoken_ErrUnknown(token, "(");
	return expr;
}


VMExpr *MakeImageMapFunc(void (*fn)(VMExpr *))
{
	VMExpr *expr = NULL;
	TOKEN *fntoken = g_cur_token;
	Image *img = NULL;

	token = gettoken();
	if (token == OP_LPAREN)
	{
		/* Get the quoted file name and first comma... */
		if ((token = gettoken()) == TK_QUOTESTRING)
		{
			FILE *fp;
			/* Open the file... */
			if ((fp = SCN_FindFile(g_token_buffer, READBIN,
				scn_include_paths, SCN_FINDFILE_CHK_CUR_FIRST)) != NULL)
			{
				/* Load the image... */
				img = Image_Load(fp, g_token_buffer); 
				fclose(fp);
				if (img == NULL)
				{
					logerror("Unable to load image file: %s", g_token_buffer);
					file_PrintFileAndLineNumber();
				}
			}
			else
			{
				logerror("Unable to open image file: %s", g_token_buffer);
				file_PrintFileAndLineNumber();
			}
		}
		else
			gettoken_ErrUnknown(token, "expecting image file name");
		
		/* Eat the first comma... */
		if ((token = gettoken()) != OP_COMMA)
			gettoken_ErrUnknown(token, ",");
		/* Get the U, V expressions and finish. */
		token = gettoken();
		expr = term0();
		if (token == OP_RPAREN)
		{
			CheckParamCount(expr, fntoken->name, 2);
			token = gettoken();
			if (expr != NULL)
			{
				expr->data = img;
				expr->fn = fn;
				expr->isvec = 1;
			}
		}
		else
			gettoken_ErrUnknown(token, ")");
	}
	else
		gettoken_ErrUnknown(token, "(");
	return expr;
}

VMExpr *MakeHandleFunc(void (*fn)(VMExpr *), int handle_token,
	int nparams, int returns_vec)
{
	VMExpr *expr = NULL;
	void *data = NULL;
	TOKEN *fntoken = g_cur_token;
	token = gettoken();
	if (token == OP_LPAREN)
	{
		/*
		 * Look for a handle to a declared type that matches
		 * the type specified by handle_token.
		 */
		token = gettoken();
/* TODO:		if (token == handle_token)
		{
			switch(token)
			{
				case DECL_COLOR_MAP:
					data = (void *)ColorMap_Copy((ColorMap *)g_cur_token->data);
					break;
			}
		}
		else */
		{
			switch(handle_token)
			{
				case DECL_COLOR_MAP:
					gettoken_ErrUnknown(token, "color map handle id");
					break;
				default:
					gettoken_ErrUnknown(token, "handle to a declared item");
					break;
			}
		}
		/* Get the first comma if more params follow. */
		if (nparams > 0)
		{
			token = gettoken();
			if (token != OP_COMMA)
				gettoken_ErrUnknown(token, ",");
		}
		if (nparams < 2)
		{
			expr = new_exprtree();
			expr->fn = fn;
			expr->data = data;
			expr->isvec = returns_vec ? 1 : 0;
			token = gettoken();
			expr->l = term0();
			if (token == OP_RPAREN)
			{
				CheckParamCount(expr->l, fntoken->name, nparams);
				token = gettoken();
			}
			else
				gettoken_ErrUnknown(token, ")");
		}
		else
		{
			token = gettoken();
			expr = term0();
			if (token == OP_RPAREN)
			{
				CheckParamCount(expr, fntoken->name, nparams);
				token = gettoken();
				if (expr != NULL)
				{
					expr->fn = fn;
					expr->data = data;
					expr->isvec = returns_vec ? 1 : 0;
				}
				else if (data != NULL)
				{
					switch(token)
					{
/* TODO:						case DECL_COLOR_MAP:
							ColorMap_Delete((ColorMap *)data);
							break;
*/					}
				}
			}
			else
				gettoken_ErrUnknown(token, ")");
		}
	}
	else
		gettoken_ErrUnknown(token, "(");
	return expr;
}

VMExpr *MakeVec(void)
{
	VMExpr *expr;
	expr = new_exprtree();
	expr->r = new_exprtree();
	expr->fn = vmeval_vector;
	expr->isvec = 1;
	token = gettoken();
	if ((expr->r->r = term1()) != NULL)
	{
		if (token == OP_COMMA)
		{
			token = gettoken();
			if ((expr->r->l = term1()) != NULL)
			{
				if (token == OP_COMMA)
				{
					token = gettoken();
					vector_closing++;
					if ((expr->l = term1()) != NULL)
					{
						if (token == OP_GREATERTHAN)
						{
							token = gettoken();
							vector_closing--;
							return expr;
						}
						else
							gettoken_ErrUnknown(token, ">");
					}
				}
				else
					gettoken_ErrUnknown(token, ",");
			}
		}
		else
			gettoken_ErrUnknown(token, ",");
	}
	return expr;
}
