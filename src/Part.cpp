#include "Part.h"

#include "Extrude.h"

#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <gp.hxx>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <cmath>

namespace
{
double volumeOf(const TopoDS_Shape& shape)
{
  GProp_GProps props;
  BRepGProp::VolumeProperties(shape, props);
  return props.Mass();
}

// Sólido que se resta o se suma: la cara del boceto llevada a su plano y extruida.
TopoDS_Shape featureTool(const Feature& feature, const TopoDS_Shape& part)
{
  gp_Trsf toPlane;
  toPlane.SetDisplacement(gp_Ax3(gp::XOY()), feature.plane);
  const TopoDS_Shape face = BRepBuilderAPI_Transform(feature.sketch.Face(), toPlane, Standard_True).Shape();

  const gp_Vec normal(feature.plane.Direction());
  double length = feature.depth;
  if (feature.kind == Feature::Kind::Cut && feature.throughAll)
  {
    // Más largo que cualquier recorrido dentro de la pieza.
    Bnd_Box box;
    BRepBndLib::Add(part, box);
    length = std::sqrt(box.SquareExtent()) + 1.0;
  }
  const gp_Vec direction = feature.kind == Feature::Kind::Cut ? -normal * length : normal * length;
  return BRepPrimAPI_MakePrism(face, direction).Shape();
}

// Aplica una operación. Devuelve una forma nula si falla o si no cambia la pieza.
TopoDS_Shape applyFeature(const TopoDS_Shape& part, const Feature& feature)
{
  if (!feature.sketch.HasProfile() || (!feature.throughAll && !(feature.depth > 0.0)))
    return TopoDS_Shape();

  const TopoDS_Shape tool = featureTool(feature, part);
  TopoDS_Shape result;
  if (feature.kind == Feature::Kind::Cut)
  {
    BRepAlgoAPI_Cut cut(part, tool);
    if (!cut.IsDone() || cut.HasErrors())
      return TopoDS_Shape();
    result = cut.Shape();
  }
  else
  {
    BRepAlgoAPI_Fuse fuse(part, tool);
    if (!fuse.IsDone() || fuse.HasErrors())
      return TopoDS_Shape();
    result = fuse.Shape();
  }

  // Junta las caras coplanares que dejan las operaciones booleanas (por ejemplo, la tapa
  // de un saliente que queda al ras), para que la pieza no muestre aristas de más.
  ShapeUpgrade_UnifySameDomain unify(result, Standard_True, Standard_True, Standard_True);
  unify.Build();
  result = unify.Shape();

  if (result.IsNull() || !BRepCheck_Analyzer(result).IsValid())
    return TopoDS_Shape();
  const double before = volumeOf(part);
  const double after = volumeOf(result);
  const double tolerance = 1e-6 * std::max(1.0, before);
  const bool changed = feature.kind == Feature::Kind::Cut ? after < before - tolerance
                                                          : after > before + tolerance;
  if (!changed || after <= tolerance)
    return TopoDS_Shape();
  return result;
}
} // namespace

TopoDS_Shape BuildPart(const Part& part, std::vector<size_t>* failedFeatures)
{
  if (!part.extrudeDepth || !part.sketch.HasProfile())
    return TopoDS_Shape();

  TopoDS_Shape shape = ExtrudeSketch(part.sketch, *part.extrudeDepth);
  for (size_t i = 0; i < part.features.size(); ++i)
  {
    const TopoDS_Shape next = applyFeature(shape, part.features[i]);
    if (next.IsNull())
    {
      if (failedFeatures)
        failedFeatures->push_back(i);
      continue;
    }
    shape = next;
  }
  return shape;
}

std::optional<gp_Ax3> SketchPlaneFromFace(const TopoDS_Face& face)
{
  const BRepAdaptor_Surface surface(face);
  if (surface.GetType() != GeomAbs_Plane)
    return std::nullopt;

  const gp_Pln plane = surface.Plane();
  gp_Dir normal = plane.Axis().Direction();
  if (face.Orientation() == TopAbs_REVERSED)
    normal.Reverse();

  // Punto del plano más cercano al origen global.
  const gp_Vec toOrigin(plane.Location(), gp::Origin());
  const gp_Pnt origin = gp::Origin().Translated(-gp_Vec(normal) * toOrigin.Dot(gp_Vec(normal)));

  const gp_Dir xDirection = std::abs(normal.Z()) > 0.999 ? gp::DX() : gp_Dir(gp::DZ().Crossed(normal));
  return gp_Ax3(origin, normal, xDirection);
}
