#include "MainWindow.h"

#include "Extrude.h"
#include "OccView.h"
#include "RectangleDialog.h"

#include <QInputDialog>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent),
    myView(new OccView(this)),
    myExtrudeAction(new QAction("Extruir…", this))
{
  setWindowTitle("Diseño CAD");
  setCentralWidget(myView);

  QAction* rectangleAction = new QAction("Rectángulo…", this);
  rectangleAction->setStatusTip("Dibujar un rectángulo con medidas en el plano XY");
  connect(rectangleAction, &QAction::triggered, this, &MainWindow::drawRectangle);

  myExtrudeAction->setStatusTip("Convertir el boceto en una pieza 3D estirándolo en Z");
  myExtrudeAction->setEnabled(false);
  connect(myExtrudeAction, &QAction::triggered, this, &MainWindow::extrude);

  menuBar()->addMenu("Boceto")->addAction(rectangleAction);
  menuBar()->addMenu("Operaciones")->addAction(myExtrudeAction);
  QToolBar* toolBar = addToolBar("Herramientas");
  toolBar->addAction(rectangleAction);
  toolBar->addAction(myExtrudeAction);

  statusBar()->showMessage("Girar: botón izquierdo · Mover: botón del medio · Zoom: rueda");
}

void MainWindow::drawRectangle()
{
  const double width = mySketch.HasProfile() ? mySketch.Width() : 40.0;
  const double height = mySketch.HasProfile() ? mySketch.Height() : 20.0;

  RectangleDialog dialog(width, height, this);
  if (dialog.exec() != QDialog::Accepted)
    return;

  mySketch.SetRectangle(dialog.Width(), dialog.Height());
  myExtrudeAction->setEnabled(true);
  showModel();
}

void MainWindow::extrude()
{
  bool ok = false;
  const double depth = QInputDialog::getDouble(this, "Extruir", "Distancia (mm):",
                                               myExtrudeDepth.value_or(10.0),
                                               0.01, 100000.0, 2, &ok);
  if (!ok)
    return;

  myExtrudeDepth = depth;
  showModel();
}

void MainWindow::showModel()
{
  if (myExtrudeDepth)
  {
    myView->ShowPart(ExtrudeSketch(mySketch, *myExtrudeDepth));
    statusBar()->showMessage(QString("Pieza: %1 × %2 × %3 mm")
                                 .arg(mySketch.Width())
                                 .arg(mySketch.Height())
                                 .arg(*myExtrudeDepth));
  }
  else
  {
    myView->ShowSketch(mySketch);
    statusBar()->showMessage(QString("Boceto: rectángulo de %1 × %2 mm")
                                 .arg(mySketch.Width())
                                 .arg(mySketch.Height()));
  }
}
