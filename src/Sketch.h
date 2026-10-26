#pragma once

#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>

// Boceto 2D sobre el plano XY. En la v1 contiene un solo perfil: un rectángulo
// con una esquina en el origen. Las medidas están en milímetros.
class Sketch
{
public:
  void SetRectangle(double width, double height);
  bool HasProfile() const { return myHasProfile; }

  double Width() const { return myWidth; }
  double Height() const { return myHeight; }

  // Contorno cerrado del perfil.
  TopoDS_Wire Wire() const;
  // Cara plana delimitada por el contorno (la que se va a extruir).
  TopoDS_Face Face() const;
  // Lados que llevan las cotas: el inferior (ancho) y el derecho (alto).
  TopoDS_Edge WidthEdge() const;
  TopoDS_Edge HeightEdge() const;

private:
  bool myHasProfile = false;
  double myWidth = 0.0;
  double myHeight = 0.0;
};
