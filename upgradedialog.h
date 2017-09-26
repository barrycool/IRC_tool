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
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
//#include "upgrade.h"
#include <QtNetwork/QHostInfo>
//#include <QtNetwork/QNetworkAccessManager>
#include <windows.h>

extern bool check_valid_upgrade_bin_version(QString fileName,uint32_t &version, uint32_t &checkSum);
namespace Ui {
class UpgradeDialog;
}

class UpgradeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpgradeDialog(QWidget *parent,uint32_t current,uint32_t available);
    ~UpgradeDialog();

signals:
    getVersionSignal();
    sendCmdSignal(uint8_t* , int);
    signalDownloadFinished();
    UpgradeRejected(bool,uint32_t);

private slots:
    void checkForMcuUpgrade();
    void SendCmd2GetCurrentVersion();
    void updateCurrentVersionSlot(uint32_t,uint32_t);
    void ackReceivedSlot(int);
    void cmdFailSlot();
    void openUrl_slot(QString);
    void onLookupHost(QHostInfo host);
    void upChooseLocalFileButton_slot();
    void upUpgradeButton_slot();
    void upCancelButton_slot();
    void on_upClear_clicked();
    void enableFreshVersionButton(bool);


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

    //void doDownload(QUrl fileURL,QFile *dstFile);
    QString dstBinFilePath;
};

#endif // UPGRADEDIALOG_H
