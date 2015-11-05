/*************************************************************************
*
*  uvmap.c - A collection of 2D and 3D UV mapping functions that
*   take a 3D point, "P", in the range of <-1,-1,-1> <= P <= <1,1,1>,
*   and return a 2D UV coordinate pair, "u" and "v", in the ranges
*   of 0 <= u < 1, and 0 <= v < 1.
*   2D functions project the 3D point, "P", to the XY plane.
*
*************************************************************************/

#include "ray.h"

/*************************************************************************
 *
 *  Ray_PlaneMap() - Project 3D point, "P", to a plane on the
 *    X and Y axes.
 *
 ************************************************************************/
void Ray_PlaneMap(Vec3 *P, double *u, double *v)
{
  double x, y;

  x = P->x * 0.5 + 0.5;
  y = 0.5 - P->y * 0.5;
  *u = x - floor(x);
  *v = y - floor(y);
}

/*************************************************************************
 *
 *  Ray_DiscMap() - Project 3D point, "P", to a disc with radius
 *    of one unit on the X and Y axes.
 *
 ************************************************************************/
void Ray_DiscMap(Vec3 *P, double *u, double *v)
{
  double x, y, r;

  x = P->x * 0.5 + 0.5;
  y = P->y * 0.5 + 0.5;
  x = x - floor(x);
  y = y - floor(y);
  x = x * 2.0 - 1.0;
  y = y * 2.0 - 1.0;
  if(fabs(x) < EPSILON && fabs(y) < EPSILON)
  {
    *u = 0.5;
    *v = 0.5;
    return;
  }

  if((x * x + y * y) < 1.0)
  {
    r = sqrt(1.0 - y * y) * 2.0;
    if(fabs(r) > EPSILON)
      *u = x / r + 0.5;
    else
      *u =(x > 0.0) ? 1.0 : 0.0;

    r = sqrt(1.0 - x * x) * 2.0;
    if(fabs(r) > EPSILON)
      *v = 0.5 - y / r;
    else
      *v =(y > 0.0) ? 0.0 : 1.0;
  }
  else
  {
    *u = 0.0;
    *v = 0.0;
  }
}

/*************************************************************************
 *
 *  Ray_CylinderMap() - Project 3D point, "P", to a cylinder aligned
 *  along the Z axis.
 *
 ************************************************************************/
void Ray_CylinderMap(Vec3 *P, double *u, double *v)
{
  double x, y, z, r;

  x = P->x;
  y = P->y;
  z = P->z;

  *v = z - floor(z);

  r = sqrt(x * x + y * y);
  if(r > EPSILON)
  {
    y /= r;
    if(fabs(y) > 1.0)
      y = (y < 0.0) ? -1.0 : 1.0;
    *u = acos(y) / TWOPI;
    if(x > 0.0)
      *u = 1.0 - *u;
  }
  else
  {
    *u = 0.0;
  }
}

/*************************************************************************
 *
 *  Ray_SphereMap() - Project 3D point, "P", to a sphere with its poles
 *  along the Z axis.
 *
 ************************************************************************/
void Ray_SphereMap(Vec3 *P, double *u, double *v)
{
  double x, y, z, r;

  x = P->x;
  y = P->y;
  z = P->z;
  r = V3Mag(P);
  if(r < EPSILON)
  {
    *u = 0.0;
    *v = 0.5;
    return;
  }

  z /= r;
  if(fabs(z) > 1.0)
    z = (z < 0.0) ? -1.0 : 1.0;
  *v = acos(-z) / PI;

  r = sqrt(x * x + y * y);
  if(r > EPSILON)
  {
    y /= r;
    if(fabs(y) > 1.0)
      y = (y < 0.0) ? -1.0 : 1.0;
    *u = acos(y) / TWOPI;
    if(x > 0.0)
      *u = 1.0 - *u;
  }
  else
  {
    *u = 0.0;
  }
}

