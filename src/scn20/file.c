/**
 *****************************************************************************
 *  @file file.c
 *  File managment stuff.
 *
 *****************************************************************************
 */

#include "local.h"

/*
 * Maximum include file nesting depth.
 */
#define MAX_INCLUDE_DEPTH  16
static FILEDATA *filestack = NULL;
static int include_level;

int file_Init(void)
{
	int i;
	include_level = 0;
	filestack = (FILEDATA *)calloc(MAX_INCLUDE_DEPTH, sizeof(FILEDATA));
	if(filestack == NULL)
	{
		logmemerror("file");
		return 0;
	}
	for(i = 0; i < MAX_INCLUDE_DEPTH; i++)
		filestack[i].fp = NULL;
	return 1;
}

void file_Close(void)
{
	if(filestack != NULL)
	{
		int i;
		for(i = 0; i < MAX_INCLUDE_DEPTH; i++)
		{
			if(filestack[i].fp != NULL)
			{
				fclose(filestack[i].fp);
				filestack[i].fp = NULL;
			}
		}
		free(filestack);
		filestack = NULL;
	}
}

FILEDATA *file_OpenMain(const char *fname)
{
	assert(include_level == 0);

	if((filestack->fp = SCN_FindFile(fname, READ,
		scn_source_paths, SCN_FINDFILE_CHK_CUR_FIRST)) != NULL)
	{
		strcpy(filestack->name, fname);
		filestack->line_num = 1;
	}
	else
	{
		logerror("Unable to open: %s", fname);
		file_Close();
		return NULL;
	}

	return filestack; /* &filestack[0] */
}

FILEDATA *file_OpenInclude(const char *fname)
{
	if(include_level < MAX_INCLUDE_DEPTH)
	{
		include_level++;
		if((filestack[include_level].fp = SCN_FindFile(fname, READ,
			scn_include_paths, SCN_FINDFILE_CHK_CUR_FIRST)) != NULL)
		{
			strcpy(filestack[include_level].name, fname);
			filestack[include_level].line_num = 1;
		}
		else
		{
			include_level--;
			logerror("Unable to open include file: %s", fname);
			file_PrintFileAndLineNumber();
		}
	}
	else
	{
		logerror("Too many nested include files!");
		logmsg("  Maximum nested include file depth = %d",
			MAX_INCLUDE_DEPTH);
		logmsg("  Attempted to open: %s", fname);
		file_PrintFileAndLineNumber();
	}
	return &filestack[include_level];
}

FILEDATA *file_CloseInclude(void)
{
	assert(filestack[include_level].fp != NULL);
	fclose(filestack[include_level].fp);
	filestack[include_level].fp = NULL;
	if(include_level > 0)
	{
		include_level--;
		return &filestack[include_level];
	}
	else
		return NULL;
}

void file_PrintFileAndLineNumber(void)
{
	if(filestack != NULL)
	{
		logmsg("  File: %s, Line: %d", 
			filestack[include_level].name, filestack[include_level].line_num);
	}
}
