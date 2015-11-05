/*************************************************************************
*
*  rend2dc.h - The Rend2D C library API.
*
*************************************************************************/

#ifndef REND2DC_H
#define REND2DC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Maximum level of sub division for adaptive anti-aliasing.
 */
#define MAX_AA_DEPTH  4


/*************************************************************************
*
*  Color calculation proc. 
*  Generates a color for the screen UV coordinate passed in "u" and "v".
*  UV range is determined by the "umin", "umax", "vmin", and "vmax"
*  fields in the Rend2D struct.
*  Set RGB color in pointers "r", "g", and "b". 0 = min, 255 = max.
*  Ruturn non-zero to set current pixel, or zero to ignore this color
*  and not set a pixel.
*  If there is more than one sample per pixel (eg. anti-aliasing), just
*  those calls to the color proc that return non-zero will be averaged
*  into the total pixel color. For the rest, a specified background color
*  will be averaged into the total pixel color. If all calls to color
*  proc in a super-sampled pixel return zero, then no pixel will be set.
*
*************************************************************************/
typedef int (*ColorProc) /* Return non-zero to set current pixel. */
	(
	double u,            /* Screen U value: umin <= u < umax */
	double v,            /* Screen V value: vmin <= v < vmax */
	unsigned char *r,    /* Return red level (0 - 255) */
	unsigned char *g,    /* Return green level (0 - 255) */
	unsigned char *b     /* Return blue level (0 - 255) */
	);


/*************************************************************************
*
*  Data structure that is passed to Rend2D_SetState() and 
*    Rend2D_GetState(). Used to set up the renderer and get info from
*    the renderer.
*
*************************************************************************/
typedef struct tag_rend2d
{
	/* These fields are set by the user to setup the renderer. */
	/* Rendering mode - see REND2D_MODE_XXX codes below. */
	int mode;
	/* True if the renderer is in preview mode. */
	int preview;
	/* Min and max bounds for the UV values passed to color proc. */
	double umin, umax, vmin, vmax;
	/* Screen resolution. */
	int xres, yres;
	/* Start and end points on screen */
	int xstart, xend, ystart, yend;
	/* Adaptive anti-aliasing threshold. */
	int aa_threshold;
	/* Adaptive anti-aliasing level. */
	int aa_level;
	/* Jitter scale. 0.0 = none, 1.0 = +- one pixel. */
	double jitter;
	/* Color proc - Calc a color for current UV point on screen. */
	ColorProc calc_color;
	/* Background color for color proc calls that return zero in
	 * a super-sampled pixel.
	 */
	unsigned char bgr, bgg, bgb;

	/* These fields are set by the renderer. */
	/* Present state of the renderer - see REND2D_STATUS_XXX codes below. */
	int status;
	/* Current pixel being rendered. */
	int x, y;  
	/* Step increments for x and y. */
	int xstep, ystep;
	double uwidth, vheight;
	/* Misc. internal info. */
	int even;
	int xevenstep;
} Rend2D;


/*************************************************************************
*
*  Data structure used to pass pixel information between the rendered
*  and the application.
*
*************************************************************************/
typedef struct tag_pixeldata
{
int x, y;               /* Top left corner of pixel. */
int width, height;      /* Width and height of pixel. */
unsigned char r, g, b;  /* Color of pixel. */
} Rend2DPixel;


/*************************************************************************
*
*  Status codes returned by Rend2D_Init(), Rend2D_DoPixel(), and
*    Rend2D_GetStatus() and in the "status" field of the Rend2D struct
*    after a call to Rend2D_GetState().
*
*************************************************************************/
enum
{
	REND2D_STATUS_NOT_INITIALIZED = 0,
	REND2D_STATUS_READY,
	REND2D_STATUS_RENDERING,
	REND2D_STATUS_PAUSED,
	REND2D_STATUS_FINISH,
	REND2D_STATUS_OUT_OF_MEMORY,
	REND2D_NUM_STATUS_CODES
};


/*************************************************************************
*
*  Rendering modes for the "mode" entry in the Rend2D struct.
*
*************************************************************************/
enum
{
	REND2D_MODE_ONCE_PER_PIXEL = 0,
	REND2D_MODE_ADAPTIVE_ANTIALIAS,
	REND2D_NUM_MODE_CODES
};


/*************************************************************************
*
*  Rend2D API
*
*************************************************************************/
extern int   Rend2D_Init(void);
extern void  Rend2D_Close(void);
extern int   Rend2D_GetStatus(void);

extern void  Rend2D_GetState(Rend2D *r);
extern void  Rend2D_SetState(Rend2D *r);
extern int   Rend2D_GetX(void);
extern int   Rend2D_GetY(void);
extern int   Rend2D_GetXStep(void);
extern int   Rend2D_GetYStep(void);
extern void  Rend2D_Start(void);
extern int   Rend2D_DoPixel(Rend2DPixel *pixel);
extern int   Rend2D_EndOfLine(void);
extern void  Rend2D_Stop(void);


#ifdef __cplusplus
}
#endif


#endif   /* REND2DC_H */