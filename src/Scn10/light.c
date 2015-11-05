/*************************************************************************
*
*  light.c - Light statement.
*
*  syntax: light vexpr from;
*          light vexpr from, vexpr color;
*          light vexpr from, vexpr color, fexpr falloff;
*
*************************************************************************/

#include "local.h"

typedef struct tag_lightstmtdata
{
	Expr *from;
	Expr *color;
	Expr *falloff;
	Stmt *block;
} LightStmtData;

typedef struct tag_infinitelightstmtdata
{
	Expr *dir;
	Expr *color;
	Stmt *block;
} InfiniteLightStmtData;

typedef struct tag_lightconestmtdata
{
	Expr *radius1;
	Expr *radius2;
} LightConeStmtData;

static Stmt *ParseLightConeStmt(void);
static void ExecLightStmt(Stmt *stmt);
static void DeleteLightStmt(Stmt *stmt);
static void ExecInfiniteLightStmt(Stmt *stmt);
static void DeleteInfiniteLightStmt(Stmt *stmt);
static void ExecLightXformStmt(Stmt *stmt);
static void DeleteLightXformStmt(Stmt *stmt);
static void ExecLightFlagStmt(Stmt *stmt);
static void DeleteLightFlagStmt(Stmt *stmt);
static void ExecLightJitterStmt(Stmt *stmt);
static void DeleteLightJitterStmt(Stmt *stmt);
static void ExecLightAtStmt(Stmt *stmt);
static void DeleteLightAtStmt(Stmt *stmt);
static void ExecLightSpotStmt(Stmt *stmt);
static void DeleteLightSpotStmt(Stmt *stmt);
static void ExecLightConeStmt(Stmt *stmt);
static void DeleteLightConeStmt(Stmt *stmt);

static int ParseLightDetails(Stmt **stmt);

StmtProcs light_stmt_procs =
{
	TK_LIGHT,
	ExecLightStmt,
	DeleteLightStmt
};

StmtProcs infinite_light_stmt_procs =
{
	TK_INFINITE_LIGHT,
	ExecInfiniteLightStmt,
	DeleteInfiniteLightStmt
};

StmtProcs light_xform_stmt_procs =
{
	0,
	ExecLightXformStmt,
	DeleteLightXformStmt
};

StmtProcs light_flag_stmt_procs =
{
	0,
	ExecLightFlagStmt,
	DeleteLightFlagStmt
};

StmtProcs light_jitter_stmt_procs =
{
	0,
	ExecLightJitterStmt,
	DeleteLightJitterStmt
};

StmtProcs light_at_stmt_procs =
{
	0,
	ExecLightAtStmt,
	DeleteLightAtStmt
};

StmtProcs light_spot_stmt_procs =
{
	0,
	ExecLightSpotStmt,
	DeleteLightSpotStmt
};

StmtProcs light_cone_stmt_procs =
{
	0,
	ExecLightConeStmt,
	DeleteLightConeStmt
};

static Light *cur_light = NULL;

