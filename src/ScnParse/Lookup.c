/*************************************************************************
*
*  lookup.c - Functions that return a value based on a one, two or three
*  dimensional index.
*
*  CAVEAT: Call Lookup_Initialize() before using!
*
*************************************************************************/

#include "scn.h"

/*#define ALT_NOISE_FN*/

/*************************************************************************
 *  Global stuff...
 */

/* Seed value for rand # lookup table initialization. */
unsigned short Lookup_Noise_Seed = 7777;
/* Scaling factors for the noise hash function. */
Vec3 Lookup_Noise_Scale = { 314.159, 618.033, 141.421 };


/*************************************************************************
 *
 * Noise & Turbulence generation functions.
 *
 ************************************************************************/

double frand(register long s)
{
  s = (s << 13) ^ s;
	return (1.0 - ((s * (s * s * 15731 + 789221) + 1376312589) & 0x7FFFFFFF)
	  / 1073741824.0);
}

/*************************************************************************
 *
 * Lookup_Noise3D() - Generate a smooth noise value in the range of
 * -1.0 -> 1.0 based on the location of a three dimensional point, "pt".
 *
 ************************************************************************/

#ifdef ALT_NOISE_FN

#define hermite(p0,p1,r0,r1,t) (p0 * ((2.0 * t - 3.0) * t * t + 1.0) + \
                                p1 * (-2.0 * t + 3.0) * t * t + \
																r0 * ((t - 2.0) * t + 1.0) * t + \
																r1 * (t - 1.0) * t * t)

#define rand3a(x,y,z)   frand(67*(x)+59*(y)+71*(z))
#define rand3b(x,y,z)   frand(73*(x)+79*(y)+83*(z))
#define rand3c(x,y,z)   frand(89*(x)+97*(y)+101*(z))
#define rand3d(x,y,z)   frand(103*(x)+107*(y)+109*(z))

long xlim[3][2];
double xarg[3];

void interpolate(double f[4], register int i, register int n)
{
  double f0[4], f1[4];

	if(n == 0)
	{
	  f[0] = rand3a(xlim[0][i&1],xlim[1][(i>>1)&1],xlim[2][i>>2]);
	  f[1] = rand3b(xlim[0][i&1],xlim[1][(i>>1)&1],xlim[2][i>>2]);
	  f[2] = rand3c(xlim[0][i&1],xlim[1][(i>>1)&1],xlim[2][i>>2]);
	  f[3] = rand3d(xlim[0][i&1],xlim[1][(i>>1)&1],xlim[2][i>>2]);
		return;
	}
	n--;
	interpolate(f0, i, n);
	interpolate(f1, (i | 1) << n, n);
	f[0] = (1.0 - xarg[n]) * f0[0] + xarg[n] * f1[0];
	f[1] = (1.0 - xarg[n]) * f0[1] + xarg[n] * f1[1];
	f[2] = (1.0 - xarg[n]) * f0[2] + xarg[n] * f1[2];
	f[3] = hermite(f0[3], f1[3], f0[n], f1[n], xarg[n]);
}

double *noise3(Vec3 *v)
{
  static double f[4];

	xlim[0][0] = (long)floor(v->x); xlim[0][1] = xlim[0][0] + 1;
	xlim[1][0] = (long)floor(v->y); xlim[1][1] = xlim[1][0] + 1;
	xlim[2][0] = (long)floor(v->z); xlim[2][1] = xlim[2][0] + 1;
	xarg[0] = v->x - xlim[0][0];
	xarg[1] = v->y - xlim[1][0];
	xarg[2] = v->z - xlim[2][0];
	interpolate(f, 0, 3);

	return f;
}

double Lookup_Noise3D(Vec3 *pt)
{
  double *noise;
	noise = noise3(pt);
	return noise[0];
}

#else

/* Random # lookup table array. */
#define RAND_TABLE_SIZE 16384
#define RAND_LOOKUP_MAX 65536
static unsigned short Rand_Lookup_Table[RAND_TABLE_SIZE];


#define Hash3D(_x, _y, _z) \
  ((unsigned short)((unsigned int)((_x) * Lookup_Noise_Scale.x + \
                  (_y) * Lookup_Noise_Scale.y + \
                  (_z) * Lookup_Noise_Scale.z) & (RAND_TABLE_SIZE-1)))

