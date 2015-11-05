/*************************************************************************
*
*  dopixel.c - Handles the pixel level details.
*
*************************************************************************/

#include "local.h"

/* Pointers to the appropriate pixel procs for rendering mode. */
void (*DoPixel)(Rend2DPixel *pixel);
void (*DoPixelStartOfLine)(void);
void (*DoPixelEndOfLine)(void);
void (*DoPixelSetup)(void);
void (*DoPixelCleanup)(void);

typedef struct tag_icolor
{
	int r, g, b;
} IColor;

#define AAGRIDSIZE  (1<<MAX_AA_DEPTH)
int threshsqrd;
int level;
double uinc, vinc;
unsigned char *this_line, *next_line, *tl, *nl;

unsigned char cooked[AAGRIDSIZE+1][AAGRIDSIZE+1];
IColor samples[AAGRIDSIZE+1][AAGRIDSIZE+1];


static int SampleColorGrid(int gridx, int gridy, int size);
static void SubDividePixel(IColor *color, int gridx, int gridy, int size); 
static double Frand(register long s);
static void Jitter(double *u, double *v, double uscale, double vscale);

/*************************************************************************
*
*  DoPixelOnce()
*
*  Just sample the top-left corner of pixel.
*
*************************************************************************/
void DoPixelOnce(Rend2DPixel *pixel)
{
	double u, v;
	u = rend.umin + ((double)pixel->x /	(double)rend.xres) * rend.uwidth;
	v = rend.vmin + ((double)pixel->y /	(double)rend.yres) * rend.vheight;
	Jitter(&u, &v, uinc * rend.jitter, vinc * rend.jitter); 
	rend.calc_color(u, v, &pixel->r, &pixel->g, &pixel->b);
}

void DoPixelStartOfLineOnce(void)
{
}

void DoPixelEndOfLineOnce(void)
{
}

void DoPixelSetupOnce(void)
{
	uinc = rend.uwidth / (double)rend.xres;
	vinc = rend.vheight / (double)rend.yres;
	rend.status = REND2D_STATUS_READY;
}

void DoPixelCleanupOnce(void)
{
}


/*************************************************************************
*
*  DoPixelAdaptiveAA()
*
*  Super-sample pixel if any of the corner colors vary by more than a
*  specified threshold setting.
*
*************************************************************************/
void DoPixelAdaptiveAA(Rend2DPixel *pixel)
{
  int i;
	IColor c;
	if(rend.y > rend.ystart)
	{
		/*
		 * Load the top right corner of sample grid with coresponding value
		 * saved from the previous line. 
		 */
		samples[0][AAGRIDSIZE].r = *tl++;
		samples[0][AAGRIDSIZE].g = *tl++;
		samples[0][AAGRIDSIZE].b = *tl++;
		cooked[0][AAGRIDSIZE] = 1;
	}
	/* Sub divide pixel. */
	level = rend.aa_level;
	SubDividePixel(&c, 0, 0, AAGRIDSIZE);
	pixel->r = (unsigned char)c.r;
	pixel->g = (unsigned char)c.g;
	pixel->b = (unsigned char)c.b;
	*nl++ = (unsigned char)samples[AAGRIDSIZE][0].r;
	*nl++ = (unsigned char)samples[AAGRIDSIZE][0].g;
	*nl++ = (unsigned char)samples[AAGRIDSIZE][0].b;
	for(i = 0; i <= AAGRIDSIZE; i++)
	{
		cooked[i][0] = cooked[i][AAGRIDSIZE];
		memset(&cooked[i][1], 0, sizeof(unsigned char)*AAGRIDSIZE);
		samples[i][0] = samples[i][AAGRIDSIZE];
	}
}

void DoPixelStartOfLineAdaptiveAA(void)
{
	/* Reset the line ptrs. */
	tl = this_line;
	nl = next_line;
	/* Clear all "cooked" flags. */
	memset(cooked, 0, sizeof(unsigned char)*(AAGRIDSIZE+1)*(AAGRIDSIZE+1));
	/*
	 * Load the top left corner of sample grid with coresponding value
	 * saved from the previous line if this is not the first line. 
	 */
	if(rend.y > rend.ystart)
	{
		samples[0][0].r = *tl++;
		samples[0][0].g = *tl++;
		samples[0][0].b = *tl++;
		cooked[0][0] = 1;
	}
}

void DoPixelEndOfLineAdaptiveAA(void)
{
	/*
	 * Save the sample at the far end of the next line.
	 * (Which has been moved to the first column of the sample grid.)
	 */
	*nl++ = (unsigned char)samples[AAGRIDSIZE][0].r;
	*nl++ = (unsigned char)samples[AAGRIDSIZE][0].g;
	*nl++ = (unsigned char)samples[AAGRIDSIZE][0].b;
  /* "next_line" becomes "this_line". */
	tl = this_line;
	this_line = next_line;
	next_line = tl;
}

void DoPixelSetupAdaptiveAA(void)
{
	size_t line_size;
	threshsqrd = rend.aa_threshold * rend.aa_threshold;
	line_size = sizeof(unsigned char) * (rend.xend - rend.xstart + 1) * 3;
	if((this_line = (unsigned char *)malloc(line_size)) == NULL)
	{
		rend.status = REND2D_STATUS_OUT_OF_MEMORY;
		return;
	}
	if((next_line = (unsigned char *)malloc(line_size)) == NULL)
	{
		free(this_line);
		this_line = NULL;
		rend.status = REND2D_STATUS_OUT_OF_MEMORY;
		return;
	}
	memset(this_line, 0, line_size);
	memset(next_line, 0, line_size);
	tl = this_line;
	nl = next_line;
	uinc = rend.uwidth / (double)rend.xres;
	vinc = rend.vheight / (double)rend.yres;
	rend.status = REND2D_STATUS_READY;
}

