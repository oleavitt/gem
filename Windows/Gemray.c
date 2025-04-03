/*************************************************************************
*
*  gemray.c - Handles application-level ray-trace renderer details.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"

/*
 * Raytrace renderer data.
 */
int RaytracePixel(double u, double v,
  unsigned char *r, unsigned char *g, unsigned char *b);
RaySetupData rsd;             /* Setup info for the ray-tracer. */
Rend2D renderer;              /* Setup info for the 2D renderer. */


/*************************************************************************
*
*  int RaytracePixel(double u, double v,
*    unsigned char *r, unsigned char *g, unsigned char *b)
*
*  Callback function for the 2D renderer.
*
*  Returns 1 (Rend2D requires a return value for background purposes)
*
*************************************************************************/
int RaytracePixel(double u, double v,
  unsigned char *r, unsigned char *g, unsigned char *b)
{
  Vec3 color;
  Ray_TraceRayFromViewport(u, v, &color);
  color.x = CLAMP(color.x, 0.0, 0.999999);
  color.y = CLAMP(color.y, 0.0, 0.999999);
  color.z = CLAMP(color.z, 0.0, 0.999999);
  *r = (unsigned char)(color.x * 256.0);
  *g = (unsigned char)(color.y * 256.0);
  *b = (unsigned char)(color.z * 256.0);
  return 1;
}


/*************************************************************************
*
*  BOOL CheckRayError(void)
*
*  Handles error message reporting.
*
*  Returns TRUE if an error code was returned from the renderer.
*
*************************************************************************/
BOOL CheckRayError(void)
{
  static TCHAR str[256];
  static TCHAR msgstr[256];
  switch(ray_error)
  {
    case RAY_ERROR_NONE:
      return FALSE;
    case RAY_ERROR_ALLOC:
      wsprintf(str, _T("Out of memory!"));
      break;
    default:
      wsprintf(str, _T("Unkown error code."));
      break;
  }
  wsprintf(msgstr, _T("Error: Raytrace: (%d) %s\n"), str);
  MessageBox(GetFocus(), msgstr, g_szAppName, MB_OK | MB_ICONEXCLAMATION);
  return TRUE;
}

