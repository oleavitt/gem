/*************************************************************************
*
*  surface.c - Surface statement.
*
*  syntax: surface color, ka, kd, kr, ks, Phong, kt, ior, outior
*    { modifier stmts } or ;
*
*  Phong, ior and outior are float exprs, all others are vector exprs.
*
*************************************************************************/

#include "local.h"

static Surface *cur_surface = NULL;

typedef struct tag_surfacestmtdata
{
	Expr *color;
	Expr *ka;
	Expr *kd;
	Expr *kr;
	Expr *ks;
	Expr *Phong;
	Expr *kt;
	Expr *ior;
	Expr *outior;
	Stmt *block;
} SurfaceStmtData;

typedef struct tag_declsurfacestmtdata
{
	Surface *declsurf;
	Stmt *block;
} DeclSurfaceStmtData;

static int ParseSurfaceDetails(Stmt **stmt);
static void ExecSurfaceStmt(Stmt *stmt);
static void DeleteSurfaceStmt(Stmt *stmt);
static void ExecSurfaceExprStmt(Stmt *stmt);
static void DeleteSurfaceExprStmt(Stmt *stmt);
static void ExecDeclSurfaceStmt(Stmt *stmt);
static void DeleteDeclSurfaceStmt(Stmt *stmt);
static void ExecSurfaceTransformStmt(Stmt *stmt);
static void DeleteSurfaceTransformStmt(Stmt *stmt);

StmtProcs surface_stmt_procs =
{
	TK_SURFACE,
	ExecSurfaceStmt,
	DeleteSurfaceStmt
};

StmtProcs surface_expr_stmt_procs =
{
	0,
	ExecSurfaceExprStmt,
	DeleteSurfaceExprStmt
};

StmtProcs declsurface_stmt_procs =
{
	0,
	ExecDeclSurfaceStmt,
	DeleteDeclSurfaceStmt
};

StmtProcs surface_transform_stmt_procs =
{
	0,
	ExecSurfaceTransformStmt,
	DeleteSurfaceTransformStmt
};

Stmt *ParseSurfaceStmt(void)
{
	int token;
	SurfaceStmtData *ssd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("surface");
		return NULL;
	}
	ssd = (SurfaceStmtData *)calloc(1, sizeof(SurfaceStmtData));
	if(ssd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("surface");
		return NULL;
	}
	stmt->procs = &surface_stmt_procs;
	stmt->data = (void *)ssd;
	ssd->color = ssd->ka = ssd->kd = ssd->kr = ssd->ks = ssd->Phong =
		ssd->kt = ssd->ior = ssd->outior = NULL;
	ssd->block = NULL;
	ssd->color = ExprParse();
	token = GetToken();
	if(token == OP_COMMA)
	{
		ssd->ka = ExprParse();
		token = GetToken();
		if(token == OP_COMMA)
		{
			ssd->kd = ExprParse();
			token = GetToken();
			if(token == OP_COMMA)
			{
				ssd->kr = ExprParse();
				token = GetToken();
				if(token == OP_COMMA)
				{
					ssd->ks = ExprParse();
					token = GetToken();
					if(token == OP_COMMA)
					{
						ssd->Phong = ExprParse();
						token = GetToken();
						if(token == OP_COMMA)
						{
							ssd->kt = ExprParse();
							token = GetToken();
							if(token == OP_COMMA)
							{
								ssd->ior = ExprParse();
								token = GetToken();
								if(token == OP_COMMA)
								{
									ssd->outior = ExprParse();
									token = GetToken();
								}
							}
						}
					}
				}
			}
		}
	}
	if(token != OP_SEMICOLON)
	{
		UngetToken();
		ssd->block = ParseBlock("surface", ParseSurfaceDetails);
	}
	if(!error_count)
		return stmt;
	else
		ErrUnknown(token, ";", "surface");
	DeleteStmt(stmt);
	return NULL;
}

int ParseSurfaceDetails(Stmt **stmt)
{
	int token;
	token = GetToken();
	*stmt = NULL;
	switch(token)
	{
		/* transform statements */
		case TK_ROTATE:
		case TK_SCALE:
		case TK_TRANSLATE:
			if((*stmt = NewStmt()) !=NULL)
			{
				(*stmt)->procs = &surface_transform_stmt_procs;
				(*stmt)->int_data = 
					(token == TK_ROTATE) ? XFORM_ROTATE :
					(token == TK_SCALE) ? XFORM_SCALE :
					XFORM_TRANSLATE;
				if(((*stmt)->data = (void *)ExprParse()) != NULL)
					if((token = GetToken()) != OP_SEMICOLON)
						ErrUnknown(token, ";", "transform");
			}
			break;
		
		/* Run-time exprs for lighting constants. */
		case TK_COLOR:
		case TK_AMBIENT:
		case TK_DIFFUSE:
		case TK_REFLECTION:
		case TK_SPECULAR:
		case TK_PHONG:
		case TK_TRANSMISSION:
		case TK_IOR:
		case TK_OUTIOR:
		case TK_BUMP:
			if((*stmt = NewStmt()) !=NULL)
			{
				(*stmt)->procs = &surface_expr_stmt_procs;
				(*stmt)->int_data = 
					(token == TK_COLOR) ? SURFMOD_COLOR_EXPR :
					(token == TK_AMBIENT) ? SURFMOD_KA_EXPR :
					(token == TK_DIFFUSE) ? SURFMOD_KD_EXPR :
					(token == TK_REFLECTION) ? SURFMOD_KR_EXPR :
					(token == TK_SPECULAR) ? SURFMOD_KS_EXPR :
					(token == TK_PHONG) ? SURFMOD_PHONG_EXPR :
					(token == TK_TRANSMISSION) ? SURFMOD_KT_EXPR :
					(token == TK_IOR) ? SURFMOD_IOR_EXPR :
					(token == TK_OUTIOR) ? SURFMOD_OUTIOR_EXPR :
					SURFMOD_BUMP_EXPR;
				if(((*stmt)->data = (void *)ExprParse()) != NULL)
					if((token = GetToken()) != OP_SEMICOLON)
						ErrUnknown(token, ";", "surface");
			}
			break;
		default:
			return token;
	}
	return TK_NULL;
}

