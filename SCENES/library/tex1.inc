// tex1.inc
// Some useful standard surface texture definitions.

define MatteSurface surface 1, global_ka, global_kd;
define BlackMatteSurface surface 0, global_ka, global_kd;
define RedMatteSurface surface <1, 0, 0>, global_ka, global_kd;
define YellowMatteSurface surface <1, 1, 0>, global_ka, global_kd;
define GreenMatteSurface surface <0, 1, 0>, global_ka, global_kd;
define CyanMatteSurface surface <0, 1, 1>, global_ka, global_kd;
define BlueMatteSurface surface <0, 0, 1>, global_ka, global_kd;
define MagentaMatteSurface surface <1, 0, 1>, global_ka, global_kd;

define GlossSurface surface 1, global_ka, global_kd, 0, 1, 50;
define BlackGlossSurface surface 0, global_ka, global_kd, 0, 1, 50;
define RedGlossSurface surface <1, 0, 0>, global_ka, global_kd, 0, 1, 50;
define YellowGlossSurface surface <1, 1, 0>, global_ka, global_kd, 0, 1, 50;
define GreenGlossSurface surface <0, 1, 0>, global_ka, global_kd, 0, 1, 50;
define CyanGlossSurface surface <0, 1, 1>, global_ka, global_kd, 0, 1, 50;
define BlueGlossSurface surface <0, 0, 1>, global_ka, global_kd, 0, 1, 50;
define MagentaGlossSurface surface <1, 0, 1>, global_ka, global_kd, 0, 1, 50;

define PolishedSurface surface 1, global_ka*0.2, global_kd*0.5, 0.3, 1, 200;
define BlackPolishedSurface surface 0, global_ka*0.2, global_kd*0.5, 0.3, 1, 200;
define RedPolishedSurface surface <1, 0, 0>, global_ka*0.2, global_kd*0.5, 0.3, 1, 200;
define YellowPolishedSurface surface <1, 1, 0>, global_ka*0.2, global_kd*0.5, 0.3, 1, 200;
define GreenPolishedSurface surface <0, 1, 0>, global_ka*0.2, global_kd*0.5, 0.3, 1, 200;
define CyanPolishedSurface surface <0, 1, 1>, global_ka*0.2, global_kd*0.5, 0.3, 1, 200;
define BluePolishedSurface surface <0, 0, 1>, global_ka*0.2, global_kd*0.5, 0.3, 1, 200;
define MagentaPolishedSurface surface <1, 0, 1>, global_ka*0.2, global_kd*0.5, 0.3, 1, 200;

define ChromeSurface surface 1, global_ka*0.1, global_kd*0.1, 0.8, nlights, 200;