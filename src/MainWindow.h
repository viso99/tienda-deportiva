#pragma once

#include "Sketch.h"

#include <QMainWindow>

#include <optional>

class OccView;
class QAction;

// Ventana principal: menús de boceto y operaciones, y vista 3D.
// La pieza es paramétrica: boceto + distancia de extrusión. Si cambia el boceto, se recalcula.
class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);

private:
  void drawRectangle();
  void extrude();
  void showModel();

  OccView* myView;
  QAction* myExtrudeAction;
  Sketch mySketch;
  std::optional<double> myExtrudeDepth;
};
