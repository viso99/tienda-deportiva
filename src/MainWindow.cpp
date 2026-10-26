#include "MainWindow.h"

#include "Extrude.h"
#include "FeatureDialog.h"
#include "Format.h"
#include "OccView.h"
#include "RectangleDialog.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

#include <algorithm>

namespace
{
const char* const kPartFilter = "Pieza (*.cadp)";
const char* const kStepFilter = "STEP (*.step *.stp)";
} // namespace

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent),
    myView(new OccView(this)),
    myExtrudeAction(new QAction("Extruir…", this)),
    myExportStepAction(new QAction("Exportar a STEP…", this)),
    myEditSketchAction(new QAction("Editar boceto", this)),
    myCutAction(new QAction("Agujero o corte…", this)),
    myBossAction(new QAction("Saliente…", this)),
    myRemoveFeatureAction(new QAction("Quitar última operación", this)),
    myStatusLabel(new QLabel(this))
{
  setCentralWidget(myView);
  // Los mensajes van en una etiqueta fija y no con showMessage: la ayuda de los botones
  // (status tips) usa el mensaje temporal y, al borrarse, se llevaba nuestro texto.
  statusBar()->addWidget(myStatusLabel, 1);

  QMenu* fileMenu = menuBar()->addMenu("Archivo");
  fileMenu->addAction("Nuevo", QKeySequence::New, this, &MainWindow::newPart);
  fileMenu->addAction("Abrir…", QKeySequence::Open, this, &MainWindow::openPart);
  fileMenu->addAction("Guardar", QKeySequence::Save, this, &MainWindow::savePart);
  fileMenu->addAction("Guardar como…", QKeySequence::SaveAs, this, &MainWindow::savePartAs);
  fileMenu->addSeparator();
  myExportStepAction->setStatusTip("Guardar la pieza en STEP para abrirla en otros programas CAD");
  connect(myExportStepAction, &QAction::triggered, this, &MainWindow::exportStep);
  fileMenu->addAction(myExportStepAction);
  fileMenu->addSeparator();
  fileMenu->addAction("Salir", QKeySequence::Quit, this, &QWidget::close);

  QAction* lineAction = new QAction("Línea", this);
  lineAction->setShortcut(Qt::Key_L);
  lineAction->setStatusTip("Dibujar con el mouse un contorno cerrado de líneas, clic a clic");
  connect(lineAction, &QAction::triggered, this, [this] { startSketchTool(SketchTool::Line); });

  QAction* mouseRectangleAction = new QAction("Rectángulo", this);
  mouseRectangleAction->setShortcut(Qt::Key_R);
  mouseRectangleAction->setStatusTip("Dibujar con el mouse un rectángulo por dos esquinas opuestas");
  connect(mouseRectangleAction, &QAction::triggered, this,
          [this] { startSketchTool(SketchTool::Rectangle); });

  QAction* rectangleAction = new QAction("Rectángulo con medidas…", this);
  rectangleAction->setStatusTip("Escribir el ancho y el alto de un rectángulo en el plano XY");
  connect(rectangleAction, &QAction::triggered, this, &MainWindow::drawRectangle);

  myEditSketchAction->setCheckable(true);
  myEditSketchAction->setShortcut(Qt::Key_E);
  myEditSketchAction->setStatusTip("Ver el boceto de la pieza para cambiar sus cotas");
  connect(myEditSketchAction, &QAction::toggled, this, &MainWindow::showModel);

  connect(myView, &OccView::profileFinished, this, &MainWindow::setProfile);
  connect(myView, &OccView::dimensionActivated, this, &MainWindow::editDimension);
  connect(myView, &OccView::escapePressed, this, [this] { myEditSketchAction->setChecked(false); });
  connect(myView, &OccView::toolCanceled, this, &MainWindow::cancelTool);
  connect(myView, &OccView::facePicked, this, &MainWindow::sketchOnFace);

  myCutAction->setShortcut(Qt::Key_H);
  myCutAction->setStatusTip("Restar material: elegí una cara, dibujá el contorno y su profundidad");
  connect(myCutAction, &QAction::triggered, this, [this] { startFeature(Feature::Kind::Cut); });
  myBossAction->setShortcut(Qt::Key_S);
  myBossAction->setStatusTip("Sumar material: elegí una cara, dibujá el contorno y su altura");
  connect(myBossAction, &QAction::triggered, this, [this] { startFeature(Feature::Kind::Boss); });
  myRemoveFeatureAction->setStatusTip("Borrar el último agujero, corte o saliente");
  connect(myRemoveFeatureAction, &QAction::triggered, this, &MainWindow::removeLastFeature);
  connect(myView, &OccView::toolHint, this, [this](const QString& text) { showStatus(text); });

  myExtrudeAction->setStatusTip("Convertir el boceto en una pieza 3D estirándolo en Z");
  connect(myExtrudeAction, &QAction::triggered, this, &MainWindow::extrude);

  QMenu* sketchMenu = menuBar()->addMenu("Boceto");
  sketchMenu->addAction(lineAction);
  sketchMenu->addAction(mouseRectangleAction);
  sketchMenu->addAction(rectangleAction);
  sketchMenu->addSeparator();
  sketchMenu->addAction(myEditSketchAction);
  QMenu* operationsMenu = menuBar()->addMenu("Operaciones");
  operationsMenu->addAction(myExtrudeAction);
  operationsMenu->addAction(myCutAction);
  operationsMenu->addAction(myBossAction);
  operationsMenu->addSeparator();
  operationsMenu->addAction(myRemoveFeatureAction);
  QToolBar* toolBar = addToolBar("Herramientas");
  toolBar->addAction(lineAction);
  toolBar->addAction(mouseRectangleAction);
  toolBar->addAction(rectangleAction);
  toolBar->addAction(myExtrudeAction);
  toolBar->addAction(myEditSketchAction);
  toolBar->addSeparator();
  toolBar->addAction(myCutAction);
  toolBar->addAction(myBossAction);

  updateActions();
  updateTitle();
  showStatus("Dibujá un boceto con Boceto → Línea o Rectángulo · "
             "Girar: botón izquierdo · Mover: botón del medio · Zoom: rueda");
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  if (confirmDiscardChanges())
    event->accept();
  else
    event->ignore();
}

