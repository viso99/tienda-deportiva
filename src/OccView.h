#pragma once

#include <QWidget>

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <Graphic3d_Vec2.hxx>
#include <NCollection_Sequence.hxx>
#include <TopoDS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <gp_Pnt2d.hxx>

#include <optional>
#include <vector>

class Sketch;

// Herramientas para dibujar el boceto con el mouse sobre el plano XY.
enum class SketchTool
{
  None,
  Line,      // contorno de líneas, clic a clic
  Rectangle, // rectángulo por dos esquinas opuestas
};

// Vista 3D de Open CASCADE incrustada en un widget de Qt.
// AIS_ViewController traduce el mouse en giro, desplazamiento y zoom.
// Con una herramienta de boceto activa, el botón izquierdo dibuja en lugar de girar.
class OccView : public QWidget, protected AIS_ViewController
{
  Q_OBJECT

public:
  explicit OccView(QWidget* parent = nullptr);

  // Open CASCADE pinta directo sobre la ventana nativa, no Qt.
  QPaintEngine* paintEngine() const override { return nullptr; }

  const Handle(AIS_InteractiveContext)& Context() const { return myContext; }

  // Muestra el boceto (contorno y cotas) en lugar del modelo anterior y lo mira desde arriba.
  // Con doble clic en una cota se emite dimensionActivated.
  void ShowSketch(const Sketch& sketch);
  // Muestra la pieza sólida en lugar del modelo anterior, en vista isométrica.
  void ShowPart(const TopoDS_Shape& part);

  // Empieza a dibujar un perfil nuevo con el mouse, mirando el plano XY desde arriba.
  // Al cerrarlo emite profileFinished; con Esc emite toolCanceled.
  // Mostrar otro modelo (ShowSketch, ShowPart) cancela la herramienta sin emitir nada.
  void StartSketchTool(SketchTool tool);

signals:
  void profileFinished(const std::vector<gp_Pnt2d>& points);
  void toolCanceled();
  // Doble clic en la cota del lado `edgeIndex` del boceto mostrado.
  void dimensionActivated(size_t edgeIndex);
  // Esc sin ninguna herramienta activa.
  void escapePressed();
  // Texto de ayuda para la barra de estado mientras se dibuja.
  void toolHint(const QString& text);

protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

  void handleViewRedraw(const Handle(AIS_InteractiveContext)& ctx,
                        const Handle(V3d_View)& view) override;

private:
  void initViewer();
  void clearModel();
  void flushEvents();
  Graphic3d_Vec2i toViewPoint(const QPointF& pos) const;

  std::optional<gp_Pnt2d> pickOnSketchPlane(const Graphic3d_Vec2i& pixel) const;
  bool isNearFirstPoint(const Graphic3d_Vec2i& pixel) const;
  void addToolPoint(const Graphic3d_Vec2i& pixel);
  void finishProfile(const std::vector<gp_Pnt2d>& points);
  void endTool();
  void updatePreview();
  void updateHint();

  Handle(V3d_Viewer) myViewer;
  Handle(V3d_View) myView;
  Handle(AIS_InteractiveContext) myContext;
  NCollection_Sequence<Handle(AIS_InteractiveObject)> myModelObjects;
  // Cotas del boceto mostrado y el lado que mide cada una.
  std::vector<std::pair<Handle(AIS_InteractiveObject), size_t>> myDimensions;

  SketchTool myTool = SketchTool::None;
  std::vector<gp_Pnt2d> myToolPoints;     // puntos ya marcados
  std::optional<gp_Pnt2d> myCursorPoint;  // posición del mouse sobre el plano, ajustada al mm
  QString myToolError;                    // aviso del último clic rechazado
  NCollection_Sequence<Handle(AIS_InteractiveObject)> myPreviewObjects;
};
