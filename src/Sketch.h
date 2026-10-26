#pragma once

#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Pnt2d.hxx>

#include <vector>

// Boceto 2D sobre el plano XY. Contiene un solo perfil: un contorno cerrado de
// líneas rectas (polígono), dado por sus vértices en orden. Medidas en milímetros.
class Sketch
{
public:
  // Rectángulo con una esquina en el origen (el del diálogo "Rectángulo con medidas").
  void SetRectangle(double width, double height);
  // Contorno cerrado: el último vértice se une con el primero. Devuelve false
  // (y no cambia el boceto) si el contorno no es válido, ver IsValidProfile.
  bool SetPolygon(const std::vector<gp_Pnt2d>& points);

  bool HasProfile() const { return !myPoints.empty(); }
  const std::vector<gp_Pnt2d>& Points() const { return myPoints; }

  // True si el perfil es un rectángulo con lados paralelos a los ejes.
  bool IsRectangle() const;
  // Medidas de la caja que encierra el perfil (en un rectángulo, ancho y alto).
  double Width() const;
  double Height() const;

  // Contorno cerrado del perfil.
  TopoDS_Wire Wire() const;
  // Cara plana delimitada por el contorno (la que se va a extruir).
  TopoDS_Face Face() const;
  // Lado que lleva cota: su número (el lado i va del vértice i al i+1) y la arista,
  // recorrida en sentido antihorario (el exterior queda a la derecha).
  struct DimensionedEdge
  {
    size_t index;
    TopoDS_Edge edge;
  };
  // En un rectángulo, el inferior (ancho) y el derecho (alto); si no, todos los lados.
  std::vector<DimensionedEdge> DimensionedEdges() const;

  double EdgeLength(size_t index) const;
  // Cambia el largo del lado `index` dejando fijo su vértice inicial. El final se mueve
  // a lo largo del lado y arrastra a los vértices siguientes unidos por lados
  // perpendiculares, para que esos lados no se inclinen (así una L sigue siendo una L).
  // El primer lado que no es perpendicular absorbe el cambio. Devuelve false (y no
  // cambia el boceto) si el largo no es positivo o el contorno resultante no es válido.
  bool SetEdgeLength(size_t index, double length);

  // Un contorno válido tiene al menos 3 vértices, lados de largo mayor que cero,
  // área mayor que cero y ningún lado que se cruce con otro.
  static bool IsValidProfile(const std::vector<gp_Pnt2d>& points);

private:
  std::vector<gp_Pnt2d> myPoints;
};
