#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.resize(998, 823);
    w.setFixedSize(w.size());
    w.show();
    return a.exec();
}
