#include "OccView.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include "Format.h"
#include "Part.h"
#include "Sketch.h"

#include <AIS_Point.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_Grid.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Graphic3d_Camera.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Prs3d_DimensionAspect.hxx>
#include <PrsDim_DiameterDimension.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <PrsDim_RadiusDimension.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <TopoDS.hxx>
#include <TCollection_ExtendedString.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
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

// Arcos y círculos suaves: se dibujan con más tramos que los que usa Open CASCADE por defecto.
void smoothCurves(const Handle(AIS_Shape)& shape)
{
  shape->Attributes()->SetDeviationAngle(2.0 * 3.14159265358979323846 / 180.0);
  shape->Attributes()->SetDeviationCoefficient(0.0002);
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
    smoothCurves(outline);
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
      Handle(PrsDim_Dimension) dimension;
      // El texto lo armamos nosotros: redondeado y con coma, igual que en el resto de la app.
      QString text = FormatLength(sketch.DimensionValue(dimensioned.index));
      switch (dimensioned.kind)
      {
        case Sketch::DimensionKind::Length:
        {
          Handle(PrsDim_LengthDimension) length = new PrsDim_LengthDimension(dimensioned.edge, sketchPlane);
          length->SetFlyout(-0.08 * size - 3.0);
          dimension = length;
          break;
        }
        case Sketch::DimensionKind::Radius:
          dimension = new PrsDim_RadiusDimension(
              dimensioned.circle, gp_Pnt(dimensioned.arcMiddle.X(), dimensioned.arcMiddle.Y(), 0.0));
          text = "R" + text;
          break;
        case Sketch::DimensionKind::Diameter:
          dimension = new PrsDim_DiameterDimension(dimensioned.circle, sketchPlane);
          text = QString::fromUtf8("Ø") + text;
          break;
      }
      dimension->SetDimensionAspect(dimAspect);
      // El símbolo (R o Ø) ya va en el texto.
      dimension->SetDisplaySpecialSymbol(PrsDim_DisplaySpecialSymbol_No);
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
  smoothCurves(solid);
  // Aristas visibles sobre las caras, como en SolidWorks ("sombreado con aristas").
  solid->Attributes()->SetFaceBoundaryDraw(Standard_True);
  solid->Attributes()->FaceBoundaryAspect()->SetColor(Quantity_NOC_BLACK);
  myModelObjects.Append(solid);
  myContext->Display(solid, Standard_False);
  myPartObject = solid;

  myView->SetProj(V3d_XposYnegZpos);
  myView->FitAll(0.3, Standard_False);
  update();
}

