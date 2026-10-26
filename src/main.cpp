#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
#ifndef _WIN32
  // Open CASCADE dibuja sobre una ventana X11 nativa: en Linux forzamos xcb aunque haya Wayland.
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif

  QApplication app(argc, argv);
  MainWindow window;
  window.resize(1280, 800);
  window.show();
  return app.exec();
}
