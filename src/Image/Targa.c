/*************************************************************************
*
*   targa.c
*
*  Functions reading and writing Targa (.tga) format images are
*  handled here. This is also the main module for all generic
*  output file related functions and variables.
*
*  12/07/01 - code cleanup
*
*************************************************************************/

#include "local.h"

/* Output file options. */
int outfile_rle;
int outfile_bits;
int outfile_dither;

/* File buffer. */
char *file_buf;

/* Full output file name. */
char outfile_name[FILENAME_MAX];

/* File buffer size. */
size_t file_buf_size;

/*
 * Targa image header.
 */
struct TGA
{
	unsigned char	idlen;		/* ID field length.					*/
	unsigned char	cmtype;		/* Color map type.					*/
	unsigned char	imtype;		/* Image type.						*/
	unsigned short	cmstart;	/* Index of first color map entry.	*/
	unsigned short	cmcnt;		/* Number of color map entries.		*/
	unsigned char	cmbits;		/* Bits per color map entry.		*/
	unsigned short	imxstart;	/* Image x origin.					*/
	unsigned short	imystart;	/* Image y origin.					*/
	unsigned short	imxres;		/* Image width in pixels.			*/
	unsigned short	imyres;		/* IMAGE height in pixels.			*/
	unsigned char	imbits;		/* Bits per pixel.					*/
	unsigned char	imdesc;		/* IMAGE descriptor.				*/
};

#define TGA_MAPPED			0x01
#define TGA_RGB				0x02
#define TGA_MONO			0x03
#define TGA_RLE_MAPPED		0x09
#define TGA_RLE_RGB			0x0A
#define TGA_RLE_MONO		0x0B

/* Header of the Targa image being worked with. */
static struct TGA tga;
/* Buffer for current Targa line being processed. */
static unsigned char *scanline;

/* Bayer bit-pattern lookup table for dithering. */
static unsigned char bayer[8][8] =
{
	{0  ,32 ,8  ,40 ,2  ,34 ,10 ,42 },
	{48 ,16 ,56 ,24 ,50 ,18 ,58 ,26 },
	{12 ,44 ,4  ,36 ,14 ,46 ,6  ,38 },
	{60 ,28 ,52 ,20 ,62 ,30 ,54 ,22 },
	{3  ,35 ,11 ,43 ,1  ,33 ,9  ,41 },
	{51 ,19 ,59 ,27 ,49 ,17 ,57 ,25 },
	{15 ,47 ,7  ,39 ,13 ,45 ,5  ,37 },
	{63 ,31 ,55 ,23 ,61 ,29 ,53 ,21 }
};

/* Current line number being written to. */
static int Y_cur;

#define ishorzmirror() ((tga.imdesc & 0x10) ? 1 : 0)
#define isvertmirror() ((tga.imdesc & 0x20) ? 0 : 1)

static void Targa_ReadLine(FILE *fp, unsigned char *line);
static void Targa_ReadRLELine(FILE *fp, unsigned char *line);
static void Targa_ReadPalette(FILE *fp, unsigned char *pal);

/*
 * Set up a file buffer.
 * Returns 1 if an error occurs.
 */
int Targa_SetBuffer(FILE *fp)
{
	if (file_buf_size != 0)
	{
		if (file_buf == NULL)
		{
			file_buf = (char *)malloc(sizeof(char)*file_buf_size);
			if (file_buf == NULL)
				return 0;
		}
		if (setvbuf(fp, file_buf, _IOFBF, file_buf_size))
			return 0;
	}

	return 1;
}

