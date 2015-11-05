// Paper clip
// Bounds = <-0.6375, 0, 0>, <0.625, 0.275, 0.036>

float r1 = 0.18;
float r2 = 1.375 - r1;
float r3 = 1.125 - r1;
float r4 = r3 - r1;

define PaperClip object
{
 clip
 {
  torus <-5, 1.375, r1>, r2, r1;
  box <-5, 0, 0>, <-5-r2-r1, (r2+r1)*2, r1*2>;
 }
 clip
 {
  torus <5, 1.125, r1>, r3, r1;
  box <5+r3+r1, 0, 0>, <5, (r3+r1)*2, r1*2>;
 }
 clip
 {
  torus <-2.5, r3+2*r1, r1>, r4, r1;
  box <-2.5-r4-r1, r1*2, 0>, <-2.5, (r1+r4+r1)*2, r1*2>;
 }
 cylinder <-5, r1, r1>, <5, r1, r1>, r1;
 cylinder <-2.5, (r4+r1)*2+r1, r1>, <5, (r4+r1)*2+r1, r1>, r1;
 closed_cylinder <-2.5, 3*r1, r1>, <2.5, 3*r1, r1>, r1;
 closed_cylinder <-5, r2*2+r1, r1>, <2.5, r2*2+r1, r1>, r1;
 surface White, global_ka * 0.1, global_kd * 0.2, 0.7, nlights, 200;
 bound box <-6.38, -0.01, -0.01>, <6.26, 2.76, 0.37>;
 scale 0.1;
}
