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
    sendCmdSignal(uint8_t* , int);
    signalDownloadFinished();

private slots:
    void checkForMcuUpgrade();
    void SendCmd2GetCurrentVersion();
    void updateCurrentVersionSlot(IR_MCU_Version_t *);
    void ackReceivedSlot(int);
    void cmdFailSlot();
    void openUrl_slot(QString);
    //void onLookupHost(QHostInfo host);
    //void httpDowload();
    //void httpDowloadFinished();
    //void analysisVersionfromBin();
    void upChooseLocalFileButton_slot();
    void upUpgradeButton_slot();

private:
    Ui::UpgradeDialog *ui;

    uint32_t currentMcuVersion;
    uint32_t availableMcuVersion;

    QLabel *upWebLinklable;
    QList<uint8_t *> upgradePacketList;
    QSemaphore *cmdSemaphore;

    void sendUpgradeStartPacket();
    void sendUpgradeFinishPacket();
    void sendUpgradeBinPacket();

    void doDownload(QUrl fileURL,QFile *dstFile);
    //QNetworkReply *avatorReply;
    //QNetworkAccessManager *avatorManager;

    QString dstBinFilePath;

};

#endif // UPGRADEDIALOG_H
