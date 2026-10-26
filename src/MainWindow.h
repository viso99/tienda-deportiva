#pragma once

#include "OccView.h"
#include "PartDocument.h"

#include <QMainWindow>

class QAction;
class QLabel;

// Ventana principal: menús de archivo, boceto y operaciones, y vista 3D.
// La pieza es paramétrica: boceto + distancia de extrusión + operaciones (agujeros, cortes
// y salientes). Si cambia algo, se recalcula.
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
  void editDimension(size_t edgeIndex);
  void extrude();
  void startFeature(Feature::Kind kind);
  void sketchOnFace(const gp_Ax3& plane);
  void addFeature(const std::vector<gp_Pnt2d>& points);
  void removeLastFeature();
  void cancelTool();

  void setPart(const Part& part, const QString& filePath);
  void markModified();
  bool confirmDiscardChanges();
  bool writeTo(const QString& filePath);
  void updateActions();
  void updateTitle();
  void showModel();
  void showStatus(const QString& text);

  OccView* myView;
  QAction* myExtrudeAction;
  QAction* myExportStepAction;
  QAction* myEditSketchAction;
  QAction* myCutAction;
  QAction* myBossAction;
  QAction* myRemoveFeatureAction;
  QLabel* myStatusLabel;
  Part myPart;
  QString myFilePath;
  bool myModified = false;

  // Operación en curso: el tipo, desde que se elige en el menú, y el plano, desde que se
  // hace clic en la cara. Mientras hay plano, Línea y Rectángulo dibujan sobre él.
  std::optional<Feature::Kind> myPendingKind;
  std::optional<gp_Ax3> myPendingPlane;
  TopoDS_Shape myShape; // la pieza calculada que se muestra
  std::vector<size_t> myFailedFeatures;
};
