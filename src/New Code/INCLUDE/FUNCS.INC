//-----------------------------------------------------------------
//
//   funcs.inc - A collection of useful functions that can be used
//     in expressions. Organized by area of most usefulness.   
//
//-----------------------------------------------------------------

//
//  Bump functions...
//  All of these functions are most useful for the "bump"
//  modifier in a texture {} block.
//  All return a vector.
//

//
// Dents1
//
rt_proc Dents1(
  vector Pt,          // Point in 3D space.
  float Scale = 1,    // Scaling factor for return result.
  float Power = 3)    // Sets the "tightness" of the dents. Higher = tighter 
  {
  float n
  vector V = <(n=turb(Pt*1.5+10,3)*1.5)*|n|^Power,
              (n=turb(Pt,3)*1.5)*|n|^Power,
              (n=turb(Pt*2.5-5,3)*1.5)*|n|^Power>
  return <clamp(V[0],-1,1),clamp(V[1],-1,1),clamp(V[2],-1,1)>*Scale
  }


//
// Lookup functions that return a float in the range of 0 -> 1
//

//
// Simple gradient functions.
//
rt_proc Gradient(float n)
  {
  return n - floor(n)
  }

rt_proc Triangle_Wave(float n)
  {
  return |(n - floor(n)) * 2 - 1|
  }

//
// Leopard - POVRay's "leopard" function.
//
rt_proc Leopard(vector Pt)
  {
  return (sin(Pt[0])^2 + sin(Pt[1])^2 + sin(Pt[2])^2) / 3
  }