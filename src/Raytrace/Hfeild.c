/*************************************************************************
*
*  hfield.c
*
*  The height field primitive.
*
*************************************************************************/

#include "ray.h"

unsigned long ray_hfield_tests;
unsigned long ray_hfield_hits;

static int IntersectHField(Object *obj, HitData *hits);
static void CalcNormalHField(Object *obj, Vec3 *P, Vec3 *N);
static int IsInsideHField(Object *obj, Vec3 *P);
static void CalcUVMapHField(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsHField(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformHField(Object *obj, Vec3 *params, int type);
static void CopyHField(Object *destobj, Object *srcobj);
static void DeleteHField(Object *obj);
static void DrawHField(Object *obj);

/* Helper functions. */
static void IntersectCell(int x, int y, double zmin, double zmax);
static void InsertHit(double t, Vec3 *N);
static HFHit *NewHFHit(void);
static void DeleteHFHit(HFHit *h);
static HFHit *GetNextHFHit(HFHit *h);
static HFLocalData *NewHFLocalData(void);
static void DeleteHFLocalData(HFLocalData *hfl);
static HFieldData *NewHFData(void);
static void DeleteHFData(HFieldData *hf);

#define sgn(n) (((n)>0) ? 1 : -1)

static Vec3 B, D;           /* Global transformed ray base and direction. */
static double tmin, tmax;   /* Ray interval that is with HF's bounding box. */
static Object *hf_object;   /* Global ptr to object. */
static HFieldData *hf_data; /* Global ptr to object's HF data. */
static HFLocalData *hf_localdata; /* Ptr to object's local HF data. */

static ObjectProcs hfield_procs =
{
	OBJ_HFIELD,
	IntersectHField,
	CalcNormalHField,
	IsInsideHField,
	CalcUVMapHField,
	CalcExtentsHField,
	TransformHField,
	CopyHField,
	DeleteHField,
	DrawHField
};


Object *Ray_MakeHField(Image *img)
{
	HFieldData *	hf = NULL;
	HFLocalData *	hfl = NULL;
	Object *		obj = NULL;

	assert(img != NULL);

	if ((hf = NewHFData()) == NULL)
		return NULL;
	img->nusers++;  /* Need a reference copy. */
	hf->img = img;
	if ((hfl = NewHFLocalData()) == NULL)
		goto fail_create;
	if ((obj = NewObject()) == NULL)
		goto fail_create;
	if ((obj->T = Ray_NewXform()) == NULL)
		goto fail_create;

	V3Set(&hf->bmin, 0.0, 0.0, 0.0);
	V3Set(&hf->bmax, (double)(hf->img->xres - 1),
		(double)(hf->img->yres - 1), 1.0);
	V3Set(&B, 2.0 / hf->bmax.x, 2.0 / hf->bmax.y, 1.0);
	XformXforms(obj->T, &B, XFORM_SCALE);
	V3Set(&B, -1.0, -1.0, 0.0);
	XformXforms(obj->T, &B, XFORM_TRANSLATE);
	hf->bmin.z -= EPSILON;
	hf->bmax.z += EPSILON;

	obj->data.hf = hf;
	obj->localdata.hf = hfl;
	obj->procs = &hfield_procs;

	return obj;

	fail_create:
	DeleteHFData(hf);
	DeleteHFLocalData(hfl);
	Ray_DeleteObject(obj);

	return NULL;
}


HFieldData *NewHFData(void)
{
	HFieldData *hf = (HFieldData *)Calloc(1, sizeof(HFieldData));
	if (hf != NULL)
	{
		hf->nrefs = 1;
		hf->img = NULL;
		hf->qtree = NULL;
	}
	return hf;
}

void DeleteHFData(HFieldData *hf)
{
	if (hf != NULL)
	{
		if (--hf->nrefs == 0)
		{
			Delete_Image(hf->img);
			Free(hf->qtree, hf->qtreesize);
			Free(hf, sizeof(HFieldData));
		}
	}
}


int IntersectHField(Object *obj, HitData *hits)
{
	hf_data = obj->data.hf;
	hf_localdata = obj->localdata.hf;

	ray_hfield_tests++;

	/*
	 * Transform ray base point and direction cosines to object
	 * coordinates.
	 */
	B = ct.B;
	D = ct.D;
	assert(obj->T != NULL);  /* Must have transform matrix. */
	PointToObject(&B, obj->T);
	DirToObject(&D, obj->T);

	hf_localdata->nhits = 0;
 	if (Intersect_Box(&B, &D, &hf_data->bmin, &hf_data->bmax, &tmin, &tmax))
	{
		if ((tmax > ct.tmin) && (tmin < ct.tmax))
		{
			double d, x1, y1, z1, x2, y2, z2, ax, ay, dx, dy, dz,
				slope, zslope, z;
			int x, y, sx, sy;

			hf_object = obj;

			/* Get entry and exit points of ray through HF's box... */
			if (tmin < ct.tmin)
				tmin = ct.tmin;
			if (tmax > ct.tmax)
				tmax = ct.tmax;
			x1 = B.x + D.x * tmin;
			y1 = B.y + D.y * tmin;
			z1 = B.z + D.z * tmin;
			x2 = B.x + D.x * tmax;
			y2 = B.y + D.y * tmax;
			z2 = B.z + D.z * tmax;

			/*
			 * Step through points connecting the start and end points using
			 * Bresenham's line drawing algorithm, checking for height field
			 * cell intersections at each point...
			 */
			dx = x2 - x1;
			dy = y2 - y1;
			dz = z2 - z1;
			ax = fabs(dx);
			ay = fabs(dy);
			sx = sgn(dx);
			sy = sgn(dy);
			x = (int)x1;
			y = (int)y1;

			if ((x == (int)x2) && (y == (int)y2))
			{ /* Ray only passes through one cell. */
				IntersectCell(x, y, z1, z2);
			}
			else if (ax > ay)    /* X dominant. */
			{
				slope = ay / ax * (double)sy;
				zslope = dz / ax;
				if (x1 > x2)
				{
					y1 -= slope * ((int)(x1 + 1) - x1);
					z = z1 - ((int)(x1 + 1) - x1) * zslope;
					x1 = (int)x1 + 1;
				}
				else
				{
					y1 -= slope * (x1 - (int)x1);
					z = z1 - (x1 - (int)x1) * zslope;
					x1 = (int)x1;
				}
				ax = fabs(x2 - x1) + 1;
				d = y1;
				for (;;)
				{
					IntersectCell(x, y, z, z + zslope);
					if (fabs((double)x - x1) > ax)
						break;
					if ((int)(d + slope) != y)
					{
						y += sy;
					}
					else
					{
						x += sx;
						d += slope;
						z += zslope;
					}
				}
			}
			else           /* Y dominant. */
			{
				slope = ax / ay * (double)sx;
				zslope = dz / ay;
				if (y1 > y2)
				{
					x1 -= slope * ((int)(y1 + 1) - y1);
					z = z1 - ((int)(y1 + 1) - y1) * zslope;
					y1 = (int)y1 + 1;
				}
	  			else
				{
					x1 -= slope * (y1 - (int)y1);
					z = z1 - (y1 - (int)y1) * zslope;
					y1 = (int)y1;
				}
				ay = fabs(y2 - y1) + 1;
				d = x1;
				for (;;)
				{
  					IntersectCell(x, y, z, z + zslope);
  					if (fabs((double)y - y1) > ay)
	  					break;
		  			if ((int)(d + slope) != x)
					{
						x += sx;
					}
					else
					{
						y += sy;
						d += slope;
						z += zslope;
					}
				}
			}
		}

		/* Copy hit data, if any, to caller's hit list. */
		if (hf_localdata->nhits)
		{
			HFHit *h;
			int i, entering;

			ray_hfield_hits++;
			h = hf_localdata->hits;
			entering = ((hf_localdata->nhits & 1) == 0);
			if (obj->flags & OBJ_FLAG_INVERSE)
				entering = 1 - entering;
			for (i = 0; i < hf_localdata->nhits; i++)
			{
				hits->obj = obj;
				hits->t = h->t;
				hits->entering = entering;
				entering = 1 - entering;
				hits = GetNextHit(hits);
				h = h->next;
			}
		}
	}

	return hf_localdata->nhits;
}


void IntersectCell(int x, int y, double zmin, double zmax)
{
	unsigned short	z1, z2, z3, z4;
	double			d, t, u, v, fz1, fz2, fz3, fz4, z;
	Vec3			P, N;

	/* Get the "z" values for the four corners of HF pixel. */
	Image_GetHeightFieldCell(hf_data->img, x, y, &z1, &z2, &z3, &z4);
	fz1 = (double)z1 / (double)USHRT_MAX;
	fz2 = (double)z2 / (double)USHRT_MAX;
	fz3 = (double)z3 / (double)USHRT_MAX;
	fz4 = (double)z4 / (double)USHRT_MAX;

	if (zmin > zmax)
	{
		z = zmin;
		zmin = zmax;
		zmax = z;
	}

	/* Does ray intersect bounding box containing triangles? */
	z = fmax(fz1, fz2);
	if (fz3 > z) 
		z = fz3;
	if (fz4 > z)
		z = fz4;
	if (z < zmin)
		return;
	z = fmin(fz1, fz2);
	if (fz3 < z)
		z = fz3;
	if (fz4 < z)
		z = fz4;
	if (z > zmax)
		return;

	/* Test the z1, z2, z3 triangle. */

	P.x = B.x - (double)x;
	P.y = B.y - (double)y;
	P.z = B.z - fz1;
	N.x = fz1 - fz2;
	N.y = fz1 - fz3;
	N.z = 1.0;
	V3Normalize(&N);
	d = V3Dot(&N, &D);
	if (fabs(d) > EPSILON)
	{
		t = -V3Dot(&N, &P) / d;
		if ((t > tmin) && (t < tmax))
		{
			u = P.x + D.x * t; v = P.y + D.y * t;
			if ((u >= 0.0) && (v >= 0.0) && ((u + v) <= 1.0))
				InsertHit(t, &N);
		}
	}

	/* Test the z4, z3, z2 triangle. */
	x++; y++;
	P.x = B.x - (double)x;
	P.y = B.y - (double)y;
	P.z = B.z - fz4;
	N.x = fz3 - fz4;
	N.y = fz2 - fz4;
	N.z = 1.0;
	V3Normalize(&N);
	d = V3Dot(&N, &D);
	if (fabs(d) > EPSILON)
	{
		t = -V3Dot(&N, &P) / d;
		if ((t > tmin) && (t < tmax))
		{
			u = P.x + D.x * t; v = P.y + D.y * t;
			if ((u <= 0.0) && (v <= 0.0) && ((u + v) >= -1.0))
				InsertHit(t, &N);
		}
	}
}


void CalcNormalHField(Object *obj, Vec3 *P, Vec3 *N)
{
	HFieldData *	hf;
	HFLocalData *	hfl;
	HFHit *			h;
	int				i;
	Vec3			Pt;

	hf = obj->data.hf;
	hfl = obj->localdata.hf;

	V3Copy(&Pt, P);
	assert(obj->T != NULL);  /* Must have transform matrix. */
	PointToObject(&Pt, obj->T);

	/* What triangle did we hit? */
	h = hfl->hits;
	for (i = hfl->nhits; i > 0; i--)
	{
		assert(h != NULL);
		if (fabs(h->t - ct.t) < EPSILON)
			break;
		h = h->next;
	}
	assert(i > 0);  /* A match must always be found! */

	/*
	 * If smooth shading is enabled, interpolate from the normals at
	 * each vertex to get actual normal for Pt, else use patch's
	 * plane normal.
	 */
	if (obj->flags & OBJ_FLAG_SMOOTH)
	{
		unsigned short	z;
		double			fz1, fz2, fz3, fztmp, u, v, w;
		int				x, y, x2, y2;
		Vec3			N1, N2, N3, N4;

		x = (int)Pt.x; y = (int)Pt.y; x2 = x + 1; y2 = y + 1;
		z = Image_GetHeightFieldPixel(hf->img, x, y);
		fz1 = (double)z / (double)USHRT_MAX;
		z = Image_GetHeightFieldPixel(hf->img, x2, y);
		fz2 = (double)z / (double)USHRT_MAX;
		z = Image_GetHeightFieldPixel(hf->img, x, y2);
		fz3 = (double)z / (double)USHRT_MAX;
		N1.x = fz1 - fz2; N1.y = fz1 - fz3; N1.z = 1.0;
		V3Normalize(&N1);

		fz1 = fz2;
		z = Image_GetHeightFieldPixel(hf->img, x2 + 1, y);
		fz2 = (double)z / (double)USHRT_MAX;
		fztmp = fz3;
		z = Image_GetHeightFieldPixel(hf->img, x2, y2);
		fz3 = (double)z / (double)USHRT_MAX;
		N2.x = fz1 - fz2; N2.y = fz1 - fz3; N2.z = 1.0;
		V3Normalize(&N2);

		fz1 = fztmp;
		fz2 = fz3;
		z = Image_GetHeightFieldPixel(hf->img, x, y2 + 1);
		fz3 = (double)z / (double)USHRT_MAX;
		N3.x = fz1 - fz2; N3.y = fz1 - fz3; N3.z = 1.0;
		V3Normalize(&N3);

		fz1 = fz2;
		z = Image_GetHeightFieldPixel(hf->img, x2 + 1, y2);
		fz2 = (double)z / (double)USHRT_MAX;
		z = Image_GetHeightFieldPixel(hf->img, x2, y2 + 1);
		fz3 = (double)z / (double)USHRT_MAX;
		N4.x = fz1 - fz2; N4.y = fz1 - fz3; N4.z = 1.0;
		V3Normalize(&N4);

		/*
		u = Pt.x - (int)Pt.x; v = Pt.y - (int)Pt.y;
		N->x = (N1.x * (1.0 - u) + N2.x * u) * (1.0 - v) +
			(N3.x * (1.0 - u) + N4.x * u) * v;
		N->y = (N1.y * (1.0 - u) + N2.y * u) * (1.0 - v) +
			(N3.y * (1.0 - u) + N4.y * u) * v;
		N->z = (N1.z * (1.0 - u) + N2.z * u) * (1.0 - v) +
			(N3.z * (1.0 - u) + N4.z * u) * v;
		*/

		u = Pt.x - (int)Pt.x; v = Pt.y - (int)Pt.y; w = u + v;
		if (w < 1.0)
		{
			w = 1.0 - w;
			N->x = w * N1.x + u * N2.x + v * N3.x;
			N->y = w * N1.y + u * N2.y + v * N3.y;
			N->z = w * N1.z + u * N2.z + v * N3.z;
		}
		else
		{
			u = 1.0 - u; v = 1.0 - v; w -= 1.0;
			N->x = w * N4.x + u * N3.x + v * N2.x;
			N->y = w * N4.y + u * N3.y + v * N2.y;
			N->z = w * N4.z + u * N3.z + v * N2.z;
		}

	}
	else
		V3Copy(N, &h->tri_norm);

	NormToWorld(N, obj->T);
}


int IsInsideHField(Object *obj, Vec3 *P)
{
	HFieldData *	hf = obj->data.hf;
	Vec3			Pt;
	unsigned short	z1, z2, z3, z4;
	double			fz1, fz2, fz3, fz4, u, v, w, z;

	/*
	 * Transform world point, "P", to point in height field's coordinate
	 * system, "Pt".
	 */
	V3Copy(&Pt, P);
	assert(obj->T != NULL);  /* Must have transform matrix. */
	PointToObject(&Pt, obj->T);

	/* First, see if Pt is within height field's bounding box... */
	if (Pt.x < hf->bmin.x || Pt.x > hf->bmax.x)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */

	if (Pt.y < hf->bmin.y || Pt.y > hf->bmax.y)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */

	if (Pt.z < hf->bmin.z || Pt.z > hf->bmax.z)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */

	/*
	 * See if Pt is "under" its corresponding triangle.
	 * Get the "z" values for the four corners of HF pixel.
	 */
	Image_GetHeightFieldCell(hf->img, (int)Pt.x, (int)Pt.y,
	&z1, &z2, &z3, &z4);
	fz1 = (double)z1 / (double)USHRT_MAX;
	fz2 = (double)z2 / (double)USHRT_MAX;
	fz3 = (double)z3 / (double)USHRT_MAX;
	fz4 = (double)z4 / (double)USHRT_MAX;

	/* Is point completely above or below triangles? */
	z = fmax(fz1, fz2); if(fz3 > z) z = fz3; if(fz4 > z) z = fz4;
	if (z < Pt.z)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */
	z = fmin(fz1, fz2); if(fz3 < z) z = fz3; if(fz4 < z) z = fz4;
	if (z > Pt.z)
		return (!(obj->flags & OBJ_FLAG_INVERSE)); /* inside */

	/*
	 * Point is within bounds of the triangle pair. Find out which
	 * triangle half point is in and interpolate to see if point is
	 * within that triangle's plane.
	 */
	u = Pt.x - (int)Pt.x; v = Pt.y - (int)Pt.y; w = u + v;
	if (w < 1.0)
	{
		w = 1.0 - w;
		z = w * fz1 + u * fz2 + v * fz3;
		if (z < Pt.z)
			return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */
	}
	else
	{
		u = 1.0 - u; v = 1.0 - v; w -= 1.0;
		z = w * fz4 + u * fz3 + v * fz2;
		if (z < Pt.z)
			return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */
	}

	return (!(obj->flags & OBJ_FLAG_INVERSE)); /* inside */
}


void CalcUVMapHField(Object *obj, Vec3 *P, double *u, double *v)
{
	HFieldData *	hf = obj->data.hf;

	/* Calc normalized map, (0,0) <= (u,v) < (1,1), of HF's XY plane. */
	*u = P->x / (double)hf->img->xres;
	*v = P->y / (double)hf->img->yres;
}


void CalcExtentsHField(Object *obj, Vec3 *omin, Vec3 *omax)
{
	HFieldData *	hf = obj->data.hf;

	omin->x = hf->bmin.x - EPSILON;
	omax->x = hf->bmax.x + EPSILON;
	omin->y = hf->bmin.y - EPSILON;
	omax->y = hf->bmax.y + EPSILON;
	omin->z = hf->bmin.z - EPSILON;
	omax->z = hf->bmax.z + EPSILON;

	assert(obj->T != NULL);  /* Must have transform matrix. */
	BBoxToWorld(omin, omax, obj->T);
}


void TransformHField(Object *obj, Vec3 *params, int type)
{
	assert(obj->T != NULL);  /* Must have transform matrix. */
 	XformXforms(obj->T, params, type);
}


void CopyHField(Object *destobj, Object *srcobj)
{
	HFieldData *	hf = srcobj->data.hf;
	if((destobj->localdata.hf = NewHFLocalData()) != NULL)
	{
		hf->nrefs++;
		destobj->data.hf = hf;
	}
}


void DeleteHField(Object *obj)
{
	DeleteHFLocalData(obj->localdata.hf);
	DeleteHFData(obj->data.hf);
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

void DrawHField(Object *obj)
{
	HFieldData *	hf = obj->data.hf;
	Vec3			pt;

	assert(obj->T != NULL);  /* Must have transform matrix. */

	/* If bbox only. */
	V3Set(&pt, hf->bmin.x, hf->bmin.y, hf->bmin.z);
	PointToWorld(&pt, obj->T);
	Set_Pt(0, pt.x, pt.y, pt.z);
	V3Set(&pt, hf->bmax.x, hf->bmin.y, hf->bmin.z);
	PointToWorld(&pt, obj->T);
	Set_Pt(1, pt.x, pt.y, pt.z);
	V3Set(&pt, hf->bmin.x, hf->bmax.y, hf->bmin.z);
	PointToWorld(&pt, obj->T);
	Set_Pt(2, pt.x, pt.y, pt.z);
	V3Set(&pt, hf->bmax.x, hf->bmax.y, hf->bmin.z);
	PointToWorld(&pt, obj->T);
	Set_Pt(3, pt.x, pt.y, pt.z);
	V3Set(&pt, hf->bmin.x, hf->bmin.y, hf->bmax.z);
	PointToWorld(&pt, obj->T);
	Set_Pt(4, pt.x, pt.y, pt.z);
	V3Set(&pt, hf->bmax.x, hf->bmin.y, hf->bmax.z);
	PointToWorld(&pt, obj->T);
	Set_Pt(5, pt.x, pt.y, pt.z);
	V3Set(&pt, hf->bmin.x, hf->bmax.y, hf->bmax.z);
	PointToWorld(&pt, obj->T);
	Set_Pt(6, pt.x, pt.y, pt.z);
	V3Set(&pt, hf->bmax.x, hf->bmax.y, hf->bmax.z);
	PointToWorld(&pt, obj->T);
	Set_Pt(7, pt.x, pt.y, pt.z);
	Move_To(0);
	Line_To(1);
	Line_To(3);
	Line_To(2);
	Line_To(0);
	Line_To(4);
	Line_To(5);
	Line_To(7);
	Line_To(6);
	Line_To(4);
	Move_To(1);
	Line_To(5);
	Move_To(3);
	Line_To(7);
	Move_To(2);
	Line_To(6);

	/* Else, draw the actual object. */
}


/*************************************************************************
*
*  Helper functions.
*
*************************************************************************/

void InsertHit(double t, Vec3 *N)
{
	HFHit *h, *p, *last, *plast = NULL;
	int i;

	hf_localdata->nhits++;

	last = hf_localdata->hits;
	for (i = 1; i < hf_localdata->nhits; i++)
	{
		plast = last;
		last = GetNextHFHit(last);
	}
	V3Copy(&last->tri_norm, N);
	last->t = t;
	if (hf_localdata->nhits > 1)
	{
		plast->next = last->next;
		p = NULL;
		h = hf_localdata->hits;
		for (i = 1; i < hf_localdata->nhits; i++)
		{
			if (t < h->t)
			{
				last->next = h;
				if (p != NULL)
					p->next = last;
				else
					hf_localdata->hits = last;
				break;
			}
			p = h;
			h = h->next;
		}
		if (i == hf_localdata->nhits)
			plast->next = last;
	}
}

HFHit *NewHFHit(void)
{
	HFHit *h = (HFHit *)Calloc(1, sizeof(HFHit));
	h->next = NULL;
	return h;
}

void DeleteHFHit(HFHit *h)
{
	Free(h, sizeof(HFHit));
}

HFHit *GetNextHFHit(HFHit *h)
{
	if (h->next == NULL)
		h->next = NewHFHit();
	return h->next;
}


HFLocalData *NewHFLocalData(void)
{
	HFLocalData *hfl = (HFLocalData *)Calloc(1, sizeof(HFLocalData));
	hfl->hits = NewHFHit();
	return hfl;
}

void DeleteHFLocalData(HFLocalData *hfl)
{
	HFHit *h;
	if (hfl != NULL)
	{
		while (hfl->hits != NULL)
		{
			h = hfl->hits;
			hfl->hits = h->next;
			DeleteHFHit(h);
		}
	}
	Free(hfl, sizeof(HFLocalData));
}

