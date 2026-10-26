#include "OccView.h"

#include <QMouseEvent>
#include <QWheelEvent>

#include "Sketch.h"

#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Prs3d_DimensionAspect.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <gp_Pln.hxx>

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
  if (myView.IsNull())
    initViewer();

  for (const Handle(AIS_InteractiveObject)& object : mySketchObjects)
    myContext->Remove(object, Standard_False);
  mySketchObjects.Clear();

  if (sketch.HasProfile())
  {
    Handle(AIS_Shape) outline = new AIS_Shape(sketch.Wire());
    outline->SetColor(Quantity_NOC_DEEPSKYBLUE1);
    outline->SetWidth(2.0);
    mySketchObjects.Append(outline);

    // Flechas proporcionales al boceto, para que no tapen piezas chicas.
    Handle(Prs3d_DimensionAspect) dimAspect = new Prs3d_DimensionAspect();
    dimAspect->MakeArrows3d(Standard_False);
    dimAspect->ArrowAspect()->SetLength(0.04 * Max(sketch.Width(), sketch.Height()));
    dimAspect->SetCommonColor(Quantity_NOC_GREEN2);

    const gp_Pln sketchPlane(gp::XOY());
    Handle(PrsDim_LengthDimension) widthDim =
        new PrsDim_LengthDimension(sketch.WidthEdge(), sketchPlane);
    widthDim->SetDimensionAspect(dimAspect);
    widthDim->SetFlyout(-0.25 * sketch.Height() - 5.0);
    mySketchObjects.Append(widthDim);

    Handle(PrsDim_LengthDimension) heightDim =
        new PrsDim_LengthDimension(sketch.HeightEdge(), sketchPlane);
    heightDim->SetDimensionAspect(dimAspect);
    heightDim->SetFlyout(-0.25 * sketch.Width() - 5.0);
    mySketchObjects.Append(heightDim);

    for (const Handle(AIS_InteractiveObject)& object : mySketchObjects)
      myContext->Display(object, Standard_False);
  }

  myView->SetProj(V3d_Zpos);
  myView->FitAll(0.3, Standard_False);
  update();
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
  if (UpdateMouseButtons(toViewPoint(event->position()), toMouseButtons(event->buttons()),
                         toKeyFlags(event->modifiers()), false))
    update();
}

void OccView::mouseReleaseEvent(QMouseEvent* event)
{
  if (UpdateMouseButtons(toViewPoint(event->position()), toMouseButtons(event->buttons()),
                         toKeyFlags(event->modifiers()), false))
    update();
}

void OccView::mouseMoveEvent(QMouseEvent* event)
{
  if (UpdateMousePosition(toViewPoint(event->position()), toMouseButtons(event->buttons()),
                          toKeyFlags(event->modifiers()), false))
    update();
}

void OccView::wheelEvent(QWheelEvent* event)
{
  if (UpdateZoom(Aspect_ScrollDelta(toViewPoint(event->position()),
                                    event->angleDelta().y() / 8.0)))
    update();
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