double Lookup_Noise3D(Vec3 *pt)
{
  unsigned short n[8]; /* Noise values for eight corners of noise cube. */
  int cx, cy, cz;      /* Integral corner of cube containing point. */
  double noise;        /* Final noise value. */
  double wx, wy, wz;   /* Interpolation amounts for each axis. */
  int i;

  /*
   * Generate noise values for each of the eight corners of a cube
   * defined by the closest integral point values surrounding the point,
   * "pt", and place in the n[] array.
   */
  for(i = 0; i < 8; i++)
  {
    cx = (int)((i & 1) ? ceil(pt->x) : floor(pt->x));
    cy = (int)((i & 2) ? ceil(pt->y) : floor(pt->y));
    cz = (int)((i & 4) ? ceil(pt->z) : floor(pt->z));
    n[i] = Rand_Lookup_Table[Hash3D(cx, cy, cz)];
  }

  /*
   * Get weights for lattice interpolation along each axis.
   */
  wx = pt->x - floor(pt->x);
  /* Add or subtract a quadratic term to make it more like a spline. */
  if(wx > 0.5)
  {
    wx = 1.0 - wx;
    wx = 2.0 * wx * wx;
    wx = 1.0 - wx;
  }
  else
  {
    wx = 2.0 * wx * wx;
  }

  wy = pt->y - floor(pt->y);
  if(wy > 0.5)
  {
    wy = 1.0 - wy;
    wy = 2.0 * wy * wy;
    wy = 1.0 - wy;
  }
  else
  {
    wy = 2.0 * wy * wy;
  }

  wz = pt->z - floor(pt->z);
  if(wz > 0.5)
  {
    wz = 1.0 - wz;
    wz = 2.0 * wz * wz;
    wz = 1.0 - wz;
  }
  else
  {
    wz = 2.0 * wz * wz;
  }

  /*
   * Linear interpolate all eight noise values to get
   * the composite noise value at point "pt".
   */
  noise = 0;
  for(i = 0; i < 8; i++)
    noise += (((double)n[i] / RAND_LOOKUP_MAX) * 2.0 - 1.0) *
    ((i & 1) ? wx : 1.0 - wx) *
    ((i & 2) ? wy : 1.0 - wy) *
    ((i & 4) ? wz : 1.0 - wz);

  return noise;
}

#endif

/*************************************************************************
 *
 * Lookup_VNoise3D() - Generate a smooth vector noise value in the range of
 * -1.0 -> 1.0 based on the location of a three dimensional point, "pt".
 * Vector noise value is returned in "noise_vec". Point, "pt", is
 * perturbed slightly for each point in the return vector, "noise_vec".
 *
 ************************************************************************/
#ifdef ALT_NOISE_FN
void Lookup_VNoise3D(Vec3 *pt, Vec3 *noise_vec)
{
  double *noise;
	noise = noise3(pt);
	noise_vec->x = noise[0];
	noise_vec->y = noise[1];
	noise_vec->z = noise[2];
}
#else
void Lookup_VNoise3D(Vec3 *pt, Vec3 *noise_vec)
{
  Vec3 p;

  p.x = pt->x + Lookup_Noise_Scale.x;
  p.y = pt->y + Lookup_Noise_Scale.y;
  p.z = pt->z + Lookup_Noise_Scale.z;
  noise_vec->x = Lookup_Noise3D(&p);
  p.x = pt->x + Lookup_Noise_Scale.z;
  p.y = pt->y + Lookup_Noise_Scale.x;
  p.z = pt->z + Lookup_Noise_Scale.y;
  noise_vec->y = Lookup_Noise3D(&p);
  p.x = pt->x + Lookup_Noise_Scale.y;
  p.y = pt->y + Lookup_Noise_Scale.z;
  p.z = pt->z + Lookup_Noise_Scale.x;
  noise_vec->z = Lookup_Noise3D(&p);
}
#endif

/*************************************************************************
 *
 * Lookup_Turb3D() - Generate a summation of recursively sub-divided noise values
 * based on 3D point, "pt", with "octaves" number of sub-divisions.
 *
 ************************************************************************/
double Lookup_Turb3D(Vec3 *pt, int octaves, double freq_factor, double amp_factor)
{
  int i;
  double total_noise, freq_scale, amp_scale, total_amp_scale;
  Vec3 p;

  total_noise = Lookup_Noise3D(pt);
	total_amp_scale = 1.0;
  freq_scale = freq_factor;
  amp_scale = amp_factor;
  for(i = 1; i < octaves; i++)
  {
    V3ScalMul(&p, pt, freq_scale);
    total_noise += Lookup_Noise3D(&p) * amp_scale;
		total_amp_scale += fabs(amp_scale);
    freq_scale *= freq_factor;
    amp_scale *= amp_factor;
  }

  return total_noise / total_amp_scale;
}

/*************************************************************************
 *
 * Lookup_VTurb3D() - Generate a summation of recursively sub-divided noise values
 * based on 3D point, "pt", with "octaves" number of sub-divisions. This is
 * done three times, once for each element of "turb_vec", with point, "pt",
 * perturbed slightly for each point in return vector, "turb_vec".
 *
 ************************************************************************/
