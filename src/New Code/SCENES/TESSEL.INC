//-------------------------------------------------------------------------------
//
//  Procedures for generating a variety of tesselated surfaces.
//
//-------------------------------------------------------------------------------

//
// Procedure that sub-divides a triangular segment of a sphere
// into a mesh of triangles that is projected onto the sphere's surface.
//
proc SubDivideSphereTriangle(
  vector Center = 0,            // Center of sphere
  float Radius = 1,
  float Level = 2,
  vector P1,
  vector P2,
  vector P3)
  {
  vector MP12, MP13, MP23

  if Level > 0
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
    Level -= 1
    SubDivideSphereTriangle(Center, Radius, Level, P1, MP12, MP13)
    SubDivideSphereTriangle(Center, Radius, Level, MP12, P2, MP23)
    SubDivideSphereTriangle(Center, Radius, Level, MP13, MP23, P3)
    //
    // Comment out this line, and see what you get... ;^)
    //
    SubDivideSphereTriangle(Center, Radius, Level, MP12, MP23, MP13)
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
// TessSphere1
// Make a sphere or hemisphere by sweeping a number of triangles,
// with common vertices at the poles, around the equator of the sphere.
// The sphere fills out as the triangles are sub-divided.
//
proc TessSphere1(
  vector Center = 0,            // Center of sphere
  float Radius = 1,             // Radius of sphere
  float Level = 2,              // Recursion level for sub-division
  float NumberOfSegments = 6,   // Must be at least 3
  float HemisphereFlags)        // 1 = upper, 2 = lower, 3 = both
  {
  float th = 0, Angle = 360 / NumberOfSegments

  while th < 359
    {
    if HemisphereFlags == 1 || HemisphereFlags == 3
      {
      SubDivideSphereTriangle(Center, Radius, Level,
        <0,Radius,0>,
        rotate(<0,0,Radius>,<0,th,0>),
        rotate(<0,0,Radius>,<0,th+Angle,0>))
      }
    if HemisphereFlags == 2 || HemisphereFlags == 3
      {
      SubDivideSphereTriangle(Center, Radius, Level,
        <0,-Radius,0>,
        rotate(<0,0,Radius>,<180,th,0>),
        rotate(<0,0,Radius>,<180,th+Angle,0>))
      }
    th += Angle
    }
  }


//
// TessSphere2
// This one recursively sub-divides the faces of a tetrahedron.
//
proc TessSphere2(
  vector Center = 0,            // Center of sphere
  float Radius = 1,             // Radius of sphere
  float Level = 2)              // Recursion level for sub-division
  {
  vector P1, P2, P3, P4

  P1 = rotate(<0, 1, 0>, <120, -120, 0>) * Radius
  P2 = rotate(<0, 1, 0>, <120, 120, 0>) * Radius
  P3 = rotate(<0, 1, 0>, <120, 0, 0>) * Radius
  P4 = <0, 1, 0> * Radius
  SubDivideSphereTriangle(P1, P2, P3, Level, Center, Radius)
  SubDivideSphereTriangle(P1, P2, P4, Level, Center, Radius)
  SubDivideSphereTriangle(P1, P3, P4, Level, Center, Radius)
  SubDivideSphereTriangle(P2, P3, P4, Level, Center, Radius)
  }