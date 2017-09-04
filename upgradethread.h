#ifndef UPGRADETHREAD_H
#define UPGRADETHREAD_H

#include <QThread>
#include <QApplication>
#include <QDir>
#include <QDebug>

class UpgradeThread : public QThread
{
    Q_OBJECT
public:
    UpgradeThread();

    virtual void run();

    void serialSetReady(bool isReady);

signals:
    void finish(bool);
    void getVersionSignal();

private:
    bool isSerialReady;
};

#endif // UPGRADETHREAD_H