Image *Targa_LoadImage(FILE *fp)
{
	Image *img;
	int startline, incline, endline, RLEflag, i, nbytesline;
	unsigned char *line;

	/* Read in header...*/
	tga.idlen = (unsigned char)getc(fp);
	tga.cmtype = (unsigned char)getc(fp);
	tga.imtype = (unsigned char)getc(fp);
	tga.cmstart = (unsigned short)getc(fp);
	tga.cmstart |= (unsigned short)(getc(fp) << 8);
	tga.cmcnt = (unsigned short)getc(fp);
	tga.cmcnt |= (unsigned short)(getc(fp) << 8);
	tga.cmbits = (unsigned char)getc(fp);
	tga.imxstart = (unsigned short)getc(fp);
	tga.imxstart |= (unsigned short)(getc(fp) << 8);
	tga.imystart = (unsigned short)getc(fp);
	tga.imystart |= (unsigned short)(getc(fp) << 8);
	tga.imxres = (unsigned short)getc(fp);
	tga.imxres |= (unsigned short)(getc(fp) << 8);
	tga.imyres = (unsigned short)getc(fp);
	tga.imyres |= (unsigned short)(getc(fp) << 8);
	tga.imbits = (unsigned char)getc(fp);
	tga.imdesc = (unsigned char)getc(fp);
	if(ferror(fp))
	{
		rewind(fp);
		return NULL;
	}

	/* Is it valid and of a supported type? */
	if (tga.imtype != TGA_MAPPED &&
		tga.imtype != TGA_RGB &&
		tga.imtype != TGA_MONO)
	{
		if (tga.imtype != TGA_RLE_MAPPED &&
			tga.imtype != TGA_RLE_RGB &&
			tga.imtype != TGA_RLE_MONO)
		{
			rewind(fp);
			return NULL;
		}
		RLEflag = 1;   /* Run length encoded. */
	}
	else RLEflag = 0;   /* Uncompressed. */

	if (tga.imbits != 1 &&
		tga.imbits != 8 &&
		tga.imbits != 15 &&
		tga.imbits != 16 &&
		tga.imbits != 24 &&
		tga.imbits != 32)
	{
		rewind(fp);
		return NULL;
	}

	/* Skip identifier name field. */
	fseek(fp, (long)tga.idlen, SEEK_CUR);
	if (ferror(fp) || feof(fp))
	{
		rewind(fp);
		return NULL;
	}

	/* Allocate a temporary scan line buffer. */
	scanline  = (unsigned char *)malloc(sizeof(unsigned char) *
		tga.imxres * 3);
	if (scanline == NULL)
		return NULL;

	/* Allocate an Image data structure. */
	img = (Image *)malloc(sizeof(Image));
	if (img == NULL)
	{
		free(scanline);
		return NULL;
	}

	img->xres = tga.imxres;
	img->yres = tga.imyres;
	img->palsize = 0;
	img->pal = NULL;
	img->image = NULL;

	if (tga.imbits > 8)
		img->bits = 24;
	else if(tga.imbits < 8)
		img->bits = 1;
	else
		img->bits = 8;

	/* If color mapped, get palette. */
	if (tga.cmtype)
	{
		img->palsize = sizeof(unsigned char) * 768;
		if ((img->pal = (unsigned char *)malloc(img->palsize)) == NULL)
			goto load_fail;
		Targa_ReadPalette(fp, img->pal);
	}

	/* Allocate image bitmap. */
	switch (img->bits)
	{
		case 1:
			nbytesline = (img->xres + 7) / 8;
			break;
		case 8:
			nbytesline = img->xres;
			break;
		default: // 24
			nbytesline = img->xres * 3;
			break;
	}

	if ((img->image = (unsigned char *)malloc(sizeof(unsigned char) *
		nbytesline * img->yres)) == NULL)
		goto load_fail;

	/* Determine orientation of Targa image. */
	if (isvertmirror())
	{
		startline = tga.imyres - 1;
		endline = -1;
		incline = -1;
	}
	else
	{
		startline = 0;
		endline = tga.imyres;
		incline = 1;
	}

	/* Read it in line by line. */
	for (i = startline; i != endline; i += incline)
	{
		line = img->image + i * nbytesline;
		if (RLEflag)
			Targa_ReadRLELine(fp, line);
		else
			Targa_ReadLine(fp, line);
		if (ferror(fp))
		{
			rewind(fp);
			goto load_fail;
		}
	}

	free(scanline);

	return img;

	load_fail:
	if (scanline != NULL)
		free(scanline);
	Delete_Image(img);

	return NULL;
}

