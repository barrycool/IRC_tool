#ifndef UPGRADEDIALOG_H
#define UPGRADEDIALOG_H

#include <QDialog>

#include <protocol.h>
#include <QMessageBox>
#include <QDebug>
#include <QtDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QSemaphore>
#include <crc32.h>
#include <QDesktopServices>
#include "upgrade.h"
//#include <QtNetwork/QHostInfo>
//#include <QtNetwork/QNetworkAccessManager>


namespace Ui {
class UpgradeDialog;
}

class UpgradeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpgradeDialog(QWidget *parent = 0);
    ~UpgradeDialog();

signals:
    getVersionSignal();
    sendCmdSignal(char* , int);
    signalDownloadFinished();

private slots:
    void checkForMcuUpgrade();
    void SendCmd2GetCurrentVersion();
    void updateCurrentVersionSlot(IR_MCU_Version_t *);
    void ackReceivedSlot();
    void finishReceivedSlot();
    void cmdFailSlot();
    void openUrl_slot(QString);
    //void onLookupHost(QHostInfo host);
    //void httpDowload();
    //void httpDowloadFinished();
    void analysisVersionfromBin();

private:
    Ui::UpgradeDialog *ui;
    int currentMcuVersionYear;
    int currentMcuVersionMonth;
    int currentMcuVersionDay;
    QLabel *upWebLinklable;
    QList<char *> upgradePacketList;
    QList<uint8_t *> upgradeCmdList;
    QSemaphore *cmdSemaphore;
    void upUpgradeButton_slot();
    void upCancelButton_slot();
    void addUpgradeStartPacket();
    void addUpgradeFinishPacket();
    void addUpgradeBinPacket();
    void getUpgradeCmdList();
    void startUpgrade();
    void doDownload(QUrl fileURL,QFile *dstFile);
    //QNetworkReply *avatorReply;
    //QNetworkAccessManager *avatorManager;

    QString dstBinFilePath;
    int binfileLen;

};

#endif // UPGRADEDIALOG_H
