#include "Sketch.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <gp.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec2d.hxx>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
// Tolerancia de las comparaciones (mm). Mucho menor que el ajuste del mouse (1 mm).
const double kTolerance = 1e-6;
const double kPi = 3.14159265358979323846;

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

// Datos de un arco: centro, radio, ángulo inicial y barrido (positivo antihorario).
struct Arc
{
  gp_Pnt2d center;
  double radius;
  double start;
  double sweep;
};

Arc arcOf(const gp_Pnt2d& a, const gp_Pnt2d& b, double bulge)
{
  const double length = a.Distance(b);
  const gp_Pnt2d middle((a.X() + b.X()) / 2.0, (a.Y() + b.Y()) / 2.0);
  // Normal a la izquierda del lado, de largo 1.
  const gp_Vec2d left(-(b.Y() - a.Y()) / length, (b.X() - a.X()) / length);
  const gp_Pnt2d center = middle.Translated(left * (length * (1.0 - bulge * bulge) / (4.0 * bulge)));
  return {center, length * (1.0 + bulge * bulge) / (4.0 * std::abs(bulge)),
          std::atan2(a.Y() - center.Y(), a.X() - center.X()), 4.0 * std::atan(bulge)};
}

// El contorno con los arcos aproximados por tramos rectos (cada 5 grados como máximo),
// para validar y orientar con las mismas cuentas que un polígono.
std::vector<gp_Pnt2d> sampled(const std::vector<gp_Pnt2d>& points, const std::vector<double>& bulges)
{
  std::vector<gp_Pnt2d> result;
  for (size_t i = 0; i < points.size(); ++i)
  {
    result.push_back(points[i]);
    if (bulges[i] == 0.0)
      continue;
    const Arc arc = arcOf(points[i], points[(i + 1) % points.size()], bulges[i]);
    const int steps = std::max(4, int(std::ceil(std::abs(arc.sweep) / (kPi / 36.0))));
    for (int k = 1; k < steps; ++k)
    {
      const double angle = arc.start + arc.sweep * k / steps;
      result.emplace_back(arc.center.X() + arc.radius * std::cos(angle),
                          arc.center.Y() + arc.radius * std::sin(angle));
    }
  }
  return result;
}

// Caja que encierra el contorno: los vértices más los puntos extremos de cada arco.
void bounds(const std::vector<gp_Pnt2d>& points, const std::vector<double>& bulges,
            double* minX, double* maxX, double* minY, double* maxY)
{
  *minX = *minY = std::numeric_limits<double>::max();
  *maxX = *maxY = std::numeric_limits<double>::lowest();
  auto add = [&](double x, double y) {
    *minX = std::min(*minX, x);
    *maxX = std::max(*maxX, x);
    *minY = std::min(*minY, y);
    *maxY = std::max(*maxY, y);
  };
  for (size_t i = 0; i < points.size(); ++i)
  {
    add(points[i].X(), points[i].Y());
    if (bulges[i] == 0.0)
      continue;
    const Arc arc = arcOf(points[i], points[(i + 1) % points.size()], bulges[i]);
    for (int k = 0; k < 4; ++k)
    {
      // ¿El ángulo k * 90° cae dentro del barrido del arco?
      const double angle = k * kPi / 2.0;
      double offset = arc.sweep > 0 ? angle - arc.start : arc.start - angle;
      offset = std::fmod(offset, 2.0 * kPi);
      if (offset < 0)
        offset += 2.0 * kPi;
      if (offset <= std::abs(arc.sweep))
        add(arc.center.X() + arc.radius * std::cos(angle), arc.center.Y() + arc.radius * std::sin(angle));
    }
  }
}

bool isValidPolygon(const std::vector<gp_Pnt2d>& points);
} // namespace

void Sketch::SetRectangle(double width, double height)
{
  myPoints = {gp_Pnt2d(0, 0), gp_Pnt2d(width, 0), gp_Pnt2d(width, height), gp_Pnt2d(0, height)};
  myBulges.assign(4, 0.0);
}

