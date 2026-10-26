// Pruebas del núcleo: boceto, guardar/abrir piezas y exportar a STEP.
// Se corren con `ctest` después de compilar. Devuelve 0 si todo pasa.

#include "Extrude.h"
#include "Format.h"
#include "PartDocument.h"

#include <QFile>
#include <QTemporaryDir>

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <STEPControl_Reader.hxx>

#include <cmath>
#include <cstdio>

namespace
{
int failures = 0;

void check(bool condition, const char* what)
{
  if (!condition)
  {
    std::printf("FALLA: %s\n", what);
    ++failures;
  }
}

double volumeOf(const TopoDS_Shape& shape)
{
  GProp_GProps props;
  BRepGProp::VolumeProperties(shape, props);
  return props.Mass();
}

void writeText(const QString& path, const char* text)
{
  QFile file(path);
  check(file.open(QIODevice::WriteOnly), "se puede escribir el archivo de prueba");
  file.write(text);
}
} // namespace

int main()
{
  QTemporaryDir dir;
  check(dir.isValid(), "se crea la carpeta temporal");
  QString error;

  // Guardar y abrir conserva boceto y extrusión.
  Part part;
  part.sketch.SetRectangle(40.0, 20.0);
  part.extrudeDepth = 10.0;
  const QString partPath = dir.filePath("bloque.cadp");
  check(SavePart(part, partPath, &error), "guarda la pieza");

  Part loaded;
  check(LoadPart(partPath, &loaded, &error), "abre la pieza guardada");
  check(loaded.sketch.HasProfile(), "la pieza abierta tiene boceto");
  check(loaded.sketch.Width() == 40.0 && loaded.sketch.Height() == 20.0, "el boceto conserva sus medidas");
  check(loaded.extrudeDepth && *loaded.extrudeDepth == 10.0, "la extrusión conserva su distancia");

  // Un boceto sin extruir también se guarda y se abre.
  Part sketchOnly;
  sketchOnly.sketch.SetRectangle(5.5, 3.25);
  const QString sketchPath = dir.filePath("boceto.cadp");
  check(SavePart(sketchOnly, sketchPath, &error), "guarda un boceto sin extruir");
  Part loadedSketch;
  check(LoadPart(sketchPath, &loadedSketch, &error), "abre un boceto sin extruir");
  check(!loadedSketch.extrudeDepth, "el boceto abierto no tiene extrusión");

  // Archivos inválidos se rechazan sin tocar la pieza actual.
  const QString badPath = dir.filePath("mala.cadp");
  writeText(badPath, "esto no es JSON");
  Part untouched = part;
  check(!LoadPart(badPath, &untouched, &error), "rechaza un archivo que no es JSON");
  check(untouched.extrudeDepth && *untouched.extrudeDepth == 10.0, "un archivo inválido no cambia la pieza");

  writeText(badPath, R"({"format":"cad-app-part","version":1,"sketch":{"type":"rectangle","width":-4,"height":2}})");
  check(!LoadPart(badPath, &untouched, &error), "rechaza medidas negativas");

  writeText(badPath, R"({"format":"cad-app-part","version":3})");
  check(!LoadPart(badPath, &untouched, &error), "rechaza una versión de formato desconocida");

  writeText(badPath, R"({"format":"otro"})");
  check(!LoadPart(badPath, &untouched, &error), "rechaza un JSON que no es una pieza");

  check(!LoadPart(dir.filePath("no-existe.cadp"), &untouched, &error), "falla si el archivo no existe");

  // Contornos válidos e inválidos.
  const std::vector<gp_Pnt2d> lShape = {gp_Pnt2d(0, 0), gp_Pnt2d(30, 0), gp_Pnt2d(30, 10),
                                        gp_Pnt2d(10, 10), gp_Pnt2d(10, 25), gp_Pnt2d(0, 25)};
  check(Sketch::IsValidProfile(lShape), "una L es un contorno válido");
  const std::vector<gp_Pnt2d> clockwise(lShape.rbegin(), lShape.rend());
  check(Sketch::IsValidProfile(clockwise), "el sentido del recorrido no importa");
  check(!Sketch::IsValidProfile({gp_Pnt2d(0, 0), gp_Pnt2d(10, 0)}), "rechaza menos de 3 puntos");
  check(!Sketch::IsValidProfile({gp_Pnt2d(0, 0), gp_Pnt2d(10, 0), gp_Pnt2d(20, 0)}),
        "rechaza puntos alineados (sin área)");
  check(!Sketch::IsValidProfile({gp_Pnt2d(0, 0), gp_Pnt2d(10, 10), gp_Pnt2d(10, 0), gp_Pnt2d(0, 10)}),
        "rechaza un contorno que se cruza (moño)");
  check(!Sketch::IsValidProfile({gp_Pnt2d(0, 0), gp_Pnt2d(10, 0), gp_Pnt2d(5, 0), gp_Pnt2d(5, 8)}),
        "rechaza un lado que vuelve sobre el anterior");
  check(!Sketch::IsValidProfile({gp_Pnt2d(0, 0), gp_Pnt2d(10, 0), gp_Pnt2d(10, 0), gp_Pnt2d(0, 10)}),
        "rechaza un lado de largo cero");
  check(!Sketch::IsValidProfile({gp_Pnt2d(0, 0), gp_Pnt2d(20, 0), gp_Pnt2d(20, 10), gp_Pnt2d(10, 0),
                                 gp_Pnt2d(0, 10)}),
        "rechaza un vértice que toca otro lado");

  Sketch invalid;
  check(!invalid.SetPolygon({gp_Pnt2d(0, 0), gp_Pnt2d(10, 10), gp_Pnt2d(10, 0), gp_Pnt2d(0, 10)}),
        "SetPolygon rechaza un contorno inválido");
  check(!invalid.HasProfile(), "un contorno rechazado no queda en el boceto");

  Sketch rectangle;
  check(rectangle.SetPolygon({gp_Pnt2d(5, 5), gp_Pnt2d(5, 15), gp_Pnt2d(25, 15), gp_Pnt2d(25, 5)}),
        "acepta un rectángulo dibujado en cualquier lugar y sentido");
  check(rectangle.IsRectangle() && rectangle.Width() == 20.0 && rectangle.Height() == 10.0,
        "reconoce el rectángulo y sus medidas");
  check(rectangle.DimensionedEdges().size() == 2, "un rectángulo lleva 2 cotas (ancho y alto)");

  // Un contorno cualquiera se guarda, se abre y se extruye (L: 30×10 + 10×15 = 450 mm², × 4 = 1800 mm³).
  Part lPart;
  check(lPart.sketch.SetPolygon(lShape), "acepta la L en el boceto");
  check(!lPart.sketch.IsRectangle(), "la L no es un rectángulo");
  check(lPart.sketch.DimensionedEdges().size() == 6, "la L lleva una cota por lado");
  lPart.extrudeDepth = 4.0;
  const QString lPath = dir.filePath("ele.cadp");
  check(SavePart(lPart, lPath, &error), "guarda la L");
  Part loadedL;
  check(LoadPart(lPath, &loadedL, &error), "abre la L");
  check(loadedL.sketch.Points().size() == lShape.size(), "la L conserva sus 6 vértices");
  bool samePoints = loadedL.sketch.Points().size() == lShape.size();
  for (size_t i = 0; samePoints && i < lShape.size(); ++i)
    samePoints = loadedL.sketch.Points()[i].IsEqual(lShape[i], 0.0);
  check(samePoints, "la L conserva sus coordenadas exactas");
  check(std::abs(volumeOf(ExtrudeSketch(loadedL.sketch, *loadedL.extrudeDepth)) - 1800.0) < 1e-6,
        "la L extruida tiene 1800 mm³");
  const TopoDS_Shape clockwiseSolid = [&] {
    Sketch sketch;
    sketch.SetPolygon(clockwise);
    return ExtrudeSketch(sketch, 4.0);
  }();
  check(std::abs(volumeOf(clockwiseSolid) - 1800.0) < 1e-6, "la L recorrida al revés da el mismo sólido");

  // Cambiar la cota de un lado (L de 60 × 50: lados 0 a 5).
  Sketch edited;
  edited.SetPolygon({gp_Pnt2d(0, 0), gp_Pnt2d(60, 0), gp_Pnt2d(60, 20),
                     gp_Pnt2d(20, 20), gp_Pnt2d(20, 50), gp_Pnt2d(0, 50)});
  check(edited.EdgeLength(0) == 60.0, "el lado inferior de la L mide 60");
  check(edited.SetEdgeLength(0, 80.0), "alarga el lado inferior");
  check(edited.Points()[1].IsEqual(gp_Pnt2d(80, 0), 0.0) && edited.Points()[2].IsEqual(gp_Pnt2d(80, 20), 0.0),
        "el lado perpendicular siguiente se mueve con él");
  check(edited.Points()[3].IsEqual(gp_Pnt2d(20, 20), 0.0), "el lado paralelo absorbe el cambio");
  check(edited.EdgeLength(2) == 60.0 && edited.EdgeLength(1) == 20.0, "la L sigue siendo una L");
  check(std::abs(volumeOf(ExtrudeSketch(edited, 1.0)) - 2200.0) < 1e-6, "la L editada tiene el área esperada");
  check(edited.SetEdgeLength(3, 40.0) && edited.Height() == 60.0, "alarga el brazo vertical de la L");
  const std::vector<gp_Pnt2d> beforeBadEdit = edited.Points();
  check(!edited.SetEdgeLength(2, 90.0), "rechaza una cota que haría cruzar el contorno");
  check(edited.Points().size() == beforeBadEdit.size() && edited.Points()[3].IsEqual(beforeBadEdit[3], 0.0),
        "una cota rechazada no cambia el boceto");
  check(!edited.SetEdgeLength(0, 0.0) && !edited.SetEdgeLength(0, -5.0) && !edited.SetEdgeLength(9, 5.0),
        "rechaza largos no positivos y lados que no existen");

  Sketch box;
  box.SetRectangle(40.0, 20.0);
  check(box.SetEdgeLength(0, 55.0) && box.IsRectangle() && box.Width() == 55.0 && box.Height() == 20.0,
        "cambiar el ancho de un rectángulo lo deja rectángulo");
  check(box.SetEdgeLength(1, 30.0) && box.IsRectangle() && box.Height() == 30.0, "cambiar el alto también");

  Sketch triangle;
  triangle.SetPolygon({gp_Pnt2d(0, 0), gp_Pnt2d(20, 0), gp_Pnt2d(0, 20)});
  check(triangle.SetEdgeLength(0, 30.0) && triangle.Points()[1].IsEqual(gp_Pnt2d(30, 0), 1e-9)
            && triangle.Points()[2].IsEqual(gp_Pnt2d(0, 20), 0.0),
        "en un triángulo solo se mueve el vértice final");

  // Formato de las medidas que ve el usuario.
  check(FormatLength(40.0) == "40", "40 se muestra sin decimales");
  check(FormatLength(28.2842712) == "28,28", "redondea a 2 decimales con coma");
  check(FormatLength(12.5) == "12,5", "saca los ceros finales");
  check(FormatLength(-0.001) == "0", "no muestra -0");

  // Las piezas del formato anterior (versión 1, rectángulo) se siguen abriendo.
  const QString v1Path = dir.filePath("v1.cadp");
  writeText(v1Path, R"({"format":"cad-app-part","version":1,"sketch":{"type":"rectangle","width":40,"height":20},"extrude":{"depth":10}})");
  Part v1;
  check(LoadPart(v1Path, &v1, &error), "abre una pieza de la versión 1");
  check(v1.sketch.IsRectangle() && v1.sketch.Width() == 40.0 && v1.sketch.Height() == 20.0,
        "la pieza de la versión 1 conserva el rectángulo");

  writeText(badPath, R"({"format":"cad-app-part","version":2,"sketch":{"type":"polygon","points":[[0,0],[10,10],[10,0],[0,10]]}})");
  check(!LoadPart(badPath, &untouched, &error), "rechaza un contorno guardado que se cruza");
  writeText(badPath, R"({"format":"cad-app-part","version":2,"sketch":{"type":"polygon","points":[[0,0],[10,"a"],[10,10]]}})");
  check(!LoadPart(badPath, &untouched, &error), "rechaza coordenadas que no son números");

  // La exportación STEP se puede volver a leer y tiene el volumen esperado (40 × 20 × 10 = 8000 mm³).
  const QString stepPath = dir.filePath("bloque.step");
  const TopoDS_Shape solid = ExtrudeSketch(part.sketch, *part.extrudeDepth);
  check(ExportStep(solid, stepPath, &error), "exporta a STEP");

  STEPControl_Reader reader;
  check(reader.ReadFile(stepPath.toUtf8().constData()) == IFSelect_RetDone, "el STEP se puede leer");
  reader.TransferRoots();
  const TopoDS_Shape imported = reader.OneShape();
  check(!imported.IsNull(), "el STEP contiene una pieza");
  check(std::abs(volumeOf(imported) - 8000.0) < 1e-6, "el STEP conserva el volumen de 8000 mm³");

  if (failures == 0)
    std::printf("Todas las pruebas pasaron.\n");
  return failures == 0 ? 0 : 1;
}
