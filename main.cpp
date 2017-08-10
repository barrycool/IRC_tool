#include "mainwindow.h"
#include <QApplication>
#define VERSION "1.0"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QString title = "IRC_TOOL ";
    title.append(VERSION);
    w.setWindowTitle(title);
    w.setWindowIcon(QIcon(":/new/icon/resource-icon/logo.png"));
    w.show();

    return a.exec();
}
