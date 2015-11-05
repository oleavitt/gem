/**
 *****************************************************************************
 *  @file global.c
 *  Global variables for SCN 2.0
 *
 *****************************************************************************
 */

#include "local.h"



/** Error status code. */
int g_scn20error = SCN_OK;

/** If non-zero, statements are compiled for the virtual machine */
int g_compile_mode = 0;

/** If non-zero, object being created is to be a defined symbol */
int g_define_mode = 0;

/** If non-zero, we are parsing a function declaration, except 'main' */
int g_function_mode = 0;

/** If g_define_mode is set, this buffer will contain the name 
 *  of the new symbol.
 */
char g_define_name[MAX_TOKEN_LEN];

/**
 *	Pointer to the ray trace setup data that has been passed into
 *	the parser.
 */
RaySetupData * g_rsd = NULL;

/** Contains the scanned text of a parsed token */
char g_token_buffer[MAX_TOKEN_LEN];

/** Points to details about the token that was last scanned */
TOKEN *g_cur_token;
