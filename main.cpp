#include "mainwindow.h"
#include <QApplication>
#define VERSION "1.0"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QString title = "Smart IR ";
    title.append(VERSION);
    w.setWindowTitle(title);
    w.setWindowIcon(QIcon(":/new/icon/resource-icon/logo.png"));
    //w.setSizePolicy(QSizePolicy::Fixed);
    w.setFixedSize(678,530);
    w.show();

    return a.exec();
}
