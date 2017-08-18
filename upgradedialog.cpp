#include "upgradedialog.h"
#include "ui_upgradedialog.h"

UpgradeDialog::UpgradeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpgradeDialog)
{
    ui->setupUi(this);
    ui->upProgressBar->hide();
    //QObject::connect(ui->upDownloadButton,SIGNAL(clicked()),this,SLOT(upDownloadButton_slot()));
    //QObject::connect(ui->upCancelButton,SIGNAL(clicked()),this,SLOT(upCancelButton_slot()));
    QObject::connect(ui->upCheckUpdateButton,SIGNAL(clicked()),this,SLOT(checkForMcuUpgrade()));
    QObject::connect(ui->upFreshAvailableButton,SIGNAL(clicked()),this,SLOT(checkForMcuUpgrade()));
    QObject::connect(ui->upFreshCurrentButton,SIGNAL(clicked()),this,SLOT(SendCmd2GetCurrentVersion()));

    connect(this,SIGNAL(getVersionSignal()),parent,SLOT(getCurrentMcuVersion()));
    connect(this,SIGNAL(sendCmdSignal(char *,int)),parent,SLOT(sendCmdforUpgradeSlot(char*,int)));
    connect(parent,SIGNAL(receiveAckSignal()),this,SLOT(ackReceivedSlot()));
    connect(parent,SIGNAL(receiveFinshSignal()),this,SLOT(finishReceivedSlot()));
    connect(parent,SIGNAL(cmdFailSignal()),this,SLOT(cmdFailSlot()));
    connect(parent,SIGNAL(updateVersionSignal(IR_MCU_Version_t *)),this,SLOT(updateCurrentVersionSlot(IR_MCU_Version_t *)));
    cmdSemaphore = new QSemaphore(1);

    upWebLinklable = new QLabel( "<a href = http://www.mediatek.inc >www.mediatek.inc</a>", this );
    QRect r1(340, 160, 131, 20);
    upWebLinklable->setGeometry(r1);
    //upWebLinklable->setParent(ui);
    QObject::connect(upWebLinklable,SIGNAL(linkActivated(QString)),this,SLOT(openUrl_slot(QString)));

}

UpgradeDialog::~UpgradeDialog()
{
    delete ui;
}

void UpgradeDialog::openUrl_slot(QString str)
{
    QDesktopServices::openUrl(QUrl(str));
}
void UpgradeDialog::updateCurrentVersionSlot(IR_MCU_Version_t * mcuVersion)
{
    //currentMcuVersionYear = 2017;
    //currentMcuVersionMonth = 07;
    //currentMcuVersionDay = 07;

    uint8_t yearHigh = mcuVersion->year_high;
    uint8_t yearLow = mcuVersion->year_low;

    currentMcuVersionMonth = mcuVersion->month;
    currentMcuVersionDay = mcuVersion->day;
    currentMcuVersionYear = QString::number(yearHigh).append(QString::number(yearLow)).toInt();

    QString tmp = QString::number(currentMcuVersionYear).append("_").append(QString::number(currentMcuVersionMonth)).append("_").append(QString::number(currentMcuVersionDay));
    ui->upCurrentlineEdit->setText(tmp);

    qDebug() << "get Current mcu version:" << tmp;

    ui->upFreshCurrentButton->setStyleSheet("QPushButton{border-image: url(:/new/icon/resource-icon/accepted-48.png)");
}

void UpgradeDialog::SendCmd2GetCurrentVersion()
{
    qDebug() << "send signal to mainwindow to getcurrent mcu version";
    emit getVersionSignal();
}

void UpgradeDialog::checkForMcuUpgrade()
{
    QString versionPath = "D:/Program_Files/Qt/MyApplications/upgrade_files/VersionHistory.txt";
    QFile versionFile(versionPath);
    if(!versionFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "read file " << versionPath << "fail!\n";
        QMessageBox::critical(this,"Load Error","Open file " + versionPath +"Error");
        return;
    }
    QTextStream in(&versionFile);
    QString line;
    while (!in.atEnd()) {
        line = in.readLine();
    }
    QStringList list1= line.split('_');
    int versionYear = list1.at(0).toInt();
    int versionMonth = list1.at(1).toInt();
    int versionDay = list1.at(2).toInt();

    QString tmp = QString::number(versionYear).append("_").append(QString::number(versionMonth)).append("_").append(QString::number(versionDay));
    ui->upAvailablelineEdit->setText(tmp);

    if(versionYear > currentMcuVersionYear
       || (versionYear == currentMcuVersionYear && versionMonth > currentMcuVersionMonth)
       || (versionYear == currentMcuVersionYear && versionMonth == currentMcuVersionMonth && versionDay > currentMcuVersionDay))
    {
        //QMessageBox::information(this,"Upgrade available","Upgrade for MCU is available");
        QString str = "The latest version of MCU is: ";
        str.append(tmp).append("\n");
        str.append("Do you want to upgrade for MCU?");
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Upgrade available", str, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if(reply == QMessageBox::Yes)
        {
             //ui->UpgradeSubWindow->showMaximized();
             //ui->upDownloadButton->setDisabled(true);
             //ui->upCancelButton->setDisabled(false);
             upDownloadButton_slot();
             return;
        }

    }
    else
    {
        ui->upStatusText->setText("It's latest version,no need to upgrade");
        qDebug() <<"no need to upgrade!";
    }

    versionFile.close();
    //return false;
}

