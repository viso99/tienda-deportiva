#include "FeatureDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>

FeatureDialog::FeatureDialog(bool isCut, double depth, QWidget* parent)
  : QDialog(parent),
    myDepth(new QDoubleSpinBox(this)),
    myThroughAll(isCut ? new QCheckBox("A través de todo", this) : nullptr)
{
  setWindowTitle(isCut ? "Agujero o corte" : "Saliente");

  myDepth->setRange(0.01, 100000.0);
  myDepth->setDecimals(2);
  myDepth->setSuffix(" mm");
  myDepth->setValue(depth);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  auto* layout = new QFormLayout(this);
  layout->addRow(isCut ? "Profundidad:" : "Altura:", myDepth);
  if (myThroughAll)
  {
    connect(myThroughAll, &QCheckBox::toggled, myDepth, &QWidget::setDisabled);
    layout->addRow(myThroughAll);
  }
  layout->addRow(buttons);
}

double FeatureDialog::Depth() const
{
  return myDepth->value();
}

bool FeatureDialog::ThroughAll() const
{
  return myThroughAll && myThroughAll->isChecked();
}
