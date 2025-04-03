/*************************************************************************
*
*  solve.c - Polynomial root finding functions. Based on code garnered
*    from Graphics Gems (vol I), 1990 Academic Press, Inc. Original
*    author's comments appear below.
*
*************************************************************************/
/*************************************************************************
*
*  Utility functions to find cubic and quartic roots,
*  coefficients are passed like this:
*
*      c[0] + c[1]*x + c[2]*x^2 + c[3]*x^3 + c[4]*x^4 = 0
*
*  The functions return the number of non-complex roots and
*  put the values into the s array.
*
*  Author:         Jochen Schwarze (schwarze@isa.de)
*
*  Jan 26, 1990    Version for Graphics Gems
*  Oct 11, 1990    Fixed sign problem for negative q's in SolveQuartic
*  	    	    (reported by Mark Podlipec),
*  	    	    Old-style function definitions,
*  	    	    is_zero() as a macro
*  Nov 23, 1990    Some systems do not declare acos() and cbrt() in
*                  <math.h>, though the functions exist in the library.
*                  If large coefficients are used, EQU_EPS should be
*                  reduced considerably (e.g. to 1E-30), results will be
*                  correct but multiple roots might be reported more
*                  than once.
*
**************************************************************************/

#include "local.h"

/* Minimum value for coefficient to be significant. */
#define COEFF_EPS 1e-20

#define EQN_EPS 1e-10

#define is_zero(a) ((a) > -EQN_EPS && (a) < EQN_EPS)

#define cube_root(x) \
	((x) > 0.0 ? pow((double)(x), 1.0/3.0) : \
	((x) < 0.0 ? -pow((double)-(x), 1.0/3.0) : 0.0))

/*************************************************************************
*
*  int SolvePoly(double *c, double *s, int order, double lo, double hi)
*
*  Finds roots in a polynomial of a specified order for a specific range.
*
*  Parameters:
*    double *c: Array of coefficients for polynomial of the form:
*      c[0] + c[1]*x + c[2]*x^2 + ... c[order]*x^order = 0
*      (array must be of size (order+1)*sizeof(double))
*    double *s: Array of roots. Receives the roots for the polynomial
*      if successful. Must be of size order*sizeof(double).
*    int order: Specifies the order (degree) of polynomial.
*    double lo, hi: The range in which to search for roots.
*
*  Returns:
*    The number of roots found. The roots are sorted from smallest to
*  largest and returned in the array s.
*
*************************************************************************/
int SolvePoly(double *c, double *s, int order, double lo, double hi)
	{
	int i, nroots, valid_roots;
	double tmp;

	if(order <= 0)
		return 0;

	/* If a leading coefficient is insignificant, bump the order down until
	 * we find a meaningful coefficient.
	 */
	while((fabs(c[order]) < COEFF_EPS) && order)
		order--;

	/* Select appropriate solver. */
	switch(order)
		{
		case 0:
			return 0;
		case 1:
			return SolveLinear(c, s);
		case 2:
			nroots = SolveQuadric(c, s);
			break;
		case 3:
			nroots = SolveCubic(c, s);
			break;
		case 4:
			nroots = SolveQuartic(c, s);
			break;
		default:
			break;
		}

	if(nroots == 0)
		return 0;

	/*
	 * Validate only those roots that are within our specified bracket.
	 * Set out of range values to HUGE. This will ensure that all valid
	 * roots will be first in the array after sorting.
	 */
	valid_roots = nroots;
	for(i = 0;i < nroots; i++)
		{
		if(s[i] < lo || s[i] > hi)
			{
			s[i] = HUGE;
			valid_roots--;
			}
		}

	if(valid_roots == 0)
		return 0;

	/* Sort the roots from smallest to largest if more than one... */
	if(nroots > 2)
		{
		register int j;

		for(i = nroots-1; i >= 1; i--)
			for(j = 1; j <= i; j++)
				if(s[j-1] > s[j])
					{
					tmp = s[j-1];
					s[j-1] = s[j];
					s[j] = tmp;
					}
		}
	else if(nroots == 2)
		{
		if(s[0] > s[1])
			{
			tmp = s[0];
			s[0] = s[1];
			s[1] = tmp;
			}
		}

	return valid_roots;
	}


int SolveLinear(double *c, double *s)
	{
	*s = -c[0] / c[1];
	return 1;
	}


