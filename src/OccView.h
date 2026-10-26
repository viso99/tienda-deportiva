#pragma once

#include <QWidget>

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <Graphic3d_Vec2.hxx>
#include <NCollection_Sequence.hxx>
#include <TopoDS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

class Sketch;

// Vista 3D de Open CASCADE incrustada en un widget de Qt.
// AIS_ViewController traduce el mouse en giro, desplazamiento y zoom.
class OccView : public QWidget, protected AIS_ViewController
{
  Q_OBJECT

public:
  explicit OccView(QWidget* parent = nullptr);

  // Open CASCADE pinta directo sobre la ventana nativa, no Qt.
  QPaintEngine* paintEngine() const override { return nullptr; }

  const Handle(AIS_InteractiveContext)& Context() const { return myContext; }

  // Muestra el boceto (contorno y cotas) en lugar del modelo anterior y lo mira desde arriba.
  void ShowSketch(const Sketch& sketch);
  // Muestra la pieza sólida en lugar del modelo anterior, en vista isométrica.
  void ShowPart(const TopoDS_Shape& part);

protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

  void handleViewRedraw(const Handle(AIS_InteractiveContext)& ctx,
                        const Handle(V3d_View)& view) override;

private:
  void initViewer();
  void clearModel();
  void flushEvents();
  Graphic3d_Vec2i toViewPoint(const QPointF& pos) const;

  Handle(V3d_Viewer) myViewer;
  Handle(V3d_View) myView;
  Handle(AIS_InteractiveContext) myContext;
  NCollection_Sequence<Handle(AIS_InteractiveObject)> myModelObjects;
};
