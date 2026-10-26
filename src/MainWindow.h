#pragma once

#include "History.h"
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
  void setProfile(const Sketch& profile);
  void editDimension(size_t edgeIndex);
  void extrude();
  void startFeature(Feature::Kind kind);
  void sketchOnFace(const gp_Ax3& plane);
  void addFeature(const Sketch& profile);
  void removeLastFeature();
  void undo();
  void redo();
  void cancelTool();

  void setPart(const Part& part, const QString& filePath);
  // Registra el cambio `what` en el historial (para deshacer) y marca la pieza como modificada.
  void markModified(const QString& what);
  // Muestra la pieza del historial después de deshacer o rehacer.
  void restore(const Part& part, const QString& message);
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
  QAction* myUndoAction;
  QAction* myRedoAction;
  QLabel* myStatusLabel;
  Part myPart;
  QString myFilePath;
  History myHistory;

  // Operación en curso: el tipo, desde que se elige en el menú, y el plano, desde que se
  // hace clic en la cara. Mientras hay plano, Línea y Rectángulo dibujan sobre él.
  std::optional<Feature::Kind> myPendingKind;
  std::optional<gp_Ax3> myPendingPlane;
  TopoDS_Shape myShape; // la pieza calculada que se muestra
  std::vector<size_t> myFailedFeatures;
};
