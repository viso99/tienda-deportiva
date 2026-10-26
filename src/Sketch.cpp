#include "Sketch.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <gp_Pnt.hxx>

void Sketch::SetRectangle(double width, double height)
{
  myWidth = width;
  myHeight = height;
  myHasProfile = true;
}

TopoDS_Wire Sketch::Wire() const
{
  BRepBuilderAPI_MakePolygon polygon(gp_Pnt(0, 0, 0),
                                     gp_Pnt(myWidth, 0, 0),
                                     gp_Pnt(myWidth, myHeight, 0),
                                     gp_Pnt(0, myHeight, 0),
                                     Standard_True);
  return polygon.Wire();
}

TopoDS_Face Sketch::Face() const
{
  return BRepBuilderAPI_MakeFace(Wire(), Standard_True).Face();
}

TopoDS_Edge Sketch::WidthEdge() const
{
  return BRepBuilderAPI_MakeEdge(gp_Pnt(0, 0, 0), gp_Pnt(myWidth, 0, 0)).Edge();
}

TopoDS_Edge Sketch::HeightEdge() const
{
  return BRepBuilderAPI_MakeEdge(gp_Pnt(myWidth, 0, 0), gp_Pnt(myWidth, myHeight, 0)).Edge();
}
