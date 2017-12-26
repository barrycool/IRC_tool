#ifndef UPGRADETHREAD_H
#define UPGRADETHREAD_H

#include <QThread>
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <windows.h>

class UpgradeThread : public QThread
{
    Q_OBJECT
public:
    UpgradeThread();

    virtual void run();

    void serialSetReady(bool isReady);
    void download_manifestini();
    void download_mcuBin();
    void download_mainTool();
    void checkToolVersion();

signals:
    void finish(bool);
    void getVersionSignal();
    void maintoolNeedUpdate(QString);

private:
    bool isSerialReady;
};

#endif // UPGRADETHREAD_H
