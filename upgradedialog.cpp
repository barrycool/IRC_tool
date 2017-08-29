#include "upgradedialog.h"
#include "ui_upgradedialog.h"
static bool isUpgradefileDownloaded = 0;
static bool isNetworkAccessable = 0;
QFile *dstbinfile;

#define VALID_UPGRADE_FLAG 0xA55AA55A

struct upgrade_bin_tailer_t{
    uint32_t upgrade_flag;
    uint32_t upgrade_version;
    uint32_t upgrade_fileLength;
    uint32_t upgrade_crc32;
};
//FILE *upgrade_file;
bool check_valid_upgrade_bin_version(QString fileName,uint32_t &version, uint32_t &checkSum);

UpgradeDialog::UpgradeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpgradeDialog)
{
    ui->setupUi(this);
    ui->upProgressBar->hide();
    QObject::connect(ui->upUpgradeButton,SIGNAL(clicked()),this,SLOT(upUpgradeButton_slot()));
    QObject::connect(ui->upCancelButton,SIGNAL(clicked()),this,SLOT(upCancelButton_slot()));
    //QObject::connect(ui->upCancelButton,SIGNAL(clicked()),this,SLOT(upCancelButton_slot()));
    //QObject::connect(ui->upCheckUpdateButton,SIGNAL(clicked()),this,SLOT(checkForMcuUpgrade()));
    QObject::connect(ui->upFreshAvailableButton,SIGNAL(clicked()),this,SLOT(checkForMcuUpgrade()));
    QObject::connect(ui->upFreshCurrentButton,SIGNAL(clicked()),this,SLOT(SendCmd2GetCurrentVersion()));
    QObject::connect(ui->upChooseLocalFileButton,SIGNAL(clicked()),this,SLOT(upChooseLocalFileButton_slot()));

    connect(this,SIGNAL(getVersionSignal()),parent,SLOT(getCurrentMcuVersion()));
    connect(this,SIGNAL(sendCmdSignal(char *,int)),parent,SLOT(sendCmdforUpgradeSlot(char*,int)));
    connect(parent,SIGNAL(receiveAckSignal()),this,SLOT(ackReceivedSlot()));
    connect(parent,SIGNAL(receiveFinshSignal()),this,SLOT(finishReceivedSlot()));
    connect(parent,SIGNAL(cmdFailSignal()),this,SLOT(cmdFailSlot()));
    connect(parent,SIGNAL(updateVersionSignal(IR_MCU_Version_t *)),this,SLOT(updateCurrentVersionSlot(IR_MCU_Version_t *)));
    //connect(this,SIGNAL(signalDownloadFinished()),parent,SLOT(analysisVersionfromBin()));
    cmdSemaphore = new QSemaphore(1);

    upWebLinklable = new QLabel( "<a href = http://www.mediatek.inc >www.mediatek.inc</a>", this );
    QRect r1(340, 160, 131, 20);
    upWebLinklable->setGeometry(r1);
    //upWebLinklable->setParent(ui);
    QObject::connect(upWebLinklable,SIGNAL(linkActivated(QString)),this,SLOT(openUrl_slot(QString)));

    //check if can access the internet
    //QHostInfo::lookupHost("www.baidu.com",this,SLOT(onLookupHost(QHostInfo)));
    dstBinFilePath = qApp->applicationDirPath().append("\\UpgradeBin");
    ui->upUpgradeButton->setDisabled(true);

}