void OccView::StartSketchTool(SketchTool tool, const gp_Ax3& plane, const TopoDS_Shape& context)
{
  clearModel();
  myTool = tool;
  mySketchPlane = plane;
  // La grilla se dibuja sobre el plano del boceto.
  myViewer->SetPrivilegedPlane(plane);

  if (!context.IsNull())
  {
    Handle(AIS_Shape) part = new AIS_Shape(context);
    part->SetDisplayMode(AIS_Shaded);
    part->SetColor(Quantity_NOC_SLATEGRAY);
    part->SetTransparency(0.5);
    smoothCurves(part);
    myModelObjects.Append(part);
    myContext->Display(part, Standard_False);
    myContext->Deactivate(part); // la pieza es solo referencia: no se selecciona

    // El plano de frente, centrado en la pieza y con toda la pieza a la vista.
    Bnd_Box box;
    BRepBndLib::Add(context, box);
    const gp_Pnt boxCenter = (box.CornerMin().XYZ() + box.CornerMax().XYZ()) / 2.0;
    const gp_Vec normal(plane.Direction());
    const gp_Pnt center = boxCenter.Translated(-normal * gp_Vec(plane.Location(), boxCenter).Dot(normal));
    const double size = std::sqrt(box.SquareExtent());
    myView->Camera()->SetEyeAndCenter(center.Translated(normal * (2.0 * size + 100.0)), center);
    myView->Camera()->SetUp(plane.YDirection());
    myView->SetSize(1.3 * size);
    myView->ZFitAll();
    myView->Invalidate();

    updatePreview();
    updateHint();
    setFocus();
    update();
    return;
  }

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

void OccView::StartFacePick()
{
  if (myPartObject.IsNull())
    return;
  endTool();
  myPickingFace = true;
  // Se seleccionan caras sueltas, no la pieza entera.
  myContext->Deactivate(myPartObject);
  myContext->Activate(myPartObject, AIS_Shape::SelectionMode(TopAbs_FACE));
  emit toolHint("Hacé clic en una cara plana de la pieza · Girar: arrastrar con el botón izquierdo · "
                "Esc cancela");
  setFocus();
  update();
}

bool OccView::RemoveLastToolPoint()
{
  if (myTool == SketchTool::None || (!myArcEnd && myToolPoints.empty()))
    return false;
  if (myArcEnd)
  {
    // Primero se descarta el extremo del arco que espera su punto de paso.
    myArcEnd.reset();
    myArcCloses = false;
  }
  else
  {
    myToolPoints.pop_back();
    if (!myToolBulges.empty())
      myToolBulges.pop_back();
  }
  myToolError.clear();
  updatePreview();
  updateHint();
  update();
  return true;
}

bool OccView::IsDrawingChain() const
{
  return (myTool == SketchTool::Line || myTool == SketchTool::Arc) && !myToolPoints.empty();
}

void OccView::SwitchChainTool(SketchTool tool)
{
  if (!IsDrawingChain() || (tool != SketchTool::Line && tool != SketchTool::Arc))
    return;
  myTool = tool;
  myArcEnd.reset();
  myArcCloses = false;
  myToolError.clear();
  updatePreview();
  updateHint();
  setFocus();
  update();
}

void OccView::endFacePick()
{
  if (!myPickingFace)
    return;
  myPickingFace = false;
  if (!myPartObject.IsNull() && myContext->IsDisplayed(myPartObject))
  {
    myContext->ClearSelected(Standard_False);
    myContext->Deactivate(myPartObject);
    myContext->Activate(myPartObject, 0);
  }
}

void OccView::pickFace(const Graphic3d_Vec2i& pixel)
{
  myContext->MoveTo(pixel.x(), pixel.y(), myView, Standard_False);
  Handle(StdSelect_BRepOwner) owner = Handle(StdSelect_BRepOwner)::DownCast(myContext->DetectedOwner());
  if (owner.IsNull() || owner->Shape().ShapeType() != TopAbs_FACE)
    return;
  const std::optional<gp_Ax3> plane = SketchPlaneFromFace(TopoDS::Face(owner->Shape()));
  if (!plane)
  {
    emit toolHint("Esa cara no es plana: elegí una cara plana · Esc cancela");
    return;
  }
  endFacePick();
  emit facePicked(*plane);
}

void OccView::clearModel()
{
  if (myView.IsNull())
    initViewer();

  endTool();
  endFacePick();
  myPartObject.Nullify();
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
  if (event->button() == Qt::LeftButton)
    myPressPoint = toViewPoint(event->position());
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
  // Un clic (sin arrastrar) elige la cara; arrastrar gira la vista como siempre.
  const Graphic3d_Vec2i releasePoint = toViewPoint(event->position());
  const bool isClick = std::abs(releasePoint.x() - myPressPoint.x()) <= 3
                    && std::abs(releasePoint.y() - myPressPoint.y()) <= 3;
  const bool pick = myPickingFace && event->button() == Qt::LeftButton && isClick;
  Qt::MouseButtons buttons = event->buttons();
  if (myTool != SketchTool::None)
    buttons &= ~Qt::LeftButton;
  if (UpdateMouseButtons(toViewPoint(event->position()), toMouseButtons(buttons),
                         toKeyFlags(event->modifiers()), false))
    update();
  if (pick)
    pickFace(releasePoint);
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
  if (myPickingFace && event->key() == Qt::Key_Escape)
  {
    endFacePick();
    emit toolCanceled();
    return;
  }
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
      RemoveLastToolPoint();
      break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
      // Cierra con una línea recta del último punto al primero.
      if (IsDrawingChain() && !myArcEnd && myToolPoints.size() >= 2)
      {
        std::vector<double> bulges = myToolBulges;
        bulges.push_back(0.0);
        finishChain(bulges);
      }
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
  // Intersección del rayo del mouse con el plano del boceto.
  const gp_Vec normal(mySketchPlane.Direction());
  const gp_Vec ray(dx, dy, dz);
  const double alongNormal = ray.Dot(normal);
  if (std::abs(alongNormal) < 1e-9)
    return std::nullopt; // la vista mira el plano de canto
  const gp_Pnt start(x, y, z);
  const double t = gp_Vec(start, mySketchPlane.Location()).Dot(normal) / alongNormal;
  const gp_Vec local(mySketchPlane.Location(), start.Translated(ray * t));
  return gp_Pnt2d(snap(local.Dot(gp_Vec(mySketchPlane.XDirection()))),
                  snap(local.Dot(gp_Vec(mySketchPlane.YDirection()))));
}

bool OccView::isNearFirstPoint(const Graphic3d_Vec2i& pixel) const
{
  if (myToolPoints.empty())
    return false;
  Standard_Integer px = 0, py = 0;
  const gp_Pnt first = toWorld(myToolPoints.front());
  myView->Convert(first.X(), first.Y(), first.Z(), px, py);
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
      Sketch rectangle;
      if (rectangle.SetPolygon({a, gp_Pnt2d(point->X(), a.Y()), *point, gp_Pnt2d(a.X(), point->Y())}))
      {
        finishProfile(rectangle);
        return;
      }
      myToolError = "El rectángulo no puede tener ancho ni alto cero.";
    }
  }
  else if (myTool == SketchTool::Circle)
  {
    if (myToolPoints.empty())
    {
      myToolPoints.push_back(*point);
    }
    else
    {
      Sketch circle;
      if (circle.SetCircle(myToolPoints.front(), snap(myToolPoints.front().Distance(*point))))
      {
        finishProfile(circle);
        return;
      }
      myToolError = "El círculo no puede tener radio cero.";
    }
  }
  else
  {
    addChainPoint(*point, isNearFirstPoint(pixel));
    if (myTool == SketchTool::None)
      return; // se cerró el contorno
  }

  updatePreview();
  updateHint();
  update();
}