/*************************************************************************
 *
 *  Ray_TorusMap() - Project 3D point, "P", to a torus with its poles
 *  along the Z axis and a major radius of one unit.
 *
 ************************************************************************/
void Ray_TorusMap(Vec3 *P, double *u, double *v)
{
  double x, y, z, rx, ry, r;

  x = rx = P->x;
  y = ry = P->y;
  z = P->z;
  r = sqrt(x * x + y * y);
  if(r > EPSILON)
  {
	  rx /= r;
    ry /= r;
    if(fabs(ry) > 1.0)
      ry = (ry < 0.0) ? -1.0 : 1.0;
    *u = acos(ry) / TWOPI;
    if(rx > 0.0)
      *u = 1.0 - *u;
		x = P->x - rx;
		y = P->y - ry;
    r = sqrt(x * x + y * y + z * z);
		if(r > EPSILON)
		{
		  x /= r;
			y /= r;
			r = -(x * rx + y * ry);
      if(fabs(r) > 1.0)
        r = (r < 0.0) ? -1.0 : 1.0;
			*v = acos(r) / TWOPI;
      if(z > 0.0)
        *v = 1.0 - *v;
		}
		else
		{
		  *v = 0.0;
		}
  }
  else
  {
    *u = 0.0;
		*v = 0.0;
  }
}

/*
void Ray_TorusMap(Vec3 *P, double *u, double *v)
{
  double x, y, z, rx, ry, r;

  rx = P->x;
  ry = P->y;
  r = sqrt(rx * rx + ry * ry);
  if(r > EPSILON)
  {
    rx /= r;
    ry /= r;
    if(fabs(ry) > 1.0)
      *u = (ry < 0.0) ? 0.5 : 0.0;
    else
      *u = acos(ry) / TWOPI;
    if(rx > 0.0)
      *u = 1.0 - *u;

    x = P->x - rx;
    y = P->y - ry;
    z = P->z;
    r = sqrt(x * x + y * y + z * z);
    if(r > EPSILON)
    {
      x /= r;
      y /= r;
      rx = -rx;
      ry = -ry;
      r = x * rx + y * ry;
      if(fabs(r) > 1.0)
        *v =(r < 0.0) ? 0.5 : 0.0;
      else
        *v = acos(r) / TWOPI;
      if(y < 0.0)
        *v = 1.0 - *v;
    }
    else
    {
      *v = 0.5;
    }
  }
  else
  {
    *u = 0.0;
    *v = 0.5;
  }
}
*/

int Ray_EnvironmentMap(Vec3 *D, double *u, double *v)
{
  int face;
  double du, dv, dn, ax, ay, az, t;

  /* Determine dominant projection axis. */
  ax = fabs(D->x);
  ay = fabs(D->y);
  az = fabs(D->z);
  if(ax > az)
  {
    if(ax > ay)
    {
      if(D->x < 0.0)
      {
        face = 0;
        du = D->y;
        dv = D->z;
        dn = ax;
      }
      else
      {
        face = 1;
        du = -D->y;
        dv = D->z;
        dn = ax;
      }
    }
    else if(D->y < 0.0)
    {
      face = 2;
      du = -D->x;
      dv = D->z;
      dn = ay;
    }
    else
    {
      face = 3;
      du = D->x;
      dv = D->z;
      dn = ay;
    }
  }
  else if(ay > az)
  {
    if(D->y < 0.0)
    {
      face = 2;
      du = -D->x;
      dv = D->z;
      dn = ay;
    }
    else
    {
      face = 3;
      du = D->x;
      dv = D->z;
      dn = ay;
    }
  }
  else if(D->z < 0.0)
  {
    face = 4;
    du = D->x;
    dv = D->y;
    dn = az;
  }
  else
  {
    face = 5;
    du = D->x;
    dv = -D->y;
    dn = az;
  }

  t = 0.5 / dn;
  *u = t * du + 0.5;
  *v = t * dv + 0.5;

  return face;
}

