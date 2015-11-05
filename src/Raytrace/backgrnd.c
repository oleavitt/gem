/**
 *****************************************************************************
 * @file backgrnd.c
 *  Do something pretty for those poor rays that didn't hit any objects.
 *
 *****************************************************************************
 */

#include "ray.h"

Vec3 ray_background_color1;
Vec3 ray_background_color2;
Shader *ray_background_shader_list = NULL;

void InitializeBackground(void)
{
	ray_background_shader_list = NULL;
}


void CloseBackground(void)
{
	Ray_DeleteShaderList(ray_background_shader_list);
	ray_background_shader_list = NULL;
}


void Ray_DoBackground(void)
{
	Shader *shader;
	double dot = fabs(V3Dot(&ct.D, &ray_up_vector));
	V3Interpolate(&ct.total_color, &ray_background_color1, dot,
		&ray_background_color2);
	
	// Run the shader(s)
	//
	for (shader = ray_background_shader_list;
		shader != NULL;
		shader = shader->next)
	{
		Ray_RunShader(shader, &ct.total_color); 
	}
}
