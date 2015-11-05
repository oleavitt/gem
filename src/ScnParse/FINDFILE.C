/*************************************************************************
 *
 *  findfile.c - File managment services for SCN file reader.
 *
 ************************************************************************/

#include "scn.h"

#define PATH_DELIM  '/'
#define MSDOS_PATH_DELIM  '\\'


/*************************************************************************
 * File search paths.
 */
char *scn_bin_paths;
char *scn_ini_paths;
char *scn_include_paths;
char *scn_source_paths;
char *scn_bitmap_paths;

void FindFileInitialize(void)
{
  scn_bin_paths = NULL;
  scn_ini_paths = NULL;
  scn_include_paths = NULL;
  scn_source_paths = NULL;
  scn_bitmap_paths = NULL;
}

void FindFileClose(void)
{
  str_free(scn_bin_paths);
  str_free(scn_ini_paths);
  str_free(scn_include_paths);
  str_free(scn_source_paths);
  str_free(scn_bitmap_paths);
  scn_bin_paths = NULL;
  scn_ini_paths = NULL;
  scn_include_paths = NULL;
  scn_source_paths = NULL;
  scn_bitmap_paths = NULL;
}

/*************************************************************************
 *
 *  SCN_FindFile() - Attempt to open file: "name". First checking current
 *    directory, if specified in "flags". Failing that, search through
 *    a list of paths: "paths". Failing that, return the NULL "fp".
 *
 *************************************************************************/
FILE *SCN_FindFile(const char *name, const char *mode,	const char *paths,
  unsigned char flags)
{
  FILE *fp;
  char *path, *all_paths, *b;
  static char buf[FILENAME_MAX];

  if(flags & SCN_FINDFILE_CHK_CUR_FIRST)
    fp = fopen(name, mode);

  if(fp == NULL && paths != NULL)
  {
    if((all_paths = (char *)malloc((strlen(paths)+1)*sizeof(char)))
      != NULL)
    {
      strcpy(all_paths, paths);
      path = strtok(all_paths, "; ");
      while(path != NULL && fp == NULL)
      {
        for(b = buf; *path != '\0'; )
          *b++ = *path++;
        if((*(b-1) != PATH_DELIM) && (*(b-1) != MSDOS_PATH_DELIM))
          *b++ = PATH_DELIM;
        *b = '\0';
        strcat(buf, name);
        fp = fopen(buf, mode);
        path = strtok(NULL, "; ");
      }
      free(all_paths);
    }
  }
  return fp;
}

void SCN_AddPath(char **pathlist, const char *newpath)
{
  if(newpath == NULL)
    return;

  if(*pathlist != NULL)
  {
    size_t old_size = (strlen(*pathlist) + 1) * sizeof(char);
    *pathlist = mrealloc(*pathlist, old_size, old_size +
      (strlen(newpath) + 1) * sizeof(char));
    strcat(*pathlist, ";");
    strcat(*pathlist, newpath);
  }
  else
    *pathlist = str_dup(newpath);
}


