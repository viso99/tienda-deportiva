#pragma once

#include <TopoDS_Shape.hxx>

class Sketch;

// Extruye el perfil del boceto en +Z la distancia indicada (mm) y devuelve el sólido.
TopoDS_Shape ExtrudeSketch(const Sketch& sketch, double depth);
