#pragma once

#include "OccView.h"
#include "PartDocument.h"

#include <QMainWindow>

class QAction;

// Ventana principal: menús de archivo, boceto y operaciones, y vista 3D.
// La pieza es paramétrica: boceto + distancia de extrusión. Si cambia el boceto, se recalcula.
class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  void newPart();
  void openPart();
  bool savePart();
  bool savePartAs();
  void exportStep();

  void drawRectangle();
  void startSketchTool(SketchTool tool);
  void setProfile(const std::vector<gp_Pnt2d>& points);
  void extrude();

  void setPart(const Part& part, const QString& filePath);
  void markModified();
  bool confirmDiscardChanges();
  bool writeTo(const QString& filePath);
  void updateActions();
  void updateTitle();
  void showModel();

  OccView* myView;
  QAction* myExtrudeAction;
  QAction* myExportStepAction;
  Part myPart;
  QString myFilePath;
  bool myModified = false;
};
