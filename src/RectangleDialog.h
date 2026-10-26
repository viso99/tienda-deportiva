#pragma once

#include <QDialog>

class QDoubleSpinBox;

// Pide el ancho y el alto del rectángulo del boceto, en milímetros.
class RectangleDialog : public QDialog
{
  Q_OBJECT

public:
  RectangleDialog(double width, double height, QWidget* parent = nullptr);

  double Width() const;
  double Height() const;

private:
  QDoubleSpinBox* myWidth;
  QDoubleSpinBox* myHeight;
};