Stmt *ParseLightStmt(void)
{
	int nparams, i;
	Param params[4];
	LightStmtData *lsd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("light");
		return NULL;
	}
	lsd = (LightStmtData *)calloc(1, sizeof(LightStmtData));
	if(lsd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("light");
		return NULL;
	}
	stmt->procs = &light_stmt_procs;
	stmt->data = (void *)lsd;
	lsd->from = lsd->color = lsd->falloff = NULL;
	nparams = ParseParams("OEOEOEOB", "light", ParseLightDetails, params);
	for(i = 0; i < nparams; i++)
	{
		switch(params[i].type)
		{
			case PARAM_EXPR:
				if(lsd->from == NULL) /* First one is the from point. */
					lsd->from = params[i].data.expr;
				else if(lsd->color == NULL) /* Second one is the color. */
					lsd->color = params[i].data.expr;
				else                 /* Third one is the falloff. */
					lsd->falloff = params[i].data.expr;
				break;
			case PARAM_BLOCK:
				lsd->block = params[i].data.block;
				break;
		}
	}
	if(!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseInfiniteLightStmt(void)
{
	int nparams, i;
	Param params[3];
	InfiniteLightStmtData *lsd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("infinite_light");
		return NULL;
	}
	lsd = (InfiniteLightStmtData *)calloc(1, sizeof(InfiniteLightStmtData));
	if(lsd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("infinite_light");
		return NULL;
	}
	stmt->procs = &infinite_light_stmt_procs;
	stmt->data = (void *)lsd;
	lsd->dir = lsd->color = NULL;
	nparams = ParseParams("OEOEOB", "infinite_light", ParseLightDetails, params);
	for(i = 0; i < nparams; i++)
	{
		switch(params[i].type)
		{
			case PARAM_EXPR:
				if(lsd->dir == NULL) /* First one is the direction vector. */
					lsd->dir = params[i].data.expr;
				else                 /* Second one is the color. */
					lsd->color = params[i].data.expr;
				break;
			case PARAM_BLOCK:
				lsd->block = params[i].data.block;
				break;
		}
	}
	if(!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}


int ParseLightDetails(Stmt **stmt)
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
				(*stmt)->procs = &light_xform_stmt_procs;
				(*stmt)->int_data = 
					(token == TK_ROTATE) ? XFORM_ROTATE :
					(token == TK_SCALE) ? XFORM_SCALE :
					XFORM_TRANSLATE;
				if(((*stmt)->data = (void *)ExprParse()) != NULL)
					if((token = GetToken()) != OP_SEMICOLON)
						ErrUnknown(token, ";", "transform");
			}
			break;

		/* flag setting statments */
		case TK_NO_SHADOW:
		case TK_NO_SPECULAR:
		case TK_AMBIENT:
			if((*stmt = NewStmt()) !=NULL)
			{
				(*stmt)->procs = &light_flag_stmt_procs;
				(*stmt)->int_data = 
					(token == TK_NO_SHADOW) ? LIGHT_FLAG_NO_SHADOW :
					(token == TK_NO_SPECULAR) ? LIGHT_FLAG_NO_SPECULAR :
					LIGHT_FLAG_NO_SHADOW | LIGHT_FLAG_NO_SPECULAR;
				if((token = GetToken()) != OP_SEMICOLON)
					ErrUnknown(token, ";", "light");
			}
			break;

		case TK_JITTER:
			if((*stmt = NewStmt()) !=NULL)
			{
				(*stmt)->procs = &light_jitter_stmt_procs;
				if(((*stmt)->data = (void *)ExprParse()) != NULL)
					if((token = GetToken()) != OP_SEMICOLON)
						ErrUnknown(token, ";", "jitter");
			}
			break;

		case TK_AT:
			if((*stmt = NewStmt()) !=NULL)
			{
				(*stmt)->procs = &light_at_stmt_procs;
				if(((*stmt)->data = (void *)ExprParse()) != NULL)
					if((token = GetToken()) != OP_SEMICOLON)
						ErrUnknown(token, ";", "jitter");
			}
			break;

		case TK_SPOT:
			if((*stmt = NewStmt()) !=NULL)
			{
				(*stmt)->procs = &light_spot_stmt_procs;
				if(((*stmt)->data = (void *)ExprParse()) != NULL)
					if((token = GetToken()) != OP_SEMICOLON)
						ErrUnknown(token, ";", "spot");
			}
			break;

		case TK_CONE:
			*stmt = ParseLightConeStmt();
			break;

		default:
			/* Look for any light type specific statements. */
			return token;
	}
	return TK_NULL;
}

Stmt *ParseLightConeStmt(void)
{
	int nparams;
	Param params[2];
	LightConeStmtData *sd;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("light: cone");
		return NULL;
	}
	sd = (LightConeStmtData *)calloc(1, sizeof(LightConeStmtData));
	if(sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("light: cone");
		return NULL;
	}
	stmt->procs = &light_cone_stmt_procs;
	stmt->data = (void *)sd;
	sd->radius1 = NULL;
	sd->radius2 = NULL;
	nparams = ParseParams("EOE;", "light: cone", NULL, params);
	if(nparams > 0)
	{
		sd->radius1 = params[0].data.expr;
		if(nparams > 1)
			sd->radius2 = params[1].data.expr;
	}
	if(!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}


void ExecLightStmt(Stmt *stmt)
{
	LightStmtData *lsd = (LightStmtData *)stmt->data;
	Light *light, *prev_light;
	Vec3 from, color;
	double falloff;
	V3Set(&from, 0.0, 0.0, 0.0);
	V3Set(&color, 1.0, 1.0, 1.0);
	falloff = 0.0;
	if(lsd->from != NULL)
		ExprEvalVector(lsd->from, &from);
	if(lsd->color != NULL)
		ExprEvalVector(lsd->color, &color);
	if(lsd->falloff != NULL)
		falloff = ExprEvalDouble(lsd->falloff);
	if((light = Ray_MakePointLight(&from, &color, falloff)) != NULL)
	{
		prev_light = cur_light;
		cur_light = light;
		ExecBlock(stmt, lsd->block);
		Ray_AddLight(&ray_setup_data->lights, light);
		cur_light = prev_light;
	}
}

