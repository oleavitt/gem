/*************************************************************************
*
*  colormap.c - Color map statements and color map implementation.
*
*************************************************************************/

#include "local.h"

typedef struct tag_loadcmapstmtdata
{
	char *name;
	Stmt *block;
} LoadCMapStmtData;

typedef struct tag_colorstmtdata
{
	Expr *color;
	Expr *value;
} ColorStmtData;

static void ExecLoadColorMapStmt(Stmt *stmt);
static void DeleteLoadColorMapStmt(Stmt *stmt);
static void ExecColorStmt(Stmt *stmt);
static void DeleteColorStmt(Stmt *stmt);
static int ParseColorMapDetails(Stmt **stmt);

StmtProcs loadcmap_stmt_procs =
{
	TK_LOAD_COLOR_MAP,
	ExecLoadColorMapStmt,
	DeleteLoadColorMapStmt
};

StmtProcs color_stmt_procs =
{
	TK_COLOR,
	ExecColorStmt,
	DeleteColorStmt
};

static ColorMap *cur_cmap = NULL;

Stmt *ParseLoadColorMapStmt(void)
{
	LoadCMapStmtData *sd;
	int token;
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("load_color_map");
		return NULL;
	}
	sd = (LoadCMapStmtData *)calloc(1, sizeof(LoadCMapStmtData));
	if (sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("load_color_map");
		return NULL;
	}
	stmt->procs = &loadcmap_stmt_procs;
	stmt->data = (void *)sd;

	token = GetNewIdentifier();
	if (token == TK_UNKNOWN_ID)
	{
		sd->name = (char *)malloc(strlen(token_buffer) + 1);
		if (sd->name == NULL)
		{
			DeleteStmt(stmt);
			LogMemError("load_color_map");
			return NULL;
		}
		strcpy(sd->name, token_buffer);
	}
	else
	{
		LogError("load_color_map: Expecting an unused identifier name.");
		LogError("  Found '%s'.", token_buffer);
		PrintFileAndLineNumber();
		UngetToken();
	}
	sd->block = ParseBlock("load_color_map", ParseColorMapDetails);

	if (!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

Stmt *ParseColorStmt(void)
{
	ColorStmtData *sd;
	int nparams;
	Param params[2];
	Stmt *stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("color");
		return NULL;
	}
	sd = (ColorStmtData *)calloc(1, sizeof(ColorStmtData));
	if (sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("color");
		return NULL;
	}
	stmt->procs = &color_stmt_procs;
	stmt->data = (void *)sd;
	sd->color = NULL;
	sd->value = NULL;
	nparams = ParseParams("EE;", "color", NULL, params);
	if (nparams > 0)
	{
		sd->color = params[0].data.expr;
		if(nparams > 1)
			sd->value = params[1].data.expr;
	}
	if (!error_count)
		return stmt;
	DeleteStmt(stmt);
	return NULL;
}

int ParseColorMapDetails(Stmt **stmt)
{
	int token;
	token = GetToken();
	*stmt = NULL;
	switch (token)
	{
		case TK_COLOR:
			*stmt = ParseColorStmt();
			break;
		default:
			return token;
	}
	return TK_NULL;
}


void ExecLoadColorMapStmt(Stmt *stmt)
{
	LoadCMapStmtData *sd = (LoadCMapStmtData *)stmt->data;
	ColorMap *old_cmap = cur_cmap;
	if ((cur_cmap = ColorMap_Create()) != NULL)
	{
		if (Symbol_AddLocal(sd->name, DECL_COLOR_MAP, 0, (void *)cur_cmap))
			ExecBlock(stmt, sd->block);
		/* Note: Color map will be deleted if symbol table fails to add it. */
	}
	cur_cmap = old_cmap;
}

void ExecColorStmt(Stmt *stmt)
{
	ColorStmtData *sd = (ColorStmtData *)stmt->data;
	Vec3 color;
	double value;

	assert(sd->color != NULL);
	assert(sd->value != NULL);
	assert(cur_cmap != NULL);

	ExprEvalVector(sd->color, &color);
	value = ExprEvalDouble(sd->value);

	ColorMap_AddColor(cur_cmap, &color, value);
}

void DeleteLoadColorMapStmt(Stmt *stmt)
{
	LoadCMapStmtData *sd = (LoadCMapStmtData *)stmt->data;
	if (sd != NULL)
	{
		if(sd->name != NULL)
			free(sd->name);
		DeleteStatements(sd->block);
		free(sd);
	}
}

void DeleteColorStmt(Stmt *stmt)
{
	ColorStmtData *sd = (ColorStmtData *)stmt->data;
	if (sd != NULL)
	{
		ExprDelete(sd->color);
		ExprDelete(sd->value);
		free(sd);
	}
}


void ColorMap_LookupColor(ColorMap *cmap, double val, Vec3 *color)
{
	float *clo, *chi;
	int i;
	assert(cmap != NULL);
	assert(cmap->colordata != NULL);
	clo = chi = cmap->colordata;
	for (i = 0; i < cmap->ncolors; i++)
	{
		if (val <= chi[3])
			break;
		clo = chi;
		chi += 4;
	}
	if (i == cmap->ncolors)
		chi = clo;
	if (clo != chi)
	{
		double a, d;
		d = chi[3] - clo[3];
		a = (d != 0.0) ? (val - clo[3]) / d : 0.0;
		color->x = LERP(a, clo[0], chi[0]);
		color->y = LERP(a, clo[1], chi[1]);
		color->z = LERP(a, clo[2], chi[2]);
	}
	else
	{
		V3Set(color, chi[0], chi[1], chi[2]);
	}
}

ColorMap *ColorMap_Create(void)
{
	ColorMap *cmap = (ColorMap *)malloc(sizeof(ColorMap));
	if (cmap != NULL)
	{
		cmap->nrefs = 1;
		cmap->ncolors = 0;
		cmap->colordata = NULL;
	}
	return cmap;
}

int ColorMap_AddColor(ColorMap *cmap, Vec3 *color, double val)
{
	float *colorentry;
	assert(cmap != NULL);
	if (cmap->colordata != NULL)
	{
		size_t newsize, oldsize;
		float *newcolordata;
		oldsize = sizeof(float) * 4 * cmap->ncolors;
		newsize = oldsize + sizeof(float) * 4;
		if ((newcolordata = (float *)malloc(newsize)) == NULL)
			return 0;
		memcpy(newcolordata, cmap->colordata, oldsize);
		free(cmap->colordata);
		cmap->colordata = newcolordata;
		colorentry = &newcolordata[cmap->ncolors * 4];
		cmap->ncolors++;
	}
	else
	{
		if ((cmap->colordata = (float *)malloc(sizeof(float) * 4)) == NULL)
			return 0;
		colorentry = cmap->colordata;
		cmap->ncolors = 1;
	}
	colorentry[0] = (float)color->x;
	colorentry[1] = (float)color->y;
	colorentry[2] = (float)color->z;
	colorentry[3] = (float)val;
	return 1;
}

ColorMap *ColorMap_Copy(ColorMap *cmap)
{
	if (cmap != NULL)
		cmap->nrefs++;
	return cmap;
}

void ColorMap_Delete(ColorMap *cmap)
{
	if ((cmap != NULL) && (cmap->nrefs-- == 0))
	{
		if (cmap->colordata != NULL)
			free(cmap->colordata);
		free(cmap);
	}
}

