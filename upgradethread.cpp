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
    QString fileDir = appPath.remove("/debug").append("/IR_stm32f103C8.bin");
    //QDir *dir = new QDir(appPath);
    //dir->setCurrent(appPath);
    qDebug() << fileDir;
    QFile binFile(fileDir);
    if(binFile.exists())
    {
        qDebug() << "IR_stm32f103C8.bin exsit,delete it first";
        QFile::remove("IR_stm32f103C8.bin");
    }

    QString srcBinFilePath = "https://github.com/barrycool/bin/raw/master/IR_MCU_upgrade_bin/IR_stm32f103C8.bin";
    QString cmd = "wget " + srcBinFilePath;
    system(cmd.toLatin1().data());

    while(binFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Download finished";
        binFile.close();
        //QFile::remove("IR_stm32f103C8.bin");
        emit finish(true);
        return;
    }

}

void UpgradeThread::serialSetReady(bool isReady)
{
    isSerialReady = isReady;
}