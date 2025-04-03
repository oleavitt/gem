// A basic chair.

define Chair1_cushion object
{
	intersection
	{
		object
		{
			sphere <2, 2, 0>, 2;
			cylinder <2, 2, 0>, <14, 2, 0>, 2;
			sphere <14, 2, 0>, 2;
			cylinder <14, 2, 0>, <14, 14, 0>, 2;
			sphere <14, 14, 0>, 2;
			cylinder <14, 14, 0>, <2, 14, 0>, 2;
			sphere <2, 14, 0>, 2;
			cylinder <2, 14, 0>, <2, 2, 0>, 2;
			box <2, 2, -0.01>, <14, 14, 2>;
		}
		box <-0.001, -0.001, 0>, <16.001, 16.001, 2.001>;
	}
	bound box <-0.001, -0.001, -0.001>, <16.001, 16.001, 2.001>;
}

define Chair1_backrest object
{
	object
	{
		cylinder <0.5, 0, 0>, <0.5, 0, 20>, 0.5;
		sphere <0.5, 0, 20>, 0.5;
		cylinder <17.5, 0, 0>, <17.5, 0, 20>, 0.5;
		sphere <17.5, 0, 20>, 0.5;
		ChromeSurface;
	}
	Chair1_cushion
	{
		rotate <-90, 0, 0>;
		translate <1, 0, 4>;
	}
}

define Chair1_side object
{
	sphere <0.5, 0.5, 0.5>, 0.5;
	cylinder <0.5, 0.5, 0.5>, <0.5, 2.5, 16>, 0.5;
	sphere <0.5, 2.5, 16>, 0.5;
	cylinder <0.5, 2.5, 16>, <0.5, 16.5, 16>, 0.5;
	sphere <0.5, 16.5, 16>, 0.5;
	cylinder <0.5, 16.5, 16>, <0.5, 18.5, 0.5>, 0.5;
	sphere <0.5, 18.5, 0.5>, 0.5;
	ChromeSurface;
	bound box <-0.001, -0.001, -0.001>, <1.001, 19.001, 16.501>;
}

define Chair1 object
{
	Chair1_side {}
	Chair1_side { translate <17, 0, 0>; }
	Chair1_cushion { translate <1, 1, 16>; }
	Chair1_backrest
	{
		rotate <8, 0, 0>;
		translate <0, 16.5, 16>;
	}
	bound box <-0.001, -0.001, -0.001>, <18.001, 21, 36.501>;
}