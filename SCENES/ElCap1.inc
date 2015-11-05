// Small electrolytic capacitor #1
// 0.4" high, 0.2" diameter, 0.25" leads

define ElCap1 object
{
	disc <0, 0, 3.9>, <0, 0, 1>, 0.75
	{
		Mirror
		{
			reflection = 0.6;
			Phong = 400;
		}
	}

	torus <0, 0, 3.75>, 0.75, 0.25;

	difference
	{
		closed_cylinder <0, 0, 0>, <0, 0, 3.75>, 1;
		torus <0, 0, 0.5>, 1, 0.25;
		closed_cylinder <0, 0, -0.1>, <0, 0, 0.05>, 0.75
		{
			surface
			{
				color = 0.25;
				ambient = 0.2;
				diffuse = 0.8;
				specular = 1;
				Phong = 100;
			}
		}
	}

	// The leads
	object
	{
		closed_cylinder <-0.5, 0, 0.06>, <-0.5, 0, -2.5>, 0.09;
		closed_cylinder <0.5, 0, 0.06>, <0.5, 0, -2.5>, 0.09;
		surface
		{
			color = 1;
			ambient = 0.2;
			diffuse = 0.5;
			reflection = 0.3;
			specular = 2;
			Phong = 300;
		}
	}

	surface
	{
		surface_shader
		{
			vector v1;

			v1 = cylinder_map(O*<1, 1, 0.25>);
			color = image_map("10mF35V.tga", v1.y, v1.x) + <0.2, 0.2, 0.7>;
		}
        ambient = 0.2;
        diffuse = 0.8;
        specular = 1;
        Phong = 200;
	}

	scale 0.1;
}

