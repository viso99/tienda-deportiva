#include "OccView.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include "Format.h"
#include "Sketch.h"

#include <AIS_Point.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_Grid.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Graphic3d_Camera.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Prs3d_DimensionAspect.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <TCollection_ExtendedString.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>

#include <cmath>

#ifdef _WIN32
#include <WNT_Window.hxx>
#else
#include <Xw_Window.hxx>
#endif

namespace
{
Aspect_VKeyMouse toMouseButtons(Qt::MouseButtons buttons)
{
  Aspect_VKeyMouse result = Aspect_VKeyMouse_NONE;
  if (buttons & Qt::LeftButton)   result |= Aspect_VKeyMouse_LeftButton;
  if (buttons & Qt::MiddleButton) result |= Aspect_VKeyMouse_MiddleButton;
  if (buttons & Qt::RightButton)  result |= Aspect_VKeyMouse_RightButton;
  return result;
}

Aspect_VKeyFlags toKeyFlags(Qt::KeyboardModifiers modifiers)
{
  Aspect_VKeyFlags result = Aspect_VKeyFlags_NONE;
  if (modifiers & Qt::ShiftModifier)   result |= Aspect_VKeyFlags_SHIFT;
  if (modifiers & Qt::ControlModifier) result |= Aspect_VKeyFlags_CTRL;
  if (modifiers & Qt::AltModifier)     result |= Aspect_VKeyFlags_ALT;
  return result;
}

// Los puntos dibujados con el mouse se ajustan a este paso (mm).
const double kSnapStep = 1.0;
// Distancia en píxeles para "enganchar" el primer punto y cerrar el contorno.
const int kCloseDistancePx = 10;

double snap(double value)
{
  return std::round(value / kSnapStep) * kSnapStep;
}

gp_Pnt to3d(const gp_Pnt2d& point)
{
  return gp_Pnt(point.X(), point.Y(), 0.0);
}

} // namespace

OccView::OccView(QWidget* parent)
  : QWidget(parent)
{
  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_NoSystemBackground);
  setAttribute(Qt::WA_NativeWindow);
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);
}

void OccView::initViewer()
{
  Handle(Aspect_DisplayConnection) display = new Aspect_DisplayConnection();
  Handle(OpenGl_GraphicDriver) driver = new OpenGl_GraphicDriver(display);

  myViewer = new V3d_Viewer(driver);
  myViewer->SetDefaultLights();
  myViewer->SetLightOn();
  myViewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);
  // Grilla cada 10 mm, como referencia para dibujar el boceto.
  myViewer->SetRectangularGridValues(0.0, 0.0, 10.0, 10.0, 0.0);
  myViewer->Grid()->SetColors(Quantity_Color(0.30, 0.33, 0.38, Quantity_TOC_RGB),
                              Quantity_Color(0.45, 0.48, 0.54, Quantity_TOC_RGB));

  myContext = new AIS_InteractiveContext(myViewer);

  myView = myViewer->CreateView();
#ifdef _WIN32
  Handle(Aspect_Window) window = new WNT_Window((Aspect_Handle)winId());
#else
  Handle(Aspect_Window) window = new Xw_Window(display, (Aspect_Drawable)winId());
#endif
  myView->SetWindow(window);
  if (!window->IsMapped())
    window->Map();

  myView->SetBackgroundColor(Quantity_Color(0.20, 0.22, 0.26, Quantity_TOC_RGB));
  myView->SetProj(V3d_XposYnegZpos);

  // Cubo de orientación en la esquina, como en SolidWorks o NX.
  Handle(AIS_ViewCube) viewCube = new AIS_ViewCube();
  viewCube->SetSize(55);
  viewCube->SetTransformPersistence(new Graphic3d_TransformPers(
      Graphic3d_TMF_TriedronPers, Aspect_TOTP_RIGHT_UPPER, Graphic3d_Vec2i(85, 85)));
  myContext->Display(viewCube, Standard_False);

  myView->MustBeResized();
}

