#pragma once

#include <QWidget>

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <Graphic3d_Vec2.hxx>
#include <NCollection_Sequence.hxx>
#include <TopoDS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <gp.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pnt2d.hxx>

#include <optional>
#include <vector>

class AIS_Shape;
class Sketch;

// Herramientas para dibujar el boceto con el mouse sobre un plano (el XY o una cara).
enum class SketchTool
{
  None,
  Line,      // contorno de líneas, clic a clic
  Arc,       // como Línea, pero cada lado es un arco por tres puntos; se mezclan en un mismo contorno
  Rectangle, // rectángulo por dos esquinas opuestas
  Circle,    // círculo por centro y un punto del borde
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

  // Empieza a dibujar un perfil nuevo con el mouse sobre `plane`, mirándolo de frente.
  // Los puntos se entregan en coordenadas del plano. Si se pasa `context` (la pieza),
  // se muestra semitransparente detrás del boceto. Al cerrar el perfil emite
  // profileFinished; con Esc emite toolCanceled. Mostrar otro modelo (ShowSketch,
  // ShowPart) cancela la herramienta sin emitir nada.
  void StartSketchTool(SketchTool tool, const gp_Ax3& plane = gp_Ax3(gp::XOY()),
                       const TopoDS_Shape& context = TopoDS_Shape());
  // Con la pieza mostrada (ShowPart), espera un clic en una cara plana y emite facePicked
  // con su plano de boceto. Se puede girar la vista arrastrando. Esc emite toolCanceled.
  void StartFacePick();
  // True mientras se dibuja un contorno con Línea o Arco y ya hay puntos marcados.
  bool IsDrawingChain() const;
  // Mientras se dibuja, borra el último punto marcado (lo mismo que Retroceso). Devuelve
  // false si no hay herramienta activa o todavía no se marcó ningún punto.
  bool RemoveLastToolPoint();
  // Pasa de Línea a Arco (o al revés) sin perder lo dibujado: el próximo lado usa la nueva.
  void SwitchChainTool(SketchTool tool);

signals:
  void profileFinished(const Sketch& profile);
  void toolCanceled();
  void facePicked(const gp_Ax3& plane);
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
  void addChainPoint(const gp_Pnt2d& point, bool nearFirst);
  void finishProfile(const Sketch& profile);
  void finishChain(const std::vector<double>& bulges);
  void endTool();
  void endFacePick();
  void pickFace(const Graphic3d_Vec2i& pixel);
  gp_Pnt toWorld(const gp_Pnt2d& point) const;
  void updatePreview();
  void updateHint();

  Handle(V3d_Viewer) myViewer;
  Handle(V3d_View) myView;
  Handle(AIS_InteractiveContext) myContext;
  NCollection_Sequence<Handle(AIS_InteractiveObject)> myModelObjects;
  // Cotas del boceto mostrado y el lado que mide cada una.
  std::vector<std::pair<Handle(AIS_InteractiveObject), size_t>> myDimensions;

  Handle(AIS_Shape) myPartObject;          // la pieza mostrada por ShowPart
  bool myPickingFace = false;
  Graphic3d_Vec2i myPressPoint;            // dónde se apretó el botón izquierdo

  SketchTool myTool = SketchTool::None;
  gp_Ax3 mySketchPlane;                    // plano donde se dibuja
  std::vector<gp_Pnt2d> myToolPoints;     // puntos ya marcados
  std::vector<double> myToolBulges;       // en Línea y Arco, el bulge de cada lado ya dibujado
  std::optional<gp_Pnt2d> myArcEnd;       // en Arco, el extremo ya elegido (falta el punto de paso)
  bool myArcCloses = false;               // el extremo del arco es el primer punto: cierra el contorno
  std::optional<gp_Pnt2d> myCursorPoint;  // posición del mouse sobre el plano, ajustada al mm
  QString myToolError;                    // aviso del último clic rechazado
  NCollection_Sequence<Handle(AIS_InteractiveObject)> myPreviewObjects;
};
