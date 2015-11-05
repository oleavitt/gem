/**************************************************************************
*
*  nff.h - API for the NFF scene file parser.
*
**************************************************************************/

#ifndef NFF_H
#define NFF_H

#include "raytrace.h"

enum
{
	NFF_OK = 0,
	NFF_CANT_OPEN_FILE
};

extern int Nff_Parse(const char *filename, RaySetupData *rsd);
extern void Nff_SetMsgFunc(void (*msgfn)(const char *msg));

#endif  /* NFF_H */

