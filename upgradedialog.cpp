#include "upgradedialog.h"
#include "ui_upgradedialog.h"
bool isUpgradefileDownloaded = 0;
static bool isNetworkAccessable = 0;
//static bool needUpgradeTool = 0;
QFile *dstbinfile;

#define VALID_UPGRADE_FLAG 0xA55AA55A

struct upgrade_bin_tailer_t{
    uint32_t upgrade_flag;
    uint32_t upgrade_version;
    uint32_t upgrade_fileLength;
    uint32_t upgrade_crc32;
};
FILE *upgrade_file;
int upgrade_flag = 0;

bool check_valid_upgrade_bin_version(QString fileName,uint32_t &version, uint32_t &checkSum);

UpgradeDialog::UpgradeDialog(QWidget *parent,uint32_t current,uint32_t available) :
    QDialog(parent),
    ui(new Ui::UpgradeDialog)
{
    ui->setupUi(this);
    ui->upProgressBar->hide();
    //serial = port;

    currentMcuVersion = current;
    availableMcuVersion = available;
    ui->upCurrentlineEdit->setText(QString::number(currentMcuVersion));
    ui->upAvailablelineEdit->setText(QString::number(availableMcuVersion));
    if(availableMcuVersion>0)
    {
        isUpgradefileDownloaded = 1;
    }

    dstBinFilePath.resize(0);

    QObject::connect(ui->upUpgradeButton,SIGNAL(clicked()),this,SLOT(upUpgradeButton_slot()));
    QObject::connect(ui->upCancelButton,SIGNAL(clicked()),this,SLOT(upCancelButton_slot()));
    QObject::connect(ui->upFreshAvailableButton,SIGNAL(clicked()),this,SLOT(checkForMcuUpgrade()));
    QObject::connect(ui->upFreshCurrentButton,SIGNAL(clicked()),this,SLOT(SendCmd2GetCurrentVersion()));
    QObject::connect(ui->upChooseLocalFileButton,SIGNAL(clicked()),this,SLOT(upChooseLocalFileButton_slot()));

    connect(this,SIGNAL(getVersionSignal()),parent,SLOT(getCurrentMcuVersion()));
    connect(this,SIGNAL(sendCmdSignal(uint8_t *,int)),parent,SLOT(sendCmdforUpgradeSlot(uint8_t*,int)));
    connect(parent,SIGNAL(receiveAckSignal(int)),this,SLOT(ackReceivedSlot(int)));

    connect(parent,SIGNAL(cmdFailSignal()),this,SLOT(cmdFailSlot()));
    connect(parent,SIGNAL(updateVersionSignal(IR_MCU_Version_t *)),this,SLOT(updateCurrentVersionSlot(IR_MCU_Version_t *)));

    cmdSemaphore = new QSemaphore(1);

    upWebLinklable = new QLabel( "<a href = https://www.mediatek.com/ >www.mediatek.com</a>", this );
    QRect r1(340, 160, 131, 20);
    upWebLinklable->setGeometry(r1);
    //upWebLinklable->setParent(ui);
    QObject::connect(upWebLinklable,SIGNAL(linkActivated(QString)),this,SLOT(openUrl_slot(QString)));

    //check if can access the internet
    QHostInfo::lookupHost("www.baidu.com",this,SLOT(onLookupHost(QHostInfo)));

    if(currentMcuVersion >= availableMcuVersion)
    {
        //just for test
        //ui->upUpgradeButton->setDisabled(true);

        ui->upStatusText->setText("No need to upgrade!");
    }
    else
    {
        ui->upUpgradeButton->setDisabled(false);
        ui->upStatusText->setText("Please press upgrade button!");
    }
    //connect(&serial, SIGNAL(readyRead()), this, SLOT(serial_receive_ack()));
    //getPortInfoByName(portName);
}

//int oldPortOpened = 0;

UpgradeDialog::~UpgradeDialog()
{
    //serial->close();
    upgrade_flag = 0;
    delete ui;
}