void OccView::addChainPoint(const gp_Pnt2d& point, bool nearFirst)
{
  if (myArcEnd)
  {
    // Segundo clic del arco: el punto por donde pasa.
    const std::optional<double> bulge = Sketch::BulgeThrough(myToolPoints.back(), *myArcEnd, point);
    if (!bulge)
    {
      myToolError = "El punto de paso no puede estar alineado con los extremos del arco. Probá con otro.";
      return;
    }
    if (myArcCloses)
    {
      std::vector<double> bulges = myToolBulges;
      bulges.push_back(*bulge);
      finishChain(bulges);
      return;
    }
    myToolBulges.push_back(*bulge);
    myToolPoints.push_back(*myArcEnd);
    myArcEnd.reset();
    return;
  }

  if (nearFirst && myToolPoints.size() >= 2)
  {
    if (myTool == SketchTool::Arc)
    {
      // El arco termina en el primer punto: falta el punto de paso para cerrar.
      myArcEnd = myToolPoints.front();
      myArcCloses = true;
      return;
    }
    std::vector<double> bulges = myToolBulges;
    bulges.push_back(0.0);
    finishChain(bulges);
    return;
  }
  if (!myToolPoints.empty() && myToolPoints.back().Distance(point) < kSnapStep / 2)
    return; // clic repetido en el mismo punto: no agrega un lado de largo cero

  if (myTool == SketchTool::Arc && !myToolPoints.empty())
  {
    myArcEnd = point;
    myArcCloses = false;
    return;
  }
  if (!myToolPoints.empty())
    myToolBulges.push_back(0.0);
  myToolPoints.push_back(point);
}

void OccView::finishChain(const std::vector<double>& bulges)
{
  Sketch profile;
  if (!profile.SetProfile(myToolPoints, bulges))
  {
    myArcEnd.reset();
    myArcCloses = false;
    myToolError = "El contorno se cruza consigo mismo o no encierra un área. "
                  "Borrá puntos con Retroceso o empezá de nuevo con Esc.";
    updatePreview();
    updateHint();
    update();
    return;
  }
  finishProfile(profile);
}

void OccView::finishProfile(const Sketch& profile)
{
  const Sketch finished = profile;
  endTool();
  emit profileFinished(finished);
}

gp_Pnt OccView::toWorld(const gp_Pnt2d& point) const
{
  return mySketchPlane.Location()
      .Translated(gp_Vec(mySketchPlane.XDirection()) * point.X() + gp_Vec(mySketchPlane.YDirection()) * point.Y());
}