bool Sketch::SetPolygon(const std::vector<gp_Pnt2d>& points)
{
  return SetProfile(points, std::vector<double>(points.size(), 0.0));
}

bool Sketch::SetProfile(const std::vector<gp_Pnt2d>& points, const std::vector<double>& bulges)
{
  if (!IsValidProfile(points, bulges))
    return false;
  myPoints = points;
  myBulges = bulges;
  return true;
}

bool Sketch::SetCircle(const gp_Pnt2d& center, double radius)
{
  if (!(radius > kTolerance) || !std::isfinite(radius))
    return false;
  // Dos medias vueltas en sentido antihorario: de izquierda a derecha por abajo y de vuelta por arriba.
  return SetProfile({gp_Pnt2d(center.X() - radius, center.Y()), gp_Pnt2d(center.X() + radius, center.Y())},
                    {1.0, 1.0});
}

bool Sketch::HasArcs() const
{
  return std::any_of(myBulges.begin(), myBulges.end(), [](double bulge) { return bulge != 0.0; });
}

bool Sketch::IsRectangle() const
{
  if (myPoints.size() != 4 || HasArcs())
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

bool Sketch::IsCircle() const
{
  return myPoints.size() == 2 && std::abs(std::abs(myBulges[0]) - 1.0) <= 1e-9
      && std::abs(myBulges[1] - myBulges[0]) <= 1e-9;
}

gp_Pnt2d Sketch::CircleCenter() const
{
  return gp_Pnt2d((myPoints[0].X() + myPoints[1].X()) / 2.0, (myPoints[0].Y() + myPoints[1].Y()) / 2.0);
}

double Sketch::CircleRadius() const
{
  return myPoints[0].Distance(myPoints[1]) / 2.0;
}

double Sketch::Width() const
{
  if (myPoints.empty())
    return 0.0;
  double minX, maxX, minY, maxY;
  bounds(myPoints, myBulges, &minX, &maxX, &minY, &maxY);
  return maxX - minX;
}

double Sketch::Height() const
{
  if (myPoints.empty())
    return 0.0;
  double minX, maxX, minY, maxY;
  bounds(myPoints, myBulges, &minX, &maxX, &minY, &maxY);
  return maxY - minY;
}

TopoDS_Wire Sketch::Wire() const
{
  BRepBuilderAPI_MakeWire wire;
  for (size_t i = 0; i < myPoints.size(); ++i)
  {
    const gp_Pnt2d& a = myPoints[i];
    const gp_Pnt2d& b = myPoints[(i + 1) % myPoints.size()];
    if (myBulges[i] == 0.0)
    {
      wire.Add(BRepBuilderAPI_MakeEdge(to3d(a), to3d(b)).Edge());
    }
    else
    {
      const Handle(Geom_TrimmedCurve) arc =
          GC_MakeArcOfCircle(to3d(a), to3d(ArcMiddle(a, b, myBulges[i])), to3d(b)).Value();
      wire.Add(BRepBuilderAPI_MakeEdge(arc).Edge());
    }
  }
  return wire.Wire();
}

TopoDS_Face Sketch::Face() const
{
  return BRepBuilderAPI_MakeFace(Wire(), Standard_True).Face();
}

std::vector<Sketch::DimensionedEdge> Sketch::DimensionedEdges() const
{
  if (IsCircle())
  {
    const gp_Circ circle(gp_Ax2(to3d(CircleCenter()), gp::DZ()), CircleRadius());
    return {{0, DimensionKind::Diameter, TopoDS_Edge(), circle, myPoints[0]}};
  }

  const size_t count = myPoints.size();
  const bool clockwise = signedArea2(sampled(myPoints, myBulges)) < 0.0;
  const bool rectangle = IsRectangle();

  std::vector<DimensionedEdge> edges;
  for (size_t i = 0; i < count; ++i)
  {
    // En sentido horario se recorre el lado i al revés, del vértice i+1 al i.
    const gp_Pnt2d& a = clockwise ? myPoints[(i + 1) % count] : myPoints[i];
    const gp_Pnt2d& b = clockwise ? myPoints[i] : myPoints[(i + 1) % count];
    if (myBulges[i] != 0.0)
    {
      const Arc arc = arcOf(myPoints[i], myPoints[(i + 1) % count], myBulges[i]);
      const gp_Circ circle(gp_Ax2(to3d(arc.center), gp::DZ()), arc.radius);
      edges.push_back({i, DimensionKind::Radius, TopoDS_Edge(), circle,
                       ArcMiddle(myPoints[i], myPoints[(i + 1) % count], myBulges[i])});
      continue;
    }
    if (rectangle)
    {
      // Recorrido antihorario: el lado inferior va hacia +X y el derecho hacia +Y.
      const bool bottom = b.X() - a.X() > kTolerance;
      const bool right = b.Y() - a.Y() > kTolerance;
      if (!bottom && !right)
        continue;
    }
    edges.push_back({i, DimensionKind::Length, BRepBuilderAPI_MakeEdge(to3d(a), to3d(b)).Edge(), gp_Circ(),
                     gp_Pnt2d()});
  }
  return edges;
}

Sketch::DimensionKind Sketch::KindOfDimension(size_t index) const
{
  if (IsCircle())
    return DimensionKind::Diameter;
  return IsArc(index) ? DimensionKind::Radius : DimensionKind::Length;
}

double Sketch::DimensionValue(size_t index) const
{
  switch (KindOfDimension(index))
  {
    case DimensionKind::Diameter:
      return 2.0 * CircleRadius();
    case DimensionKind::Radius:
      return ArcRadius(index);
    default:
      return EdgeLength(index);
  }
}

bool Sketch::SetDimensionValue(size_t index, double value)
{
  if (index >= myPoints.size())
    return false;
  switch (KindOfDimension(index))
  {
    case DimensionKind::Diameter:
      return SetCircleDiameter(value);
    case DimensionKind::Radius:
      return SetArcRadius(index, value);
    default:
      return SetEdgeLength(index, value);
  }
}

double Sketch::EdgeLength(size_t index) const
{
  return myPoints[index].Distance(myPoints[(index + 1) % myPoints.size()]);
}

bool Sketch::SetEdgeLength(size_t index, double length)
{
  const size_t count = myPoints.size();
  if (index >= count || IsArc(index) || !(length > kTolerance) || !std::isfinite(length))
    return false;

  const gp_Pnt2d& a = myPoints[index];
  const gp_Pnt2d& b = myPoints[(index + 1) % count];
  const gp_Vec2d direction(a, b);
  const double current = direction.Magnitude();
  const gp_Vec2d shift = direction.Normalized() * (length - current);

  std::vector<gp_Pnt2d> points = myPoints;
  size_t k = (index + 1) % count;
  points[k].Translate(shift);
  // Arrastra los vértices siguientes mientras el lado que llega a ellos sea perpendicular
  // (en un arco, la cuerda). Nunca llega al vértice inicial, que queda fijo.
  for (size_t moved = 1; moved + 1 < count; ++moved)
  {
    const size_t next = (k + 1) % count;
    const gp_Vec2d side(myPoints[k], myPoints[next]);
    if (std::abs(side.Normalized().Dot(direction.Normalized())) > 1e-9)
      break;
    points[next].Translate(shift);
    k = next;
  }

  return SetProfile(points, myBulges);
}

double Sketch::ArcRadius(size_t index) const
{
  const double bulge = std::abs(myBulges[index]);
  return EdgeLength(index) * (1.0 + bulge * bulge) / (4.0 * bulge);
}

bool Sketch::SetArcRadius(size_t index, double radius)
{
  if (index >= myPoints.size() || !IsArc(index) || !std::isfinite(radius))
    return false;
  const double chord = EdgeLength(index);
  if (radius < chord / 2.0 - kTolerance)
    return false;
  // Con radio r y cuerda c: |bulge| = (2r ∓ √(4r² − c²)) / c, con − si el arco es menor que
  // media vuelta y + si es mayor.
  const double root = std::sqrt(std::max(0.0, 4.0 * radius * radius - chord * chord));
  const double current = myBulges[index];
  const double magnitude = std::abs(current) <= 1.0 ? (2.0 * radius - root) / chord : (2.0 * radius + root) / chord;
  std::vector<double> bulges = myBulges;
  bulges[index] = std::copysign(magnitude, current);
  return SetProfile(myPoints, bulges);
}

bool Sketch::SetCircleDiameter(double diameter)
{
  if (!IsCircle())
    return false;
  return SetCircle(CircleCenter(), diameter / 2.0);
}

bool Sketch::IsValidProfile(const std::vector<gp_Pnt2d>& points)
{
  return IsValidProfile(points, std::vector<double>(points.size(), 0.0));
}

bool Sketch::IsValidProfile(const std::vector<gp_Pnt2d>& points, const std::vector<double>& bulges)
{
  const size_t count = points.size();
  if (count < 2 || bulges.size() != count)
    return false;
  for (size_t i = 0; i < count; ++i)
  {
    if (!std::isfinite(points[i].X()) || !std::isfinite(points[i].Y()) || !std::isfinite(bulges[i])
        || std::abs(bulges[i]) > 1e3)
      return false;
    if (points[i].Distance(points[(i + 1) % count]) <= kTolerance)
      return false;
  }
  return isValidPolygon(sampled(points, bulges));
}

gp_Pnt2d Sketch::ArcMiddle(const gp_Pnt2d& a, const gp_Pnt2d& b, double bulge)
{
  // La flecha del arco (distancia del punto medio a la cuerda) es bulge · cuerda / 2, hacia la derecha.
  const gp_Vec2d right(b.Y() - a.Y(), -(b.X() - a.X()));
  return gp_Pnt2d((a.X() + b.X()) / 2.0, (a.Y() + b.Y()) / 2.0).Translated(right * (bulge / 2.0));
}

std::optional<double> Sketch::BulgeThrough(const gp_Pnt2d& a, const gp_Pnt2d& b, const gp_Pnt2d& through)
{
  const double length = a.Distance(b);
  if (length <= kTolerance || through.Distance(a) <= kTolerance || through.Distance(b) <= kTolerance)
    return std::nullopt;
  const double turn = cross(a, b, through);
  if (std::abs(turn) <= kTolerance * length)
    return std::nullopt; // alineados: sería una línea

  // Centro de la circunferencia que pasa por los tres puntos.
  const double ax = a.X(), ay = a.Y(), bx = b.X(), by = b.Y(), px = through.X(), py = through.Y();
  const double d = 2.0 * (ax * (by - py) + bx * (py - ay) + px * (ay - by));
  const double a2 = ax * ax + ay * ay, b2 = bx * bx + by * by, p2 = px * px + py * py;
  const gp_Pnt2d center((a2 * (by - py) + b2 * (py - ay) + p2 * (ay - by)) / d,
                        (a2 * (px - bx) + b2 * (ax - px) + p2 * (bx - ax)) / d);
  const double radius = center.Distance(a);

  // Distancia con signo del centro a la cuerda, positiva a la izquierda.
  const gp_Pnt2d middle((ax + bx) / 2.0, (ay + by) / 2.0);
  const gp_Vec2d left(-(by - ay) / length, (bx - ax) / length);
  const double offset = gp_Vec2d(middle, center).Dot(left);
  // El arco va del lado del punto: a la derecha (giro negativo) el bulge es positivo.
  return turn < 0 ? 2.0 * (radius - offset) / length : -2.0 * (radius + offset) / length;
}

namespace
{
// Un polígono válido tiene al menos 3 vértices, lados de largo mayor que cero, área y
// ningún lado que se cruce con otro.
bool isValidPolygon(const std::vector<gp_Pnt2d>& points)
{
  const size_t count = points.size();
  if (count < 3)
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
} // namespace
