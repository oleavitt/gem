/*************************************************************************
*
*  heightfield.c - Height field statement.
*
*  syntax: height_field "image filename"
*    { modifier stmts }
*
*************************************************************************/

#include "local.h"

typedef struct tag_hfstmtdata
{
	char *	imgfname;
	Stmt *	block;
	int		smooth;
} HFStmtData;

static void ExecHFStmt(Stmt *stmt);
static void DeleteHFStmt(Stmt *stmt);

StmtProcs hf_stmt_procs =
{
	TK_HEIGHT_FIELD,
	ExecHFStmt,
	DeleteHFStmt
};

Stmt *ParseHeightFieldStmt(int smooth)
{
	int				nparams, prev_object_token;
	Param			params[2];
	HFStmtData *	sd;
	Stmt *			stmt = NewStmt();
	if (stmt == NULL)
	{
		LogMemError("height_field");
		return NULL;
	}
	stmt->procs = &hf_stmt_procs;
	
	sd = (HFStmtData *)calloc(1, sizeof(HFStmtData));
	if (sd == NULL)
	{
		DeleteStmt(stmt);
		LogMemError("height_field");
		return NULL;
	}
	sd->imgfname = NULL;
	sd->block = NULL;
	sd->smooth = smooth;

	stmt->data = (void *)sd;
	
	prev_object_token = cur_object_token;
	cur_object_token = TK_HEIGHT_FIELD;

	nparams = ParseParams("SOB", "height_field", ParseObjectDetails, params);
	cur_object_token = prev_object_token;

	sd->imgfname = params[0].data.str;
	sd->block = params[1].data.block;

	if (!error_count)
		return stmt;
	
	DeleteStmt(stmt);
	return NULL;
}

void ExecHFStmt(Stmt *stmt)
{
	HFStmtData *sd = (HFStmtData *)stmt->data;
	Image *img = NULL;
	Object *obj, *oldobj;
	FILE *fp;

	/* Load an Image from the filename */
	assert(sd->imgfname != NULL);
	if ((fp = SCN_FindFile(sd->imgfname, READBIN,
		scn_include_paths, SCN_FINDFILE_CHK_CUR_FIRST)) != NULL)
	{
		img = Image_Load(fp, sd->imgfname); 
		fclose(fp);
		if (img == NULL)
		{
			LogError("Unable to load image file: %s", sd->imgfname);
			PrintFileAndLineNumber();
			return;
		}
	}
	else
	{
		LogError("Unable to open image file: %s", sd->imgfname);
		PrintFileAndLineNumber();
		return;
	}

	if ((obj = Ray_MakeHField(img)) != NULL)
	{
		if (sd->smooth)
			obj->flags |= OBJ_FLAG_SMOOTH;
		obj->surface = Ray_ShareSurface(default_surface);
		ScnBuild_AddObject(obj);
		oldobj = objstack_ptr->curobj;
		objstack_ptr->curobj = obj;
		ExecBlock(stmt, sd->block);
		objstack_ptr->curobj = oldobj;
	}
}

void DeleteHFStmt(Stmt *stmt)
{
	HFStmtData *sd = (HFStmtData *)stmt->data;
	if (sd != NULL)
	{
		if (sd->imgfname != NULL)
			free(sd->imgfname);
		DeleteStatements(sd->block);
		free(sd);
	}
}