void UpgradeDialog::onLookupHost(QHostInfo host)
{
    if (host.error() != QHostInfo::NoError)
    {
        qDebug() << "Lookup failed:" << host.errorString();
        isNetworkAccessable = false;
        ui->upStatusText->append("Network is not accessible");
    }
    else{
        isNetworkAccessable = true;
        ui->upStatusText->append("Network is accessible");
    }
}

void UpgradeDialog::openUrl_slot(QString str)
{
    QDesktopServices::openUrl(QUrl(str));
}
void UpgradeDialog::updateCurrentVersionSlot(IR_MCU_Version_t * mcuVersion)
{

    QString currentVersionStr = QString::asprintf("%02x%02x%02x%02x",
                                                 mcuVersion->year_high, mcuVersion->year_low,
                                                 mcuVersion->month, mcuVersion->day);

    currentMcuVersion = currentVersionStr.toInt();

    ui->upCurrentlineEdit->setText(currentVersionStr);
    if(availableMcuVersion <= currentMcuVersion)
    {
        QString logstr ="currentMcuVersion is the latest version,no need to upgrade";
        qDebug() << logstr;
        ui->upStatusText->append(logstr);
        ui->upUpgradeButton->setEnabled(false);
    }
    else
    {
        QString logstr = "Newer MCU version is available,Please Upgrade";
        qDebug() << logstr;
        ui->upStatusText->append(logstr);
        ui->upUpgradeButton->setEnabled(true);
    }

    qDebug() << "get Current mcu version:" << currentVersionStr;

   // ui->upFreshCurrentButton->setStyleSheet("QPushButton{image: url(:/new/icon/resource-icon/accepted-48.png)");
}

void UpgradeDialog::SendCmd2GetCurrentVersion()
{
    qDebug() << "send signal to mainwindow to getcurrent mcu version";
    emit getVersionSignal();
}

