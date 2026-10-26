#pragma once

#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Circ.hxx>
#include <gp_Pnt2d.hxx>

#include <optional>
#include <vector>

// Boceto 2D sobre el plano XY. Contiene un solo perfil: un contorno cerrado dado por sus
// vértices en orden. Cada lado (del vértice i al i+1, y el último al primero) es una línea
// recta o un arco. Un círculo es un contorno de dos medias vueltas. Medidas en milímetros.
//
// Cada lado lleva un "bulge" (el mismo dato que usan los archivos DXF): tan(ángulo del arco / 4),
// con signo positivo si el arco gira en sentido antihorario. 0 es una línea recta, 1 o -1 una
// media vuelta. El arco de un bulge positivo queda a la derecha del lado, mirando del vértice
// inicial al final.
class Sketch
{
public:
  // Rectángulo con una esquina en el origen (el del diálogo "Rectángulo con medidas").
  void SetRectangle(double width, double height);
  // Contorno cerrado de líneas rectas: el último vértice se une con el primero. Devuelve
  // false (y no cambia el boceto) si el contorno no es válido, ver IsValidProfile.
  bool SetPolygon(const std::vector<gp_Pnt2d>& points);
  // Contorno cerrado con líneas y arcos: `bulges[i]` es el del lado i.
  bool SetProfile(const std::vector<gp_Pnt2d>& points, const std::vector<double>& bulges);
  // Círculo de centro y radio dados. Devuelve false si el radio no es positivo.
  bool SetCircle(const gp_Pnt2d& center, double radius);

  bool HasProfile() const { return !myPoints.empty(); }
  const std::vector<gp_Pnt2d>& Points() const { return myPoints; }
  const std::vector<double>& Bulges() const { return myBulges; }
  bool IsArc(size_t index) const { return myBulges[index] != 0.0; }
  bool HasArcs() const;

  // True si el perfil es un rectángulo con lados paralelos a los ejes.
  bool IsRectangle() const;
  // True si el perfil es un círculo; entonces vale el centro y el radio.
  bool IsCircle() const;
  gp_Pnt2d CircleCenter() const;
  double CircleRadius() const;
  // Medidas de la caja que encierra el perfil (en un rectángulo, ancho y alto).
  double Width() const;
  double Height() const;

  // Contorno cerrado del perfil.
  TopoDS_Wire Wire() const;
  // Cara plana delimitada por el contorno (la que se va a extruir).
  TopoDS_Face Face() const;

  // Qué mide una cota: el largo de un lado recto, el radio de un arco o el diámetro de un círculo.
  enum class DimensionKind
  {
    Length,
    Radius,
    Diameter,
  };
  // Lado que lleva cota: su número, qué mide, la arista recorrida en sentido antihorario
  // (el exterior queda a la derecha) y, en arcos y círculos, la circunferencia y el punto
  // medio del arco (donde se apoya la cota).
  struct DimensionedEdge
  {
    size_t index;
    DimensionKind kind;
    TopoDS_Edge edge;
    gp_Circ circle;
    gp_Pnt2d arcMiddle;
  };
  // En un rectángulo, el inferior (ancho) y el derecho (alto); en un círculo, una cota de
  // diámetro; si no, todos los lados (largo en los rectos, radio en los arcos).
  std::vector<DimensionedEdge> DimensionedEdges() const;
  DimensionKind KindOfDimension(size_t index) const;
  // El valor de la cota del lado `index` y cómo cambiarlo (ver SetEdgeLength, SetArcRadius
  // y SetCircleDiameter).
  double DimensionValue(size_t index) const;
  bool SetDimensionValue(size_t index, double value);

  // Largo de un lado recto (en un arco, la cuerda).
  double EdgeLength(size_t index) const;
  // Cambia el largo del lado recto `index` dejando fijo su vértice inicial. El final se mueve
  // a lo largo del lado y arrastra a los vértices siguientes unidos por lados
  // perpendiculares, para que esos lados no se inclinen (así una L sigue siendo una L).
  // El primer lado que no es perpendicular absorbe el cambio. Devuelve false (y no
  // cambia el boceto) si el lado es un arco, si el largo no es positivo o si el contorno
  // resultante no es válido.
  bool SetEdgeLength(size_t index, double length);
  double ArcRadius(size_t index) const;
  // Cambia el radio del arco `index` sin mover sus extremos: el arco se abre o se cierra,
  // y sigue siendo menor o mayor que media vuelta. El radio no puede ser menor que media
  // cuerda.
  bool SetArcRadius(size_t index, double radius);
  // Cambia el diámetro de un círculo sin mover su centro.
  bool SetCircleDiameter(double diameter);

  // Un contorno válido tiene al menos 2 vértices (3 si es todo de líneas), lados de largo
  // mayor que cero, área mayor que cero y ningún lado que se cruce con otro.
  static bool IsValidProfile(const std::vector<gp_Pnt2d>& points);
  static bool IsValidProfile(const std::vector<gp_Pnt2d>& points, const std::vector<double>& bulges);

  // Punto medio del arco que va de `a` a `b` con ese bulge (con bulge 0, el del segmento).
  static gp_Pnt2d ArcMiddle(const gp_Pnt2d& a, const gp_Pnt2d& b, double bulge);
  // Bulge del arco que va de `a` a `b` pasando por `through`. Vacío si los tres puntos
  // están alineados o se repiten.
  static std::optional<double> BulgeThrough(const gp_Pnt2d& a, const gp_Pnt2d& b, const gp_Pnt2d& through);

private:
  std::vector<gp_Pnt2d> myPoints;
  std::vector<double> myBulges; // uno por lado, del mismo tamaño que myPoints
};
