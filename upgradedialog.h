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

private slots:
    void checkForMcuUpgrade();
    void SendCmd2GetCurrentVersion();
    void updateCurrentVersionSlot(IR_MCU_Version_t *);
    void ackReceivedSlot();
    void finishReceivedSlot();
    void cmdFailSlot();
    void openUrl_slot(QString);

private:
    Ui::UpgradeDialog *ui;
    int currentMcuVersionYear;
    int currentMcuVersionMonth;
    int currentMcuVersionDay;
    QLabel *upWebLinklable;
    QList<char *> upgradePacketList;
    QList<uint8_t *> upgradeCmdList;
    QSemaphore *cmdSemaphore;
    void upDownloadButton_slot();
    void addUpgradeStartPacket();
    void addUpgradeBinPacket();
    void getUpgradeCmdList();
    void startUpgrade();

};

#endif // UPGRADEDIALOG_H
