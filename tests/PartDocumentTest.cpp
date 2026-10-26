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
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <cmath>
#include <optional>
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

// Plano de la cara plana de la pieza cuya normal hacia afuera es `normal`.
std::optional<gp_Ax3> facePlane(const TopoDS_Shape& shape, const gp_Dir& normal)
{
  for (TopExp_Explorer it(shape, TopAbs_FACE); it.More(); it.Next())
  {
    const std::optional<gp_Ax3> plane = SketchPlaneFromFace(TopoDS::Face(it.Current()));
    if (plane && plane->Direction().IsEqual(normal, 1e-9))
      return plane;
  }
  return std::nullopt;
}

Feature makeFeature(Feature::Kind kind, const gp_Ax3& plane, double x, double y, double w, double h,
                    double depth, bool throughAll = false)
{
  Feature feature;
  feature.kind = kind;
  feature.plane = plane;
  feature.sketch.SetPolygon({gp_Pnt2d(x, y), gp_Pnt2d(x + w, y), gp_Pnt2d(x + w, y + h), gp_Pnt2d(x, y + h)});
  feature.depth = depth;
  feature.throughAll = throughAll;
  return feature;
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

  writeText(badPath, R"({"format":"cad-app-part","version":99})");
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

  // Operaciones sobre un bloque de 40 × 20 × 10 (8000 mm³).
  {
    Part block;
    block.sketch.SetRectangle(40.0, 20.0);
    block.extrudeDepth = 10.0;
    const TopoDS_Shape base = BuildPart(block);
    check(std::abs(volumeOf(base) - 8000.0) < 1e-6, "sin operaciones, BuildPart es la extrusión");

    const std::optional<gp_Ax3> top = facePlane(base, gp::DZ());
    check(top && std::abs(top->Location().Z() - 10.0) < 1e-9, "la cara de arriba está en z = 10");
    check(top && top->XDirection().IsEqual(gp::DX(), 1e-9) && top->YDirection().IsEqual(gp::DY(), 1e-9),
          "en la cara de arriba, las coordenadas del boceto son las X e Y globales");
    const std::optional<gp_Ax3> side = facePlane(base, gp::DX());
    check(side && std::abs(side->Location().X() - 40.0) < 1e-9, "la cara derecha está en x = 40");
    const std::optional<gp_Ax3> bottom = facePlane(base, -gp::DZ());

    if (top && side && bottom)
    {
      std::vector<size_t> failed;
      Part pocket = block;
      pocket.features.push_back(makeFeature(Feature::Kind::Cut, *top, 5, 5, 10, 10, 4.0));
      check(std::abs(volumeOf(BuildPart(pocket, &failed)) - 7600.0) < 1e-6 && failed.empty(),
            "un corte de 10 × 10 y 4 de profundidad resta 400 mm³");

      Part hole = block;
      hole.features.push_back(makeFeature(Feature::Kind::Cut, *top, 5, 5, 10, 10, 1.0, true));
      check(std::abs(volumeOf(BuildPart(hole)) - 7000.0) < 1e-6, "un agujero pasante resta 1000 mm³");

      Part boss = block;
      boss.features.push_back(makeFeature(Feature::Kind::Boss, *top, 20, 5, 10, 10, 5.0));
      check(std::abs(volumeOf(BuildPart(boss)) - 8500.0) < 1e-6, "un saliente de 10 × 10 × 5 suma 500 mm³");

      // En la cara derecha el eje X del boceto es horizontal y el Y es el Z global.
      Part sideCut = block;
      sideCut.features.push_back(makeFeature(Feature::Kind::Cut, *side, 2, 2, 5, 5, 2.0));
      check(std::abs(volumeOf(BuildPart(sideCut)) - 7950.0) < 1e-6, "un corte en la cara del costado resta 50 mm³");

      Part bottomCut = block;
      bottomCut.features.push_back(makeFeature(Feature::Kind::Cut, *bottom, 5, -15, 10, 10, 3.0));
      check(std::abs(volumeOf(BuildPart(bottomCut)) - 7700.0) < 1e-6, "un corte en la cara de abajo resta 300 mm³");

      Part combined = block;
      combined.features.push_back(makeFeature(Feature::Kind::Boss, *top, 20, 5, 10, 10, 5.0));
      combined.features.push_back(makeFeature(Feature::Kind::Cut, *top, 5, 5, 10, 10, 1.0, true));
      failed.clear();
      check(std::abs(volumeOf(BuildPart(combined, &failed)) - 7500.0) < 1e-6 && failed.empty(),
            "las operaciones se aplican en orden");

      Part outside = block;
      outside.features.push_back(makeFeature(Feature::Kind::Cut, *top, 100, 100, 10, 10, 1.0, true));
      outside.features.push_back(makeFeature(Feature::Kind::Cut, *top, 5, 5, 10, 10, 4.0));
      failed.clear();
      const TopoDS_Shape outsideShape = BuildPart(outside, &failed);
      check(failed.size() == 1 && failed[0] == 0, "un corte que no toca la pieza se marca como fallido");
      check(std::abs(volumeOf(outsideShape) - 7600.0) < 1e-6, "y las demás operaciones se aplican igual");

      // Las operaciones se guardan y se abren, y la pieza abierta da el mismo sólido.
      const QString featuresPath = dir.filePath("operaciones.cadp");
      check(SavePart(combined, featuresPath, &error), "guarda una pieza con operaciones");
      Part reopened;
      check(LoadPart(featuresPath, &reopened, &error), "abre una pieza con operaciones");
      check(reopened.features.size() == 2 && reopened.features[0].kind == Feature::Kind::Boss
                && reopened.features[1].kind == Feature::Kind::Cut && reopened.features[1].throughAll,
            "las operaciones conservan su tipo y si son pasantes");
      check(std::abs(volumeOf(BuildPart(reopened)) - 7500.0) < 1e-6, "la pieza abierta da el mismo sólido");

      const QString sideStepPath = dir.filePath("costado.step");
      check(ExportStep(BuildPart(sideCut), sideStepPath, &error), "exporta a STEP una pieza con operaciones");
    }

    writeText(badPath, R"({"format":"cad-app-part","version":3,"sketch":{"type":"polygon","points":[[0,0],[10,0],[10,10]]},"extrude":{"depth":5},"features":[{"type":"cut","plane":{"origin":[0,0,5],"normal":[0,0,1],"xdir":[0,1,1]},"sketch":{"type":"polygon","points":[[1,1],[2,1],[2,2]]},"depth":1}]})");
    check(!LoadPart(badPath, &untouched, &error), "rechaza un plano con ejes no perpendiculares");
    writeText(badPath, R"({"format":"cad-app-part","version":3,"sketch":{"type":"polygon","points":[[0,0],[10,0],[10,10]]},"features":[]})");
    check(!LoadPart(badPath, &untouched, &error), "rechaza operaciones sin extrusión");
  }

  // Arcos y círculos.
  {
    const double pi = std::acos(-1.0);
    check(Sketch::ArcMiddle(gp_Pnt2d(0, 0), gp_Pnt2d(10, 0), 1.0).IsEqual(gp_Pnt2d(5, -5), 1e-9),
          "con bulge 1, el arco es media vuelta a la derecha del lado");
    const std::optional<double> right = Sketch::BulgeThrough(gp_Pnt2d(0, 0), gp_Pnt2d(10, 0), gp_Pnt2d(5, -5));
    const std::optional<double> left = Sketch::BulgeThrough(gp_Pnt2d(0, 0), gp_Pnt2d(10, 0), gp_Pnt2d(5, 5));
    check(right && std::abs(*right - 1.0) < 1e-9 && left && std::abs(*left + 1.0) < 1e-9,
          "el bulge por tres puntos tiene el signo del lado del punto de paso");
    const std::optional<double> shallow = Sketch::BulgeThrough(gp_Pnt2d(0, 0), gp_Pnt2d(10, 0), gp_Pnt2d(1, -3));
    check(shallow && Sketch::ArcMiddle(gp_Pnt2d(0, 0), gp_Pnt2d(10, 0), *shallow).Distance(gp_Pnt2d(5, 0)) > 0.0,
          "un punto de paso que no es el medio también da un arco");
    check(!Sketch::BulgeThrough(gp_Pnt2d(0, 0), gp_Pnt2d(10, 0), gp_Pnt2d(20, 0)),
          "tres puntos alineados no forman un arco");

    check(Sketch::IsValidProfile({gp_Pnt2d(0, 0), gp_Pnt2d(10, 0)}, {0.0, 1.0}),
          "una línea y un arco cierran un contorno (una D)");
    check(!Sketch::IsValidProfile({gp_Pnt2d(0, 0), gp_Pnt2d(10, 0), gp_Pnt2d(10, 3), gp_Pnt2d(0, 3)},
                                  {-1.0, 0.0, 0.0, 0.0}),
          "rechaza un arco que cruza el lado de enfrente");
    check(!Sketch::IsValidProfile({gp_Pnt2d(0, 0), gp_Pnt2d(10, 0)}, {0.0}),
          "rechaza bulges que no coinciden con los lados");

    Sketch circle;
    check(circle.SetCircle(gp_Pnt2d(20, 10), 5.0) && circle.IsCircle(), "dibuja un círculo");
    check(std::abs(circle.Width() - 10.0) < 1e-9 && std::abs(circle.Height() - 10.0) < 1e-9,
          "el círculo de radio 5 ocupa 10 × 10");
    check(circle.DimensionedEdges().size() == 1 && circle.DimensionedEdges()[0].kind == Sketch::DimensionKind::Diameter
              && std::abs(circle.DimensionValue(0) - 10.0) < 1e-9,
          "el círculo lleva una cota de diámetro");
    check(circle.SetDimensionValue(0, 16.0) && std::abs(circle.CircleRadius() - 8.0) < 1e-9
              && circle.CircleCenter().IsEqual(gp_Pnt2d(20, 10), 1e-9),
          "cambiar el diámetro deja fijo el centro");
    check(!circle.SetCircle(gp_Pnt2d(0, 0), 0.0), "rechaza un círculo de radio cero");

    Part cylinder;
    cylinder.sketch.SetCircle(gp_Pnt2d(0, 0), 5.0);
    cylinder.extrudeDepth = 10.0;
    check(std::abs(volumeOf(BuildPart(cylinder)) - pi * 25.0 * 10.0) < 1e-6, "un círculo extruido es un cilindro");

    // Ranura: rectángulo de 20 × 10 con los lados cortos en media vuelta hacia afuera.
    Sketch slot;
    check(slot.SetProfile({gp_Pnt2d(0, 0), gp_Pnt2d(20, 0), gp_Pnt2d(20, 10), gp_Pnt2d(0, 10)}, {0.0, 1.0, 0.0, 1.0}),
          "dibuja una ranura");
    check(!slot.IsRectangle() && !slot.IsCircle() && slot.HasArcs(), "la ranura no es rectángulo ni círculo");
    check(std::abs(slot.Width() - 30.0) < 1e-9 && std::abs(slot.Height() - 10.0) < 1e-9,
          "la caja de la ranura incluye los arcos");
    const std::vector<Sketch::DimensionedEdge> slotDimensions = slot.DimensionedEdges();
    check(slotDimensions.size() == 4 && slotDimensions[0].kind == Sketch::DimensionKind::Length
              && slotDimensions[1].kind == Sketch::DimensionKind::Radius && std::abs(slot.DimensionValue(1) - 5.0) < 1e-9,
          "la ranura lleva largo en los lados rectos y radio en los arcos");

    Part slotPart;
    slotPart.sketch = slot;
    slotPart.extrudeDepth = 10.0;
    check(std::abs(volumeOf(BuildPart(slotPart)) - (200.0 + pi * 25.0) * 10.0) < 1e-6,
          "la ranura extruida tiene el volumen esperado");

    Sketch wider = slot;
    check(wider.SetEdgeLength(0, 30.0) && wider.Points()[2].IsEqual(gp_Pnt2d(30, 10), 1e-9)
              && std::abs(wider.Width() - 40.0) < 1e-9 && std::abs(wider.ArcRadius(1) - 5.0) < 1e-9,
          "alargar el lado recto de la ranura arrastra el arco");
    check(!wider.SetEdgeLength(1, 20.0), "el largo de un arco no se cambia como un lado recto");
    Sketch flatter = slot;
    check(flatter.SetArcRadius(1, 10.0) && std::abs(flatter.ArcRadius(1) - 10.0) < 1e-9
              && flatter.Points()[1].IsEqual(gp_Pnt2d(20, 0), 0.0) && flatter.Points()[2].IsEqual(gp_Pnt2d(20, 10), 0.0)
              && std::abs(flatter.Width() - (25.0 + 10.0 - std::sqrt(75.0))) < 1e-9,
          "cambiar el radio de un arco deja fijos sus extremos");
    check(!flatter.SetArcRadius(1, 4.0), "el radio no puede ser menor que media cuerda");

    // Agujero redondo pasante en la cara de arriba de un bloque de 40 × 20 × 10.
    Part block;
    block.sketch.SetRectangle(40.0, 20.0);
    block.extrudeDepth = 10.0;
    const std::optional<gp_Ax3> top = facePlane(BuildPart(block), gp::DZ());
    if (top)
    {
      Feature roundHole;
      roundHole.kind = Feature::Kind::Cut;
      roundHole.plane = *top;
      roundHole.sketch.SetCircle(gp_Pnt2d(20, 10), 3.0);
      roundHole.throughAll = true;
      block.features.push_back(roundHole);
      Feature roundBoss = roundHole;
      roundBoss.kind = Feature::Kind::Boss;
      roundBoss.throughAll = false;
      roundBoss.depth = 5.0;
      roundBoss.sketch = slot; // ranura de 30 × 10 sobre la cara, de x -5 a 25
      roundBoss.sketch.SetEdgeLength(0, 10.0);
      block.features.push_back(roundBoss);
      std::vector<size_t> failed;
      const double expected = 8000.0 - pi * 9.0 * 10.0 + (100.0 + pi * 25.0) * 5.0;
      check(std::abs(volumeOf(BuildPart(block, &failed)) - expected) < 1e-6 && failed.empty(),
            "un agujero redondo pasante y un saliente con arcos");

      const QString roundPath = dir.filePath("redondos.cadp");
      check(SavePart(block, roundPath, &error), "guarda una pieza con círculos y arcos");
      Part reopened;
      check(LoadPart(roundPath, &reopened, &error), "abre una pieza con círculos y arcos");
      check(reopened.features.size() == 2 && reopened.features[0].sketch.IsCircle()
                && std::abs(reopened.features[0].sketch.CircleRadius() - 3.0) < 1e-9
                && reopened.features[1].sketch.Bulges() == roundBoss.sketch.Bulges(),
            "los círculos y arcos se guardan tal cual");
      check(std::abs(volumeOf(BuildPart(reopened)) - expected) < 1e-6, "la pieza con arcos abierta da el mismo sólido");
      const QString roundStepPath = dir.filePath("redondos.step");
      check(ExportStep(BuildPart(reopened), roundStepPath, &error), "exporta a STEP una pieza con arcos");
    }

    writeText(badPath, R"({"format":"cad-app-part","version":4,"sketch":{"type":"polygon","points":[[0,0],[10,0]],"bulges":[0]}})");
    check(!LoadPart(badPath, &untouched, &error), "rechaza bulges que no coinciden con los vértices");
    writeText(badPath, R"({"format":"cad-app-part","version":4,"sketch":{"type":"circle","center":[0,0],"radius":-1}})");
    check(!LoadPart(badPath, &untouched, &error), "rechaza un círculo de radio negativo");
  }

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
