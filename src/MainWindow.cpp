#include "MainWindow.h"

#include "OccView.h"
#include "RectangleDialog.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent),
    myView(new OccView(this))
{
  setWindowTitle("Diseño CAD");
  setCentralWidget(myView);

  QAction* rectangleAction = new QAction("Rectángulo…", this);
  rectangleAction->setStatusTip("Dibujar un rectángulo con medidas en el plano XY");
  connect(rectangleAction, &QAction::triggered, this, &MainWindow::drawRectangle);

  menuBar()->addMenu("Boceto")->addAction(rectangleAction);
  addToolBar("Boceto")->addAction(rectangleAction);

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
  myView->ShowSketch(mySketch);
  statusBar()->showMessage(QString("Boceto: rectángulo de %1 × %2 mm")
                               .arg(mySketch.Width())
                               .arg(mySketch.Height()));
}
