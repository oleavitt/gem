/*************************************************************************
*
*  draw3d.c - Modeler view 3D to screen projection and drawing functions.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"

/* Allocation size increment of points array (in points). */
#define MAX_POINTS       512
/* Allocation size increment for points array. */
#define POINT_SIZE      (sizeof(POINT))

static int npoints;    /* Number of points used in array. */
static int maxpoints;  /* Max number of points in array. */
static POINT *points;  /* The points array. */

static Vec3 vWinMin, vWinMax, vWinCenter;
static int projection_plane, cxWindow, cyWindow, cxCenter, cyCenter;
static double widest_span;

static HDC hDC;

/*************************************************************************
*
*  Initialization, setup and cleanup.
*
*************************************************************************/

int Draw3D_Initialize(void)
{
	V3Set(&vWinMin, -1.0, -1.0, -1.0);
	V3Set(&vWinMax, 1.0, 1.0, 1.0);
	V3Zero(&vWinCenter);
	projection_plane = DRAW3D_XY;
	npoints = 0;
	maxpoints = MAX_POINTS;
	if ((points = calloc(MAX_POINTS, POINT_SIZE)) == NULL)
		return 0;
	return 1;
}


void Draw3D_Close(void)
{
	if (points != NULL)
		free(points);
	points = NULL;
}


void Draw3D_SetWindow(Vec3 *wmin, Vec3 *wmax, int width, int height,
	int plane, HDC hdc)
{
	double f;

	V3Copy(&vWinMin, wmin);
	V3Copy(&vWinMax, wmax);
	vWinCenter.x = (vWinMin.x + vWinMax.x) / 2.0;
	vWinCenter.y = (vWinMin.y + vWinMax.y) / 2.0;
	vWinCenter.z = (vWinMin.z + vWinMax.z) / 2.0;
	cxWindow = width;
	cyWindow = height;
	cxCenter = width / 2;
	cyCenter = height / 2;
	projection_plane = plane;
	hDC = hdc;
	widest_span = wmax->x - wmin->x;
	f = wmax->y - wmin->y;
	if (fabs(f) > fabs(widest_span))
		widest_span = f;
	f = wmax->z - wmin->z;
	if (fabs(f) > fabs(widest_span))
		widest_span = f;
}


/*************************************************************************
*
*  Drawing functions that are passed to Ray_DrawScene().
*
*************************************************************************/

void Draw3D_SetPt(int pt_ndx, double x, double y, double z)
{
	int ix, iy;
	assert(points != NULL);
	if (pt_ndx < 0)  /* Reset the point list. */
		npoints = 0;
	else
	{
		if (pt_ndx >= npoints)
			npoints = pt_ndx + 1;
		if (npoints > maxpoints)
		{
			POINT *newpoints, *pold, *pnew;
			int oldsize;

			oldsize = maxpoints;
			while (npoints > maxpoints)
				maxpoints += MAX_POINTS;
			newpoints = calloc(maxpoints, POINT_SIZE);
			pold = points;
			pnew = newpoints;
			while (oldsize--)
				*pnew++ = *pold++; 
			free(points);
			points = newpoints;
		}
		switch (projection_plane)
		{
			case DRAW3D_XY:
				ix = (int)(((x - vWinCenter.x) / widest_span) *
					(double)cxWindow) + cxCenter;
				iy = cyWindow - (int)(((y - vWinCenter.y) / widest_span) *
					(double)cyWindow) - cyCenter;
				points[pt_ndx].x = (vWinMin.x > vWinMax.x) ? (ix + cxWindow) : ix; 
				points[pt_ndx].y = (vWinMin.y > vWinMax.y) ? (iy + cyWindow) : iy; 
				break;
			case DRAW3D_XZ:
				ix = (int)(((x - vWinCenter.x) / widest_span) *
					(double)cxWindow) + cxCenter;
				iy = cyWindow - (int)(((z - vWinCenter.z) / widest_span) *
					(double)cyWindow) - cyCenter;
				points[pt_ndx].x = (vWinMin.x > vWinMax.x) ? (ix + cxWindow) : ix; 
				points[pt_ndx].y = (vWinMin.z > vWinMax.z) ? (iy + cyWindow) : iy; 
				break;
			default:  /* DRAW3D_YZ */
				ix = (int)(((y - vWinCenter.y) / widest_span) *
					(double)cxWindow) + cxCenter;
				iy = cyWindow - (int)(((z - vWinCenter.z) / widest_span) *
					(double)cyWindow) - cyCenter;
				points[pt_ndx].x = (vWinMin.y > vWinMax.y) ? (ix + cxWindow) : ix; 
				points[pt_ndx].y = (vWinMin.z > vWinMax.z) ? (iy + cyWindow) : iy; 
				break;
		}
	}
}


void Draw3D_MoveTo(int pt_ndx)
{
	assert(points != NULL);
	assert(pt_ndx < npoints);
	MoveToEx(hDC, points[pt_ndx].x, points[pt_ndx].y, NULL);
}


void Draw3D_LineTo(int pt_ndx)
{
	assert(points != NULL);
	assert(pt_ndx < npoints);
	LineTo(hDC, points[pt_ndx].x, points[pt_ndx].y);
}
