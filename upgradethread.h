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

signals:
    void finish(bool);
    void getVersionSignal();
    void downloadIniFinish();

private:
    bool isSerialReady;
};

#endif // UPGRADETHREAD_H
