#include "PartDocument.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <IFSelect_ReturnStatus.hxx>
#include <Interface_Static.hxx>
#include <STEPControl_Writer.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_XYZ.hxx>

#include <cmath>

namespace
{
const char* const kFormatName = "cad-app-part";
// Versión 1: el boceto es un rectángulo con una esquina en el origen (ancho y alto).
// Versión 2: el boceto es un contorno cerrado (lista de vértices).
// Versión 3: además, la lista de operaciones (agujeros, cortes y salientes).
// Se siguen abriendo todas.
const int kFormatVersion = 3;

bool readPositive(const QJsonObject& object, const char* key, double* value)
{
  const QJsonValue jsonValue = object.value(key);
  if (!jsonValue.isDouble() || jsonValue.toDouble() <= 0.0)
    return false;
  *value = jsonValue.toDouble();
  return true;
}

bool readRectangle(const QJsonObject& object, Sketch* sketch)
{
  double width = 0.0;
  double height = 0.0;
  if (object.value("type").toString() != "rectangle"
      || !readPositive(object, "width", &width)
      || !readPositive(object, "height", &height))
    return false;
  sketch->SetRectangle(width, height);
  return true;
}

QJsonObject writePolygon(const Sketch& sketch)
{
  QJsonArray points;
  for (const gp_Pnt2d& point : sketch.Points())
    points.append(QJsonArray{point.X(), point.Y()});
  QJsonObject object;
  object["type"] = "polygon";
  object["points"] = points;
  return object;
}

QJsonArray writeVector(double x, double y, double z)
{
  return QJsonArray{x, y, z};
}

bool readVector(const QJsonValue& value, gp_XYZ* vector)
{
  const QJsonArray array = value.toArray();
  if (array.size() != 3)
    return false;
  for (int i = 0; i < 3; ++i)
    if (!array[i].isDouble() || !std::isfinite(array[i].toDouble()))
      return false;
  vector->SetCoord(array[0].toDouble(), array[1].toDouble(), array[2].toDouble());
  return true;
}

QJsonObject writeFeature(const Feature& feature)
{
  const gp_Ax3& plane = feature.plane;
  QJsonObject planeObject;
  planeObject["origin"] = writeVector(plane.Location().X(), plane.Location().Y(), plane.Location().Z());
  planeObject["normal"] = writeVector(plane.Direction().X(), plane.Direction().Y(), plane.Direction().Z());
  planeObject["xdir"] = writeVector(plane.XDirection().X(), plane.XDirection().Y(), plane.XDirection().Z());

  QJsonObject object;
  object["type"] = feature.kind == Feature::Kind::Cut ? "cut" : "boss";
  object["plane"] = planeObject;
  object["sketch"] = writePolygon(feature.sketch);
  object["depth"] = feature.depth;
  object["throughAll"] = feature.throughAll;
  return object;
}

bool readPolygon(const QJsonObject& object, Sketch* sketch);

bool readFeature(const QJsonObject& object, Feature* feature)
{
  const QString type = object.value("type").toString();
  if (type != "cut" && type != "boss")
    return false;
  feature->kind = type == "cut" ? Feature::Kind::Cut : Feature::Kind::Boss;

  const QJsonObject planeObject = object.value("plane").toObject();
  gp_XYZ origin, normal, xDirection;
  if (!readVector(planeObject.value("origin"), &origin)
      || !readVector(planeObject.value("normal"), &normal)
      || !readVector(planeObject.value("xdir"), &xDirection))
    return false;
  // La normal y el eje X tienen que ser direcciones perpendiculares.
  if (normal.Modulus() < 1e-9 || xDirection.Modulus() < 1e-9
      || std::abs(normal.Normalized().Dot(xDirection.Normalized())) > 1e-6)
    return false;
  feature->plane = gp_Ax3(gp_Pnt(origin), gp_Dir(normal), gp_Dir(xDirection));

  if (!readPolygon(object.value("sketch").toObject(), &feature->sketch))
    return false;

  feature->throughAll = object.value("throughAll").toBool(false);
  if (feature->throughAll && feature->kind != Feature::Kind::Cut)
    return false;
  if (!feature->throughAll && !readPositive(object, "depth", &feature->depth))
    return false;
  if (feature->throughAll)
    feature->depth = object.value("depth").toDouble(10.0);
  return true;
}

bool readPolygon(const QJsonObject& object, Sketch* sketch)
{
  if (object.value("type").toString() != "polygon" || !object.value("points").isArray())
    return false;
  std::vector<gp_Pnt2d> points;
  for (const QJsonValue& value : object.value("points").toArray())
  {
    const QJsonArray pair = value.toArray();
    if (pair.size() != 2 || !pair[0].isDouble() || !pair[1].isDouble())
      return false;
    points.emplace_back(pair[0].toDouble(), pair[1].toDouble());
  }
  return sketch->SetPolygon(points);
}
} // namespace

