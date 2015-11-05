/*************************************************************************
*
*  rend2d.c - Main implementation file for the Rend2D library.
*
*************************************************************************/

#include "local.h"

/* Working copy of user settings for the renderer. */
Rend2D rend;

/* Preview sub-division depth. */
#define PREVIEW_DEPTH    4

static int xevenoffset[] =
	{ 1, 2, 4, 8, 0 };
static int xevenstep[] =
	{ 2, 4, 8, 16, 16 };

static int DefaultCalcColor(double u, double v,
	unsigned char *r, unsigned char *g, unsigned char *b)
{
	double dx, dy, rdist, gdist, bdist;
	#define SQRT3 1.7321

	dx = u;
	dy = v + 0.875;
	rdist = sqrt(dx * dx + dy * dy);
	dx = u - 0.866;
	dy = v - 0.625;
	gdist = sqrt(dx * dx + dy * dy);
	dx = u + 0.866;
	dy = v - 0.625;
	bdist = sqrt(dx * dx + dy * dy);
	if(rdist < SQRT3 && gdist < SQRT3 && bdist < SQRT3)
	{
		*r = (unsigned char)(255 - (rdist/SQRT3 * 255));
		*g = (unsigned char)(255 - (gdist/SQRT3 * 255));
		*b = (unsigned char)(255 - (bdist/SQRT3 * 255));
	}
	else
	{
		dx = u + 1.0;
		dy = v + 1.0;
		*r = *g = *b = (unsigned char)((dx > dy) ? dy * 127.9 : dx * 127.9);
	}
  return 1;
}


int Rend2D_Init(void)
{
	rend.calc_color = DefaultCalcColor;
	rend.xstart = 0;
	rend.xend = 160;
	rend.ystart = 0;
	rend.yend = 120;
	rend.xres = 160;
	rend.yres = 120;
	rend.umin = -1.0;
	rend.umax = 1.0;
	rend.vmin = -1.0;
	rend.vmax = 1.0;
	rend.mode = REND2D_MODE_ONCE_PER_PIXEL;
	rend.preview = 0;
	rend.aa_threshold = 5;
	rend.aa_level = 2;
	rend.jitter = 0.0;
	rend.bgr = 0;
	rend.bgg = 0;
	rend.bgb = 0;
	rend.status = REND2D_STATUS_READY;
	rend.x = 0;
	rend.y = 0;
	rend.xstep = 0;
	rend.ystep = 0;
	rend.even = 1;
	rend.xevenstep = 0;
	return rend.status;
}


void Rend2D_Close(void)
{
	if(rend.status == REND2D_STATUS_RENDERING)
  {
  	DoPixelCleanup();
  }
	rend.status = REND2D_STATUS_NOT_INITIALIZED;
}


void Rend2D_Start(void)
{
	if(rend.status == REND2D_STATUS_READY ||
	   rend.status == REND2D_STATUS_FINISH)
	{
		rend.uwidth = rend.umax - rend.umin;
		rend.vheight = rend.vmax - rend.vmin;
		if(rend.xstart > rend.xend)
		{
			int tmp = rend.xstart;
			rend.xstart = rend.xend;
			rend.xend = tmp;
		}
		if(rend.ystart > rend.yend)
		{
			int tmp = rend.ystart;
			rend.ystart = rend.yend;
			rend.yend = tmp;
		}
		rend.x = rend.xstart;
		rend.y = rend.ystart;
		if(rend.preview > 0)
		{
			rend.preview = PREVIEW_DEPTH; /* Use "preview" as the pass counter. */
			rend.even = 1;
			rend.xstep = 1 << rend.preview;
			rend.ystep = 1 << rend.preview;
			rend.xevenstep = xevenstep[rend.preview];
			DoPixel = DoPixelOnce;
			DoPixelStartOfLine = DoPixelStartOfLineOnce;
			DoPixelEndOfLine = DoPixelEndOfLineOnce;
			DoPixelSetup = DoPixelSetupOnce;
			DoPixelCleanup = DoPixelCleanupOnce;
		}
		else
		{
			rend.xstep = 1;
			rend.xevenstep = 1;
			rend.ystep = 1;
			switch(rend.mode)
			{
				case REND2D_MODE_ADAPTIVE_ANTIALIAS:
					DoPixel = DoPixelAdaptiveAA;
					DoPixelStartOfLine = DoPixelStartOfLineAdaptiveAA;
					DoPixelEndOfLine = DoPixelEndOfLineAdaptiveAA;
					DoPixelSetup = DoPixelSetupAdaptiveAA;
					DoPixelCleanup = DoPixelCleanupAdaptiveAA;
					break;
				default: /* REND2D_MODE_ONCE_PER_PIXEL */
					DoPixel = DoPixelOnce;
					DoPixelStartOfLine = DoPixelStartOfLineOnce;
					DoPixelEndOfLine = DoPixelEndOfLineOnce;
					DoPixelSetup = DoPixelSetupOnce;
					DoPixelCleanup = DoPixelCleanupOnce;
					break;
			}
		}
		DoPixelSetup();
		if(rend.status == REND2D_STATUS_READY) /* Setup was successful. */
			rend.status = REND2D_STATUS_RENDERING;
	}
	else if(rend.status == REND2D_STATUS_PAUSED)
		rend.status = REND2D_STATUS_RENDERING;
}


