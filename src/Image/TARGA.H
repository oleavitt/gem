/*************************************************************************
*
*  targa.h - Functions for reading and writing Targa (.TGA) format
*  images.
*
*************************************************************************/

#ifndef TARGA_H
#define TARGA_H


extern int    Targa_SetBuffer(FILE *fp);
extern int    Targa_Create(char *name, int xres, int yres);
extern int    Targa_WriteLine(unsigned char *line);
extern void   Targa_Close(void);
extern Image *Targa_LoadImage(FILE *fp);


#endif     /* TARGA_H defined */

