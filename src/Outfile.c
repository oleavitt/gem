/*************************************************************************
*
*  outfile.c - Application-level output file management functions.
*
*************************************************************************/

#include "gempch.h"
#include "gem.h"

/* Buffers to and receive path name parts from _splitpath(). */
static TCHAR s_szDrive[_MAX_DRIVE];
static TCHAR s_szDir[_MAX_DIR];
static TCHAR s_szFilename[_MAX_FNAME];
static TCHAR s_szExt[_MAX_EXT];

/*
 * Length of the "_nnnnn" that gets appended to animation output
 * file names.
 */
#define INDEX_LEN   6

/*
 * Targa image header.
 */
struct TGA
{
  unsigned char    idlen;          /* ID field length. */
  unsigned char    cmtype;         /* Color map type. */
  unsigned char    imtype;         /* Image type. */
  unsigned short   cmstart;        /* Index of first color map entry. */
  unsigned short   cmcnt;          /* Number of color map entries. */
  unsigned char    cmbits;         /* Bits per color map entry. */
  unsigned short   imxstart;       /* Image x origin. */
  unsigned short   imystart;       /* Image y origin. */
  unsigned short   imxres;         /* Image width in pixels. */
  unsigned short   imyres;         /* IMAGE height in pixels. */
  unsigned char    imbits;         /* Bits per pixel. */
  unsigned char    imdesc;         /* IMAGE descriptor. */
};

#define TGA_MAPPED           0x01
#define TGA_RGB              0x02
#define TGA_MONO             0x03
#define TGA_RLE_MAPPED       0x09
#define TGA_RLE_RGB          0x0A
#define TGA_RLE_MONO         0x0B


OutfileData *Outfile_Create(LPCTSTR lpszFilename, int xres, int yres, int bits,
	int rle)
{
	OutfileData *ofd;  /* Newly created output file ptr. */
	struct TGA tga;    /* Targa file header. */
	size_t bufsize;    /* Scanline buffer size in bytes. */

	if ((ofd = (OutfileData *)malloc(sizeof(OutfileData))) == NULL)
		return NULL;
	memset(ofd, 0, sizeof(OutfileData));

	/* If odd, pad to next higher even value. */
	if (xres & 1) xres++;
	if (yres & 1) yres++;
	ofd->xres = xres;
	ofd->yres = yres;

	/* Only 16 or 24 bit Targa output files supported. */
	ofd->bits = (bits < 24) ? 16 : 24;
	
	ofd->rle = rle;
	  
	/* If no file name was given, make name from the global scene file name. */
	if (lpszFilename == NULL || *lpszFilename == '\0')
		Outfile_MakeName(ofd->fname, g_szScnFileName);
	else
		_tcsncpy(ofd->fname, lpszFilename, MAXFNAME - 2);

	/* Allocate the scan line buffer. */
	bufsize = (ofd->bits >> 3) * xres * sizeof(unsigned char);
	if ((ofd->scanline = (unsigned char *)malloc(bufsize)) == NULL)
	{
		Outfile_Close(ofd);
		return NULL;
	}

	/* Open (create) the FILE stream for binary write. */
	if ((ofd->fp = _tfopen(ofd->fname, _T("wb"))) == NULL)
	{
		Outfile_Close(ofd);
		return NULL;
	}

	/* Fill in the Targa header. */
	memset(&tga, 0, sizeof(struct TGA));
	tga.imtype = (unsigned char)((ofd->rle) ? TGA_RLE_RGB : TGA_RGB);
	tga.imbits = (unsigned char)ofd->bits;
	tga.imxres = (unsigned short)ofd->xres;
	tga.imyres = (unsigned short)ofd->yres;
	tga.imdesc = 0x20;  /* Scan lines run from top to bottom. */

	/* Write Targa header to file. */
	putc(tga.idlen, ofd->fp);
	putc(tga.cmtype, ofd->fp);
	putc(tga.imtype , ofd->fp);
	putc((unsigned char)(tga.cmstart & 0xFF), ofd->fp);
	putc((unsigned char)(tga.cmstart >> 8), ofd->fp);
	putc((unsigned char)(tga.cmcnt & 0xFF), ofd->fp);
	putc((unsigned char)(tga.cmcnt >> 8), ofd->fp);
	putc(tga.cmbits, ofd->fp);
	putc((unsigned char)(tga.imxstart & 0xFF), ofd->fp);
	putc((unsigned char)(tga.imxstart >> 8), ofd->fp);
	putc((unsigned char)(tga.imystart & 0xFF), ofd->fp);
	putc((unsigned char)(tga.imystart >> 8), ofd->fp);
	putc((unsigned char)(tga.imxres & 0xFF), ofd->fp);
	putc((unsigned char)(tga.imxres >> 8), ofd->fp);
	putc((unsigned char)(tga.imyres & 0xFF), ofd->fp);
	putc((unsigned char)(tga.imyres >> 8), ofd->fp);
	putc(tga.imbits, ofd->fp);
	putc(tga.imdesc, ofd->fp);

	if(ferror(ofd->fp))
	{
		Outfile_Close(ofd);
		return NULL;
	}

	return ofd;
}