void Targa_ReadPalette(FILE *fp, unsigned char *pal)
{
	int nbytes, nbits, mask, i;
	unsigned long c;
	unsigned char r, g, b;

	switch(tga.cmbits)
	{
		case 32:
			nbytes = 4;
			nbits = 8;
			break;
		case 24:
			nbytes = 3;
			nbits = 8;
			break;
		default:  /* 15 or 16 bit. */
			nbytes = 2;
			nbits = 5;
			break;
	}

	mask = (1 << nbits) - 1;

	for (i = tga.cmstart; i < tga.cmcnt; i++)
	{
		if (i >= 256)
			break;
		c = (unsigned char)getc(fp);
		c <<= 8;
		c |= (unsigned char)getc(fp);
		if (nbytes > 2)
		{
			c <<= 8;
			c |= (unsigned char)getc(fp);
			if (nbytes > 3)
				(void)getc(fp);
		}
		b = (unsigned char)(c & mask);
		c >>= nbits;
		g = (unsigned char)(c & mask);
		c >>= nbits;
		r = (unsigned char)(c & mask);
		*pal++ = r;
		*pal++ = g;
		*pal++ = b;
	}
}

void Targa_ReadLine(FILE *fp, unsigned char *line)
{
	int i, nbytes;
	unsigned short c;
	unsigned char *buf, tr, tg, tb;

	assert(line != NULL);
	if (tga.imbits <= 8)
	{
		if(tga.imbits == 1)
			nbytes = (tga.imxres + 7) / 8;
		else
			nbytes = tga.imxres;
		buf = scanline;
		for (i = 0; i < nbytes; i++)
			*buf++ = (unsigned char)getc(fp);
	}
	else   /* Else 15, 16, 24, or 32 bit images. */
	{
		nbytes = tga.imxres * 3;
		buf = scanline;
		switch (tga.imbits)
		{
			case 32:
				for (i = 0; i < tga.imxres; i++)
				{
					tb = (unsigned char)getc(fp);
					tg = (unsigned char)getc(fp);
					tr = (unsigned char)getc(fp);
					(void)getc(fp);
					*buf++ = tr;
					*buf++ = tg;
					*buf++ = tb;
				}
				break;

			case 24:
				for (i = 0; i < tga.imxres; i++)
				{
					tb = (unsigned char)getc(fp);
					tg = (unsigned char)getc(fp);
					tr = (unsigned char)getc(fp);
					*buf++ = tr;
					*buf++ = tg;
					*buf++ = tb;
				}
				break;

			default:  /* 15 or 16 bits */
				for (i = 0; i < tga.imxres; i++)
				{
					c = (unsigned short)getc(fp);
					c |= (unsigned short)(getc(fp) << 8);
					tr = (unsigned char)(((c >> 10) & 0x1F) << 3);
					tg = (unsigned char)(((c >> 5) & 0x1F) << 3);
					tb = (unsigned char)((c & 0x1F) << 3);
					*buf++ = tr;
					*buf++ = tg;
					*buf++ = tb;
				}
				break;
		}
	}  /* End of else 15, 16, 24, or 32 bit images. */

	if (ishorzmirror())
	{
		buf = scanline + (nbytes - 1);
		for (i = 0; i < nbytes; i++)
			*line++ = *buf--;
	}
	else  /* ...else not mirrored horizontally. */
	{
		memcpy(line, scanline, nbytes * sizeof(unsigned char));
	}
}