int Rend2D_DoPixel(Rend2DPixel *pixel)
{
	if(rend.status == REND2D_STATUS_RENDERING)
	{
		if(rend.y == rend.yend)
		{
			rend.status = REND2D_STATUS_FINISH;
		}
		else
		{
			if(rend.x == rend.xstart)
				DoPixelStartOfLine();
			pixel->x = rend.x;
			pixel->y = rend.y;
			pixel->width = rend.xstep;
			pixel->height = rend.ystep;
			DoPixel(pixel);
			pixel->height = rend.yend - rend.y;
			if(pixel->height > rend.ystep)
				pixel->height = rend.ystep;
			rend.x += (rend.even ? rend.xevenstep : rend.xstep);
			if(rend.x >= rend.xend)
			{
				DoPixelEndOfLine();
				if((pixel->x + pixel->width) > rend.xend)
					pixel->width = rend.xend - pixel->x;
				rend.y += rend.ystep;
				rend.even = 1 - rend.even;
				if(rend.y >= rend.yend)
				{
					if(rend.preview)
					{
						rend.preview--;
						rend.x = rend.xstart;
						rend.y = rend.ystart;
						rend.xstep = 1 << rend.preview;
						rend.ystep = 1 << rend.preview;
						rend.xevenstep = xevenstep[rend.preview];
						rend.even = 1;
					}
					else
						DoPixelCleanup();
				}
				rend.x = rend.xstart + ((rend.even && rend.xevenstep > 1) ?
					xevenoffset[rend.preview] : 0);
			}
		}
	}
	return rend.status;
}


int Rend2D_EndOfLine(void)
{
	return ((rend.x + rend.xstep) == rend.xend);
}


int Rend2D_GetX(void)
{
  return rend.x;
}


int Rend2D_GetY(void)
{
  return rend.y;
}


int Rend2D_GetXStep(void)
{
	return rend.xstep;
}


int Rend2D_GetYStep(void)
{
	return rend.ystep;
}


void Rend2D_Stop(void)
{
	if(rend.status == REND2D_STATUS_RENDERING)
		rend.status = REND2D_STATUS_PAUSED;
}


int Rend2D_GetStatus(void)
{
	return rend.status;
}


void Rend2D_GetState(Rend2D *r)
{
	*r = rend;
}


void Rend2D_SetState(Rend2D *r)
{
	if(rend.status == REND2D_STATUS_READY ||
	   rend.status == REND2D_STATUS_FINISH)
	{
	 	rend = *r;
		if(rend.calc_color == NULL)
			rend.calc_color = DefaultCalcColor;
		if(rend.xres == 0)
			rend.xres = 1;
		if(rend.yres == 0)
			rend.yres = 1;
		if(rend.uwidth == 0.0)
			rend.uwidth = 1.0;
		if(rend.vheight == 0.0)
			rend.vheight = 1.0;
		rend.x = rend.xstart;
		rend.y = rend.ystart;
		rend.xstep = (rend.xstart < rend.xend) ? 1 : -1;
		rend.ystep = (rend.ystart < rend.yend) ? 1 : -1;
		rend.uwidth = rend.umax - rend.umin;
		rend.vheight = rend.vmax - rend.vmin;
		rend.status = REND2D_STATUS_READY;
	}
}

