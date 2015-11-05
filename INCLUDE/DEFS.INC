// defs.inc
// Lighting and other convenient definitions.

float global_ka = 0.3;
float global_kd = 0.7;

// Number of lights in scene, change to match actual number
// before including the texture libraries, which use it to
// scale the specular highlight intensity.
// Example:
/*
include "colors.inc" // pre-defined colors
include "defs.inc"   // nlights defined in here

nlights = 4;         // 4 lights in scene

include "tex1.inc"   // basic texture definitions

// lights with their combined brightness normalized
// by the use of nlights
light <-10, -10, 10>, White / nlights;
light <10, -10, 10>, White / nlights;
light <-10, 10, 10>, White / nlights;
light <10, 10, 10>, White / nlights;
*/

float nlights = 1;

// Handy constants

float SQRT2 = sqrt(2);
float SQRT3 = sqrt(3);
float INVSQRT2 = 1/sqrt(2);
float TWOPI = PI * 2;