UpgradeDialog::~UpgradeDialog()
{
    delete ui;
}
/*
void UpgradeDialog::onLookupHost(QHostInfo host)
{

    if (host.error() != QHostInfo::NoError)
    {
        qDebug() << "Lookup failed:" << host.errorString();
        isNetworkAccessable = true;
    }
    else{
        isNetworkAccessable = false;
    }

}
*/
/*
//请求完成 文件下载成功
void UpgradeDialog::httpDowloadFinished(){
    //刷新文件
    dstbinfile->flush();
    dstbinfile->close();
    dstbinfile=0;
    isUpgradefileDownloaded = true;
    emit signalDownloadFinished(); //send signal to analysis version from bin file
}
*/
/*
void UpgradeDialog::doDownload(QUrl fileURL,QFile *dstFile)
{
    qDebug() << "doDownload30";
    //int total = -1;
    QNetworkAccessManager * ma = new QNetworkAccessManager;
    QNetworkRequest headReq(fileURL);
    //qDebug()<<"获取下载文件的大小......";
    headReq.setRawHeader("User-Agent", "");  //Content-Length

    QNetworkReply*  headReply = NULL;
    bool connectError = false;
    int tryTimes = 1;
    //如果失败,连接尝试3次;
    do{
//        qDebug()<<"正在进行"<<4 - tryTimes<<"次连接尝试...";
        connectError = false;
        headReply =  ma->head(headReq);
        if(!headReply)
          {
            connectError = true;
            continue;
          }
        QEventLoop loop;
        //connect(headReply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();
        connectError = (headReply->error() != QNetworkReply::NoError);
//        if(connectError)
//            qDebug()<<"连接失败!";
        headReply->deleteLater();
      } while (connectError && --tryTimes);

//    qDebug() << headReply << "headReply^^^^^^^^^^^^^^^^^^^^^^^^^^";
    int statusCode = headReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if(statusCode == 302)
    {
        QUrl newUrl = headReply->header(QNetworkRequest::LocationHeader).toUrl();
        if(newUrl.isValid())
          {
//            qDebug()<<"重定向："<<newUrl;
            fileURL = newUrl.toString();
            return doDownload(fileURL);
          }
    }
    else
    {
        binfileLen = headReply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        qDebug() << "binFile len:" << binfileLen;

        HttpThread *thread=new HttpThread(fileURL,dstFile,binfileLen);

        //connect(thread,SIGNAL(progressChanged(long)),this,SLOT(progressChanged(long)));
        connect(thread,SIGNAL(finish(bool)),this,SLOT(httpDowloadFinished(bool)));
        thread->start();
     }

    headReply->deleteLater();
    ma->deleteLater();
}
*/
void UpgradeDialog::openUrl_slot(QString str)
{
    QDesktopServices::openUrl(QUrl(str));
}
void UpgradeDialog::updateCurrentVersionSlot(IR_MCU_Version_t * mcuVersion)
{
    //currentMcuVersionYear = 2017;
    //currentMcuVersionMonth = 07;
    //currentMcuVersionDay = 07;

    QString currentVersionStr = QString::asprintf("%02x%02x%02x%02x",
                                                 mcuVersion->year_high, mcuVersion->year_low,
                                                 mcuVersion->month, mcuVersion->day);

    currentMcuVersion = currentVersionStr.toInt();

    ui->upCurrentlineEdit->setText(currentVersionStr);

    qDebug() << "get Current mcu version:" << currentVersionStr;

   // ui->upFreshCurrentButton->setStyleSheet("QPushButton{image: url(:/new/icon/resource-icon/accepted-48.png)");
}

void UpgradeDialog::SendCmd2GetCurrentVersion()
{
    qDebug() << "send signal to mainwindow to getcurrent mcu version";
    emit getVersionSignal();
}

