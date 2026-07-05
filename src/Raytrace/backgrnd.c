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
double ray_background_no_hit_alpha = 0.0;
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
    if (ct.trace_level == 0 &&
        ray_background_shader_list == NULL &&
        ISZERO(ray_background_no_hit_alpha)) {
        // Full alpha and no shaders for missed viewport rays.
        // No need to calculate background colors.
        RGBASet(ct.total_color, 0.0, 0.0, 0.0, 0.0);
        return;
    }

	Shader *shader;
	double dot = fabs(V3Dot(&ct.D, &ray_up_vector));
    Background background;
	V3Interpolate(&background.color, &ray_background_color1, dot,
		&ray_background_color2);
    background.alpha = ray_background_no_hit_alpha;

	// Run the shader(s)
	//
	for (shader = ray_background_shader_list;
		shader != NULL;
		shader = shader->next)
	{
		Ray_RunShader(shader, &background);
	}

    ct.total_color.r += background.color.x;
    ct.total_color.g += background.color.y;
    ct.total_color.b += background.color.z;

    // Set the background alpha amount if ray is a miss on the first shot.
    ct.total_color.a = ct.trace_level == 0 ? background.alpha : 1.0;
}
