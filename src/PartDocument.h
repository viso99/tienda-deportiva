#pragma once

#include "Sketch.h"

#include <QString>
#include <TopoDS_Shape.hxx>

#include <optional>

// Pieza paramétrica: el boceto y, si ya se extruyó, la distancia de extrusión (mm).
struct Part
{
  Sketch sketch;
  std::optional<double> extrudeDepth;
};

// Formato propio (.cadp): JSON con los parámetros de la pieza, no la geometría.
// Al abrirlo, la geometría se reconstruye a partir de los parámetros.
bool SavePart(const Part& part, const QString& path, QString* error);
bool LoadPart(const QString& path, Part* part, QString* error);

// Exporta el sólido a STEP (AP214), para abrirlo en otros programas CAD.
bool ExportStep(const TopoDS_Shape& shape, const QString& path, QString* error);