void DeleteLightStmt(Stmt *stmt)
{
	LightStmtData *lsd = (LightStmtData *)stmt->data;
	if(lsd != NULL)
	{
		ExprDelete(lsd->from);
		ExprDelete(lsd->color);
		ExprDelete(lsd->falloff);
		DeleteStatements(lsd->block);
		free(lsd);
	}
}


void ExecInfiniteLightStmt(Stmt *stmt)
{
	InfiniteLightStmtData *lsd = (InfiniteLightStmtData *)stmt->data;
	Light *light, *prev_light;
	Vec3 dir, color;
	V3Set(&dir, 0.0, 0.0, -1.0);
	V3Set(&color, 1.0, 1.0, 1.0);
	if(lsd->dir != NULL)
		ExprEvalVector(lsd->dir, &dir);
	if(lsd->color != NULL)
		ExprEvalVector(lsd->color, &color);
	if((light = Ray_MakeInfiniteLight(&dir, &color)) != NULL)
	{
		prev_light = cur_light;
		cur_light = light;
		ExecBlock(stmt, lsd->block);
		Ray_AddLight(&ray_setup_data->lights, light);
		cur_light = prev_light;
	}
}

void DeleteInfiniteLightStmt(Stmt *stmt)
{
	InfiniteLightStmtData *lsd = (InfiniteLightStmtData *)stmt->data;
	if(lsd != NULL)
	{
		ExprDelete(lsd->dir);
		ExprDelete(lsd->color);
		DeleteStatements(lsd->block);
		free(lsd);
	}
}


void ExecLightXformStmt(Stmt *stmt)
{
	Vec3 V;
	assert(cur_light != NULL);
	ExprEvalVector((Expr *)stmt->data, &V);
	XformVector(&cur_light->loc, &V, stmt->int_data);
	XformVector(&cur_light->at, &V, stmt->int_data);
}

void DeleteLightXformStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}


void ExecLightFlagStmt(Stmt *stmt)
{
	assert(cur_light != NULL);
	cur_light->flags |= stmt->int_data;
}

void DeleteLightFlagStmt(Stmt *stmt)
{
}


void ExecLightJitterStmt(Stmt *stmt)
{
	assert(cur_light != NULL);
	ExprEvalVector((Expr *)stmt->data, &cur_light->jitter);
	cur_light->flags |= LIGHT_FLAG_JITTER;
}

void DeleteLightJitterStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}


void ExecLightAtStmt(Stmt *stmt)
{
	assert(cur_light != NULL);
	ExprEvalVector((Expr *)stmt->data, &cur_light->at);
}

void DeleteLightAtStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}


void ExecLightSpotStmt(Stmt *stmt)
{
	double angle;
	assert(cur_light != NULL);
	angle = ExprEvalDouble((Expr *)stmt->data);
	angle = 90.0 - CLAMP(angle, 0.0001, 90.0);
	cur_light->focus = tan(angle * DTOR);
	cur_light->type = LIGHT_DIRECTIONAL;
}

void DeleteLightSpotStmt(Stmt *stmt)
{
	ExprDelete((Expr *)stmt->data);
}


void ExecLightConeStmt(Stmt *stmt)
{
	LightConeStmtData *sd = (LightConeStmtData *)stmt->data;
	double r1, r2;
	assert(cur_light != NULL);
	r1 = ExprEvalDouble(sd->radius1);
	r2 = (sd->radius2 != NULL) ? ExprEvalDouble(sd->radius2) : r1;
	if(r1 > r2) { double t = r1; r1 = r2; r2 = t; }
	cur_light->angle_max = cos(r1 * DTOR);
	cur_light->angle_min = cos(r2 * DTOR);
	cur_light->angle_diff = cur_light->angle_max - cur_light->angle_min;
	cur_light->type = LIGHT_DIRECTIONAL;
}

void DeleteLightConeStmt(Stmt *stmt)
{
	LightConeStmtData *sd = (LightConeStmtData *)stmt->data;
	if(sd != NULL)
	{
		ExprDelete(sd->radius1);
		ExprDelete(sd->radius2);
		free(sd);
	}
}
