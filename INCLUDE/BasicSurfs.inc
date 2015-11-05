// A collection of basic surfaces

define Matte surface
{
	color = <1, 1, 1>;
	ambient = 0.2;
	diffuse = 0.8;
}

define Gloss surface
{
	color = <1, 1, 1>;
	ambient = 0.3;
	diffuse = 0.7;
	specular = 1;
	Phong = 50;
}

define Polished surface
{
	color = <1, 1, 1>;
	ambient = 0.1;
	diffuse = 0.8;
	reflection = 0.1;
	specular = 1;
	Phong = 500;
}

define Mirror surface
{
	color = <1, 1, 1>;
	ambient = 0.1;
	diffuse = 0.1;
	reflection = 0.8;
	specular = 1;
	Phong = 1000;
}

define Glass surface
{
	color = <1, 1, 1>;
	ambient = 0.0;
	diffuse = 0.1;
	transmission = 0.9;
	ior = 1.52;
	specular = 1;
	Phong = 1000;
}

