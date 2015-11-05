// An urn

define Urn object
{
	// Form the urn's body.
	clip
	{
		object
		{
			clip
			{
				sphere <0, 0, 0>, 1;
				difference
				{
					disc <0, 0, 0>, <0, 0, -1>, 1;
					closed_cylinder <0, 0, 0.5>, <0, 0, 1.5>, 0.35;
				}
			}
			clip
			{
				sphere <0, 0, 0>, 1;
				disc <0, 0, 0>, <0, 0, 1>, 1;
				scale <1, 1, 2.5>;
			}
		}
		disc <0, 0, -2>, <0, 0, -1>, 1;
	}

	// The handles.
//	clip
//	{
		object
		{
			torus <0, 0, 0>, 0.3, 0.05
			{
				rotate <90, 0, 0>;
				translate <0.707, 0, 0.707>;
			}
			torus <0, 0, 0>, 0.3, 0.05
			{
				rotate <90, 0, 0>;
				translate <-0.707, 0, 0.707>;
			}
		}
//		sphere <0, 0, 0>, 1 { inverse = 1; }
//	}

	// The narrow neck and top opening.
	clip
	{
		torus <0, 0, 1.11>, 0.45, 0.2;
		closed_cylinder <0, 0, 0.5>, <0, 0, 1.5>, 0.35;
	}

	// A rim around the mouth.
	torus <0, 0, 1.26>, 0.35, 0.015;

	// A rim around the base.
	torus <0, 0, -2.03>, 0.55, 0.05;

	// A cap for the hole in the base.
	disc <0, 0, -2.08>, <0, 0, -1>, 0.55;

	// A marbled surface.
	surface
	{
		surface_shader
		{
			float a = turb2(O*3, 5, 2, 0.618) * 5;
			color = (abs(a) < 1) ? (DarkSlateBlue + abs(a) * (White - DarkSlateBlue)) : White;
		}
		ambient = 0.2;
		diffuse = 0.8;
		specular = 1;
		Phong = 100;
	}

	// Translate it so that the center point is at the base.
	translate <0, 0, 2.085>;

	// Scale it up so that the urn stands approximately 25 units high
	// by 16 units wide.
	scale 8;
}
