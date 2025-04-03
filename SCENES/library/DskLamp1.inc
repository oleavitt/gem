// A metal desk lamp
// bounds = <-3, -5.5, 0.01>, <3, 4.5, 16>

define DeskLamp1_Base object
{
	closed_cone <0, 0, 0>, <0, 0, 0.75>, 3, 2.75;
	closed_cone <0, -2, 0.75>, <0, -2, 1.25>, 0.3, 0.25
	{
		BlackGlossSurface;
	}
	bound box <-3, -3, 0>, <3, 3, 1.25>;
}

define DeskLamp1_Arm object
{
	closed_cylinder <-0.5, 0, 0>, <0.5, 0, 0>, 0.375;
	box <-0.15, -0.15, 0>, <0.15, 0.15, 8>;
}

define DeskLamp1_Head object
{
	difference
	{
		closed_cone <0, 2, 0>, <0, -2, 0>, 0.75, 1.5;
		closed_cone <0, 2, 0>, <0, -2, 0>, 0.75, 1.5
		{
			translate <0, -0.15, 0>;
			GlossSurface;
		}
	}
	closed_cylinder <-0.5, 0, -1.125>, <0.5, 0, -1.125>, 0.375;
	sphere <0, -1, 0>, 0.75
	{
		surface White, global_ka, global_kd, 0, 1, 200;
	}
	bound box <-1.5, -2, -1.5>, <1.5, 2, 1.5>;
	translate <0, 0, 1.125>;
}


define DeskLamp1_Head_On object
{
	difference
	{
		closed_cone <0, 2, 0>, <0, -2, 0>, 0.75, 1.5;
		closed_cone <0, 2, 0>, <0, -2, 0>, 0.75, 1.5
		{
			translate <0, -0.15, 0>;
			GlossSurface;
		}
	}
	closed_cylinder <-0.5, 0, -1.125>, <0.5, 0, -1.125>, 0.375;
	sphere <0, -1, 0>, 0.75
	{
		surface White, 5;
		no_shadow;
	}
	bound box <-1.5, -2, -1.5>, <1.5, 2, 1.5>;
	translate <0, 0, 1.125>;
}

define DeskLamp1_001 object
{
	DeskLamp1_Head { translate <0, 0, 8>; }
	DeskLamp1_Arm {}
}

define DeskLamp1_001_On object
{
	DeskLamp1_Head_On { translate <0, 0, 8>; }
	DeskLamp1_Arm {}
}

define DeskLamp1_002 object
{
	DeskLamp1_001 { rotate <-80, 0, 0>; translate <0, 0, 8>; }
	DeskLamp1_Arm {}
}

define DeskLamp1_002_On object
{
	DeskLamp1_001_On { rotate <-80, 0, 0>; translate <0, 0, 8>; }
	DeskLamp1_Arm {}
}

define DeskLamp1 object
{
	DeskLamp1_Base {}
	DeskLamp1_002 { rotate <30, 0, 0>; translate <0, 0, 0.75>; }
	bound box <-3, -5.5, 0.01>, <3, 4.5, 16>;
}

define DeskLamp1_On object
{
	DeskLamp1_Base {}
	DeskLamp1_002_On { rotate <30, 0, 0>; translate <0, 0, 0.75>; }
	bound box <-3, -5.5, 0.01>, <3, 4.5, 16>;
}

vector DeskLamp1_LightOrigin = vrotate(
	vrotate(<0, -1, 1.125> + <0, 0, 8>, <1, 0, 0>, -80 * PI/180) + <0, 0, 8>,
	<1, 0, 0>, 30*PI/180) + <0, 0, 0.75>;