void UpgradeDialog::checkForMcuUpgrade()
{
    //need to download upgrade files from github,and get the version
        QString srcBinFilePath;

        QString logstr;
        if(isNetworkAccessable)
        {
            //step 1 :delete old bin
            QString appPath = qApp->applicationDirPath();
            QString fileDir = appPath.append("/Download_files");
            QDir *dir = new QDir(fileDir);
            if(!dir->exists())
            {
                logstr = fileDir + " doesn't exist";
                qDebug() << logstr;

                bool ok = dir->mkdir(fileDir);
                if( ok )
                {
                    logstr = "fileDir created success.";
                    qDebug() << logstr;
                }
                else
                {
                    logstr = "download fail.";
                    qDebug() << logstr;
                    return;
                }

            }
            QString filename = appPath.append("/IR_stm32f103C8.bin");
            QFile binFile(filename);
            ui->upStatusText->append(filename);
            if(binFile.exists())
            {
                qDebug() << "IR_stm32f103C8.bin exsit,delete it first";
                ui->upStatusText->append("IR_stm32f103C8.bin exsit,delete it first");
                QFile::remove(filename);
            }
            //step 2 :download new bin from github
            srcBinFilePath = "https://github.com/barrycool/bin/raw/master/IR_MCU_upgrade_bin/IR_stm32f103C8.bin";
            //QString cmd = "wget " + srcBinFilePath;
            QString cmd = "wget -P " + fileDir + " " + srcBinFilePath;
            if(system(cmd.toLatin1().data()))
            {
                ui->upStatusText->setText("download fail!");
                return;
            }
            else
            {
                logstr = "download success to " + filename;
                ui->upStatusText->setText(logstr);
                dstBinFilePath = filename;
            }

            //step 3 :analysis version
            uint32_t availableVersion;
            uint32_t checksum;
            QString logstr;

            if (binFile.size() != 0)
            {
                qDebug () << filename;

                if (check_valid_upgrade_bin_version(filename, availableVersion, checksum))
                {
                    QString tmp = QString::number(availableVersion,16);
                    availableMcuVersion = tmp.toInt();

                    if(availableMcuVersion <= currentMcuVersion)
                    {
                        logstr ="currentMcuVersion is the latest version,no need to upgrade";
                        qDebug() << logstr;
                        ui->upStatusText->append(logstr);
                        ui->upUpgradeButton->setEnabled(false);
                    }
                    else
                    {
                        logstr = "Newer MCU version is available,Please Upgrade";
                        qDebug() << logstr;
                        ui->upStatusText->append(logstr);
                        ui->upUpgradeButton->setEnabled(true);
                    }
                    ui->upAvailablelineEdit->setText(QString::number(availableMcuVersion));

                    ui->upLocalPathEdit->setText(dstBinFilePath);
                    isUpgradefileDownloaded = true;
                }
                else
                {
                    logstr = "Upgrade file is crrupted!";
                    qDebug() << logstr;
                    ui->upStatusText->append(logstr);
                    ui->upUpgradeButton->setEnabled(false);
                }

            }
        }
        else
        {
            //get from local disk
            QString str = "Your Network is not accessible , Do you want to upgrade from Local file?";
            QMessageBox::StandardButton reply = QMessageBox::question(this, "No Network ", str, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if(reply == QMessageBox::Yes)
            {
                 upChooseLocalFileButton_slot();
                 return;
            }
            else
            {
                ui->upUpgradeButton->setEnabled(false);
            }

        }
}
void UpgradeDialog::upChooseLocalFileButton_slot()
{
    uint32_t availableVersion;
    uint32_t checksum;
    QString logstr;
    QString path = QFileDialog::getOpenFileName(this, "choose upgrade file", "", "bin file(*.bin)");
    dstBinFilePath = path;
    if (path.size() != 0)
    {
        if (check_valid_upgrade_bin_version(path, availableVersion, checksum))
        {
            QString tmp = QString::number(availableVersion,16); //10进制转成16进制
            availableMcuVersion = tmp.toInt();

            if(availableMcuVersion <= currentMcuVersion)
            {
                logstr ="currentMcuVersion is newer than local upgrade file,no need to upgrade";
                qDebug() << logstr;
                ui->upStatusText->append(logstr);
                ui->upUpgradeButton->setEnabled(false);
            }
            else
            {
                logstr = "local upgrade file is newer than currentMcuVersion,Please upgrade";
                qDebug() << logstr;
                ui->upStatusText->append(logstr);
                ui->upUpgradeButton->setEnabled(true);
            }

            ui->upAvailablelineEdit->setText(QString::number(availableMcuVersion));

            ui->upLocalPathEdit->setText(path);
            isUpgradefileDownloaded = true;
            //emit signalDownloadFinished();  //send signal to analysis version from bin file
        }
        else
        {
            ui->upUpgradeButton->setEnabled(false);
        }
    }
}

int packetid = 0;
int total_file_length;
double current_file_length = 0;
void UpgradeDialog::upUpgradeButton_slot()
{

    if(!isUpgradefileDownloaded)
    {
        checkForMcuUpgrade(); //if binfile has not been download,download it first
    }

    ui->upProgressBar->setMinimum(0);
    ui->upProgressBar->setMaximum(100);

    ui->upProgressBar->show();

    QByteArray name = dstBinFilePath.toLatin1();

    upgrade_file = fopen(name.data(), "rb");
    if (upgrade_file == NULL)
    {
        QMessageBox::warning(NULL, "upgrade", "open file error");
        return;
    }

    fseek(upgrade_file, 0, SEEK_END);
    total_file_length = ftell(upgrade_file);
    fseek(upgrade_file, 0, SEEK_SET);
    ui->upCancelButton->setDisabled(true);

    qDebug() << "total_file_length" << total_file_length;
    ui->upStatusText->append("Start Upgrade");

    ui->upProgressBar->setValue(0);

    sendUpgradeStartPacket();

}

extern int seqnum;
void UpgradeDialog::sendUpgradeStartPacket()
{
    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);

    packetid = 0;
    current_file_length = 0;
    upgrade_flag = 1;
    seqnum = 1;

    struct frame_t *frame = (struct frame_t *)buf;
    ui->upStatusText->append("Sending start packet...");
    frame->data_len = sizeof(struct frame_t);
    frame->header = FRAME_HEADER;
    frame->msg = UPGRADE_START;
    frame->seq_num = seqnum++;

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);
    //ui->upProgressBar->setValue(1);
    //cmdSemaphore->acquire();

    emit sendCmdSignal(buf,frame->data_len + 1);
    //serial->write((char*)buf, frame->data_len + 1);

}
uint32_t crc32;
void UpgradeDialog::sendUpgradeFinishPacket()
{
    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);
     ui->upStatusText->append("Sending finish packet...");
    struct frame_t *frame = (struct frame_t *)buf;

    frame->header = FRAME_HEADER;
    frame->data_len = sizeof(struct frame_t);
    frame->seq_num = seqnum++;
    frame->msg = UPGRADE_FINISH;

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    //cmdSemaphore->acquire();

    emit sendCmdSignal(buf,frame->data_len + 1);
    //serial->write((char*)buf, frame->data_len + 1);

    ui->upStatusText->append("Upgrade Done!");
    if (upgrade_file != NULL)
    {
        fclose(upgrade_file);
        upgrade_file = NULL;
    }
    ui->upCancelButton->setText("Finish");
    ui->upCancelButton->setDisabled(false);
    ui->upUpgradeButton->setDisabled(true);
    upgrade_flag = 0;
    QMessageBox::information(this,"Upgrade Finish","ugrade finish,please reset your device!");

}

