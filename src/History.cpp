#include "History.h"

void History::Reset(const Part& part)
{
  myStates = {State{part, QString(), myNextId++}};
  myIndex = 0;
  mySavedId = myStates.front().id;
}

void History::Commit(const Part& part, const QString& what)
{
  myStates.erase(myStates.begin() + myIndex + 1, myStates.end());
  myStates.push_back(State{part, what, myNextId++});
  // Los cambios más viejos se olvidan. Si el estado guardado era uno de ellos, la pieza
  // queda como modificada, que es lo correcto.
  if (myStates.size() > kMaxUndo + 1)
    myStates.erase(myStates.begin(), myStates.begin() + (myStates.size() - kMaxUndo - 1));
  myIndex = myStates.size() - 1;
}

QString History::UndoText() const
{
  return CanUndo() ? myStates[myIndex].what : QString();
}

QString History::RedoText() const
{
  return CanRedo() ? myStates[myIndex + 1].what : QString();
}

const Part& History::Undo()
{
  if (CanUndo())
    --myIndex;
  return Current();
}

const Part& History::Redo()
{
  if (CanRedo())
    ++myIndex;
  return Current();
}

bool History::IsModified() const
{
  return myStates[myIndex].id != mySavedId;
}

void History::MarkSaved()
{
  mySavedId = myStates[myIndex].id;
}
