// A collection of control knobs for our synth.

define KnobInsetSurface surface LightSteelBlue, global_ka, global_kd;
//define KnobSurface surface Black, global_ka, global_kd, 0, nlights*0.5, 100;
define KnobPointerSurface surface 0.25, global_ka, global_kd, 0, nlights, 100
{
	color u < 0.0125 || u > 0.9875 ? White : 0.25;
}

define Knob_0_75 object
{
	difference
	{
		closed_cone <0, 0, 0>, <0, 0, 0.5>, 0.3, 0.25;
		closed_cone <0, 0, -0.1>, <0, 0, 0.6>, 0.2, 0.2;
	}
	disc <0, 0, 0.475>, <0, 0, 1>, 0.21
	{
		KnobInsetSurface;
	}
	closed_cone <0, 0, 0>, <0, 0, 0.25>, 0.375;
	bound box <-0.376, -0.376, -0.01>, <0.376, 0.376, 0.501>;
	KnobPointerSurface;
}

define Knob_1_00 object
{
	difference
	{
		closed_cone <0, 0, 0>, <0, 0, 0.62>, 0.375, 0.325;
		closed_cone <0, 0, -0.1>, <0, 0, 0.7>, 0.25, 0.25;
	}
	disc <0, 0, 0.595>, <0, 0, 1>, 0.3
	{
		KnobInsetSurface;
	}
	closed_cone <0, 0, 0>, <0, 0, 0.35>, 0.5;
	bound box <-0.501, -0.501, -0.01>, <0.501, 0.501, 0.621>;
	KnobPointerSurface;
}