void UpgradeDialog::sendUpgradeBinPacket()
{
    /*
    bool isEmpty = upgradePacketList.isEmpty();
    qDebug() << "sendUpgradeBinPacket,upgradePacketList.isEmpty:"<<isEmpty;
    if(!isEmpty)
    {
    */

    if (!upgrade_flag)
    {
        return;
    }
    uint8_t buf[255] = {0};
    size_t read_cnt = 0 ;
    ui->upStatusText->append("Sending bin packet...");
    struct frame_t *frame = (struct frame_t *)buf;

    frame->header = FRAME_HEADER;
    frame->data_len = sizeof(struct frame_t);
    frame->seq_num = seqnum++;
    frame->msg = SEND_UPGRADE_PACKET;

    buf[frame->data_len++] = packetid & 0xFF;
    buf[frame->data_len++] = (packetid >> 8) & 0xFF;
    packetid++;

    read_cnt = fread(buf + frame->data_len, 1, UPGRADE_PACKET_SIZE, upgrade_file);
    //ui->upStatusText->append("read_cnt:" + QString::number(read_cnt));
    if (read_cnt == 0)
    {
        ui->upProgressBar->setValue(100);
        ui->upStatusText->append("set progress bar as 100%");
        sendUpgradeFinishPacket();
        return;
    }
    else if (read_cnt < UPGRADE_PACKET_SIZE)
    {
        memset(buf + frame->data_len + read_cnt, 0xFF, UPGRADE_PACKET_SIZE - read_cnt);
    }

    current_file_length += read_cnt; 

    frame->data_len += UPGRADE_PACKET_SIZE;
    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    emit sendCmdSignal(buf,frame->data_len + 1);

    //serial->write((char*)buf, frame->data_len + 1);
}

void UpgradeDialog::ackReceivedSlot(int msg_id)
{
    //qDebug() << "ackReceivedSlot:receive ack of " << msg_id;
    //cmdSemaphore->release();

    if(msg_id == UPGRADE_START)
    {
        //SEND PACKET
        upgrade_flag = 1;
        qDebug() <<"receive ack of UPGRADE_START";
        sendUpgradeBinPacket();
    }
    else if(msg_id == SEND_UPGRADE_PACKET)
    {
        //SEND PACKET
        //if(!upgradePacketList.isEmpty())
            //upgradePacketList.removeAt(0);
        qDebug() <<"receive ack of SEND_UPGRADE_PACKET";
        ui->upProgressBar->setValue((current_file_length / total_file_length) * 100);
        sendUpgradeBinPacket();
    }
    /*    //mcu receive UPGRADE_FINISH,it reboot itself,so we cannot receive ack of UPGRADE_FINISH
    else if(msg_id == UPGRADE_FINISH)
    {
        //upgrade finished
        ui->upProgressBar->setValue(100);
        ui->upStatusText->append("Upgrade Done!");
        //ui->upCancelButton->setDisabled(false);
        QMessageBox::information(this,"Upgrade Finish","ugrade finish,please reset your device!");
        //ui->upProgressBar->hide();
    }
    */

}

