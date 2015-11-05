/*************************************************************************
*
*  file.c - File managment stuff.
*
*************************************************************************/

#include "local.h"

/*
 * Maximum include file nesting depth.
 */
#define MAX_INCLUDE_DEPTH  16
static FILEDATA *filestack = NULL;
static int include_level;

int File_Init(void)
{
	int i;
	include_level = 0;
	filestack = (FILEDATA *)calloc(MAX_INCLUDE_DEPTH, sizeof(FILEDATA));
	if(filestack == NULL)
	{
		LogMemError("file");
		return 0;
	}
	for(i = 0; i < MAX_INCLUDE_DEPTH; i++)
		filestack[i].fp = NULL;
	return 1;
}

void File_Close(void)
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

FILEDATA *File_OpenMain(const char *fname)
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
		LogError("Unable to open: %s", fname);
		File_Close();
		return NULL;
	}

	return filestack; /* &filestack[0] */
}

FILEDATA *File_OpenInclude(const char *fname)
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
			LogError("Unable to open include file: %s", fname);
			PrintFileAndLineNumber();
		}
	}
	else
	{
		LogError("Too many nested include files!");
		LogMessage("  Maximum nested include file depth = %d",
			MAX_INCLUDE_DEPTH);
		LogMessage("  Attempted to open: %s", fname);
		PrintFileAndLineNumber();
	}
	return &filestack[include_level];
}

FILEDATA *File_CloseInclude(void)
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

void PrintFileAndLineNumber(void)
{
	if(filestack != NULL)
	{
		LogMessage("  File: %s, Line: %d", 
			filestack[include_level].name, filestack[include_level].line_num);
	}
}
