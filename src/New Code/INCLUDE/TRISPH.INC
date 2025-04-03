//-------------------------------------------------------------------------------
//
//  Procedures for generating a sphere or hemisphere of triangles.
//
//  Parameters (default values shown) for TriSphere & TriHemisphere functions:
//  Usage: TriSphere(vector Center = 0, float Radius = 1, float Level = 2,
//              float NumberOfSegments = 6)
//  
//  NumberOfSegments should be at least 3.
//
//-------------------------------------------------------------------------------

//
// Procedure that sub-divides a segment of the sphere's surface
// into a mesh of triangles.
//
proc SubDivideTriangle(vector P1, vector P2, vector P3, float level,
  vector Center, float Radius)
  {
  vector MP12, MP13, MP23

  if level > 0
    {
    //
    // Compute mid-points and project onto sphere
    //
    MP12 = vnorm((P2 - P1) / 2 + P1) * Radius
    MP13 = vnorm((P3 - P1) / 2 + P1) * Radius
    MP23 = vnorm((P3 - P2) / 2 + P2) * Radius
    //
    // Sub-divide down another level...
    //
    level -= 1
    SubDivideTriangle(P1, MP12, MP13, level, Center, Radius)
    SubDivideTriangle(MP12, P2, MP23, level, Center, Radius)
    SubDivideTriangle(MP13, MP23, P3, level, Center, Radius)
    //
    // Comment out this line, and see what you get... ;^)
    //
//    SubDivideTriangle(MP12, MP23, MP13, level, Center, Radius)
    }
  else
    {
    //
    // Emit a triangle.
    //
    object
      {
      tri P1, P2, P3
      translate Center
      }
    }
  }


//
// Make a hemisphere.
//
proc TriHemisphere(vector Center = 0, float Radius = 1, float Level = 2,
  float NumberOfSegments = 6)
  {
  float th = 0, Angle = 360 / NumberOfSegments

  while th < 359
    {
    SubDivideTriangle(<0,Radius,0>,rotate(<0,0,Radius>,<0,th,0>),
      rotate(<0,0,Radius>,<0,th+Angle,0>), Level, Center, Radius)
    th += Angle
    }
  }


//
// Make a full sphere
//
proc TriSphere1(vector Center = 0, float Radius = 1, float Level = 2,
  float NumberOfSegments = 6)
  {
  float th = 0, Angle = 360 / NumberOfSegments

  while th < 359
    {
    SubDivideTriangle(<0,Radius,0>,rotate(<0,0,Radius>,<0,th,0>),
      rotate(<0,0,Radius>,<0,th+Angle,0>), Level, Center, Radius)
    SubDivideTriangle(<0,-Radius,0>,rotate(<0,0,Radius>,<180,th,0>),
      rotate(<0,0,Radius>,<180,th+Angle,0>), Level, Center, Radius)
    th += Angle
    }
  }


//
// Make a full sphere, another recipe...
//
proc TriSphere2(vector Center = 0, float Radius = 1, float Level = 2)
  {
  vector P1, P2, P3, P4

  P1 = rotate(<0, 1, 0>, <120, -120, 0>) * Radius
  P2 = rotate(<0, 1, 0>, <120, 120, 0>) * Radius
  P3 = rotate(<0, 1, 0>, <120, 0, 0>) * Radius
  P4 = <0, 1, 0> * Radius
  SubDivideTriangle(P1, P2, P3, Level, Center, Radius)
  SubDivideTriangle(P1, P2, P4, Level, Center, Radius)
  SubDivideTriangle(P1, P3, P4, Level, Center, Radius)
  SubDivideTriangle(P2, P3, P4, Level, Center, Radius)
  }