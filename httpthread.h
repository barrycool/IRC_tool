#ifndef HTTPTHREAD_H
#define HTTPTHREAD_H
/*
#include <QThread>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>
#include <QEventLoop>
#include <QUrl>
#include <QFile>
#include "QReadWriteLock"


class HttpThread : public QThread
{
    Q_OBJECT
public:
    HttpThread(QUrl url,QFile *file,long fileLen);
    QUrl url;
    QFile *file;
    QReadWriteLock lock;
    //short ThreadIndex;
    long startByte;
    long endByte;
    long doneBytes;
    long totalBytes;
    virtual void run();
signals:
    void progressChanged(long);
    void finish(bool);
public slots:
    void writeToFile();
    void finishedDownload();
    void managerFnish(QNetworkReply * tmpReply);
private:
    QNetworkReply * reply;
    QNetworkRequest * request;
    QNetworkAccessManager * manager;
};
*/
#endif // HTTPTHREAD_H