void DoPixelCleanupAdaptiveAA(void)
{
	if(this_line != NULL)
		free(this_line);
	this_line = NULL;
	if(next_line != NULL)
		free(next_line);
	next_line = NULL;
}


void SubDividePixel(IColor *color, int gridx, int gridy, int size)
{
	if(SampleColorGrid(gridx, gridy, size) /* There's a color difference... */
		 && (size > 1 && level > 1)) /* ...and more sub-division can be done. */
	{
		IColor tr, bl, br; /* "color" is top left. */
		/* Split grid into quadrants and recursively sub-divide each one. */
		size /= 2;
		level--;
		SubDividePixel(color, gridx, gridy, size);
		SubDividePixel(&tr, gridx+size, gridy, size);
		SubDividePixel(&bl, gridx, gridy+size, size);
		SubDividePixel(&br, gridx+size, gridy+size, size);
		level++;
		/* Average the color values from each quadrant. */
		color->r += tr.r + bl.r + br.r;
		color->r /= 4;
		color->g += tr.g + bl.g + br.g;
		color->g /= 4;
		color->b += tr.b + bl.b + br.b;
		color->b /= 4;
	}
	else
	{
		/* Average the four corners. */
		color->r = (samples[gridy][gridx].r +
		  samples[gridy][gridx+size].r +
			samples[gridy+size][gridx].r +
			samples[gridy+size][gridx+size].r) / 4;
		color->g = (samples[gridy][gridx].g +
		  samples[gridy][gridx+size].g +
			samples[gridy+size][gridx].g +
			samples[gridy+size][gridx+size].g) / 4;
		color->b = (samples[gridy][gridx].b +
		  samples[gridy][gridx+size].b +
			samples[gridy+size][gridx].b +
			samples[gridy+size][gridx+size].b) / 4;
	} 
}


/*
 * Sample the color grid and then compare the colors. If color
 * difference reaches threshold return 1, otherwise return 0.
 */
int SampleColorGrid(int gridx, int gridy, int size)
{
	double u1, v1, u2, v2, u, v, uscale, vscale;
	IColor *c1, *c2, *c3, *c4;
	int dr, dg, db;
	unsigned char r, g, b;

	c1 = &samples[gridy][gridx];
	c2 = &samples[gridy][gridx+size];
	c3 = &samples[gridy+size][gridx+size];
	c4 = &samples[gridy+size][gridx];

	u1 = rend.umin + ((double)rend.x / (double)rend.xres) * rend.uwidth +
		(double)gridx / (double)AAGRIDSIZE * uinc;
	v1 = rend.vmin + ((double)rend.y / (double)rend.yres) * rend.vheight +
		(double)gridy / (double)AAGRIDSIZE * vinc;
	uscale = (double)size / (double)AAGRIDSIZE * uinc;
	vscale = (double)size / (double)AAGRIDSIZE * vinc;
	u2 = u1 + uscale;
	v2 = v1 + vscale;
  uscale *= rend.jitter;
  vscale *= rend.jitter;

	if(!cooked[gridy][gridx])
	{
		u = u1; v = v1;
		Jitter(&u, &v, uscale, vscale);
		rend.calc_color(u, v, &r, &g, &b);
		c1->r = r;
		c1->g = g;
		c1->b = b;
		cooked[gridy][gridx] = 1;
	}
	if(!cooked[gridy][gridx+size])
	{
		u = u2; v = v1;
		Jitter(&u, &v, uscale, vscale);
		rend.calc_color(u, v, &r, &g, &b);
		c2->r = r;
		c2->g = g;
		c2->b = b;
		cooked[gridy][gridx+size] = 1;
	}
	if(!cooked[gridy+size][gridx+size])
	{
		u = u2; v = v2;
		Jitter(&u, &v, uscale, vscale);
		rend.calc_color(u, v, &r, &g, &b);
		c3->r = r;
		c3->g = g;
		c3->b = b;
		cooked[gridy+size][gridx+size] = 1;
	}
	if(!cooked[gridy+size][gridx])
	{
		u = u1; v = v2;
		Jitter(&u, &v, uscale, vscale);
		rend.calc_color(u, v, &r, &g, &b);
		c4->r = r;
		c4->g = g;
		c4->b = b;
		cooked[gridy+size][gridx] = 1;
	}

	dr = c1->r - c2->r;
	dg = c1->g - c2->g;
	db = c1->b - c2->b;
	if((dr * dr + dg * dg + db * db) >= threshsqrd)
		return 1;
	dr = c2->r - c3->r;
	dg = c2->g - c3->g;
	db = c2->b - c3->b;
	if((dr * dr + dg * dg + db * db) >= threshsqrd)
		return 1;
	dr = c3->r - c4->r;
	dg = c3->g - c4->g;
	db = c3->b - c4->b;
	if((dr * dr + dg * dg + db * db) >= threshsqrd)
		return 1;
	dr = c4->r - c1->r;
	dg = c4->g - c1->g;
	db = c4->b - c1->b;
	if((dr * dr + dg * dg + db * db) >= threshsqrd)
		return 1;
	return 0;
}


/*************************************************************************
*
*  Jitter generation functions.
*
*************************************************************************/

double Frand(register long s)
{
  s = (s << 13) ^ s;
	return (1.0 - ((s * (s * s * 15731 + 789221) + 1376312589) & 0x7FFFFFFF)
	  / 1073741824.0);
}

void Jitter(double *u, double *v, double uscale, double vscale)
{
	if(rend.jitter)
	{
		double tmp = *u;
		*u += Frand((long)(tmp * 10709 + *v * 11011)) * uscale;
		*v += Frand((long)(tmp * 12307 + *v * 10909)) * vscale;
	}
}
 