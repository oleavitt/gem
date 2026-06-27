//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#include "GemCtoSwift.h"
// raytrace.h declares a struct field named `I` (an inverse matrix). When this
// header is imported in a context where <complex.h> is in scope (e.g. the
// GemTests target via @testable import), `I` expands to the complex-imaginary
// macro and breaks the parse. The engine uses no C99 complex numbers, so drop
// the macro before including raytrace.h.
#undef I
#include "raytrace.h"
#include "scn20.h"