void Targa_ReadRLELine(FILE *fp, unsigned char *line)
{
	int cnt, len, key, br, bg, bb;
	unsigned short c;

	assert(line != NULL);
	len = 0;
	while (len < tga.imxres && (! ferror(fp)) && (! feof(fp)) )
	{
		key = getc(fp);
		cnt = (key & 0x7F) + 1;
		len += cnt;
		if (key & 0x80)    /* A compressed run of like pixels. */
		{
			switch (tga.imbits)
			{
				case 1:
				case 8:
					br = getc(fp);
					while (cnt--)
						*line++ = (unsigned char)br;
					break;

				case 15:
				case 16:
					bb = getc(fp);
					br = getc(fp);
					c = (unsigned short)((bb) | (br << 8));
					while (cnt--)
					{
						*line++ = (unsigned char)(((c >> 10) & 0x1F) << 3);
						*line++ = (unsigned char)(((c >> 5) & 0x1F) << 3);
						*line++ = (unsigned char)((c & 0x1F) << 3);
					}
					break;

				case 24:
					bb = getc(fp);
					bg = getc(fp);
					br = getc(fp);
					while (cnt--)
					{
						*line++ = (unsigned char)br;
						*line++ = (unsigned char)bg;
						*line++ = (unsigned char)bb;
					}
					break;

				case 32:
					bb = getc(fp);
					bg = getc(fp);
					br = getc(fp);
					(void)getc(fp);
					while (cnt--)
					{
						*line++ = (unsigned char)br;
						*line++ = (unsigned char)bg;
						*line++ = (unsigned char)bb;
					}
					break;
			}
		}
		else	/* An uncompressed run of mixed pixels. */
		{
			switch (tga.imbits)
			{
				case 1:
				case 8:
					while (cnt--)
						*line++ = (unsigned char)getc(fp);
					break;

				case 15:
				case 16:
					while (cnt--)
					{
						bb = getc(fp);
						br = getc(fp);
						c = (unsigned short)((bb) | (br << 8));
						*line++ = (unsigned char)(((c >> 10) & 0x1F) << 3);
						*line++ = (unsigned char)(((c >> 5) & 0x1F) << 3);
						*line++ = (unsigned char)((c & 0x1F) << 3);
					}
					break;

				case 24:
					while (cnt--)
					{
						bb = getc(fp);
						bg = getc(fp);
						br = getc(fp);
						*line++ = (unsigned char)br;
						*line++ = (unsigned char)bg;
						*line++ = (unsigned char)bb;
					}
					break;

				case 32:
					while (cnt--)
					{
						bb = getc(fp);
						bg = getc(fp);
						br = getc(fp);
						(void)getc(fp);
						*line++ = (unsigned char)br;
						*line++ = (unsigned char)bg;
						*line++ = (unsigned char)bb;
					}
					break;
			}
		}
	}
}

/*************************************************************************
*
*   targa.c - Targa image file functions.
*
*************************************************************************/

/* Header of the output Targa image being worked with. */
static struct TGA out_tga;

static FILE *out_fp = NULL;
static unsigned char * str_buf;
#define RLE_BUF_SIZE sizeof(unsigned char) * 384

