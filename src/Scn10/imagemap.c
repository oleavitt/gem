/*************************************************************************
*
*  imagemap.c - Image map parsing and loading statements.
*
*************************************************************************/

#include "local.h"

typedef struct tag_loadimagemapstmtdata
{
	char *name;
	char *filename;
} LoadImageMapStmtData;

static void ExecLoadImageMapStmt(Stmt *stmt);
static void DeleteLoadImageMapStmt(Stmt *stmt);

StmtProcs loadimagemap_stmt_procs =
{
	TK_LOAD_IMAGE_MAP,
	ExecLoadImageMapStmt,
	DeleteLoadImageMapStmt
};


Stmt *ParseLoadImageMapStmt(void)
{
	LoadImageMapStmtData *sd;
	int token;
	Stmt *stmt = NewStmt();
	if(stmt == NULL)
	{
		LogMemError("load_image_map");
		return NULL;
	}
	sd = (LoadImageMapStmtData *)calloc(1, sizeof(LoadImageMapStmtData));
	if(sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("load_image_map");
		return NULL;
	}
	stmt->procs = &loadimagemap_stmt_procs;
	stmt->data = (void *)sd;

	token = GetNewIdentifier();
	if(token == TK_UNKNOWN_ID)
	{
		sd->name = (char *)malloc(strlen(token_buffer) + 1);
		if(sd->name == NULL)
		{
			DeleteStmt(stmt);
			LogMemError("load_color_map");
			return NULL;
		}
		strcpy(sd->name, token_buffer);
	}
	else
	{
		LogError("load_image_map: Expecting an unused identifier name.");
		LogError("  Found '%s'.", token_buffer);
		PrintFileAndLineNumber();
		UngetToken();
		DeleteStmt(stmt);
		return NULL;
	}

	return stmt;
}


void ExecLoadImageMapStmt(Stmt *stmt)
{
	LoadImageMapStmtData *sd = (LoadImageMapStmtData *)stmt->data;
	ColorMap *old_cmap = cur_cmap;
	if((cur_cmap = ColorMap_Create()) != NULL)
	{
		if(Symbol_AddLocal(sd->name, DECL_COLOR_MAP, 0, (void *)cur_cmap))
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
	if(sd != NULL)
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
	if(sd != NULL)
	{
		ExprDelete(sd->color);
		ExprDelete(sd->value);
		free(sd);
	}
}
