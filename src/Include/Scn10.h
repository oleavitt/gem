/*************************************************************************
*
*  scn10.h - The version 1.0 scene file parser API.
*
*************************************************************************/

#ifndef SCN10_H
#define SCN10_H

#include "expr.h"
#include "findfile.h"

/*************************************************************************
*
*  Error status codes.
*
*************************************************************************/
enum
{
	SCN_OK = 0,
	SCN_INIT_FAIL,
	SCN_ERR_PARSE,
	SCN_ERR_FILE_OPEN,
	SCN_ERR_FILE_READ,
	SCN_ERR_FILE_WRITE,
	SCN_ERR_FILE_NOEXIST,
	SCN_ERR_FILE_DENIED,
	SCN_ERR_ALLOC_FAIL,
	SCN_ERR_OLD_VERSION,
	SCN_NUM_ERR_CODES
};

/*************************************************************************
*
*  Expression return result codes.
*
*************************************************************************/
enum
{
	EXPR_NULL = 0,
	EXPR_INT,
	EXPR_FLOAT,
	EXPR_VECTOR
};

/* 
 * Maximum possible size of result returned from ExprEval().
 */
#define EXPR_RESULT_SIZE_MAX    sizeof(Vec3)

/* 
 * Maximum size of message strings output to client's message function.
 */
#define SCN_MSG_MAX     256

typedef void (*MSGFN)(const char *msg);

extern MSGFN Scn10_SetMsgFunc(MSGFN msgfn);
extern int Scn10_Initialize(void);
extern int Scn10_Parse(const char *fname, RaySetupData *rsd,
	const char *searchpaths);

#endif  /* SCN10_H */
