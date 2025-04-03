// Good ol' #2
// bounds = <0, -0.145, 0.001>, <6.75, 0.145, 0.271>
float PencilTexture_Dist;

define PencilBody intersection
{
 box <0, -0.25, -0.135>, <6, 0.25, 0.135>;
 box <-0.01, -0.25, -0.135>, <6.01, 0.25, 0.135> { rotate <60, 0, 0>; }
 box <-0.02, -0.25, -0.135>, <6.02, 0.25, 0.135> { rotate <120, 0, 0>; }
 cylinder <-0.1, 0, 0>, <6.1, 0, 0>, 0.145;
 cone <-0.03, 0, 0>, <6.03, 0, 0>, 1, 0;
 surface White, global_ka, global_kd
 {
   color (PencilTexture_Dist = sqrt(z * z + y * y),
    PencilTexture_Dist > 0.134 ? YellowOrange :
    PencilTexture_Dist > 0.036 ? Tan : Black);
 }
}

define PencilEraserSleeve object
{
 cylinder <0, 0, 0>, <0.5, 0, 0>, 0.135;
 surface White * 0.75, global_ka, global_kd, 0, nlights, 200;
}

define PencilEraser object
{
 cylinder <0.05, 0, 0>, <0.5, 0, 0>, 0.13;
 torus <0, 0, 0>, 1.3 - 0.5, 0.5
 {
  rotate <0, 90, 0>;
  translate <0.5, 0, 0>;
  scale 0.1;
 }
 disc <0, 0, 0>, <-1, 0, 0>, 0.13 - 0.05;
 surface Pink, global_ka, global_kd;
}

define Pencil object
{
 PencilBody { translate <0.75, 0, 0.136>; }
 PencilEraserSleeve { translate <0.25, 0, 0.136>; }
 PencilEraser { translate <0, 0, 0.136>; }
 bound box <0, -0.145, 0.001>, <6.75, 0.145, 0.271>;
}
