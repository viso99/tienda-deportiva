#include "PartDocument.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <IFSelect_ReturnStatus.hxx>
#include <Interface_Static.hxx>
#include <STEPControl_Writer.hxx>

namespace
{
const char* const kFormatName = "cad-app-part";
// Versión 1: el boceto es un rectángulo con una esquina en el origen (ancho y alto).
// Versión 2: el boceto es un contorno cerrado (lista de vértices). Se siguen abriendo las dos.
const int kFormatVersion = 2;

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
  {
    QJsonObject sketch;
    QJsonArray points;
    for (const gp_Pnt2d& point : part.sketch.Points())
      points.append(QJsonArray{point.X(), point.Y()});
    sketch["type"] = "polygon";
    sketch["points"] = points;
    root["sketch"] = sketch;
  }
  if (part.extrudeDepth)
  {
    QJsonObject extrude;
    extrude["depth"] = *part.extrudeDepth;
    root["extrude"] = extrude;
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
  if (version != 1 && version != kFormatVersion)
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
