#include "mainwindow.h"
#include <QApplication>
#include "cefclient/init_wrapper.h"
//#include <QDebug>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
//qDebug(  ) << "initing";
  CefInitWrapper cefw( argc, argv );
  if ( !cefw.wasCefInstance() ) return cefw.resultCode();
 
//qDebug(  ) << "inited";
  MainWindow w;
  w.show();

  //a.setQuitOnLastWindowClosed(false);
  return a.exec();
}
