// Pruebas del núcleo: guardar/abrir piezas y exportar a STEP.
// Se corren con `ctest` después de compilar. Devuelve 0 si todo pasa.

#include "Extrude.h"
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
  file.open(QIODevice::WriteOnly);
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

  writeText(badPath, R"({"format":"cad-app-part","version":2})");
  check(!LoadPart(badPath, &untouched, &error), "rechaza una versión de formato desconocida");

  writeText(badPath, R"({"format":"otro"})");
  check(!LoadPart(badPath, &untouched, &error), "rechaza un JSON que no es una pieza");

  check(!LoadPart(dir.filePath("no-existe.cadp"), &untouched, &error), "falla si el archivo no existe");

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
