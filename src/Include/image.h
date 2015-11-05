/**
 *****************************************************************************
 *  @file image.h
 *  Interface to the image file operations.
 *
 *****************************************************************************
 */

#ifndef IMAGE_H
#define IMAGE_H

#include "math3d.h"   /* Vec3 type */
#include <stdio.h>    /* FILE type */

/*************************************************************************
 * Internal pixel image map data.
 */
typedef struct tag_img
{
	struct tag_img *next;		/* Next Image in linked list.		*/
	char name[FILENAME_MAX];	/* Name of image.					*/
	int nusers;					/* # refs to this image handed out.	*/
	int xres, yres;				/* Resolution.						*/
	int bits;					/* Bits per pixel.					*/
	int palsize;				/* Palette size in bytes.			*/
	unsigned char *pal;			/* Palette if color mapped.			*/
	unsigned char *image;		/* Pointer to image bitmap.			*/
} Image;


extern void Image_Initialize(void);
extern void Image_Close(void);
extern Image *Image_Load(FILE *fp, char *imname);
extern int Image_Map(Image *img, Vec3 *color,
	double u, double v, int uv_flag);
extern void Image_SmoothMap(Image *img, Vec3 *color,
	double u, double v);
extern unsigned short Image_GetHeightFieldPixel(Image *img, int x, int y);
extern void Image_GetHeightFieldCell(Image *img, int x, int y,
	unsigned short *z1, unsigned short *z2,
	unsigned short *z3, unsigned short *z4);
extern Image *Copy_Image(Image *img);
extern void Delete_Image(Image *img);

/* Output file options. */
extern int outfile_rle;
extern int outfile_bits;
extern int outfile_dither;
extern int outfile_resume;
extern int outfile_xres, outfile_yres;
extern char outfile_name[];

/* File buffer size. */
extern size_t file_buf_size;


#endif  /* IMAGE_H */