int SolveQuadric(double *c, double *s)
	{
	double p, q, D;

	/* normal form: x^2 + px + q = 0 */

	p = c[1] / (2.0 * c[2]);
	q = c[0] / c[2];

	D = p * p - q;

	if(is_zero(D))
		{
		s[0] = -p;
		return 1;
		}
	else if(D > 0)
		{
		double sqrt_D = sqrt(D);

		s[0] =   sqrt_D - p;
		s[1] = - sqrt_D - p;
		return 2;
		}

	return 0; /* D < 0 */

	}


int SolveCubic(double *c, double *s)
	{
	int     i, num;
	double  sub;
	double  A, B, C;
	double  sq_A, p, q;
	double  cb_p, D;

	/* normal form: x^3 + Ax^2 + Bx + C = 0 */

	A = c[2] / c[3];
	B = c[1] / c[3];
	C = c[0] / c[3];

	/*  substitute x = y - A/3 to eliminate quadric term:
	x^3 +px + q = 0 */

	sq_A = A * A;
	p = 1.0/3 * (- 1.0/3 * sq_A + B);
	q = 1.0/2 * (2.0/27 * A * sq_A - 1.0/3 * A * B + C);

	/* use Cardano's formula */

	cb_p = p * p * p;
	D = q * q + cb_p;

	if(is_zero(D))
		{
		if(is_zero(q)) /* one triple solution */
			{
			s[0] = 0;
			num = 1;
			}
		else /* one single and one double solution */
			{
			double u = cube_root(-q);
			s[0] = 2 * u;
			s[1] = - u;
			num = 2;
			}
		}
	else if(D < 0) /* Casus irreducibilis: three real solutions */
		{
		double phi = 1.0/3 * acos(-q / sqrt(-cb_p));
		double t = 2 * sqrt(-p);

		s[0] =   t * cos(phi);
		s[1] = - t * cos(phi + PI / 3);
		s[2] = - t * cos(phi - PI / 3);
		num = 3;
		}
	else /* one real solution */
		{
		double sqrt_D = sqrt(D);
		double u = cube_root(sqrt_D - q);
		double v = - cube_root(sqrt_D + q);

		s[0] = u + v;
		num = 1;
		}

	/* resubstitute */

	sub = 1.0/3 * A;

	for(i = 0; i < num; ++i)
		s[i] -= sub;

	return num;
	}


int SolveQuartic(double *c, double *s)
	{
	double  coeffs[4];
	double  z, u, v, sub;
	double  A, B, C, D;
	double  sq_A, p, q, r;
	int     i, num;

	/* normal form: x^4 + Ax^3 + Bx^2 + Cx + D = 0 */

	A = c[3] / c[4];
	B = c[2] / c[4];
	C = c[1] / c[4];
	D = c[0] / c[4];

	/*  substitute x = y - A/4 to eliminate cubic term:
			x^4 + px^2 + qx + r = 0 */

	sq_A = A * A;
	p = - 3.0/8 * sq_A + B;
	q = 1.0/8 * sq_A * A - 1.0/2 * A * B + C;
	r = - 3.0/256*sq_A*sq_A + 1.0/16*sq_A*B - 1.0/4*A*C + D;

	if(is_zero(r))
		{
		/* no absolute term: y(y^3 + py + q) = 0 */

		coeffs[0] = q;
		coeffs[1] = p;
		coeffs[2] = 0;
		coeffs[3] = 1;

		num = SolveCubic(coeffs, s);

		s[num++] = 0;
		}
	else
		{
		/* solve the resolvent cubic ... */

		coeffs[0] = 1.0/2 * r * p - 1.0/8 * q * q;
		coeffs[1] = - r;
		coeffs[2] = - 1.0/2 * p;
		coeffs[3] = 1;

		(void)SolveCubic(coeffs, s);

		/* ... and take the one real solution ... */

		z = s[0];

		/* ... to build two quadric equations */

		u = z * z - r;
		v = 2 * z - p;

		if(is_zero(u))
			u = 0;
		else if(u > 0)
			u = sqrt(u);
		else
			return 0;

		if(is_zero(v))
			v = 0;
		else if(v > 0)
			v = sqrt(v);
		else
			return 0;

		coeffs[0] = z - u;
		coeffs[1] = q < 0 ? -v : v;
		coeffs[2] = 1;

		num = SolveQuadric(coeffs, s);

		coeffs[0]= z + u;
		coeffs[1] = q < 0 ? v : -v;
		coeffs[2] = 1;

		num += SolveQuadric(coeffs, s + num);
		}

	/* resubstitute */

	sub = 1.0/4 * A;

	for(i = 0; i < num; ++i)
		s[i] -= sub;

	return num;
	}
