/**
 *****************************************************************************
 *  @file findfile.h
 *  File managment services for SCN file reader.
 *
 *****************************************************************************
 */

#ifndef FINDFILE_H
#define FINDFILE_H

#include <stdio.h>   /* for the FILE type */

/*************************************************************************
 * Action flags for find_file().
 */
#define SCN_FINDFILE_PATHS_ONLY     0x00
#define SCN_FINDFILE_CHK_CUR_FIRST  0x01

/*************************************************************************
 * File search paths.
 */
extern char *scn_bin_paths;
extern char *scn_ini_paths;
extern char *scn_include_paths;
extern char *scn_source_paths;
extern char *scn_bitmap_paths;

extern void FindFileInitialize(void);
extern void FindFileClose(void);
extern FILE *SCN_FindFile(const char *name, const char *mode,
  const char *paths, unsigned char flags);
extern void SCN_AddPath(char **pathlist, const char *newpath);
extern void SCN_SetPaths(char **pathlist, const char *newpaths);

#endif      /* FINDFILE_H */