#include "Sketch.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <gp_Pnt.hxx>

#include <algorithm>
#include <cmath>

namespace
{
// Tolerancia de las comparaciones (mm). Mucho menor que el ajuste del mouse (1 mm).
const double kTolerance = 1e-6;

gp_Pnt to3d(const gp_Pnt2d& point)
{
  return gp_Pnt(point.X(), point.Y(), 0.0);
}

// Doble del área con signo: positiva si el recorrido es antihorario.
double signedArea2(const std::vector<gp_Pnt2d>& points)
{
  double area = 0.0;
  for (size_t i = 0; i < points.size(); ++i)
  {
    const gp_Pnt2d& a = points[i];
    const gp_Pnt2d& b = points[(i + 1) % points.size()];
    area += a.X() * b.Y() - b.X() * a.Y();
  }
  return area;
}

double cross(const gp_Pnt2d& o, const gp_Pnt2d& a, const gp_Pnt2d& b)
{
  return (a.X() - o.X()) * (b.Y() - o.Y()) - (a.Y() - o.Y()) * (b.X() - o.X());
}

bool onSegment(const gp_Pnt2d& p, const gp_Pnt2d& a, const gp_Pnt2d& b)
{
  return std::min(a.X(), b.X()) - kTolerance <= p.X() && p.X() <= std::max(a.X(), b.X()) + kTolerance
      && std::min(a.Y(), b.Y()) - kTolerance <= p.Y() && p.Y() <= std::max(a.Y(), b.Y()) + kTolerance;
}

// True si los segmentos ab y cd se tocan en algún punto (incluye extremos y solapes).
bool segmentsTouch(const gp_Pnt2d& a, const gp_Pnt2d& b, const gp_Pnt2d& c, const gp_Pnt2d& d)
{
  const double d1 = cross(c, d, a);
  const double d2 = cross(c, d, b);
  const double d3 = cross(a, b, c);
  const double d4 = cross(a, b, d);
  if (((d1 > kTolerance && d2 < -kTolerance) || (d1 < -kTolerance && d2 > kTolerance))
      && ((d3 > kTolerance && d4 < -kTolerance) || (d3 < -kTolerance && d4 > kTolerance)))
    return true;
  return (std::abs(d1) <= kTolerance && onSegment(a, c, d))
      || (std::abs(d2) <= kTolerance && onSegment(b, c, d))
      || (std::abs(d3) <= kTolerance && onSegment(c, a, b))
      || (std::abs(d4) <= kTolerance && onSegment(d, a, b));
}
} // namespace

void Sketch::SetRectangle(double width, double height)
{
  myPoints = {gp_Pnt2d(0, 0), gp_Pnt2d(width, 0), gp_Pnt2d(width, height), gp_Pnt2d(0, height)};
}

bool Sketch::SetPolygon(const std::vector<gp_Pnt2d>& points)
{
  if (!IsValidProfile(points))
    return false;
  myPoints = points;
  return true;
}

bool Sketch::IsRectangle() const
{
  if (myPoints.size() != 4)
    return false;
  for (size_t i = 0; i < 4; ++i)
  {
    const gp_Pnt2d& a = myPoints[i];
    const gp_Pnt2d& b = myPoints[(i + 1) % 4];
    const bool horizontal = std::abs(a.Y() - b.Y()) <= kTolerance;
    const bool vertical = std::abs(a.X() - b.X()) <= kTolerance;
    if (horizontal == vertical)
      return false;
  }
  return true;
}

double Sketch::Width() const
{
  if (myPoints.empty())
    return 0.0;
  const auto [minIt, maxIt] = std::minmax_element(
      myPoints.begin(), myPoints.end(),
      [](const gp_Pnt2d& a, const gp_Pnt2d& b) { return a.X() < b.X(); });
  return maxIt->X() - minIt->X();
}

double Sketch::Height() const
{
  if (myPoints.empty())
    return 0.0;
  const auto [minIt, maxIt] = std::minmax_element(
      myPoints.begin(), myPoints.end(),
      [](const gp_Pnt2d& a, const gp_Pnt2d& b) { return a.Y() < b.Y(); });
  return maxIt->Y() - minIt->Y();
}

TopoDS_Wire Sketch::Wire() const
{
  BRepBuilderAPI_MakePolygon polygon;
  for (const gp_Pnt2d& point : myPoints)
    polygon.Add(to3d(point));
  polygon.Close();
  return polygon.Wire();
}

TopoDS_Face Sketch::Face() const
{
  return BRepBuilderAPI_MakeFace(Wire(), Standard_True).Face();
}

std::vector<TopoDS_Edge> Sketch::DimensionedEdges() const
{
  std::vector<gp_Pnt2d> points = myPoints;
  if (signedArea2(points) < 0.0)
    std::reverse(points.begin(), points.end());

  std::vector<TopoDS_Edge> edges;
  for (size_t i = 0; i < points.size(); ++i)
  {
    const gp_Pnt2d& a = points[i];
    const gp_Pnt2d& b = points[(i + 1) % points.size()];
    if (IsRectangle())
    {
      // Recorrido antihorario: el lado inferior va hacia +X y el derecho hacia +Y.
      const bool bottom = b.X() - a.X() > kTolerance;
      const bool right = b.Y() - a.Y() > kTolerance;
      if (!bottom && !right)
        continue;
    }
    edges.push_back(BRepBuilderAPI_MakeEdge(to3d(a), to3d(b)).Edge());
  }
  return edges;
}

bool Sketch::IsValidProfile(const std::vector<gp_Pnt2d>& points)
{
  const size_t count = points.size();
  if (count < 3)
    return false;
  for (const gp_Pnt2d& point : points)
    if (!std::isfinite(point.X()) || !std::isfinite(point.Y()))
      return false;
  for (size_t i = 0; i < count; ++i)
    if (points[i].Distance(points[(i + 1) % count]) <= kTolerance)
      return false;
  if (std::abs(signedArea2(points)) <= kTolerance)
    return false;

  // Cada lado solo puede tocar a sus dos vecinos, y solo en el vértice que comparten.
  for (size_t i = 0; i < count; ++i)
  {
    const gp_Pnt2d& a = points[i];
    const gp_Pnt2d& b = points[(i + 1) % count];
    for (size_t j = i + 1; j < count; ++j)
    {
      const gp_Pnt2d& c = points[j];
      const gp_Pnt2d& d = points[(j + 1) % count];
      const bool next = j == i + 1;
      const bool previous = (j + 1) % count == i;
      if (next || previous)
      {
        // Vecinos: no pueden volver sobre sí mismos (el vértice no compartido no puede caer en el otro lado).
        const gp_Pnt2d& far1 = next ? a : b;
        const gp_Pnt2d& far2 = next ? d : c;
        const gp_Pnt2d& shared = next ? b : a;
        if (std::abs(cross(shared, far1, far2)) <= kTolerance
            && (onSegment(far1, shared, far2) || onSegment(far2, shared, far1)))
          return false;
        continue;
      }
      if (segmentsTouch(a, b, c, d))
        return false;
    }
  }
  return true;
}
