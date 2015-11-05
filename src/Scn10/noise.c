/*************************************************************************
*
*  noise.c - Perlin noise generation functions.
*
*************************************************************************/

#include "local.h"

/* Scaling factors for the noise hash function. */
Vec3 Noise_Scale = { 314.159, 618.033, 141.421 };

/* Random # lookup table array. */
#define RAND_TABLE_SIZE 16384
#define RAND_LOOKUP_MAX 65536
static unsigned short Noise_Table[RAND_TABLE_SIZE];


#define Hash3D(_x, _y, _z) \
  ((unsigned short)((unsigned int)((_x) * Noise_Scale.x + \
                  (_y) * Noise_Scale.y + \
                  (_z) * Noise_Scale.z) & (RAND_TABLE_SIZE-1)))

/*************************************************************************
 *
 * Noise & Turbulence generation functions.
 *
 ************************************************************************/

/*************************************************************************
 *
 *  Noise_Initialize() - Initialize noise lookup table with a given seed.
 *
 ************************************************************************/
void Noise_Initialize(long seed)
{
	int i;

	for(i = 0; i < RAND_TABLE_SIZE; i++)
		Noise_Table[i] = (unsigned short)(Frand1(&seed) * RAND_LOOKUP_MAX);
}

/*************************************************************************
 *
 *  Noise3D() - Generate a smooth noise value in the range of
 *  -1.0 -> 1.0 based on the location of a three dimensional point, "pt".
 *
 ************************************************************************/
double Noise3D(Vec3 *pt)
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
		n[i] = Noise_Table[Hash3D(cx, cy, cz)];
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

/*************************************************************************
 *
 *  VNoise3D() - Generate a smooth vector noise value in the range of
 *  -1.0 -> 1.0 based on the location of a three dimensional point, "pt".
 *  Vector noise value is returned in "noise_vec". Point, "pt", is
 *  perturbed slightly for each point in the return vector, "noise_vec".
 *
 ************************************************************************/
void VNoise3D(Vec3 *pt, Vec3 *noise_vec)
{
	Vec3 p;

	p.x = pt->x + Noise_Scale.x;
	p.y = pt->y + Noise_Scale.y;
	p.z = pt->z + Noise_Scale.z;
	noise_vec->x = Noise3D(&p);
	p.x = pt->x + Noise_Scale.z;
	p.y = pt->y + Noise_Scale.x;
	p.z = pt->z + Noise_Scale.y;
	noise_vec->y = Noise3D(&p);
	p.x = pt->x + Noise_Scale.y;
	p.y = pt->y + Noise_Scale.z;
	p.z = pt->z + Noise_Scale.x;
	noise_vec->z = Noise3D(&p);
}

/*************************************************************************
 *
 *  Turb3D() - Generate a summation of recursively sub-divided noise values
 *  based on 3D point, "pt", with "octaves" number of sub-divisions.
 *
 ************************************************************************/
double Turb3D(Vec3 *pt, int octaves, double freq_factor,
	double amp_factor)
{
	int i;
	double total_noise, freq_scale, amp_scale, total_amp_scale;
	Vec3 p;

	total_noise = Noise3D(pt);
	total_amp_scale = 1.0;
	freq_scale = freq_factor;
	amp_scale = amp_factor;
	for(i = 1; i < octaves; i++)
	{
		V3ScalMul(&p, pt, freq_scale);
		total_noise += Noise3D(&p) * amp_scale;
		total_amp_scale += fabs(amp_scale);
		freq_scale *= freq_factor;
		amp_scale *= amp_factor;
	}

	return total_noise / total_amp_scale;
}

/*************************************************************************
 *
 *  VTurb3D() - Generate a summation of recursively sub-divided noise values
 *  based on 3D point, "pt", with "octaves" number of sub-divisions. This is
 *  done three times, once for each element of "turb_vec", with point, "pt",
 *  perturbed slightly for each point in return vector, "turb_vec".
 *
 ************************************************************************/
void VTurb3D(Vec3 *pt, int octaves, double freq_factor,
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
		VNoise3D(&p, &v);
		turb_vec->x += v.x * amp_scale;
		turb_vec->y += v.y * amp_scale;
		turb_vec->z += v.z * amp_scale;
		freq_scale *= freq_factor;
		amp_scale *= amp_factor;
	}
}

/*************************************************************************
 *
 *  Wrinkles3D()
 *
 *  Apply the classic "wrinkled" bump function to vector "N".
 *
 *************************************************************************/
void Wrinkles3D(Vec3 *N, Vec3 *P, int oct)
{
	int  i;
	double f;
	Vec3 pt, offset;

	pt = *P;
	f = 1.0;
	V3Zero(N);
	for(i = 0; i < oct; i++)
	{
		pt.x *= f;
		pt.y *= f;
		pt.z *= f;
		VNoise3D(&pt, &offset);
		N->x += offset.x;
		N->y += offset.y;
		N->z += offset.z;
		f *= 2.0;
	}
}

/*************************************************************************
 *
 *  Hexagon2D() - Hah! A hex on you! A two dimensional hexagon
 *  pattern. Returns -1, 0, or 1 depending on which hex in a triad
 *  of three was hit.
 *
 ************************************************************************/
#define SIN120 0.8660254038
#define COS120 -0.5
static unsigned char hex_masks[] = { 3, 6, 5 }; /* binary 011 , 110 , 101 */

int Hexagon2D(double u, double v)
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
 *  Legendre() - Compute the associated Legendre polynomial:
 *                             P m->l (x)
 *  Where integers "m" and "l" satisfy 0 <= m <= l, and "x" is in
 *  the range of -1.0 <= x <= 1.0.
 *
 ************************************************************************/
double Legendre(int l, int m, double x)
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
