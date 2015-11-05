/*************************************************************************
*
*  local.h - Common stuff needed within the Rend2D library.
*
*************************************************************************/

#ifndef LOCAL_H
#define LOCAL_H

#include "config.h"
#include "rend2dc.h"   /* Rend2D library externals. */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/*
 * rend2d.c
 */
/* Working copy of user settings for the renderer. */
extern Rend2D rend;

/*
 * dopixel.c
 */
/* Pointer to the appropriate pixel proc for rendering mode. */
extern void (*DoPixel)(Rend2DPixel *pixel);
extern void (*DoPixelStartOfLine)(void);
extern void (*DoPixelEndOfLine)(void);
extern void (*DoPixelSetup)(void);
extern void (*DoPixelCleanup)(void);

extern void DoPixelOnce(Rend2DPixel *pixel);
extern void DoPixelStartOfLineOnce(void);
extern void DoPixelEndOfLineOnce(void);
extern void DoPixelSetupOnce(void);
extern void DoPixelCleanupOnce(void);

extern void DoPixelAdaptiveAA(Rend2DPixel *pixel);
extern void DoPixelStartOfLineAdaptiveAA(void);
extern void DoPixelEndOfLineAdaptiveAA(void);
extern void DoPixelSetupAdaptiveAA(void);
extern void DoPixelCleanupAdaptiveAA(void);


#endif  /* LOCAL_H */