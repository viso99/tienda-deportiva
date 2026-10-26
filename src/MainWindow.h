#pragma once

#include <QMainWindow>

class OccView;

// Ventana principal: por ahora solo contiene la vista 3D.
class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);

private:
  OccView* myView;
};
