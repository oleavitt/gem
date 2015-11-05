// My Funny Face icon

define FunnyFace1 difference
{
	closed_cylinder <0, 0, -1>, <0, 0, 1>, 1;
	difference
	{
		closed_cylinder <0, 0, -1.1>, <0, 0, 1.1>, 0.9;
		box <-1, -1, -1.2>, <1, 0.25, 1.2>;
		box <-0.25, -1, -1.2>, <0.25, 1, 1.2>;
	}
	difference
	{
		closed_cylinder <0, 0, -1.1>, <0, 0, 1.1>, 0.9;
		box <-1, -0.25, -1.2>, <1, 1, 1.2>;
		rotate <0, 0, -22.5>;
	}
	bound box <-1, -1, -1>, <1, 1, 1>;
}
