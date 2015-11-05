///////////////////////////////////////////////////////////////////////////
//
//  A six shelf bookcase.
//
//  Size 52" wide, 12.25" deep, 84" high
//  Extents: <0, -12.25, 0>, <52, 0, 84>
//  Note: Small amounts of "sloppiness" have been added into the
//    assembly of the parts so as not to look too perfect.
//
///////////////////////////////////////////////////////////////////////////

// Building materials for the shelves

define _Side box <0, 0, 0>, <84, 12, 2>;

define _Shelf box <0, 0, 0>, <48, 12, 2>;

define _BackCover box <0, 0, 0>, <84, 52, 0.25>;


// The shelves

define Shelves object
{
  _Side 
  {
    scale <0.998, 0.999, 0.994>;
    rotate <0, 90, 0>;
    translate <2, -12.25, 0>;
  }
  _Side 
  {
    scale <0.998, 0.998, 0.996>;
    rotate <0, 90, 0>;
    translate <2+48+2.01, -12.25, 0>;
  }
  _Shelf 
  {
    scale <0.997, 0.999, 0.995>;
    translate <2.001, -12.252, 6.01>;
  }
  _Shelf 
  {
    scale <0.999, 0.999, 0.995>;
    translate <2.002, -12.256, 24.011>;
  }
  _Shelf 
  {
    scale <0.998, 0.997, 0.995>;
    translate <2.001, -12.256, 42.009>;
  }
  _Shelf 
  {
    scale <0.997, 0.997, 0.995>;
    translate <2.0015, -12.253, 54.01>;
  }
  _Shelf 
  {
    scale <0.998, 0.999, 0.995>;
    translate <2.001, -12.252, 66.011>;
  }
  _Shelf 
  {
    scale <0.998, 0.997, 0.995>;
    translate <2.002, -12.253, 78.01>;
  }
  _BackCover
  {
    rotate <0, 90, 90>;
  }
}