void OccView::ShowSketch(const Sketch& sketch)
{
  clearModel();

  if (sketch.HasProfile())
  {
    Handle(AIS_Shape) outline = new AIS_Shape(sketch.Wire());
    outline->SetColor(Quantity_NOC_DEEPSKYBLUE1);
    outline->SetWidth(2.0);
    myModelObjects.Append(outline);

    // Flechas y separación proporcionales al boceto, para que no tapen piezas chicas.
    const double size = Max(sketch.Width(), sketch.Height());
    Handle(Prs3d_DimensionAspect) dimAspect = new Prs3d_DimensionAspect();
    dimAspect->MakeArrows3d(Standard_False);
    dimAspect->ArrowAspect()->SetLength(0.04 * size);
    dimAspect->SetCommonColor(Quantity_NOC_GREEN2);

    // Los lados vienen en sentido antihorario: con separación negativa, la cota queda afuera.
    const gp_Pln sketchPlane(gp::XOY());
    for (const Sketch::DimensionedEdge& dimensioned : sketch.DimensionedEdges())
    {
      Handle(PrsDim_LengthDimension) dimension =
          new PrsDim_LengthDimension(dimensioned.edge, sketchPlane);
      dimension->SetDimensionAspect(dimAspect);
      dimension->SetFlyout(-0.08 * size - 3.0);
      // El texto lo armamos nosotros: redondeado y con coma, igual que en el resto de la app.
      const QString text = FormatLength(sketch.EdgeLength(dimensioned.index));
      dimension->SetCustomValue(TCollection_ExtendedString(text.toUtf8().constData(), Standard_True));
      myModelObjects.Append(dimension);
      myDimensions.emplace_back(dimension, dimensioned.index);
    }

    for (const Handle(AIS_InteractiveObject)& object : myModelObjects)
      myContext->Display(object, Standard_False);
  }

  myView->SetProj(V3d_Zpos);
  myView->FitAll(0.3, Standard_False);
  update();
}

void OccView::ShowPart(const TopoDS_Shape& part)
{
  clearModel();

  Handle(AIS_Shape) solid = new AIS_Shape(part);
  solid->SetDisplayMode(AIS_Shaded);
  solid->SetMaterial(Graphic3d_NameOfMaterial_Steel);
  solid->SetColor(Quantity_NOC_LIGHTSTEELBLUE);
  // Aristas visibles sobre las caras, como en SolidWorks ("sombreado con aristas").
  solid->Attributes()->SetFaceBoundaryDraw(Standard_True);
  solid->Attributes()->FaceBoundaryAspect()->SetColor(Quantity_NOC_BLACK);
  myModelObjects.Append(solid);
  myContext->Display(solid, Standard_False);

  myView->SetProj(V3d_XposYnegZpos);
  myView->FitAll(0.3, Standard_False);
  update();
}

void OccView::StartSketchTool(SketchTool tool)
{
  clearModel();
  myTool = tool;

  // Plano XY visto desde arriba, con el origen abajo a la izquierda: entra por lo menos
  // la zona de X -15 a 125 mm e Y -10 a 90 mm, sea cual sea la forma de la ventana.
  const double regionWidth = 140.0;
  const double regionHeight = 100.0;
  const gp_Pnt center(55.0, 40.0, 0.0);
  myView->SetProj(V3d_Zpos);
  myView->Camera()->SetEyeAndCenter(center.Translated(gp_Vec(0, 0, 500)), center);
  myView->Camera()->SetUp(gp::DY());
  const double aspect = height() > 0 ? double(width()) / height() : 1.0;
  // SetSize fija el lado más largo de la vista.
  myView->SetSize(aspect >= 1.0 ? Max(regionWidth, regionHeight * aspect)
                                : Max(regionHeight, regionWidth / aspect));
  myView->ZFitAll();
  myView->Invalidate();

  updatePreview();
  updateHint();
  setFocus();
  update();
}

