#include "mainwindow.h"
#include "sircommand.h"
#include <QApplication>
#include <getopt.h>

const char *scriptFile =NULL;
const char *protocalFile =NULL;
int totalloopcnt = 0;
bool isDebugMode = 0;
int transMode = 0; //serial
const char *serialPort = "COM5";


int main(int argc, char *argv[])
{
    QString str="this is just test for stdout";
    QTextStream out(stdout);
    out << str<< endl;
    //qDebug() << "argc：" << argc;
    QApplication a(argc, argv);
    if(argc > 1)
    {
        int c;
        //int i = 0;
        for(;;){
            //qDebug() << "idx= " << i;
            c = getopt(argc,argv,"dl:p:t:m");
            //qDebug() << "c= " << c;
            if(c < 0)
                break;
            switch(c)
            {
                case 'd':
                    isDebugMode = 1;
                    break;
                case 'l':
                    scriptFile = optarg;
                    qDebug() << "scriptFile:"<< scriptFile;
                    break;
                case 'p':
                    protocalFile = optarg;
                    qDebug() << "protocalFile:"<< protocalFile;
                    break;
                case 't':
                    totalloopcnt = atoi(optarg);
                    qDebug() << "totalloopcnt:"<< totalloopcnt;
                    break;
                case 'm':
                    transMode = atoi(optarg);
                    qDebug() << "transport mode :" << transMode;
                    break;
                case 'c':
                    serialPort = optarg;
                    qDebug() << "serialPort :" << serialPort;
                    break;
                default:
                    qDebug() << "SmartIR usage:" ;
                    qDebug() << "options:" ;
                    qDebug() << "-d  = open SmartIR in Debug mode,otherwise,open in UI mode" ;
                    qDebug() << "-p  = load IR protocl file,such as 'Sumsung%BDP.ini' " ;
                    qDebug() << "-l  = load IR test script file " ;
                    qDebug() << "-t  = set test loop count, <=0 means loop forever " ;
                    qDebug() << "-m  = set transport mode,0 means by serial;1 menas by wifi " ;
                    qDebug() << "-c  = if transMode=0,set which serialPort is used" ;
                    return -1;
            }
        }
    }

    if(isDebugMode)
    {
        SirCommand sc(transMode,QString(QLatin1String(serialPort)));
        sc.startWithDebugMode(QString(QLatin1String(protocalFile)),QString(QLatin1String(scriptFile)),totalloopcnt);
        return a.exec();
    }
    else
    {
        qDebug() << "111";
        MainWindow w;
        qDebug() << "222";
        QString title = "Smart IR ";
        QString Version = QString::number(VERSION);
        Version.insert(1,".");
        title.append(Version);
        w.setWindowTitle(title);
        qDebug() << "333";
        w.setWindowIcon(QIcon(":/new/icon/resource-icon/MIT smart ir logo.png"));
        //w.setSizePolicy(QSizePolicy::Fixed);
        w.setFixedSize(678,530);
        w.show();
        qDebug() << "444";
        return a.exec();
    }
}
