#pragma once

#include "Sketch.h"

#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax3.hxx>

#include <optional>
#include <vector>

// Operación sobre la pieza: un boceto dibujado en un plano (por ejemplo, una cara)
// que se resta (agujero o corte) o se suma (saliente).
struct Feature
{
  enum class Kind
  {
    Cut,  // resta material hacia adentro de la pieza (contra la normal del plano)
    Boss, // suma material hacia afuera (a favor de la normal)
  };

  Kind kind = Kind::Cut;
  // Plano del boceto: origen, normal hacia afuera de la pieza y eje X. Se guarda el plano y
  // no la cara, así la operación no depende de cómo se numeren las caras al recalcular.
  gp_Ax3 plane;
  Sketch sketch;           // en coordenadas del plano (mm)
  double depth = 10.0;     // mm
  bool throughAll = false; // solo para cortes: atraviesa toda la pieza
};

// Pieza paramétrica: el boceto base, su distancia de extrusión (mm) y las operaciones
// que se aplican después, en orden.
struct Part
{
  Sketch sketch;
  std::optional<double> extrudeDepth;
  std::vector<Feature> features;
};

// Geometría de la pieza: extruye el boceto base y aplica las operaciones en orden.
// Una operación que no se puede calcular o que no cambia la pieza (por ejemplo, un corte
// que no la toca) se saltea; si failedFeatures no es nulo, recibe sus números.
// Devuelve una forma nula si la pieza no está extruida.
TopoDS_Shape BuildPart(const Part& part, std::vector<size_t>* failedFeatures = nullptr);

// Plano de boceto para dibujar sobre una cara plana, con la normal hacia afuera de la pieza.
// El origen es el punto del plano más cercano al origen global, y el eje X es el X global
// si la cara es horizontal (o una dirección horizontal si no), para que las coordenadas
// sean fáciles de leer. Devuelve nada si la cara no es plana.
std::optional<gp_Ax3> SketchPlaneFromFace(const TopoDS_Face& face);
