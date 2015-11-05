/*************************************************************************
*
*  image.c - Functions for loading pixel image files and reading pixel
*  image maps.
*
*  4/23/95 - image.c created.
*  12/07/01 - code cleanup, separate height_field image list removed.
*
*************************************************************************/

#include "local.h"

/* The image map list. */
static Image *images = NULL;



/*************************************************************************
*
*  Image_Load() - Given a filename, see if image of that name has
*    already been loaded in image list. If not, pass name to the various
*    format-specific file loaders and, if successful, add an Image data
*    structure to the image-map list.
*    Returns a pointer to the image if successful, NULL if error.
*
*************************************************************************/
Image *Image_Load(FILE *fp, char *imname)
{
	Image *img;

	/* First, see if we got it already... */
	for (img = images; img != NULL; img = img->next)
	{
		if (strcmp(img->name, imname) == 0)
		{
			img->nusers++;
			return img;
		}
	}

	/*
	 * See if file is of a type that is supported.
	 */
	/* if((img = BMP_LoadImage(fp)) == NULL) */
		img = Targa_LoadImage(fp);

	/* Add image to list if file was successfully read. */
	if (img != NULL)
	{
		strcpy(img->name, imname);
		img->nusers = 1;
		img->next = images;  /* NULL if this the first. */
		images = img;
	}

	/* NULL if not successful. */
	return img;
}



unsigned short Image_GetHeightFieldPixel(Image *img, int x, int y)
{
	unsigned char *pi;
	int i, bytes;
	unsigned short z;

	x = x % img->xres;
	y = y % img->yres;

	switch (img->bits)
	{
		case 1:
			/*
			 * Lookup bytes containing pixels in the image bitmap.
			 * Shift offset bit to byte boundary and mask out, the pixel bit.
			 */
			bytes = x / 8;
			i = *(img->image + y*((img->xres+7)/8) + bytes);
			i = ((i << (x - bytes*8)) & 0x80) ? 1 : 0;
			if (img->pal != NULL)  /* Lookup 16 bit height from palette. */
			{
				register unsigned char *pp = img->pal + i * 3;
				z = (unsigned short)(*pp++ << 8);
				z |= *pp;
			}
			else   /* Use index value directly as max height or bottom. */
			{
				z = (unsigned short)(i ? USHRT_MAX : 0);
			}
			break;

		case 8:
			/*
			 * Lookup 8 bit height indices in the image bitmap.
			 */
			pi = img->image;
			i = *(pi + y*img->xres + x);
			if (img->pal != NULL)  /* Lookup 16 bit height from palette. */
			{
				register unsigned char *pp = img->pal + i * 3;
				z = (unsigned short)(*pp++ << 8);
				z |= *pp;
			}
			else     /* Use index value directly as an 8 bit height. */
			{
				z = (unsigned short)(i << 8);
			}
			break;

		case 16:
			/*
			 * Lookup 16 bit height indices in the image bitmap.
			 */
			pi = img->image + (y*img->xres + x) * 2;
			i = *pi++ << 8;
			z = (unsigned short)(*pi | i);
			break;

		default: /* 24 */
			/*
			 * Lookup 16 bit height indices in the image bitmap.
			 * Ignore the last 8 bits (blue).
			 */
			pi = img->image + (y*img->xres + x) * 3;
			i = *pi++ << 8;
			z = (unsigned short)(*pi | i);
			break;
	}

	return z;
}


