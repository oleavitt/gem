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

void InitializeVisibility(void) {
}


void CloseVisibility(void) {
}

void Ray_ApplyVisibility(void) {
   if (ray_visibility_distance > EPSILON) {
       double v = exp(ct.t/-ray_visibility_distance);
       ct.total_color.r = ray_visibility_color.x + v * (ray_visibility_color.x - ct.total_color.r);
       ct.total_color.g = ray_visibility_color.y + v * (ray_visibility_color.y - ct.total_color.g);
       ct.total_color.b = ray_visibility_color.z + v * (ray_visibility_color.z - ct.total_color.b);
   }
}
