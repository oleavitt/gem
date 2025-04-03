// An office desk.

define OfficeDesk_TopSurface surface
	Tan, global_ka, global_kd, 0, nlights * 0.25, 200
{
	// Wood grain procedure goes here.
}

define OfficeDesk_ChromeSurface surface
	White, global_ka * 0.1, global_kd * 0.2 , 0.7, nlights, 500;

define OfficeDesk_BlackSurface surface
	<0.33, 0.42, 0.67>, global_ka, global_kd , 0, nlights * 0.25, 200;

define OfficeDesk_Handle object
{
	clip
	{
		torus <-1.5, 0, 0>, 1, 0.125;
		box <-2.626, 0, -0.126>, <-1.5, -1.126, 0.126>;
	}
	clip
	{
		torus <1.5, 0, 0>, 1, 0.125;
		box <1.5, 0, -0.126>, <2.626, -1.126, 0.126>;
	}
	cylinder <-1.5, -1, 0>, <1.5, -1, 0>, 0.125;
	bound box <-2.626, 0.001, -0.126>, <2.626, -1.126, 0.126>;
	OfficeDesk_ChromeSurface;
}

define OfficeDesk_KeyLock object
{
	intersection
	{
		sphere <0, 0, 0>, 0.375;
		box <-0.375, -0.2, -0.375>, <0.375, 0, 0.375>;
	}
	difference
	{
		closed_cylinder <0, -0.2, 0>, <0, -0.25, 0>, 0.25;
		box <-0.04, -0.19, -0.15>, <0.04, -0.26, 0.15>;
	}
	OfficeDesk_ChromeSurface;
}

define OfficeDesk_LeftDrawers object
{
	box <0, 0, 0>, <16, 35, 20>;
	box <0.5, -0.25, 0.5>, <15.5, 0, 6.5>;
	OfficeDesk_Handle { translate <8, -0.25, 3.5>; }
	box <0.5, -0.25, 7>, <15.5, 0, 13>;
	OfficeDesk_Handle { translate <8, -0.25, 10>; }
	box <0.5, -0.25, 13.5>, <15.5, 0, 19.5>;
	OfficeDesk_Handle { translate <8, -0.25, 16.5>; }
	bound box <0, -2, 0>, <16, 35, 20>;
}

define OfficeDesk_RightDrawers object
{
	box <0, 0, 0>, <16, 35, 20>;
	box <0.5, -0.25, 0.5>, <15.5, 0, 13>;
	OfficeDesk_Handle { translate <8, -0.25, 6.75>; }
	box <0.5, -0.25, 13.5>, <15.5, 0, 19.5>;
	OfficeDesk_Handle { translate <8, -0.25, 16.5>; }
	bound box <0, -2, 0>, <16, 35, 20>;
}

define OfficeDesk_MiddleDrawer object
{
	box <0, 0, 0>, <25, 35, 2>;
	box <0, -0.25, 0>, <25, 0, 1.75>;
	OfficeDesk_KeyLock { translate <12.5, -0.25, 1>; }
	bound box <0, -1, 0>, <25, 35, 2>;
}

define OfficeDesk object
{
	box <0, 0, 30>, <60, 36, 31> { OfficeDesk_TopSurface; }
	box <0.5, 0.5, 0>, <1.5, 1.5, 30> { OfficeDesk_ChromeSurface; }
	box <0.5, 34.5, 0>, <1.5, 35.5, 30> { OfficeDesk_ChromeSurface; }
	box <58.5, 0.5, 0>, <59.5, 1.5, 30> { OfficeDesk_ChromeSurface; }
	box <58.5, 34.5, 0>, <59.5, 35.5, 30> { OfficeDesk_ChromeSurface; }
	box <1.5, 35.5, 6>, <58.5, 35.55, 30>;
	OfficeDesk_LeftDrawers { translate <1.5, 0.5, 10>; }
	OfficeDesk_RightDrawers { translate <42.5, 0.5, 10>; }
	OfficeDesk_MiddleDrawer { translate <17.5, 0.5, 28>; }
	bound box <0, -1, 0>, <60, 36, 31>;
	OfficeDesk_BlackSurface;
}