void Lookup_VTurb3D(Vec3 *pt, int octaves, double freq_factor,
  double amp_factor, Vec3 *turb_vec)
{
  int i;
  double freq_scale, amp_scale;
  Vec3 p, v;

  freq_scale = 1.0;
  amp_scale = 1.0;
  V3Set(turb_vec, 0.0, 0.0, 0.0);
  for(i = 0; i < octaves; i++)
  {
    V3ScalMul(&p, pt, freq_scale);
    Lookup_VNoise3D(&p, &v);
    turb_vec->x += v.x * amp_scale;
    turb_vec->y += v.y * amp_scale;
    turb_vec->z += v.z * amp_scale;
    freq_scale *= freq_factor;
    amp_scale *= amp_factor;
  }
}


/*************************************************************************
 *
 *  Lookup_Wrinkles3D()
 *
 *  Apply the classic "wrinkled" bump function to vector "N".
 *
 *************************************************************************/
void Lookup_Wrinkles3D(Vec3 *N, Vec3 *P, int oct)
{
  int  i;
  double f;
  Vec3 pt, offset;

  pt = *P;
  f = 1.0;
  for(i = 0; i < oct; i++)
  {
    pt.x *= f;
    pt.y *= f;
    pt.z *= f;
    Lookup_VNoise3D(&pt, &offset);
    N->x += offset.x;
    N->y += offset.y;
    N->z += offset.z;
    f *= 2.0;
  }
}


/*************************************************************************
 *
 * Lookup_Hexagon2D() - Hah! A hex on you! A two dimensional hexagon
 *  pattern. Returns -1, 0, or 1 depending on which hex in a triad
 *  of three was hit.
 *
 ************************************************************************/
#define SIN120 0.8660254038
#define COS120 -0.5
static unsigned char hex_masks[] = { 3, 6, 5 }; /* binary 011 , 110 , 101 */

int Lookup_Hexagon2D(double u, double v)
{
  int ndx, k;        /* accumulates hex lookup results */
  double new_u;      /* temp for rotation calculation */
  int i;             /* loop counter */

  /*
   * Lookup initial index...
   */
  k = ((int)floor(v * 2.0)) % 3;
  if (k < 0) k += 3;
  ndx = hex_masks[k];

  /*
   * Rotate in 120 degree increments and AND together successive lookups.
   * Bail when we find a single bit value, we've found our hex.
   */
  for (i = 0; i < 2; i++)
  {
    new_u = COS120 * u + SIN120 * v;
    v =    -SIN120 * u + COS120 * v;
    u = new_u;
    k = ((int)floor(v * 2.0)) % 3;
    if(k < 0) k += 3;
    ndx &= hex_masks[k];
    switch(ndx)
    {
      case 1: return -1;
      case 2: return 0;
      case 4: return 1;
    }
  }
  return -2; /* ??? should never happen */
}


/*************************************************************************
 *
 *  Lookup_Legendre() - Compute the associated Legendre polynomial:
 *                             P m->l (x)
 *  Where integers "m" and "l" satisfy 0 <= m <= l, and "x" is in
 *  the range of -1.0 <= x <= 1.0.
 *
 ************************************************************************/
double Lookup_Legendre(int l, int m, double x)
{
  double fact, pll, pmm, pmmp1, somx2;
  int i, ll;

  if(m < 0 || m > l || fabs(x) > 1.0)
    return 0.0;    /* Bad range, just return 0.0. */

  pmm = 1.0;
  if(m > 0)
  {
    somx2 = sqrt((1.0 - x) *(1.0 + x));
    fact = 1.0;
    for(i = 1; i <= m; i++)
    {
      pmm *= - fact * somx2;
      fact += 2.0;
    }
  }
  if(l == m)
  {
    return pmm;
  }
  else
  {
    pmmp1 = x * (double)(2 * m + 1) * pmm;
    if(l ==(m + 1))
    {
      return pmmp1;
    }
    else
    {
      for(ll = m + 2; ll <= l; ll++)
      {
        pll =(x * (double)(2 * ll - 1) * pmmp1 -
          (double)(ll + m - 1) * pmm) / (double)(ll - m);
        pmm = pmmp1;
        pmmp1 = pll;
      }
      return pll;
    }
  }
}


/*************************************************************************
 *
 * Interpolated lookup functions.
 *
 ************************************************************************/

/* To be implemented... */

/*************************************************************************
 *
 * Lookup_Initialize() - Initialize noise lookup table.
 *
 ************************************************************************/
void Lookup_Initialize(void)
{
#ifndef ALT_NOISE_FN
  int i;
	long s = 7777777;

  srand(Lookup_Noise_Seed);
  for(i = 0; i < RAND_TABLE_SIZE; i++)
	{
	  s = (long)((frand(s) + 1.0) * (0x3FFFFFFF));
    Rand_Lookup_Table[i] = (unsigned short)(s % RAND_LOOKUP_MAX);
	}
#endif
}