void OccView::endTool()
{
  if (myTool != SketchTool::None && !myViewer.IsNull())
    myViewer->SetPrivilegedPlane(gp_Ax3(gp::XOY()));
  myTool = SketchTool::None;
  myToolPoints.clear();
  myToolBulges.clear();
  myArcEnd.reset();
  myArcCloses = false;
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

  // Un tramo en el plano del boceto: línea recta (bulge 0) o arco.
  auto makeEdge = [this](const gp_Pnt2d& a, const gp_Pnt2d& b, double bulge) {
    if (bulge == 0.0)
      return BRepBuilderAPI_MakeEdge(toWorld(a), toWorld(b)).Edge();
    const Handle(Geom_TrimmedCurve) arc =
        GC_MakeArcOfCircle(toWorld(a), toWorld(Sketch::ArcMiddle(a, b, bulge)), toWorld(b)).Value();
    return BRepBuilderAPI_MakeEdge(arc).Edge();
  };
  auto addEdges = [this](const std::vector<TopoDS_Edge>& edges, Quantity_NameOfColor color) {
    if (edges.empty())
      return;
    BRepBuilderAPI_MakeWire wire;
    for (const TopoDS_Edge& edge : edges)
      wire.Add(edge);
    if (!wire.IsDone())
      return;
    Handle(AIS_Shape) shape = new AIS_Shape(wire.Wire());
    shape->SetColor(color);
    shape->SetWidth(2.0);
    smoothCurves(shape);
    myPreviewObjects.Append(shape);
  };

  // Lo ya dibujado (azul) y lo que se agregaría con el próximo clic (naranja).
  std::vector<TopoDS_Edge> fixed;
  std::vector<TopoDS_Edge> pending;
  if (myTool == SketchTool::Line || myTool == SketchTool::Arc)
  {
    for (size_t i = 0; i + 1 < myToolPoints.size(); ++i)
      fixed.push_back(makeEdge(myToolPoints[i], myToolPoints[i + 1], myToolBulges[i]));
    if (!myToolPoints.empty() && myCursorPoint)
    {
      const gp_Pnt2d& last = myToolPoints.back();
      if (myArcEnd)
      {
        const std::optional<double> bulge = Sketch::BulgeThrough(last, *myArcEnd, *myCursorPoint);
        pending.push_back(makeEdge(last, *myArcEnd, bulge.value_or(0.0)));
      }
      else if (last.Distance(*myCursorPoint) > kSnapStep / 2)
      {
        pending.push_back(makeEdge(last, *myCursorPoint, 0.0));
      }
    }
  }
  else if (myTool == SketchTool::Rectangle && !myToolPoints.empty() && myCursorPoint)
  {
    const gp_Pnt2d& a = myToolPoints.front();
    const gp_Pnt2d& c = *myCursorPoint;
    const std::vector<gp_Pnt2d> corners = {a, gp_Pnt2d(c.X(), a.Y()), c, gp_Pnt2d(a.X(), c.Y())};
    for (size_t i = 0; i < 4; ++i)
      if (corners[i].Distance(corners[(i + 1) % 4]) > kSnapStep / 2)
        pending.push_back(makeEdge(corners[i], corners[(i + 1) % 4], 0.0));
  }
  else if (myTool == SketchTool::Circle && !myToolPoints.empty() && myCursorPoint)
  {
    const double radius = snap(myToolPoints.front().Distance(*myCursorPoint));
    if (radius > 0.0)
    {
      const gp_Circ circle(gp_Ax2(toWorld(myToolPoints.front()), mySketchPlane.Direction(),
                                  mySketchPlane.XDirection()), radius);
      pending.push_back(BRepBuilderAPI_MakeEdge(circle).Edge());
    }
  }
  addEdges(fixed, Quantity_NOC_DEEPSKYBLUE1);
  addEdges(pending, Quantity_NOC_ORANGE);

  if (myCursorPoint)
  {
    Handle(AIS_Point) marker = new AIS_Point(new Geom_CartesianPoint(toWorld(*myCursorPoint)));
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
  const bool canClose = myToolPoints.size() >= 3 || (myToolPoints.size() == 2 && !myToolBulges.empty()
                                                     && myToolBulges.front() != 0.0);
  if (myTool == SketchTool::Line)
  {
    if (myToolPoints.empty())
    {
      text = "Línea: clic para el primer punto";
    }
    else
    {
      text = canClose ? "Línea: clic para el siguiente punto, o en el primero (o Enter) para cerrar"
                      : "Línea: clic para el siguiente punto";
      if (myCursorPoint)
        position += QString(" · largo %1 mm").arg(FormatLength(myToolPoints.back().Distance(*myCursorPoint)));
    }
  }
  else if (myTool == SketchTool::Arc)
  {
    if (myToolPoints.empty())
    {
      text = "Arco: clic para el primer punto";
    }
    else if (!myArcEnd)
    {
      text = myToolPoints.size() >= 2 ? "Arco: clic en el final del arco (en el primer punto, cierra el contorno)"
                                      : "Arco: clic en el final del arco";
    }
    else
    {
      text = "Arco: clic en un punto por donde pasa el arco";
      if (myCursorPoint)
      {
        const std::optional<double> bulge = Sketch::BulgeThrough(myToolPoints.back(), *myArcEnd, *myCursorPoint);
        if (bulge)
        {
          const double chord = myToolPoints.back().Distance(*myArcEnd);
          const double radius = chord * (1.0 + *bulge * *bulge) / (4.0 * std::abs(*bulge));
          position += QString(" · radio %1 mm").arg(FormatLength(radius));
        }
      }
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
  else if (myTool == SketchTool::Circle)
  {
    if (myToolPoints.empty())
    {
      text = "Círculo: clic en el centro";
    }
    else
    {
      text = "Círculo: clic en el borde";
      if (myCursorPoint)
        position += QString(" · radio %1 mm").arg(FormatLength(snap(myToolPoints.front().Distance(*myCursorPoint))));
    }
  }
  else
  {
    return;
  }

  if (!position.isEmpty())
    text += " · " + position;
  if (IsDrawingChain())
    text += " · L: líneas, A: arcos";
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