void ExecSurfaceStmt(Stmt *stmt)
{
	SurfaceStmtData *ssd = (SurfaceStmtData *)stmt->data;
	Surface *s, *prev_cur_surface;;
	if((s = Ray_NewSurface()) != NULL)
	{
		/* Evaluate from left to right (as exprs appear in statement). */
		if(ssd->color != NULL)
			ExprEvalVector(ssd->color, &s->color);
		if(ssd->ka != NULL)
			ExprEvalVector(ssd->ka, &s->ka);
		if(ssd->kd != NULL)
			ExprEvalVector(ssd->kd, &s->kd);
		if(ssd->kr != NULL)
			ExprEvalVector(ssd->kr, &s->kr);
		if(ssd->ks != NULL)
			ExprEvalVector(ssd->ks, &s->ks);
		if(ssd->Phong != NULL)
			s->spec_power = ExprEvalDouble(ssd->Phong);
		if(ssd->kt != NULL)
			ExprEvalVector(ssd->kt, &s->kt);
		if(ssd->ior != NULL)
			s->ior = ExprEvalDouble(ssd->ior);
		if(ssd->outior != NULL)
			s->outior = ExprEvalDouble(ssd->outior);
		prev_cur_surface = cur_surface;
		cur_surface = s;
		ExecBlock(stmt, ssd->block);
		cur_surface = prev_cur_surface;
		if(objstack_ptr->curobj != NULL)
		{
			Ray_DeleteSurface(objstack_ptr->curobj->surface);
			objstack_ptr->curobj->surface = s;
		}
		else if(parse_declsurfflag)
		{
			parse_declsurf = s;
		}
		else
		{
			Ray_DeleteSurface(default_surface);
			default_surface = s;
		}
	}
}

void DeleteSurfaceStmt(Stmt *stmt)
{
	SurfaceStmtData *ssd = (SurfaceStmtData *)stmt->data;
	if(ssd != NULL)
	{
		ExprDelete(ssd->color);
		ExprDelete(ssd->ka);
		ExprDelete(ssd->kd);
		ExprDelete(ssd->kr);
		ExprDelete(ssd->ks);
		ExprDelete(ssd->Phong);
		ExprDelete(ssd->kt);
		ExprDelete(ssd->ior);
		ExprDelete(ssd->outior);
		DeleteStatements(ssd->block);
		free(ssd);
	}
}

void ExecSurfaceExprStmt(Stmt *stmt)
{
	if(stmt->data != NULL)
	{
		if(Ray_AddSurfaceModifierExpr(cur_surface, (Expr *)stmt->data,
			stmt->int_data))
			stmt->data = NULL;
			/*
			 * Expr is now in use by the renderer. Set the pointer here
			 * to NULL so that it doesn't get deleted with this statement.
			 * It will be deleted when the renderer closes.
			 */
	}
}

void DeleteSurfaceExprStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}



Stmt *ParseDeclSurfaceStmt(Surface *declsurf)
{
	int nparams;
	Param params[1];
	DeclSurfaceStmtData *sd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("user-defined surface");
		return NULL;
	}
	sd = (DeclSurfaceStmtData *)calloc(1, sizeof(DeclSurfaceStmtData));
	if(sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("user-defined surface");
		return NULL;
	}
	stmt->procs = &declsurface_stmt_procs;
	stmt->data = (void *)sd;
	sd->declsurf = declsurf;
	sd->block = NULL;
	nparams = ParseParams("OB", "user-defined surface",
		ParseSurfaceDetails, params);
	if(nparams)
		sd->block = params[0].data.block;
	if(!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

void ExecDeclSurfaceStmt(Stmt *stmt)
{
	DeclSurfaceStmtData *sd = (DeclSurfaceStmtData *)stmt->data;
	Surface *s, *prev_cur_surface;
	s = (sd->block != NULL) ? Ray_CloneSurface(sd->declsurf) :
		Ray_ShareSurface(sd->declsurf);
	if(s != NULL)
	{
		prev_cur_surface = cur_surface;
		cur_surface = s;
		ExecBlock(stmt, sd->block);
		cur_surface = prev_cur_surface;
		if(objstack_ptr->curobj != NULL)
		{
			Ray_DeleteSurface(objstack_ptr->curobj->surface);
			objstack_ptr->curobj->surface = s;
		}
		else if(parse_declsurfflag)
		{
			parse_declsurf = s;
		}
		else
		{
			Ray_DeleteSurface(default_surface);
			default_surface = s;
		}
	}
}

void DeleteDeclSurfaceStmt(Stmt *stmt)
{
	DeclSurfaceStmtData *sd = (DeclSurfaceStmtData *)stmt->data;
	if(sd != NULL)
	{
		/* Surface is deleted when symbol table closes. */
		DeleteStatements(sd->block);
		free(sd);
	}
}


void ExecSurfaceTransformStmt(Stmt *stmt)
{
	Vec3 V;
	ExprEvalVector((Expr *)stmt->data, &V);
	Ray_Transform_Surface(cur_surface, &V, stmt->int_data);
}

void DeleteSurfaceTransformStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}