void OccView::clearModel()
{
  if (myView.IsNull())
    initViewer();

  endTool();
  for (const Handle(AIS_InteractiveObject)& object : myModelObjects)
    myContext->Remove(object, Standard_False);
  myModelObjects.Clear();
  myDimensions.clear();
}

void OccView::paintEvent(QPaintEvent*)
{
  if (myView.IsNull())
    initViewer();
  myView->InvalidateImmediate();
  flushEvents();
}

void OccView::resizeEvent(QResizeEvent*)
{
  if (!myView.IsNull())
    myView->MustBeResized();
}

void OccView::mousePressEvent(QMouseEvent* event)
{
  if (myTool != SketchTool::None && event->button() == Qt::LeftButton)
  {
    addToolPoint(toViewPoint(event->position()));
    return;
  }
  if (myTool != SketchTool::None)
  {
    // Mientras se dibuja, el botón izquierdo no gira la vista: así el plano queda de frente.
    if (UpdateMouseButtons(toViewPoint(event->position()),
                           toMouseButtons(event->buttons() & ~Qt::LeftButton),
                           toKeyFlags(event->modifiers()), false))
      update();
    return;
  }
  if (UpdateMouseButtons(toViewPoint(event->position()), toMouseButtons(event->buttons()),
                         toKeyFlags(event->modifiers()), false))
    update();
}

void OccView::mouseReleaseEvent(QMouseEvent* event)
{
  Qt::MouseButtons buttons = event->buttons();
  if (myTool != SketchTool::None)
    buttons &= ~Qt::LeftButton;
  if (UpdateMouseButtons(toViewPoint(event->position()), toMouseButtons(buttons),
                         toKeyFlags(event->modifiers()), false))
    update();
}

void OccView::mouseMoveEvent(QMouseEvent* event)
{
  Qt::MouseButtons buttons = event->buttons();
  if (myTool != SketchTool::None)
  {
    buttons &= ~Qt::LeftButton;
    myCursorPoint = pickOnSketchPlane(toViewPoint(event->position()));
    updatePreview();
    updateHint();
    update();
  }
  if (UpdateMousePosition(toViewPoint(event->position()), toMouseButtons(buttons),
                          toKeyFlags(event->modifiers()), false))
    update();
}

void OccView::mouseDoubleClickEvent(QMouseEvent* event)
{
  if (myTool != SketchTool::None || event->button() != Qt::LeftButton)
    return;

  // La cota que está bajo el mouse (la que Open CASCADE resalta al pasar por encima).
  myContext->MoveTo(toViewPoint(event->position()).x(), toViewPoint(event->position()).y(),
                    myView, Standard_False);
  if (!myContext->HasDetected())
    return;
  const Handle(AIS_InteractiveObject) detected = myContext->DetectedInteractive();
  for (const auto& [dimension, edgeIndex] : myDimensions)
  {
    if (dimension == detected)
    {
      emit dimensionActivated(edgeIndex);
      return;
    }
  }
}

void OccView::wheelEvent(QWheelEvent* event)
{
  if (UpdateZoom(Aspect_ScrollDelta(toViewPoint(event->position()),
                                    event->angleDelta().y() / 8.0)))
    update();
}

void OccView::keyPressEvent(QKeyEvent* event)
{
  if (myTool == SketchTool::None)
  {
    if (event->key() == Qt::Key_Escape)
      emit escapePressed();
    else
      QWidget::keyPressEvent(event);
    return;
  }

  switch (event->key())
  {
    case Qt::Key_Escape:
      endTool();
      emit toolCanceled();
      break;
    case Qt::Key_Backspace:
      if (!myToolPoints.empty())
        myToolPoints.pop_back();
      myToolError.clear();
      updatePreview();
      updateHint();
      update();
      break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
      if (myTool == SketchTool::Line && myToolPoints.size() >= 3)
        finishProfile(myToolPoints);
      break;
    default:
      QWidget::keyPressEvent(event);
  }
}

