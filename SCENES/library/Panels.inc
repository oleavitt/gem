// Nine inch high panels of various widths for our synth.

// include_once;

float Panel_Thickness = 0.062;
float Panel_ScrewHeight = 0.125;

define Panel_BrushedAluminum surface
	0.8, global_ka, global_kd, 0, nlights*0.25, 200;

define Panel_ScrewSurface surface
	BrightBrass, global_ka, global_kd, 0, nlights, 200;

define Panel_2_Installed object
{
	closed_cone <0.25, 0.25, Panel_Thickness>, <0.25, 0.25, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	closed_cone <0.25, 8.75, Panel_Thickness>, <0.25, 8.75, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	closed_cone <1.75, 0.25, Panel_Thickness>, <1.75, 0.25, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	closed_cone <1.75, 8.75, Panel_Thickness>, <1.75, 8.75, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	box <0, 0, 0>, <1.99, 8.99, Panel_Thickness>;
	bound box <-0.001, -0.001, -0.001>, <2, 9, Panel_Thickness + Panel_ScrewHeight + 0.001>;
	Panel_BrushedAluminum;
}

define Panel_4_Installed object
{
	closed_cone <0.25, 0.25, Panel_Thickness>, <0.25, 0.25, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	closed_cone <0.25, 8.75, Panel_Thickness>, <0.25, 8.75, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	closed_cone <3.75, 0.25, Panel_Thickness>, <3.75, 0.25, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	closed_cone <3.75, 8.75, Panel_Thickness>, <3.75, 8.75, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	box <0, 0, 0>, <3.99, 8.99, Panel_Thickness>;
	bound box <-0.001, -0.001, -0.001>, <4, 9, Panel_Thickness + Panel_ScrewHeight + 0.001>;
	Panel_BrushedAluminum;
}

define Panel_8_Installed object
{
	closed_cone <0.25, 0.25, Panel_Thickness>, <0.25, 0.25, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	closed_cone <0.25, 8.75, Panel_Thickness>, <0.25, 8.75, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	closed_cone <7.75, 0.25, Panel_Thickness>, <7.75, 0.25, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	closed_cone <7.75, 8.75, Panel_Thickness>, <7.75, 8.75, Panel_Thickness + Panel_ScrewHeight>, 0.15, 0.13
	{
		Panel_ScrewSurface;
	}
	box <0, 0, 0>, <7.99, 8.99, Panel_Thickness>;
	bound box <-0.001, -0.001, -0.001>, <8, 9, Panel_Thickness + Panel_ScrewHeight + 0.001>;
	Panel_BrushedAluminum;
}
