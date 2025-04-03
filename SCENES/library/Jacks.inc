// A collection of jacks for our synth.

define JackChromeSurface surface White, global_ka * 0.1, global_kd * 0.1,
	nlights, 300, 0.8;

define JackDullMetalSurface surface 0.5, global_ka, global_kd, 0, nlights, 100;

define JackRedSurface surface Red, global_ka, global_kd, 0, nlights*0.5, 100;

define JackBlackSurface surface 0.25, global_ka, global_kd, 0, nlights*0.5, 100;

define JackRed object
{
	difference
	{
		closed_cylinder <0, 0, 0>, <0, 0, 0.25>, 0.2;
		closed_cylinder <0, 0, -0.1>, <0, 0, 0.3>, 0.075;
	}
	cone <0, 0, 0.2>, <0, 0, 0.175>, 0.075, 0.062
	{
		JackChromeSurface;
	}
	cylinder <0, 0, 0.175>, <0, 0, 0>, 0.062
	{
		JackChromeSurface;
	}
	bound box <-0.21, -0.21, -0.01>, <0.21, 0.21, 0.26>;
	JackRedSurface;
}

define JackBlack object
{
	difference
	{
		closed_cylinder <0, 0, 0>, <0, 0, 0.25>, 0.2;
		closed_cylinder <0, 0, -0.1>, <0, 0, 0.3>, 0.075;
	}
	cone <0, 0, 0.2>, <0, 0, 0.175>, 0.075, 0.062
	{
		JackChromeSurface;
	}
	cylinder <0, 0, 0.175>, <0, 0, 0>, 0.062
	{
		JackChromeSurface;
	}
	bound box <-0.21, -0.21, -0.01>, <0.21, 0.21, 0.26>;
	JackBlackSurface;
}