void Image_GetHeightFieldCell(Image *img, int x, int y,
	unsigned short *z1, unsigned short *z2,
	unsigned short *z3, unsigned short *z4)
{
	unsigned char *pi;
	int x2, y2, i1, i2, i3, i4, bytes;

	if (y < 0)
		y = 0;
	if (x < 0)
		x = 0;

	x = x % img->xres;
	y = y % img->yres;
	x2 = (x + 1) % img->xres;
	y2 = (y + 1) % img->yres;

	switch (img->bits)
	{
		case 1:
			/* Lookup bytes containing pixels in the image bitmap. */
			/* Shift offset bit to byte boundary  */
			/* and mask out, the pixel bit.       */
			bytes = x / 8;
			i1 = *(img->image + y*((img->xres+7)/8) + bytes);
			i1 = ((i1 << (x - bytes*8)) & 0x80) ? 1 : 0;
			i3 = *(img->image + y2*((img->xres+7)/8) + bytes);
			i3 = ((i3 << (x - bytes*8)) & 0x80) ? 1 : 0;
			bytes = x2 / 8;
			i2 = *(img->image + y*((img->xres+7)/8) + bytes);
			i2 = ((i2 << (x2 - bytes*8)) & 0x80) ? 1 : 0;
			i4 = *(img->image + y2*((img->xres+7)/8) + bytes);
			i4 = ((i4 << (x2 - bytes*8)) & 0x80) ? 1 : 0;
			if (img->pal != NULL)  /* Lookup 16 bit height from palette. */
			{
				register unsigned char *pp = img->pal + i1 * 3;
				*z1 = (unsigned short)(*pp++ << 8);
				*z1 |= *pp;
				pp = img->pal + i2 * 3;
				*z2 = (unsigned short)(*pp++ << 8);
				*z2 |= *pp;
				pp = img->pal + i3 * 3;
				*z3 = (unsigned short)(*pp++ << 8);
				*z3 |= *pp;
				pp = img->pal + i4 * 3;
				*z4 = (unsigned short)(*pp++ << 8);
				*z4 |= *pp;
			}
			else   /* Use index value directly as max height or bottom. */
			{
				*z1 = (unsigned short)(i1 ? USHRT_MAX : 0);
				*z2 = (unsigned short)(i2 ? USHRT_MAX : 0);
				*z3 = (unsigned short)(i3 ? USHRT_MAX : 0);
				*z4 = (unsigned short)(i4 ? USHRT_MAX : 0);
			}
			break;

		case 8:
			/* Lookup 8 bit height indices in the image bitmap. */
			pi = img->image;
			i1 = *(pi + y*img->xres + x);
			i2 = *(pi + y*img->xres + x2);
			i3 = *(pi + y2*img->xres + x);
			i4 = *(pi + y2*img->xres + x2);
			if (img->pal != NULL)  /* Lookup 16 bit height from palette. */
			{
				register unsigned char *pp = img->pal + i1 * 3;
				*z1 = (unsigned short)(*pp++ << 8);
				*z1 |= *pp;
				pp = img->pal + i2 * 3;
				*z2 = (unsigned short)(*pp++ << 8);
				*z2 |= *pp;
				pp = img->pal + i3 * 3;
				*z3 = (unsigned short)(*pp++ << 8);
				*z3 |= *pp;
				pp = img->pal + i4 * 3;
				*z4 = (unsigned short)(*pp++ << 8);
				*z4 |= *pp;
			}
			else     /* Use index value directly as an 8 bit height. */
			{
				*z1 = (unsigned short)(i1 << 8);
				*z2 = (unsigned short)(i2 << 8);
				*z3 = (unsigned short)(i3 << 8);
				*z4 = (unsigned short)(i4 << 8);
			}
			break;

		case 16:
			/* Lookup 16 bit height indices in the image bitmap. */
			pi = img->image + (y*img->xres + x) * 2;
			i1 = *pi++ << 8;
			*z1 = (unsigned short)(*pi | i1);
			pi = img->image + (y*img->xres + x2) * 2;
			i2 = *pi++ << 8;
			*z2 = (unsigned short)(*pi | i2);
			pi = img->image + (y2*img->xres + x) * 2;
			i3 = *pi++ << 8;
			*z3 = (unsigned short)(*pi | i3);
			pi = img->image + (y2*img->xres + x2) * 2;
			i4 = *pi++ << 8;
			*z4 = (unsigned short)(*pi | i4);
			break;

		default: /* 24 */
			/* Lookup 16 bit height indices in the image bitmap. */
			/* Ignore the last 8 bits (blue). */
			pi = img->image + (y*img->xres + x) * 3;
			i1 = *pi++ << 8;
			*z1 = (unsigned short)(*pi | i1);
			pi = img->image + (y*img->xres + x2) * 3;
			i2 = *pi++ << 8;
			*z2 = (unsigned short)(*pi | i2);
			pi = img->image + (y2*img->xres + x) * 3;
			i3 = *pi++ << 8;
			*z3 = (unsigned short)(*pi | i3);
			pi = img->image + (y2*img->xres + x2) * 3;
			i4 = *pi++ << 8;
			*z4 = (unsigned short)(*pi | i4);
			break;
	}
}


