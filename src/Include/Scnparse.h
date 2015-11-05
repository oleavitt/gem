/*************************************************************************
*
*  scnparse.h - API for the SCN file parser library.
*
*************************************************************************/

#ifndef SCNPARSE_H
#define SCNPARSE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "raytrace.h"
#include "findfile.h"
#include "expr.h"

/*
 * Message type codes for message logging function.
 */
enum
{
  SCN_MSG_MESSAGE = 0,
  SCN_MSG_WARNING,
  SCN_MSG_ERROR,
  SCN_MSG_FATAL
};

/*
 * Maximum size of message strings.
 */
#define MAX_MESSAGE_SIZE 256

/*
 * User supplied message logging proc.
 */
extern void (*SCN_LogMsg)(const char *msg, int type);


/*
 * Top level initialization and shutdown functions.
 */
extern void SCN_ColdStart(void);
extern void SCN_WarmStart(void);
extern void SCN_Shutdown(void);
extern int SCN_NextFrame(void);
extern void SCN_Message(int type, const char *msg, ...);
extern void SCN_Parse(const char *fname, RaySetupData *rsd);
extern void Init_Proc_Stack(void);
extern void Close_Proc_Stack(void);


/*************************************************************************
*
*  Scnparse global variables.
*
*************************************************************************/
/* Memory usage counter. */
extern size_t scn_mem_used;

/* Animation counters. */
extern int scn_start_frame;
extern int scn_end_frame;
extern int scn_cur_frame;
extern double scn_normalized_frame;
extern double scn_normalized_frame2;


/*************************************************************************
*
*  Error codes.
*
*************************************************************************/
enum
{
  SCN_ERROR_NONE = 0,
  SCN_ERROR_ALLOC
};

/* Gets set to any of the above error codes. */
extern int scn_error;

extern int scn_warning_cnt;
extern int scn_error_cnt;
extern int scn_fatal;

#ifdef __cplusplus
}
#endif

#endif   /* SCNPARSE_H */