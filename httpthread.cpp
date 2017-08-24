#include "httpthread.h"
#include "QMessageBox"
#include "QFileInfo"
#include "QByteArray"
#include <qdebug.h>
/*
HttpThread::HttpThread(QUrl url,QFile *file,long len)
{
    moveToThread(this);
    //this->ThreadIndex=ThreadIndex;
    this->startByte= 0;
    this->endByte= len;
    this->totalBytes = len;//endByte - startByte;
    this->url=url;
    this->file=file;
    this->doneBytes=0;
}
void HttpThread::run()
{
    //manager = new QNetworkAccessManager;
    //request = new QNetworkRequest(url);

    QString strData = QString( "bytes=%0-%1" ).arg( startByte ).arg( endByte );

    //request->setRawHeader("Range", strData.toLatin1() );  //toAscii   toLatin1
    //request->setRawHeader("Connection", "keep-alive");

    //reply = manager->get( *request );
    connect(manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(managerFnish(QNetworkReply*)));
    if( ! reply ) return;

    connect( reply, SIGNAL( readyRead() ), this, SLOT( writeToFile() ) );

    this->exec();
}

void HttpThread::writeToFile()
{
    if (reply->bytesAvailable()>1024)
    {
        QByteArray tempArry = reply->readAll();
        lock.lockForWrite();
        file->seek(this->startByte+this->doneBytes);
        file->write(tempArry);
        lock.unlock();
        this->doneBytes += tempArry.size();
        emit this->progressChanged(tempArry.size());
    }
}

void HttpThread::finishedDownload()
{
    QByteArray tempArry = reply->readAll();

    lock.lockForWrite();
    file->seek(this->startByte+this->doneBytes);
    file->write(tempArry);
    lock.unlock();
    this->doneBytes += tempArry.size();
    emit this->progressChanged(tempArry.size());

    reply->close();
    reply->deleteLater();
    manager->deleteLater();
    emit finish(true);

    this->quit();
}

void HttpThread::managerFnish(QNetworkReply *tmpReply)
{
    if (tmpReply->size() > 0)
    {
        QByteArray tempArry = tmpReply->readAll();

        lock.lockForWrite();
        file->seek(this->startByte+this->doneBytes);
        file->write(tempArry);
        lock.unlock();
        this->doneBytes += tempArry.size();
        emit this->progressChanged(tempArry.size());
    }

    emit finish(true);
    this->quit();
//    this->terminate();
//    this->wait();
}
*/