void UpgradeDialog::cmdFailSlot()
{
  ui->upStatusText->append("send cmd fail!");
  ui->upProgressBar->setValue(100);
  if (upgrade_file != NULL)
  {
      fclose(upgrade_file);
      upgrade_file = NULL;
  }
  ui->upCancelButton->setDisabled(false);
  upgrade_flag = 0;
  //ui->upUpgradeButton->setDisabled(true);
  QMessageBox::critical(this,"Upgrade fail","Upgrade fail,Re-Plug usb2serial and try again!");

}
void UpgradeDialog::upCancelButton_slot()
{
    if(ui->upCancelButton->text() == "Finish")
    {
        emit UpgradeRejected(1,availableMcuVersion);
    }
    else if(ui->upCancelButton->text() == "Cancel")
    {
        emit UpgradeRejected(0,availableMcuVersion);
    }
}

typedef uint32_t u32;

//软件CRC32 u32数据计算
u32 upgrade_crc32(u32 *ptr, u32 len, u32 &CRC32)
{
    u32 xbit;
    u32 data;
    u32 bits;
    const u32 dwPolynomial = 0x04c11db7;
    u32 i;

    for(i = 0;i < len;i ++)
    {
        xbit = 1 << 31;
        data = ptr[i];
        for (bits = 0; bits < 32; bits++)
        {
            if (CRC32 & 0x80000000) {
                CRC32 <<= 1;
                CRC32 ^= dwPolynomial;
            }
            else
                CRC32 <<= 1;
            if (data & xbit)
                CRC32 ^= dwPolynomial;

            xbit >>= 1;
        }
    }
    return CRC32;
}

bool check_valid_upgrade_bin_version(QString fileName,uint32_t &version, uint32_t &checkSum)
{
    QByteArray name = fileName.toLatin1();

    FILE * bin_file = fopen(name.data(), "rb");
    if (bin_file == NULL)
    {
        qDebug() << "open file error" <<endl;
        return false;
    }

    fseek(bin_file, 0, SEEK_END);
    uint32_t bin_size = ftell(bin_file);
    if (bin_size < sizeof(struct upgrade_bin_tailer_t))
    {
        qDebug() << "invalid upgrade bin size" <<endl;
        fclose(bin_file);
        return false;
    }

    struct upgrade_bin_tailer_t upgrade_bin_tailer;
    fseek(bin_file, -sizeof(struct upgrade_bin_tailer_t), SEEK_END);
    fread(&upgrade_bin_tailer, 1, sizeof(struct upgrade_bin_tailer_t), bin_file);

    if (upgrade_bin_tailer.upgrade_flag != VALID_UPGRADE_FLAG)
    {
        qDebug() << "invalid upgrade flag" <<endl;
        fclose(bin_file);
        return false;
    }

    uint32_t buf[256];
    uint32_t buf_len;
    checkSum = 0xFFFFFFFF;

    bin_size -= sizeof(struct upgrade_bin_tailer_t);


    fseek(bin_file, 0, SEEK_SET);
    do
    {
        buf_len = fread(buf, 4, 256, bin_file);

        if (bin_size > buf_len * 4)
        {
            bin_size -= buf_len * 4;
        }
        else
        {
            buf_len = bin_size / 4;
            bin_size = 0;
        }

        upgrade_crc32(buf, buf_len, checkSum);
    }
    while (bin_size > 0);

    if (checkSum != upgrade_bin_tailer.upgrade_crc32)
    {
        qDebug() << "invalid upgrade checkSum" <<endl;
        fclose(bin_file);
        return false;
    }

    version = upgrade_bin_tailer.upgrade_version;
    fclose(bin_file);
    return true;
}

void UpgradeDialog::on_upClear_clicked()
{
    ui->upStatusText->clear();
}
