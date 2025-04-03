/*************************************************************************
*
*  rand.c - Random number generation functions garnered from
*    Numerical Recipes in C The Art of Scientific Computing (2nd ed) by
*    William H. Press, William T. Vettering, Saul A. Teukolsky, and
*    Brian R. Flannery.
*    Copyright (C) Cambridge University Press 1988, 1992
*
*************************************************************************/

#include "local.h"

/*************************************************************************
*
*  Frand1 - Random number gerator using the Park and Miller generator
*   with the addition of Bays-Durham shuffle to break up sequntial
*   correlations.
*
*************************************************************************/

#define IA 16807
#define IM 2147483647
#define AM (1.0 / IM)
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1 + (IM - 1) / NTAB)
#define EPS 1.2e-7
#define RNMX (1.0 - EPS)

double Frand1(long *seed)
{
  int j;
  long k;
  static int iy = 0;
  static int iv[NTAB];
  double temp;

  if(*seed <= 0 || iy == 0)        /* Initialize... */
  {
    if(-(*seed) < 1) *seed = 1;    /* seed cannot be zero. */
    else *seed = -(*seed);
    for(j = NTAB + 7; j >= 0; j--) /* Load the shuffle table */
    {                              /* after 8 warm-ups. */
      k = (*seed) / IQ;
      *seed = IA * (*seed - k * IQ) - IR * k;
      if(*seed < 0) *seed += IM;
      if(j < NTAB) iv[j] = (int)*seed;
    }
    iy = iv[0];
  }
  k = (*seed) / IQ;                /* Start here when not initializing. */
  *seed = IA * (*seed - k * IQ) - IR * k;  
  if(*seed < 0) *seed += IM;
  j = iy / NDIV;
  iy = iv[j];
  iv[j] = (int)*seed;
  if((temp = AM * iy) > RNMX) return RNMX;
  else return temp;
}
