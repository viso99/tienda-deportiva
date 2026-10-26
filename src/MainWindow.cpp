#include "MainWindow.h"

#include "Extrude.h"
#include "OccView.h"
#include "RectangleDialog.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

namespace
{
const char* const kPartFilter = "Pieza (*.cadp)";
const char* const kStepFilter = "STEP (*.step *.stp)";
} // namespace

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent),
    myView(new OccView(this)),
    myExtrudeAction(new QAction("Extruir…", this)),
    myExportStepAction(new QAction("Exportar a STEP…", this))
{
  setCentralWidget(myView);

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

  QAction* rectangleAction = new QAction("Rectángulo…", this);
  rectangleAction->setStatusTip("Dibujar un rectángulo con medidas en el plano XY");
  connect(rectangleAction, &QAction::triggered, this, &MainWindow::drawRectangle);

  myExtrudeAction->setStatusTip("Convertir el boceto en una pieza 3D estirándolo en Z");
  connect(myExtrudeAction, &QAction::triggered, this, &MainWindow::extrude);

  menuBar()->addMenu("Boceto")->addAction(rectangleAction);
  menuBar()->addMenu("Operaciones")->addAction(myExtrudeAction);
  QToolBar* toolBar = addToolBar("Herramientas");
  toolBar->addAction(rectangleAction);
  toolBar->addAction(myExtrudeAction);

  updateActions();
  updateTitle();
  statusBar()->showMessage("Girar: botón izquierdo · Mover: botón del medio · Zoom: rueda");
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
  if (!ExportStep(ExtrudeSketch(myPart.sketch, *myPart.extrudeDepth), path, &error))
  {
    QMessageBox::warning(this, "Exportar a STEP", QString("No se pudo exportar la pieza.\n\n%1").arg(error));
    return;
  }
  statusBar()->showMessage(QString("Exportado a %1").arg(QFileInfo(path).fileName()));
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

void MainWindow::extrude()
{
  bool ok = false;
  const double depth = QInputDialog::getDouble(this, "Extruir", "Distancia (mm):",
                                               myPart.extrudeDepth.value_or(10.0),
                                               0.01, 100000.0, 2, &ok);
  if (!ok)
    return;

  myPart.extrudeDepth = depth;
  markModified();
  showModel();
}

void MainWindow::setPart(const Part& part, const QString& filePath)
{
  myPart = part;
  myFilePath = filePath;
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
  statusBar()->showMessage(QString("Guardado en %1").arg(QFileInfo(filePath).fileName()));
  return true;
}

void MainWindow::updateActions()
{
  myExtrudeAction->setEnabled(myPart.sketch.HasProfile());
  myExportStepAction->setEnabled(myPart.extrudeDepth.has_value());
}

void MainWindow::updateTitle()
{
  const QString name = myFilePath.isEmpty() ? "Sin título" : QFileInfo(myFilePath).fileName();
  setWindowTitle(QString("%1%2 - Diseño CAD").arg(name, myModified ? "*" : ""));
}

void MainWindow::showModel()
{
  const Sketch& sketch = myPart.sketch;
  if (myPart.extrudeDepth)
  {
    myView->ShowPart(ExtrudeSketch(sketch, *myPart.extrudeDepth));
    statusBar()->showMessage(QString("Pieza: %1 × %2 × %3 mm")
                                 .arg(sketch.Width())
                                 .arg(sketch.Height())
                                 .arg(*myPart.extrudeDepth));
  }
  else if (sketch.HasProfile())
  {
    myView->ShowSketch(sketch);
    statusBar()->showMessage(QString("Boceto: rectángulo de %1 × %2 mm")
                                 .arg(sketch.Width())
                                 .arg(sketch.Height()));
  }
  else
  {
    myView->ShowSketch(sketch);
  }
}