std::optional<gp_Pnt2d> OccView::pickOnSketchPlane(const Graphic3d_Vec2i& pixel) const
{
  if (myView.IsNull())
    return std::nullopt;
  Standard_Real x, y, z, dx, dy, dz;
  myView->ConvertWithProj(pixel.x(), pixel.y(), x, y, z, dx, dy, dz);
  if (std::abs(dz) < 1e-9)
    return std::nullopt; // la vista mira el plano de canto
  const double t = -z / dz;
  return gp_Pnt2d(snap(x + t * dx), snap(y + t * dy));
}

bool OccView::isNearFirstPoint(const Graphic3d_Vec2i& pixel) const
{
  if (myToolPoints.empty())
    return false;
  Standard_Integer px = 0, py = 0;
  myView->Convert(myToolPoints.front().X(), myToolPoints.front().Y(), 0.0, px, py);
  return std::hypot(px - pixel.x(), py - pixel.y()) <= kCloseDistancePx;
}

void OccView::addToolPoint(const Graphic3d_Vec2i& pixel)
{
  const std::optional<gp_Pnt2d> point = pickOnSketchPlane(pixel);
  if (!point)
    return;
  myToolError.clear();

  if (myTool == SketchTool::Rectangle)
  {
    if (myToolPoints.empty())
    {
      myToolPoints.push_back(*point);
    }
    else
    {
      const gp_Pnt2d& a = myToolPoints.front();
      const std::vector<gp_Pnt2d> corners = {
          a, gp_Pnt2d(point->X(), a.Y()), *point, gp_Pnt2d(a.X(), point->Y())};
      if (Sketch::IsValidProfile(corners))
      {
        finishProfile(corners);
        return;
      }
      myToolError = "El rectángulo no puede tener ancho ni alto cero.";
    }
  }
  else if (myToolPoints.size() >= 3 && isNearFirstPoint(pixel))
  {
    finishProfile(myToolPoints);
    return;
  }
  else if (!myToolPoints.empty() && myToolPoints.back().Distance(*point) < kSnapStep / 2)
  {
    // Clic repetido en el mismo punto: no agrega un lado de largo cero.
  }
  else
  {
    myToolPoints.push_back(*point);
  }

  updatePreview();
  updateHint();
  update();
}

void OccView::finishProfile(const std::vector<gp_Pnt2d>& points)
{
  if (!Sketch::IsValidProfile(points))
  {
    myToolError = "El contorno se cruza consigo mismo o no encierra un área. "
                  "Borrá puntos con Retroceso o empezá de nuevo con Esc.";
    updateHint();
    return;
  }
  const std::vector<gp_Pnt2d> profile = points;
  endTool();
  emit profileFinished(profile);
}

void OccView::endTool()
{
  myTool = SketchTool::None;
  myToolPoints.clear();
  myCursorPoint.reset();
  myToolError.clear();
  if (myContext.IsNull())
    return;
  for (const Handle(AIS_InteractiveObject)& object : myPreviewObjects)
    myContext->Remove(object, Standard_False);
  myPreviewObjects.Clear();
  myView->Invalidate();
}

