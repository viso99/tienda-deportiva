#include "Extrude.h"

#include "Sketch.h"

#include <BRepPrimAPI_MakePrism.hxx>
#include <gp_Vec.hxx>

TopoDS_Shape ExtrudeSketch(const Sketch& sketch, double depth)
{
  return BRepPrimAPI_MakePrism(sketch.Face(), gp_Vec(0, 0, depth)).Shape();
}
