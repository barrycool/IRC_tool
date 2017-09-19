#include "upgradethread.h"

UpgradeThread::UpgradeThread()
{

}
void UpgradeThread::run()
{

    while(!isSerialReady)
    {
        QThread::usleep(1500); // delay 2s
    }
    emit getVersionSignal();

    QString appPath = qApp->applicationDirPath();
    QString fileDir = appPath.remove("/debug").remove("/release").append("/IR_stm32f103C8.bin");
    //QDir *dir = new QDir(appPath);
    //dir->setCurrent(appPath);
    qDebug() << fileDir;
    QFile binFile(fileDir);
    if(binFile.exists())
    {
        qDebug() << "IR_stm32f103C8.bin exsit,delete it first";
        QFile::remove(fileDir);
    }

    QString srcBinFilePath = "https://github.com/barrycool/bin/raw/master/IR_MCU_upgrade_bin/IR_stm32f103C8.bin";
    QString cmd = "wget " + srcBinFilePath;
    char * cmds= cmd.toLatin1().data();
    //system(cmd.toLatin1().data());
    //WinExec(cmd.toLatin1().data(), SW_HIDE);

    SHELLEXECUTEINFO  ShExecInfo  =   {0};
     ShExecInfo.cbSize  =   sizeof(SHELLEXECUTEINFO);
     ShExecInfo.fMask   =  SEE_MASK_NOCLOSEPROCESS;
     ShExecInfo.hwnd  =   NULL;
     ShExecInfo.lpVerb  =    NULL;
     ShExecInfo.lpFile   =   L"wget.exe";
     ShExecInfo.lpParameters  =  L"https://github.com/barrycool/bin/raw/master/IR_MCU_upgrade_bin/IR_stm32f103C8.bin";
     ShExecInfo.lpDirectory   =  NULL;
     ShExecInfo.nShow   =   SW_HIDE;
     ShExecInfo.hInstApp  =  NULL;
     ShellExecuteEx(&ShExecInfo);
     //ShellExecute(NULL, L"open", L"wget.exe", NULL, L"D://02_wind//main", SW_SHOW);
     WaitForSingleObject(ShExecInfo.hProcess,INFINITE);


    //QThread::sleep(5);

    emit finish(true);
/*
    while(binFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Download finished";
        binFile.close();
        //QFile::remove("IR_stm32f103C8.bin");

        return;
    }
*/
}

void UpgradeThread::serialSetReady(bool isReady)
{
    isSerialReady = isReady;
}
