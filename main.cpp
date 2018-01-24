#include "mainwindow.h"
#include "sircommand.h"
#include <QApplication>
#include <getopt.h>

const char *scriptFile =NULL;
const char *protocalFile =NULL;
int totalloopcnt = 0;
bool isDebugMode = 0;
bool needtoStop = 0;
int transMode = 0; //serial
const char *serialPort = "COM5";


int main(int argc, char *argv[])
{
    //QString str="this is just test for stdout";
    QTextStream out(stdout);
    //out << str<< endl;
    //qDebug() << "argc：" << argc;
    QApplication a(argc, argv);
    char * butname = NULL;
    char * customer = NULL;
    char * device = NULL;
    QString sProtoFile = NULL;
    if(argc > 1)
    {
        int c;
        //int i = 0;
        for(;;){
            //qDebug() << "idx= " << i;
            c = getopt(argc,argv,"Dxs:p:t:m:b:C:c:d:");
            //qDebug() << "c= " << c;
            if(c < 0)
                break;
            switch(c)
            {
                case 'D':
                    isDebugMode = 1;
                    break;
                case 's':
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
                case 'C':
                    serialPort = optarg;
                    qDebug() << "serialPort :" << serialPort;
                    break;
                case 'b':
                    butname = optarg;
                    qDebug() << "buttonname :" << butname;
                    break;
                case 'c':
                    customer = optarg;
                    qDebug() << "customer :" << customer;
                    break;
                case 'd':
                    device = optarg;
                    qDebug() << "device :" << device;
                    break;
                case 'x':
                    //needtoStop = 1;
                    qDebug() << "stop send!";
                    break;
                default:
                    qDebug() << "SmartIR usage:" ;
                    qDebug() << "options:" ;
                    qDebug() << "-D  = open SmartIR in Debug mode,otherwise,open in UI mode" ;
                    qDebug() << "-p  = load IR protocl file,such as 'Sumsung%BDP.ini' " ;
                    qDebug() << "-t  = set test loop count, >=1 means loop forever, <=0 means real time send once,default is 0" ;
                    qDebug() << "-s  = load IR test script file , need t<=0 valid (this function is not ready)" ;
                    qDebug() << "-b  = real time send button name,if t=1 valid " ;
                    qDebug() << "-m  = set transport mode ,0 means by serial;1 menas by wifi ,defeult is 0" ;
                    qDebug() << "-C  = if transMode=0(-m),set which COM Port is used,default is COM5" ;
                    qDebug() << "-c  = if protocalfile(-p) is not designated, set customer" ;
                    qDebug() << "-d  = if protocalfile(-p) is not designated, set device" ;
                    //qDebug() << "-x  = if smartIR is in loop test,stop send" ;

                    return -1;
            }
        }
    }

    if(isDebugMode)
    {

        SirCommand sc(transMode,QString(QLatin1String(serialPort)));
        if(needtoStop)
        {
            sc.clear_and_stop();
            return 0;
        }
        else
        {
            if(protocalFile != NULL)
            {
                sProtoFile = QString(QLatin1String(protocalFile));
            }
            else if(protocalFile == NULL  && (customer!=NULL && device !=NULL))
            {
                QString cus = QString(QLatin1String(customer));
                QString dev = QString(QLatin1String(device));
                sProtoFile = cus.append("%").append(dev).append(".ini");
            }
            else if(protocalFile == NULL && (customer==NULL || device ==NULL))
            {
                 qDebug() << "invalid parameters,protocalfile is invalid";
                 return -1;
            }

            if(totalloopcnt >= 1 && scriptFile == NULL)
            {
                qDebug() << "invalid parameters,scriptfile is needed while loopcount(-t) is set >=1";
                return -1;
            }
            else if(totalloopcnt <= 0  && butname == NULL)
            {
                qDebug() << "invalid parameters,real time send butname is needed while loopcount(-t) is set <=0";
                return -1;
            }
        }
        if(totalloopcnt >= 1)
        {
            if(sc.startWithDebugMode(sProtoFile,QString(QLatin1String(scriptFile)),totalloopcnt) < 0)
            {
                return -1;
            }
            else
            {
                return a.exec();
            }
        }
        else
        {
            //sc.clear_and_stop();
            if(sc.startWithDebugMode(sProtoFile,QString(QLatin1String(butname)),totalloopcnt) < 0)
            {
                return -1;
            }
            else
            {
                //msleep(5000);
            }

        }
        //return a.exec();
    }
    else
    {
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
}