void UpgradeDialog::upDownloadButton_slot()
{
    //ui->upCancelButton->setDisabled(true);
    ui->upProgressBar->setMaximum(0);
    ui->upProgressBar->setMinimum(0);
    ui->upProgressBar->show();
    QString binPath = "D:/Program_Files/Qt/MyApplications/upgrade_files/UPGRADE.bin";
    QFile binFile(binPath);
    if(!binFile.open(QIODevice::ReadOnly))
    {
        ui->upStatusText->setText("Download fail!\n");
        QMessageBox::critical(this,"DownLoad Error","Download file " + binPath +"Error");
        return;
    }
    ui->upStatusText->setText("Start Download");
    QDataStream in(&binFile);
    char* buf = (char *)malloc(UPGRADE_PACKET_SIZE);

    while(in.readRawData(buf,UPGRADE_PACKET_SIZE)>0)
    {
        ui->upStatusText->append("Downloading...");
        upgradePacketList.append(buf);

    }
    ui->upStatusText->append("Done");
    binFile.close();

    getUpgradeCmdList();
    startUpgrade();


}
extern int seqnum;
void UpgradeDialog::addUpgradeStartPacket()
{
    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);
    struct frame_t *frame = (struct frame_t *)buf;

    frame->data_len = sizeof(struct frame_t);
    frame->header = FRAME_HEADER;
    frame->msg = UPGRADE_START;
    frame->seq_num = seqnum++;

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);
    upgradeCmdList.append(buf);
}

void UpgradeDialog::addUpgradeBinPacket()
{
    for(int i;i<upgradePacketList.size();i++)
    {
        //ui->upStatusText->append("Sending..");
        //struct frame_t *frame = (struct frame_t *)buf;
        uint8_t buf[BUF_LEN];
        memset(buf,0x0,BUF_LEN);
        struct frame_t *frame = (struct frame_t *)buf;
        frame->data_len = sizeof(struct frame_t);
        frame->header = FRAME_HEADER;
        frame->seq_num = seqnum++;
        frame->msg = SEND_UPGRADE_PACKET;

        IR_Upgrade_Packet_t IR_upg_packet;
        IR_upg_packet.packet_id = i;
        memcpy(IR_upg_packet.data,upgradePacketList.at(i),UPGRADE_PACKET_SIZE);

        memcpy(frame->msg_parameter, &IR_upg_packet, sizeof(IR_Upgrade_Packet_t));
        frame->data_len += sizeof(IR_Upgrade_Packet_t);

        buf[frame->data_len] = CRC8Software(buf, frame->data_len);
        upgradeCmdList.append(buf);
    }
}
void UpgradeDialog::getUpgradeCmdList()
{
    addUpgradeStartPacket();
    addUpgradeBinPacket();
    qDebug() << "getUpgradeCmdList:size= " << upgradeCmdList.size();
}

void UpgradeDialog::startUpgrade()
{

    if(upgradeCmdList.size() >= 0)
    {
        uint8_t buf[BUF_LEN];
        memset(buf,0x0,BUF_LEN);
        struct frame_t *frame = (struct frame_t *)upgradeCmdList.at(0);
        cmdSemaphore->acquire();
        //MainWindow::sendCmd2MCU((char*)buf, frame->data_len + 1);
        emit sendCmdSignal((char*)buf,frame->data_len + 1);
        upgradeCmdList.removeAt(0);
    }
    else
    {
        ui->upProgressBar->setMaximum(100);
        ui->upProgressBar->setValue(99);
    }
}
void UpgradeDialog::ackReceivedSlot()
{
    cmdSemaphore->release();
    startUpgrade();
}
void UpgradeDialog::finishReceivedSlot()
{
    ui->upProgressBar->setValue(100);
    //ui->upProgressBar->hide();
    //ui->upCancelButton->setText("Finish");
    ui->upStatusText->append("Upgrade Done!");
    //ui->upCancelButton->setDisabled(false);
    QMessageBox::information(this,"Upgrade Finish","ugrade finish,please reset your device!");
}

void UpgradeDialog::cmdFailSlot()
{
  ui->upStatusText->append("send cmd fail!");
  ui->upProgressBar->setValue(100);
}
