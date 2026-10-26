#pragma once

#include <QDialog>

class QCheckBox;
class QDoubleSpinBox;

// Pide la profundidad de un agujero o corte (o "a través de todo"), o la altura de un saliente.
class FeatureDialog : public QDialog
{
  Q_OBJECT

public:
  FeatureDialog(bool isCut, double depth, QWidget* parent = nullptr);

  double Depth() const;
  bool ThroughAll() const;

private:
  QDoubleSpinBox* myDepth;
  QCheckBox* myThroughAll; // nulo en un saliente
};
