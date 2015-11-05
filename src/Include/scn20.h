/**
 *****************************************************************************
 *  @file scn20.h
 *  External functions, defiens and types for the SCN 2.0 parser
 *
 *****************************************************************************
 */

#ifndef __SCN20_H__
#define __SCN20_H__

/*
 * Reuse generic defs from 1.0
 */
//#include "scn10.h"
#include "vm.h"
#include "findfile.h"

/*************************************************************************/
/*
 * Defines
 */

/* 
 * Maximum possible size of result returned from ExprEval().
 */
#define EXPR_RESULT_SIZE_MAX           sizeof(Vec3)

/* 
 * Maximum size of message strings output to client's message function.
 */
#define SCN_MSG_MAX                    256


/*************************************************************************/
/*
 * Types
 */

typedef void (*MSGFN)(const char *msg);



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



/*************************************************************************/
/*
 * Funtion protos
 */

/**
 *	Initializes the SCN 2.0 library.
 */
extern int scn20_initialize(void);

/**
 *	Closes the SCN 2.0 library.
 */
extern int scn20_close(void);

/**
 *	Resets the SCN 2.0 library, but doesn't completely close everything.
 */
extern int scn20_reset(void);

/**
 *	Parses and generates the scene from a scene discription file.
 */
extern int scn20_parse(
	const char *	fname,
	RaySetupData *	rsd,
	const char *	searchpaths
	);

/**
 *	Supplies a user defined message output function to the parser..
 */
extern MSGFN scn20_set_msgfn(MSGFN msgfn);

/**
 *	Returns the error code of the last error that occured.
 */
extern int scn20_get_error_status(void);


#endif /* __SCN20_H__ */