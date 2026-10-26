#pragma once

#include "Part.h"

#include <QString>

#include <vector>

// Historial de la pieza para deshacer y rehacer. Guarda copias completas de la pieza (son
// solo parámetros, así que ocupan poco) y, de cada una, el nombre del cambio que la produjo.
// También sabe si la pieza actual es la que se guardó por última vez.
class History
{
public:
  // Cuántos cambios se pueden deshacer como máximo.
  static const size_t kMaxUndo = 100;

  // Empieza de cero con `part` (pieza nueva o recién abierta), que cuenta como guardada.
  void Reset(const Part& part);
  // Registra que la pieza pasó a ser `part` por el cambio `what` (por ejemplo, "extrusión").
  // Borra lo que se podía rehacer.
  void Commit(const Part& part, const QString& what);

  bool CanUndo() const { return myIndex > 0; }
  bool CanRedo() const { return myIndex + 1 < myStates.size(); }
  // Nombre del cambio que se desharía o se reharía.
  QString UndoText() const;
  QString RedoText() const;
  // Vuelve un paso atrás o adelante y devuelve la pieza de ese momento.
  const Part& Undo();
  const Part& Redo();

  const Part& Current() const { return myStates[myIndex].part; }
  // True si la pieza actual no es la última que se guardó (o la que se abrió).
  bool IsModified() const;
  void MarkSaved();

private:
  struct State
  {
    Part part;
    QString what; // el cambio que llevó a este estado (vacío en el primero)
    int id;
  };

  std::vector<State> myStates{State{Part(), QString(), 0}};
  size_t myIndex = 0;
  int myNextId = 1;
  int mySavedId = 0;
};
