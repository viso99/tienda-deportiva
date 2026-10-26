#include "PartDocument.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <IFSelect_ReturnStatus.hxx>
#include <Interface_Static.hxx>
#include <STEPControl_Writer.hxx>

namespace
{
const char* const kFormatName = "cad-app-part";
const int kFormatVersion = 1;

bool readPositive(const QJsonObject& object, const char* key, double* value)
{
  const QJsonValue jsonValue = object.value(key);
  if (!jsonValue.isDouble() || jsonValue.toDouble() <= 0.0)
    return false;
  *value = jsonValue.toDouble();
  return true;
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
    sketch["type"] = "rectangle";
    sketch["width"] = part.sketch.Width();
    sketch["height"] = part.sketch.Height();
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
  if (root.value("version").toInt() != kFormatVersion)
  {
    *error = "La pieza se guardó con una versión del formato que esta aplicación no conoce.";
    return false;
  }

  Part loaded;
  if (root.contains("sketch"))
  {
    const QJsonObject sketch = root.value("sketch").toObject();
    double width = 0.0;
    double height = 0.0;
    if (sketch.value("type").toString() != "rectangle"
        || !readPositive(sketch, "width", &width)
        || !readPositive(sketch, "height", &height))
    {
      *error = "El boceto guardado no es válido.";
      return false;
    }
    loaded.sketch.SetRectangle(width, height);
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