int Targa_Create(char *name, int xres, int yres)
{
	struct TGA *tga = &out_tga;

	Y_cur = 0;

	strcpy(outfile_name, name);
	out_fp = fopen(name, "wb");
	if (out_fp == NULL)
		return 0;

	if (outfile_rle)
	{
		/* Allocate a buffer for RLE functions... */
		if((str_buf = (unsigned char *)malloc(RLE_BUF_SIZE)) == NULL)
		{
			Targa_Close();
			return 0;
		}
	}
	else
		str_buf = NULL;

	/* Fill out the header information... */
	memset(tga, 0, sizeof (struct TGA));

	tga->imtype = (unsigned char)((outfile_rle) ? TGA_RLE_RGB : TGA_RGB);
	tga->imbits = (unsigned char)((outfile_bits < 24) ? 16 : 24);

	/* If odd, pad to next higher even value. */
	if (xres & 1)
		xres++;
	if (yres & 1)
		yres++;

	tga->imxres = (unsigned short)xres;
	tga->imyres = (unsigned short)yres;
	tga->imdesc = 0x20;  /* Scan lines run from top to bottom. */

	/* Write the header... */
	putc(tga->idlen, out_fp);
	putc(tga->cmtype, out_fp);
	putc(tga->imtype , out_fp);
	putc((unsigned char)(tga->cmstart & 0xFF), out_fp);
	putc((unsigned char)(tga->cmstart >> 8), out_fp);
	putc((unsigned char)(tga->cmcnt & 0xFF), out_fp);
	putc((unsigned char)(tga->cmcnt >> 8), out_fp);
	putc(tga->cmbits, out_fp);
	putc((unsigned char)(tga->imxstart & 0xFF), out_fp);
	putc((unsigned char)(tga->imxstart >> 8), out_fp);
	putc((unsigned char)(tga->imystart & 0xFF), out_fp);
	putc((unsigned char)(tga->imystart >> 8), out_fp);
	putc((unsigned char)(tga->imxres & 0xFF), out_fp);
	putc((unsigned char)(tga->imxres >> 8), out_fp);
	putc((unsigned char)(tga->imyres & 0xFF), out_fp);
	putc((unsigned char)(tga->imyres >> 8), out_fp);
	putc(tga->imbits, out_fp);
	putc(tga->imdesc, out_fp);

	if (ferror(out_fp))
	{
		Targa_Close();
		return 0;
	}

	/* Set up buffering, if any... */
	if (!Targa_SetBuffer(out_fp))
	{
		Targa_Close();
		return 0;
	}

	return 1;
}