void OccView::updatePreview()
{
  for (const Handle(AIS_InteractiveObject)& object : myPreviewObjects)
    myContext->Remove(object, Standard_False);
  myPreviewObjects.Clear();

  // Lo ya dibujado (azul) y lo que se agregaría con el próximo clic (gris).
  std::vector<gp_Pnt2d> fixed;
  std::vector<gp_Pnt2d> pending;
  if (myTool == SketchTool::Line)
  {
    fixed = myToolPoints;
    if (!myToolPoints.empty() && myCursorPoint)
      pending = {myToolPoints.back(), *myCursorPoint};
  }
  else if (myTool == SketchTool::Rectangle && !myToolPoints.empty() && myCursorPoint)
  {
    const gp_Pnt2d& a = myToolPoints.front();
    const gp_Pnt2d& c = *myCursorPoint;
    pending = {a, gp_Pnt2d(c.X(), a.Y()), c, gp_Pnt2d(a.X(), c.Y()), a};
  }

  auto addPolyline = [this](const std::vector<gp_Pnt2d>& points, Quantity_NameOfColor color) {
    BRepBuilderAPI_MakePolygon polyline;
    for (const gp_Pnt2d& point : points)
      polyline.Add(to3d(point));
    if (!polyline.IsDone())
      return; // menos de dos puntos distintos
    Handle(AIS_Shape) shape = new AIS_Shape(polyline.Wire());
    shape->SetColor(color);
    shape->SetWidth(2.0);
    myPreviewObjects.Append(shape);
  };
  addPolyline(fixed, Quantity_NOC_DEEPSKYBLUE1);
  addPolyline(pending, Quantity_NOC_GRAY70);

  if (myCursorPoint)
  {
    Handle(AIS_Point) marker = new AIS_Point(new Geom_CartesianPoint(to3d(*myCursorPoint)));
    marker->SetMarker(Aspect_TOM_PLUS);
    marker->SetColor(Quantity_NOC_YELLOW);
    myPreviewObjects.Append(marker);
  }

  for (const Handle(AIS_InteractiveObject)& object : myPreviewObjects)
    myContext->Display(object, Standard_False);
  myView->Invalidate();
}

void OccView::updateHint()
{
  QString position;
  if (myCursorPoint)
    position = QString("X %1  Y %2 mm").arg(FormatLength(myCursorPoint->X()), FormatLength(myCursorPoint->Y()));

  QString text;
  if (myTool == SketchTool::Line)
  {
    if (myToolPoints.empty())
    {
      text = "Línea: clic para el primer punto";
    }
    else
    {
      text = myToolPoints.size() >= 3
                 ? "Línea: clic para el siguiente punto, o en el primero (o Enter) para cerrar"
                 : "Línea: clic para el siguiente punto";
      if (myCursorPoint)
        position += QString(" · largo %1 mm").arg(FormatLength(myToolPoints.back().Distance(*myCursorPoint)));
    }
  }
  else if (myTool == SketchTool::Rectangle)
  {
    if (myToolPoints.empty())
    {
      text = "Rectángulo: clic en la primera esquina";
    }
    else
    {
      text = "Rectángulo: clic en la esquina opuesta";
      if (myCursorPoint)
        position += QString(" · %1 × %2 mm")
                        .arg(FormatLength(std::abs(myCursorPoint->X() - myToolPoints.front().X())),
                             FormatLength(std::abs(myCursorPoint->Y() - myToolPoints.front().Y())));
    }
  }
  else
  {
    return;
  }

  if (!position.isEmpty())
    text += " · " + position;
  text += " · Retroceso borra el último punto · Esc cancela";
  if (!myToolError.isEmpty())
    text = myToolError;
  emit toolHint(text);
}

void OccView::handleViewRedraw(const Handle(AIS_InteractiveContext)& ctx,
                               const Handle(V3d_View)& view)
{
  AIS_ViewController::handleViewRedraw(ctx, view);
  // Animaciones (por ejemplo, al hacer clic en el cubo de orientación) piden otro cuadro.
  if (toAskNextFrame())
    update();
}

void OccView::flushEvents()
{
  if (!myView.IsNull())
    FlushViewEvents(myContext, myView, true);
}

Graphic3d_Vec2i OccView::toViewPoint(const QPointF& pos) const
{
  const qreal ratio = devicePixelRatioF();
  return Graphic3d_Vec2i(int(pos.x() * ratio), int(pos.y() * ratio));
}