/*************************************************************************
*
*  Image_Map() - Lookup a pixel in an Image array, based on
*    UV coordinates in the range of 0->1, if "uv_flag" is set,
*    put its color in the vector "color" and return palette index
*    if 8 bit, or 1 or 0 if 1 bit.
*
*************************************************************************/
int Image_Map(Image *img, Vec3 *color, double u, double v, int uv_flag)
{
	int row, col, index, bytes;

	if (uv_flag)
	{
		/* Keep u and v within 0->1 range. */
		u -= floor(u);
		v -= floor(v);

		col = (int)(img->xres * u) % img->xres;
		row = (int)(img->yres * v) % img->yres;
	}
	else
	{
		col = (int)u % img->xres;
		row = (int)v % img->yres;
	}

	switch (img->bits)
	{
		case 1:
			bytes = col / 8;
			/* Lookup byte containing pixel in the image bitmap. */
			index = *(img->image + row*((img->xres+7)/8) + bytes);
			/* Shift offset bit to byte boundary  */
			/* and mask out, the pixel bit.       */
			index = ((index << (col-bytes*8)) & 0x80) ? 1 : 0;
			if (img->pal != NULL)  /* Lookup color from a palette. */
			{
				register unsigned char *p = img->pal + index * 3;
				color->x = (double)*p++ / 255;
				color->y = (double)*p++ / 255;
				color->z = (double)*p / 255;
			}
			else     /* Use index value directly as black or white. */
			{
				color->x = (double)index;
				color->y = (double)index;
				color->z = (double)index;
			}
			break;

		case 8:
			/* Lookup a palette index in the image bitmap. */
			index = *(img->image + row*img->xres + col);
			if (img->pal != NULL)  /* Lookup color from a palette. */
			{
				register unsigned char *p = img->pal + index * 3;
				color->x = (double)*p++ / 255;
				color->y = (double)*p++ / 255;
				color->z = (double)*p / 255;
			}
			else     /* Use index value directly as a gray-scale. */
			{
				register float gray;

				gray = (float)index / 255;
				color->x = gray;
				color->y = gray;
				color->z = gray;
			}
			break;

		default: /* 24 */
			{
				register unsigned char *p = img->image + (row*img->xres+col)*3;
				/* Lookup RGB color triplet from image bitmap. */
				color->x = (double)*p++ / 255;
				color->y = (double)*p++ / 255;
				color->z = (double)*p / 255;
				index = 0;
			}
			break;
	}

	/* index = palette index if 8 bit. index = 1 or 0 if 1 bit. */
	return index;
}


/*************************************************************************
*
*  Image_SmoothMap() - Map UV point to image, interpolating color
*    between colors of four corners of image pixel.
*
*************************************************************************/
void Image_SmoothMap(Image *img, Vec3 *color, double u, double v)
{
	Vec3	c1, c2, c3, c4;
	double	iu, iv, w1, w2, w3, w4;

	/* Keep u and v within 0->1 range. */
	u -= floor(u);
	v -= floor(v);
	u *= (double)img->xres;
	v *= (double)img->yres;

	/* Get colors at the integral corners of pixel... */
	Image_Map(img, &c1, u, v, 0);
	Image_Map(img, &c2, u + 1.0, v, 0);
	Image_Map(img, &c3, u, v + 1.0, 0);
	Image_Map(img, &c4, u + 1.0, v + 1.0, 0);

	/* Interpolate to get composite color for point in pixel... */
	u -= floor(u);
	v -= floor(v);
	iu = 1.0 - u;
	iv = 1.0 - v;
	w1 = iu * iv;
	w2 = u * iv;
	w3 = iu * v;
	w4 = u * v;
	color->x = w1 * c1.x + w2 * c2.x + w3 * c3.x + w4 * c4.x;
	color->y = w1 * c1.y + w2 * c2.y + w3 * c3.y + w4 * c4.y;
	color->z = w1 * c1.z + w2 * c2.z + w3 * c3.z + w4 * c4.z;
}


/*************************************************************************
*
*  Copy_Image() - Bump up the reference counter when an Image reference
*    is shared.
*
*************************************************************************/
Image *Copy_Image(Image *img)
{
	if (img != NULL)
		img->nusers++;
	return img;
}


/*************************************************************************
*
*  Delete_Image() - Delete an image if there are no other references
*    to it.
*
*************************************************************************/
void Delete_Image(Image *img)
{
	Image *prev;
	
	if (img == NULL)
		return;

	if (--img->nusers > 0)
		return;

	if (img == images)
		images = img->next;
	else
	{
		for (prev = images; prev != NULL; prev = prev->next)
			if (prev->next == img) break;
		if(prev == NULL)
			return; /* couldn't find it, possibly an invalid pointer */
		/* Remove img from the list. */
		prev->next = img->next;
	}

	if (img->pal != NULL)
		free(img->pal);
	if (img->image != NULL)
		free(img->image);	
	free(img);
}


/*************************************************************************
*
*  Image_Initialize() - Initialize the image module.
*
*************************************************************************/
void Image_Initialize(void)
{
	images = NULL;
}


/*************************************************************************
*
*  Image_Close() - Do any needed cleanup.
*
*************************************************************************/
void Image_Close(void)
{
}