int Targa_WriteLine(unsigned char *line)
{
	struct TGA *tga;

	if (out_fp == NULL)
		return 0;

	tga = &out_tga;

	if (tga->imtype == TGA_MONO ||      /* Uncompressed file. */
		tga->imtype == TGA_MAPPED ||
		tga->imtype == TGA_RGB)
	{
		int i, nbytes, br, bg, bb, n;
		unsigned char r, g, b;
		unsigned short c;

		switch(tga->imbits)
		{
			case 16:
				nbytes = tga->imxres;
				for (i = 0; i < nbytes; i++)
				{
					r = *line++;
					g = *line++;
					b = *line++;
					br = r >> 3;
					bg = g >> 3;
					bb = b >> 3;
					if (outfile_dither)
					{
						n = (int)bayer[i&7][Y_cur&7];
						if (br < 31 && n < ((r % 8) << 3))
							br++;
						if (bg < 31 && n < ((g % 8) << 3))
							bg++;
						if (bb < 31 && n < ((b % 8) << 3))
							bb++;
					}
					c = (unsigned short)((br << 10) | (bg << 5) | bb);
					putc((unsigned char)(c & 0xFF), out_fp);
					putc((unsigned char)(c >> 8), out_fp);
				}
				break;

			case 24:
				nbytes = tga->imxres;
				for (i = 0; i < nbytes; i++)
				{
					r = *line++;
					g = *line++;
					b = *line++;
					putc(b, out_fp);
					putc(g, out_fp);
					putc(r, out_fp);
				}
				break;

			default:
				/* fprintf (stderr,
				"Targa: Unknown bit size switched in Targa_WriteLine().");*/
				Targa_Close();
				break;
		}
	}
	else     /* Compressed file. */
	{
		int cnt, key;
		unsigned char r, g, b;
		unsigned short c0, c1;

		cnt = 0;
		switch (tga->imbits)
		{
			case 16:
				while (cnt < tga->imxres)
				{
					c0 = (unsigned short)(
						(((line[2] >> 3) & 0x1F)|
						(((line[1] >> 3) & 0x1F) << 5)|
						(((line[0] >> 3) & 0x1F) << 10)));
					c1 = (unsigned short)(
						(((line[5] >> 3) & 0x1F)|
						(((line[4] >> 3) & 0x1F) << 5)|
						(((line[3] >> 3) & 0x1F) << 10)));
				if (c0 == c1)
				{
					line += 3;
					c1 = (unsigned short)(
						(((line[5] >> 3) & 0x1F)|
						(((line[4] >> 3) & 0x1F) << 5)|
						(((line[3] >> 3) & 0x1F) << 10)));
					key = 128;
					cnt++;
					while ((c0 == c1) &&
						(cnt < tga->imxres) &&
						(key < 255))
					{
						r = *line++;
						g = *line++;
						b = *line++;
						c1 = (unsigned short)(
						(((b >> 3) & 0x1F)|
						(((g >> 3) & 0x1F) << 5)|
						(((r >> 3) & 0x1F) << 10)));
						key++;
						cnt++;
					}
					putc((unsigned char)key, out_fp);
					b = (unsigned char)c0;
					c0 >>= 8;
					r = (unsigned char)c0;
					putc(b, out_fp);
					putc(r, out_fp);
				}
				else
				{
					unsigned char *buf;

					key = -1;
					buf = str_buf;
					while ((c0 != c1) &&
						(cnt < tga->imxres) &&
						(key < 127))
					{
						*buf++ = (unsigned char)c0;
						c0 >>= 8;
						*buf++ = (unsigned char)c0;
						c0 = c1;
						line += 3;
						c1 = (unsigned short)(
							(((line[5] >> 3) & 0x1F)|
							(((line[4] >> 3) & 0x1F) << 5)|
							(((line[3] >> 3) & 0x1F) << 10)));
						key++;
						cnt++;
					}
					putc((unsigned char)key, out_fp);
					buf = str_buf;
					while (key-- >= 0)
					{
						b = *buf++;
						r = *buf++;
						putc(b, out_fp);
						putc(r, out_fp);
					}
				}
			}
			break;

		case 24:
			while (cnt < tga->imxres)
			{
				if ((line[0] == line[3]) &&
					(line[1] == line[4]) &&
					(line[2] == line[5]))
				{
					r = *line++;
					g = *line++;
					b = *line++;
					key = 128;
					cnt++;
					while ((line[0] == line[3]) &&
						(line[1] == line[4]) &&
						(line[2] == line[5]) &&
						(cnt < tga->imxres) &&
						(key < 255))
					{
						line += 3;
						key++;
						cnt++;
					}
					putc((unsigned char)key, out_fp);
					putc(b, out_fp);
					putc(g, out_fp);
					putc(r, out_fp);
				}
				else
				{
					unsigned char *buf;

					key = -1;
					buf = str_buf;
					while (((line[0] != line[3]) ||
						(line[1] != line[4]) ||
						(line[2] != line[5])) &&
						(cnt < tga->imxres) &&
						(key < 127))
					{
						r = *line++;
						g = *line++;
						b = *line++;
						*buf++ = b;
						*buf++ = g;
						*buf++ = r;
						key++;
						cnt++;
					}
					putc((unsigned char)key, out_fp);
					buf = str_buf;
					while (key-- >= 0)
					{
						b = *buf++;
						g = *buf++;
						r = *buf++;
						putc(b, out_fp);
						putc(g, out_fp);
						putc(r, out_fp);
					}
				}
			}
			break;
		}
	}

	if (ferror(out_fp))
	{
		Targa_Close();
		return 0;
	}

	if (file_buf_size == 0)  /* Flush every scan line. */
	{
		fflush(out_fp);
		out_fp = freopen(outfile_name, "ab", out_fp);
	}

	Y_cur++;

	return 1;
}

void Targa_Close(void)
{
	if (out_fp != NULL)
	{
		fclose(out_fp);
		out_fp = NULL;
	}

	if (str_buf != NULL)
	{
		free(str_buf);
		str_buf = NULL;
	}

	if (file_buf != NULL)
	{
		free(file_buf);
		file_buf = NULL;
	}
}