void Outfile_Close(OutfileData *ofd)
{
	if (ofd != NULL)
	{
		if (ofd->fp != NULL)
			fclose(ofd->fp);
		if (ofd->scanline != NULL)
		  free(ofd->scanline);
		free(ofd);
	}
}


void Outfile_PutPixel(OutfileData *ofd, unsigned char r,
  unsigned char g, unsigned char b)
{
	switch (ofd->bits)
	{
		case 24:
			putc(b, ofd->fp);
			putc(g, ofd->fp);
			putc(r, ofd->fp);
			break;
		default:  /* 16 */
			{
				unsigned short c;
				c = (unsigned char)
					((unsigned short)((r >> 3) << 10) |
					(unsigned short)((g >> 3) << 5) |
					(unsigned short) (b >> 3));
				putc((unsigned char)(c & 0xFF), ofd->fp);
				putc((unsigned char)(c >> 8), ofd->fp);
			}
			break;
	}
}

/*
 * Take a given path/filename and copy it to the output path/filename
 * replacing the prefix, if any, with the Targa ".tga" prefix.
 * If file name is NULL or zero, output file name is set to zero.
 * NOTE: inpath and outpath must be at least _MAX_PATH in size.
 */
void Outfile_MakeName(LPTSTR lpszOutpath, LPCTSTR lpszInpath)
{
	if (lpszInpath != NULL && *lpszInpath != '\0')
	{
		/* Break input path into its drive, dir, file name, and extension. */
		_tsplitpath(lpszInpath, s_szDrive, s_szDir, s_szFilename, s_szExt);
		/* If making an animation sequence, add frame index to file name. */
		if (bAnimation)
		{
			TCHAR oldfname[_MAX_FNAME];
			lstrcpy(oldfname, s_szFilename);
			/* 
			 * Ensure that there is room the frame index digits at the
			 * end of the file name.
			 * Truncate file name if its size comes to within 6 characters
			 * (plus terminating \0) of _MAX_FNAME.
			 * For platforms that support long file names, this is
			 * unlikely to be a problem.
			 */
			oldfname[_MAX_FNAME - 7] = '\0';
			/* TODO: append cur frame number to filename in animation sequence. */
			/* wsprintf(s_szFilename, "%s_%05d", oldfname, scn_cur_frame); */
		}
		/* Replace input file extension, if any, with the Targa extension. */
		lstrcpy(s_szExt, _T("tga"));
		/* Assemble the new output path. */
		_tmakepath(lpszOutpath, s_szDrive, s_szDir, s_szFilename, s_szExt);
	}
	else
		lpszOutpath[0] = '\0';
}
