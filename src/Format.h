#pragma once

#include <QString>

// Medida en mm para mostrar al usuario: hasta 2 decimales, sin ceros de más y con coma
// decimal (por ejemplo "40", "28,28", "12,5").
QString FormatLength(double millimetres);
