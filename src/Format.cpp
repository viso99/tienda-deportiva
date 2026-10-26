#include "Format.h"

QString FormatLength(double millimetres)
{
  // 'f' con 2 decimales redondea; después se sacan los ceros finales.
  QString text = QString::number(millimetres, 'f', 2);
  while (text.contains('.') && (text.endsWith('0') || text.endsWith('.')))
    text.chop(1);
  if (text == "-0")
    text = "0";
  return text.replace('.', ',');
}