/*
void UpgradeDialog::analysisVersionfromBin()
{
    //analysis version from bin
    int  versionYear = 2017;
    int  versionMonth = 8;
    int  versionDay = 24;
    QString logstr;
    QString tmp = QString::number(versionYear).append("_").append(QString::number(versionMonth)).append("_").append(QString::number(versionDay));
    ui->upAvailablelineEdit->setText(tmp);

    if(versionYear > currentMcuVersionYear
       || (versionYear == currentMcuVersionYear && versionMonth > currentMcuVersionMonth)
       || (versionYear == currentMcuVersionYear && versionMonth == currentMcuVersionMonth && versionDay > currentMcuVersionDay))
    {
        logstr ="There is new version to upgrade,press Upgrade button to start";
        ui->upStatusText->setText(logstr);
        ui->upUpgradeButton->setEnabled(true);
    }
    else
    {
        ui->upStatusText->setText("It's latest version,no need to upgrade");
        ui->upUpgradeButton->setDisabled(true);
        qDebug() <<"no need to upgrade!";
    }
}
*/
void UpgradeDialog::checkForMcuUpgrade()
{
    //need to download upgrade files from github,and get the version
        QString srcBinFilePath;

    //step 1:download bin
        if(isNetworkAccessable)
        {
            //download from github
            srcBinFilePath = "https://github.com/barrycool/IR_stm32f103VE/blob/master/IR_stm32f103";

            QUrl  srcBinFileUrl = QUrl(srcBinFilePath);
            QString filename = srcBinFileUrl.fileName();
            dstBinFilePath = dstBinFilePath.append("\\filename");
            dstbinfile = new QFile;
            dstbinfile->setFileName(dstBinFilePath);
            //doDownload(srcBinFileUrl,dstbinfile);

            /*

            dstbinfile->setFileName(dstBinFileName);
            dstbinfile->open((QIODevice::WriteOnly));
            avatorManager = new QNetworkAccessManager(this);
            //get方式请求 如需加密用post
            avatorReply = avatorManager->get(QNetworkRequest(srcBinFileUrl));
            if(!avatorReply)
            {
                qDebug() << "network error,get avatorReply fail ";
                return;
            }
            connect(avatorReply, SIGNAL(readyRead()), this, SLOT(httpDowload()));
            //QObject::connect(avatorReply,SIGNAL(readyRead()),this,SLOT(httpDowload()));//数据写入
            QObject::connect(avatorReply,SIGNAL(finished()),this,SLOT(httpDowloadFinished()));//请求完成
            */
        }
        else
        {
            //get from local disk
            QString str = "Your Network is not avilable,,Do you want to upgrade from Local file?";
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
            //QFile::copy(srcBinFilePath,dstBinFileName);

        }
}
void UpgradeDialog::upChooseLocalFileButton_slot()
{
    uint32_t checksum;
    QString logstr;
    QString path = QFileDialog::getOpenFileName(this, "choose upgrade file", "", "bin file(*.bin)");
    dstBinFilePath = path;
    if (path.size() != 0)
    {
        if (check_valid_upgrade_bin_version(path, availableMcuVersion, checksum))
        {
            if(availableMcuVersion <= currentMcuVersion)
            {
                logstr ="currentMcuVersion is newer than local upgrade file,no need to upgrade";
                qDebug() << logstr;
                ui->upStatusText->setText(logstr);
            }
            else
            {
                logstr = "local upgrade file is newer than currentMcuVersion,Please upgrade";
                qDebug() << logstr;
                ui->upStatusText->setText(logstr);
            }

            ui->upAvailablelineEdit->setText(QString::asprintf("%X", availableMcuVersion));
            //ui->LB_checkSum->setText(QString::asprintf("%X", checksum));
            ui->upUpgradeButton->setEnabled(true);
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
void UpgradeDialog::upCancelButton_slot()
{
    //upgrade_cancel(ui->upProgressBar);
    ui->upProgressBar->setValue(0);
    upgradeCmdList.clear();

    this->close();//off upgrade it's self

}
void UpgradeDialog::upUpgradeButton_slot()
{
    if(!isUpgradefileDownloaded)
    {
        checkForMcuUpgrade(); //if binfile has not been download,download it first
    }

    //ui->upCancelButton->setDisabled(true);
    ui->upProgressBar->setMinimum(0);
    ui->upProgressBar->setMaximum(0);
    //ui->upProgressBar->setValue(0);
    ui->upProgressBar->show();

    QFile binFile(dstBinFilePath);
    if(!binFile.open(QIODevice::ReadOnly))
    {
        ui->upStatusText->setText("binFile is crrupt!\n");
        //QMessageBox::critical(this,"DownLoad Error","Download file " + binPath +"Error");
        return;
    }
    ui->upStatusText->setText("Start Upgrade");
    QDataStream in(&binFile);
    char* buf = (char *)malloc(UPGRADE_PACKET_SIZE);

    while(in.readRawData(buf,UPGRADE_PACKET_SIZE)>0)
    {
        //ui->upStatusText->append("Downloading...");
        upgradePacketList.append(buf);

    }

    //ui->upStatusText->append("Done");
    binFile.close();

    getUpgradeCmdList();
    sendUpgradePacket();


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
uint32_t crc32;
void UpgradeDialog::addUpgradeFinishPacket()
{
    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);

    struct frame_t *frame = (struct frame_t *)buf;

    frame->header = FRAME_HEADER;
    frame->data_len = sizeof(struct frame_t);
    frame->seq_num = seqnum++;
    frame->msg = UPGRADE_FINISH;

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    upgradeCmdList.append(buf);

}

void UpgradeDialog::addUpgradeBinPacket()
{
    for(int i=0;i<upgradePacketList.size();i++)
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
        IR_upg_packet.packet_id1 = i & 0xFF;
        IR_upg_packet.packet_id2 = (i >> 8) & 0xFF;

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
    addUpgradeFinishPacket();
    qDebug() << "getUpgradeCmdList:size= " << upgradeCmdList.size();
    progressStep = 100/upgradeCmdList.size();
    ui->upProgressBar->setMinimum(0);
    ui->upProgressBar->setMaximum(100);
    ui->upProgressBar->setValue(0);
}

void UpgradeDialog::sendUpgradePacket()
{

    if(upgradeCmdList.size() >= 0)
    {
        uint8_t buf[BUF_LEN];
        memset(buf,0x0,BUF_LEN);
        struct frame_t *frame = (struct frame_t *)upgradeCmdList.at(0);
        ui->upProgressBar->setValue(ui->upProgressBar->value() + progressStep);
        cmdSemaphore->acquire();

        emit sendCmdSignal((char*)buf,frame->data_len + 1);
        upgradeCmdList.removeAt(0);
    }
    else
    {
        ui->upProgressBar->setValue(99);
    }
}
void UpgradeDialog::ackReceivedSlot()
{
    cmdSemaphore->release();
    sendUpgradePacket();
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
