/**
*****************************************************************************
* @file visiblty.c
*  Apply distance visibility falloff to the color value returned. 
*
*****************************************************************************
*/

#include "ray.h"

// Visibility falloff
Vec3 ray_visibility_color;
double ray_visibility_distance;

void InitializeVisibility(void)
{
}


void CloseVisibility(void)
{
}


void Ray_ApplyVisibility(void)
{
   if (ray_visibility_distance > EPSILON)
   {
      double v = exp(ct.t/-ray_visibility_distance);
      V3Interpolate(&ct.total_color, &ray_visibility_color, v, &ct.total_color);
   }
}
