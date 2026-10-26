#include "RectangleDialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>

namespace
{
QDoubleSpinBox* makeLengthBox(double value, QWidget* parent)
{
  auto* box = new QDoubleSpinBox(parent);
  box->setRange(0.01, 100000.0);
  box->setDecimals(2);
  box->setSuffix(" mm");
  box->setValue(value);
  return box;
}
} // namespace

RectangleDialog::RectangleDialog(double width, double height, QWidget* parent)
  : QDialog(parent),
    myWidth(makeLengthBox(width, this)),
    myHeight(makeLengthBox(height, this))
{
  setWindowTitle("Rectángulo");

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  auto* layout = new QFormLayout(this);
  layout->addRow("Ancho:", myWidth);
  layout->addRow("Alto:", myHeight);
  layout->addRow(buttons);
}

double RectangleDialog::Width() const
{
  return myWidth->value();
}

double RectangleDialog::Height() const
{
  return myHeight->value();
}
