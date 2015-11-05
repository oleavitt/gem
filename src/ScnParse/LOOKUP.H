/*************************************************************************
 *
 *  lookup.h - Externals for the lookup functions in lookup.c.
 *
 *  Note: Lookup_Initialize() should be called first before using
 *    lookup functions.
 *
 ************************************************************************/

#ifndef LOOKUP_H
#define LOOKUP_H

/* Seed value for rand # lookup table initialization. */
extern unsigned short Lookup_Noise_Seed;
/* Scaling factors for the noise hash function. */
extern Vec3 Lookup_Noise_Scale;

/* Call me first! */
extern void Lookup_Initialize(void);

extern double Lookup_Noise3D(Vec3 *pt);
extern void Lookup_VNoise3D(Vec3 *pt, Vec3 *noise_vec);
extern double Lookup_Turb3D(Vec3 *pt, int octaves, double freq_factor,
	double amp_factor);
extern void Lookup_VTurb3D(Vec3 *pt, int octaves, double freq_factor,
	double amp_factor, Vec3 *turb_vec);
extern void Lookup_Wrinkles3D(Vec3 *N, Vec3 *P, int oct);
extern int Lookup_Hexagon2D(double u, double v);
extern double Lookup_Legendre(int l, int m, double x);


#endif  /*  LOOKUP_H  */