bool SavePart(const Part& part, const QString& path, QString* error)
{
  QJsonObject root;
  root["format"] = kFormatName;
  root["version"] = kFormatVersion;

  if (part.sketch.HasProfile())
    root["sketch"] = writePolygon(part.sketch);
  if (part.extrudeDepth)
  {
    QJsonObject extrude;
    extrude["depth"] = *part.extrudeDepth;
    root["extrude"] = extrude;
  }
  if (!part.features.empty())
  {
    QJsonArray features;
    for (const Feature& feature : part.features)
      features.append(writeFeature(feature));
    root["features"] = features;
  }

  // QSaveFile escribe en un temporal y lo renombra: si algo falla, no se pierde el archivo anterior.
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly))
  {
    *error = file.errorString();
    return false;
  }
  file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
  if (!file.commit())
  {
    *error = file.errorString();
    return false;
  }
  return true;
}

bool LoadPart(const QString& path, Part* part, QString* error)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
  {
    *error = file.errorString();
    return false;
  }

  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject())
  {
    *error = "El archivo está dañado o no es una pieza.";
    return false;
  }

  const QJsonObject root = document.object();
  if (root.value("format").toString() != kFormatName)
  {
    *error = "El archivo no es una pieza de esta aplicación.";
    return false;
  }
  const int version = root.value("version").toInt();
  if (version < 1 || version > kFormatVersion)
  {
    *error = "La pieza se guardó con una versión del formato que esta aplicación no conoce.";
    return false;
  }

  Part loaded;
  if (root.contains("sketch"))
  {
    const QJsonObject sketch = root.value("sketch").toObject();
    const bool valid = version == 1 ? readRectangle(sketch, &loaded.sketch)
                                    : readPolygon(sketch, &loaded.sketch);
    if (!valid)
    {
      *error = "El boceto guardado no es válido.";
      return false;
    }
  }
  if (root.contains("extrude"))
  {
    double depth = 0.0;
    if (!loaded.sketch.HasProfile() || !readPositive(root.value("extrude").toObject(), "depth", &depth))
    {
      *error = "La extrusión guardada no es válida.";
      return false;
    }
    loaded.extrudeDepth = depth;
  }
  if (root.contains("features"))
  {
    if (!loaded.extrudeDepth || !root.value("features").isArray())
    {
      *error = "Las operaciones guardadas no son válidas.";
      return false;
    }
    for (const QJsonValue& value : root.value("features").toArray())
    {
      Feature feature;
      if (!readFeature(value.toObject(), &feature))
      {
        *error = "Las operaciones guardadas no son válidas.";
        return false;
      }
      loaded.features.push_back(feature);
    }
  }

  *part = loaded;
  return true;
}

bool ExportStep(const TopoDS_Shape& shape, const QString& path, QString* error)
{
  STEPControl_Writer writer;
  Interface_Static::SetCVal("write.step.unit", "MM");
  if (writer.Transfer(shape, STEPControl_AsIs) != IFSelect_RetDone)
  {
    *error = "No se pudo convertir la pieza a STEP.";
    return false;
  }
  // Open CASCADE espera rutas en UTF-8 (en Windows las convierte internamente).
  if (writer.Write(path.toUtf8().constData()) != IFSelect_RetDone)
  {
    *error = "No se pudo escribir el archivo STEP.";
    return false;
  }
  return true;
}
