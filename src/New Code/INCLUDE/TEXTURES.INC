//
// textures.inc - A collection of pre-defined textures.
//

include "colors.inc"

//
// Start with basic attributes and build
// up from here...
//

texture Matte
  {
  ambient 0.0
  diffuse 1.0
  }

texture Shiny
  {
  ambient 0.0
  diffuse 1.0
  specular 0.8, 0.02
  }

texture Semi_Reflective
  {
  ambient 0.0
  diffuse 0.8
  reflection 0.2
  specular 0.9, 0.005
  }

texture Mirror
  {
  ambient 0.0
  diffuse 0.0
  reflection 1.0
  }

texture Semi_Mirror
  {
  ambient 0.0
  diffuse 0.2
  reflection 0.8
  }


texture BW_Checker
  {
  Matte
  color fn checker(O)
  }

texture RW_Checker
  {
  Matte
  color fn checker(O) ? White : Red
  }

texture Polished_Copper
  {
  color White
  surface 0, 0.1, Copper
  specular Copper*3, 0.005
  }

texture Polished_Brass
  {
  color White
  surface 0, 0.1, Brass
  specular Brass*3, 0.005
  }

texture Polished_Chrome
  {
  color White
  surface 0, 0.1, White * 0.85
  specular 1, 0.005
  }

texture Polished_Gold
  {
  color White
  surface 0, 0.1, Gold
  specular Gold*3, 0.005
  }

texture Dented_Copper
  {
  Polished_Copper
  bump vturb(O*2, 3)*0.25
  }