void MainWindow::newPart()
{
  if (!confirmDiscardChanges())
    return;
  setPart(Part(), QString());
}

void MainWindow::openPart()
{
  if (!confirmDiscardChanges())
    return;

  const QString path = QFileDialog::getOpenFileName(this, "Abrir pieza", QString(), kPartFilter);
  if (path.isEmpty())
    return;

  Part part;
  QString error;
  if (!LoadPart(path, &part, &error))
  {
    QMessageBox::warning(this, "Abrir pieza", QString("No se pudo abrir la pieza.\n\n%1").arg(error));
    return;
  }
  setPart(part, path);
}

bool MainWindow::savePart()
{
  if (myFilePath.isEmpty())
    return savePartAs();
  return writeTo(myFilePath);
}

bool MainWindow::savePartAs()
{
  QString path = QFileDialog::getSaveFileName(this, "Guardar pieza", myFilePath, kPartFilter);
  if (path.isEmpty())
    return false;
  if (QFileInfo(path).suffix().isEmpty())
    path += ".cadp";
  return writeTo(path);
}

void MainWindow::exportStep()
{
  if (!myPart.extrudeDepth)
    return;

  const QString suggested = myFilePath.isEmpty()
                                ? QString()
                                : QFileInfo(myFilePath).path() + "/" + QFileInfo(myFilePath).completeBaseName() + ".step";
  QString path = QFileDialog::getSaveFileName(this, "Exportar a STEP", suggested, kStepFilter);
  if (path.isEmpty())
    return;
  if (QFileInfo(path).suffix().isEmpty())
    path += ".step";

  QString error;
  if (!ExportStep(BuildPart(myPart), path, &error))
  {
    QMessageBox::warning(this, "Exportar a STEP", QString("No se pudo exportar la pieza.\n\n%1").arg(error));
    return;
  }
  showStatus(QString("Exportado a %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::drawRectangle()
{
  const Sketch& sketch = myPart.sketch;
  RectangleDialog dialog(sketch.HasProfile() ? sketch.Width() : 40.0,
                         sketch.HasProfile() ? sketch.Height() : 20.0, this);
  if (dialog.exec() != QDialog::Accepted)
    return;

  myPart.sketch.SetRectangle(dialog.Width(), dialog.Height());
  markModified();
  showModel();
}

void MainWindow::startSketchTool(SketchTool tool)
{
  // Con una cara elegida, se dibuja el contorno de la operación sobre ella.
  if (myPendingPlane)
  {
    myView->StartSketchTool(tool, *myPendingPlane, myShape);
    return;
  }
  // Si no, el perfil nuevo reemplaza al boceto base; si la pieza ya estaba extruida, se
  // recalcula al cerrarlo. Primero se sale de "Editar boceto" y de cualquier operación a
  // medias, porque mostrar el modelo cancela la herramienta.
  myPendingKind.reset();
  myEditSketchAction->setChecked(false);
  myView->StartSketchTool(tool);
}

void MainWindow::startFeature(Feature::Kind kind)
{
  myEditSketchAction->setChecked(false);
  showModel(); // muestra la pieza para elegir la cara
  myPendingKind = kind;
  myView->StartFacePick();
}

void MainWindow::sketchOnFace(const gp_Ax3& plane)
{
  myPendingPlane = plane;
  // Empieza con el rectángulo, lo más común en agujeros y cortes; con L se pasa a líneas.
  myView->StartSketchTool(SketchTool::Rectangle, plane, myShape);
}

void MainWindow::cancelTool()
{
  showModel();
}

void MainWindow::addFeature(const std::vector<gp_Pnt2d>& points)
{
  Feature feature;
  feature.kind = *myPendingKind;
  feature.plane = *myPendingPlane;
  if (!feature.sketch.SetPolygon(points))
  {
    showModel();
    return; // la vista solo entrega contornos válidos
  }

  const bool isCut = feature.kind == Feature::Kind::Cut;
  FeatureDialog dialog(isCut, isCut ? 5.0 : 10.0, this);
  if (dialog.exec() != QDialog::Accepted)
  {
    showModel();
    return;
  }
  feature.depth = dialog.Depth();
  feature.throughAll = dialog.ThroughAll();

  Part candidate = myPart;
  candidate.features.push_back(feature);
  std::vector<size_t> failed;
  BuildPart(candidate, &failed);
  if (std::find(failed.begin(), failed.end(), candidate.features.size() - 1) != failed.end())
  {
    showModel();
    QMessageBox::warning(this, isCut ? "Agujero o corte" : "Saliente",
                         isCut ? "El corte no cambia la pieza: revisá que el contorno quede sobre la pieza."
                               : "No se pudo agregar el saliente.");
    return;
  }
  myPart = candidate;
  markModified();
  showModel();
}

void MainWindow::removeLastFeature()
{
  if (myPart.features.empty())
    return;
  myPart.features.pop_back();
  markModified();
  showModel();
}

void MainWindow::setProfile(const std::vector<gp_Pnt2d>& points)
{
  if (myPendingKind && myPendingPlane)
  {
    addFeature(points);
    return;
  }
  if (!myPart.sketch.SetPolygon(points))
    return; // la vista solo entrega contornos válidos
  markModified();
  showModel();
}

void MainWindow::editDimension(size_t edgeIndex)
{
  bool ok = false;
  const double length = QInputDialog::getDouble(this, "Cambiar cota", "Largo del lado (mm):",
                                                myPart.sketch.EdgeLength(edgeIndex),
                                                0.01, 100000.0, 2, &ok);
  if (!ok)
    return;
  if (!myPart.sketch.SetEdgeLength(edgeIndex, length))
  {
    QMessageBox::warning(this, "Cambiar cota",
                         "Con esa medida el contorno se cruzaría consigo mismo. Probá con otra.");
    return;
  }
  markModified();
  showModel();
}

void MainWindow::extrude()
{
  bool ok = false;
  const double depth = QInputDialog::getDouble(this, "Extruir", "Distancia (mm):",
                                               myPart.extrudeDepth.value_or(10.0),
                                               0.01, 100000.0, 2, &ok);
  if (!ok)
    return;

  myPart.extrudeDepth = depth;
  myEditSketchAction->setChecked(false);
  markModified();
  showModel();
}

void MainWindow::setPart(const Part& part, const QString& filePath)
{
  myPart = part;
  myFilePath = filePath;
  myEditSketchAction->setChecked(false);
  myModified = false;
  updateActions();
  updateTitle();
  showModel();
}

void MainWindow::markModified()
{
  myModified = true;
  updateActions();
  updateTitle();
}

bool MainWindow::confirmDiscardChanges()
{
  if (!myModified)
    return true;

  const QMessageBox::StandardButton answer = QMessageBox::question(
      this, "Cambios sin guardar", "La pieza tiene cambios sin guardar. ¿Querés guardarlos?",
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
  if (answer == QMessageBox::Save)
    return savePart();
  return answer == QMessageBox::Discard;
}

bool MainWindow::writeTo(const QString& filePath)
{
  QString error;
  if (!SavePart(myPart, filePath, &error))
  {
    QMessageBox::warning(this, "Guardar pieza", QString("No se pudo guardar la pieza.\n\n%1").arg(error));
    return false;
  }
  myFilePath = filePath;
  myModified = false;
  updateTitle();
  showStatus(QString("Guardado en %1").arg(QFileInfo(filePath).fileName()));
  return true;
}

void MainWindow::updateActions()
{
  myExtrudeAction->setEnabled(myPart.sketch.HasProfile());
  myExportStepAction->setEnabled(myPart.extrudeDepth.has_value());
  myEditSketchAction->setEnabled(myPart.extrudeDepth.has_value());
  myCutAction->setEnabled(myPart.extrudeDepth.has_value());
  myBossAction->setEnabled(myPart.extrudeDepth.has_value());
  myRemoveFeatureAction->setEnabled(!myPart.features.empty());
}

void MainWindow::updateTitle()
{
  const QString name = myFilePath.isEmpty() ? "Sin título" : QFileInfo(myFilePath).fileName();
  setWindowTitle(QString("%1%2 - Diseño CAD").arg(name, myModified ? "*" : ""));
}

void MainWindow::showModel()
{
  const Sketch& sketch = myPart.sketch;
  const QString size = QString("%1 × %2 mm").arg(FormatLength(sketch.Width()), FormatLength(sketch.Height()));
  const QString profile = sketch.IsRectangle()
                              ? QString("rectángulo de %1").arg(size)
                              : QString("contorno de %1 lados (%2)").arg(sketch.Points().size()).arg(size);
  // Mostrar el modelo cancela lo que la vista estuviera haciendo (elegir cara, dibujar),
  // así que también se descarta la operación a medias.
  myPendingKind.reset();
  myPendingPlane.reset();
  myFailedFeatures.clear();
  myShape = BuildPart(myPart, &myFailedFeatures);
  if (myPart.extrudeDepth && !myEditSketchAction->isChecked())
  {
    myView->ShowPart(myShape);
    QString text = QString("Pieza: %1, extruido %2 mm").arg(profile, FormatLength(*myPart.extrudeDepth));
    const size_t count = myPart.features.size();
    if (count > 0)
      text += count == 1 ? QString(", 1 operación") : QString(", %1 operaciones").arg(count);
    if (!myFailedFeatures.empty())
    {
      // Puede pasar al cambiar el boceto base: un corte que quedó fuera de la pieza.
      QStringList numbers;
      for (size_t index : myFailedFeatures)
        numbers << QString::number(index + 1);
      text += QString(" · Atención: no se pudo aplicar la operación %1").arg(numbers.join(", "));
    }
    showStatus(text);
  }
  else if (sketch.HasProfile())
  {
    myView->ShowSketch(sketch);
    showStatus(myPart.extrudeDepth
                   ? QString("Editando el boceto: %1 · Doble clic en una cota para cambiarla · "
                             "Esc vuelve a la pieza").arg(profile)
                   : QString("Boceto: %1 · Doble clic en una cota para cambiarla").arg(profile));
  }
  else
  {
    myView->ShowSketch(sketch);
    showStatus("Dibujá un boceto con Boceto → Línea o Rectángulo · "
               "Girar: botón izquierdo · Mover: botón del medio · Zoom: rueda");
  }
}

void MainWindow::showStatus(const QString& text)
{
  myStatusLabel->setText(text);
}
