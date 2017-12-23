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

    qDebug() << fileDir;
    QFile binFile(fileDir);
    if(binFile.exists())
    {
        qDebug() << "IR_stm32f103C8.bin exsit,delete it first";
        QFile::remove(fileDir);
    }

    SHELLEXECUTEINFO  ShExecInfo;
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

    //emit disableFreshVersion(false);
    //QThread::sleep(5);

    emit finish(true);
/*

*/

}
void UpgradeThread::download_manifestini()
{
    QString appPath = qApp->applicationDirPath();
    QString dstfilepath = appPath.remove("/debug").remove("/release").append("/manifest.ini");
    QFile iniFile(dstfilepath);
    if(iniFile.exists())
    {
        qDebug() << "manifest.ini exsit,delete it first";
        QFile::remove(dstfilepath);
    }
    SHELLEXECUTEINFO  ShExecInfo;
     ShExecInfo.cbSize  =   sizeof(SHELLEXECUTEINFO);
     ShExecInfo.fMask   =  SEE_MASK_NOCLOSEPROCESS;
     ShExecInfo.hwnd  =   NULL;
     ShExecInfo.lpVerb  =    NULL;
     ShExecInfo.lpFile   =   L"wget.exe";
     ShExecInfo.lpParameters  =  L"https://github.com/barrycool/bin/raw/master/MainTool/manifest.ini";
     ShExecInfo.lpDirectory   =  NULL;
     ShExecInfo.nShow   =   SW_HIDE;
     ShExecInfo.hInstApp  =  NULL;
     ShellExecuteEx(&ShExecInfo);

     //ShellExecute(NULL, L"open", L"wget.exe", NULL, L"D://02_wind//main", SW_SHOW);
     WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
     emit downloadIniFinish();
}
void UpgradeThread::serialSetReady(bool isReady)
{
    isSerialReady = isReady;
}
