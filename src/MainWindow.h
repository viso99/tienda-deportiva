#pragma once

#include "Sketch.h"

#include <QMainWindow>

class OccView;

// Ventana principal: menú de boceto y vista 3D.
class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);

private:
  void drawRectangle();

  OccView* myView;
  Sketch mySketch;
};
