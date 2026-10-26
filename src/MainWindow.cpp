#include "MainWindow.h"

#include "OccView.h"

#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent),
    myView(new OccView(this))
{
  setWindowTitle("Diseño CAD");
  setCentralWidget(myView);
  statusBar()->showMessage("Girar: botón izquierdo · Mover: botón del medio · Zoom: rueda");
}
