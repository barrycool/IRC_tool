#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QString title = "Smart IR ";
    QString Version = QString::number(VERSION);
    Version.insert(1,".");
    title.append(Version);
    w.setWindowTitle(title);
    w.setWindowIcon(QIcon(":/new/icon/resource-icon/MIT smart ir logo.png"));
    //w.setSizePolicy(QSizePolicy::Fixed);
    w.setFixedSize(678,530);
    w.show();

    return a.exec();
}
