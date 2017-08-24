#include "upgradedialog.h"
#include "ui_upgradedialog.h"
static bool isUpgradefileDownloaded = 0;
static bool isNetworkAccessable = 0;
QFile *dstbinfile;

UpgradeDialog::UpgradeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpgradeDialog)
{
    ui->setupUi(this);
    ui->upProgressBar->hide();
    QObject::connect(ui->upUpgradeButton,SIGNAL(clicked()),this,SLOT(upUpgradeButton_slot()));
    QObject::connect(ui->upCancelButton,SIGNAL(clicked()),this,SLOT(upCancelButton_slot()));
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
    connect(this,SIGNAL(signalDownloadFinished()),parent,SLOT(analysisVersionfromBin()));
    cmdSemaphore = new QSemaphore(1);

    upWebLinklable = new QLabel( "<a href = http://www.mediatek.inc >www.mediatek.inc</a>", this );
    QRect r1(340, 160, 131, 20);
    upWebLinklable->setGeometry(r1);
    //upWebLinklable->setParent(ui);
    QObject::connect(upWebLinklable,SIGNAL(linkActivated(QString)),this,SLOT(openUrl_slot(QString)));

    //check if can access the internet
    //QHostInfo::lookupHost("www.baidu.com",this,SLOT(onLookupHost(QHostInfo)));
    dstBinFilePath = qApp->applicationDirPath().append("\\UpgradeBin");

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

    uint8_t yearHigh = mcuVersion->year_high;
    uint8_t yearLow = mcuVersion->year_low;

    currentMcuVersionMonth = mcuVersion->month;
    currentMcuVersionDay = mcuVersion->day;
    currentMcuVersionYear = QString::number(yearHigh).append(QString::number(yearLow)).toInt();

    QString tmp = QString::number(currentMcuVersionYear).append("_").append(QString::number(currentMcuVersionMonth)).append("_").append(QString::number(currentMcuVersionDay));
    ui->upCurrentlineEdit->setText(tmp);

    qDebug() << "get Current mcu version:" << tmp;

    ui->upFreshCurrentButton->setStyleSheet("QPushButton{image: url(:/new/icon/resource-icon/accepted-48.png)");
}

void UpgradeDialog::SendCmd2GetCurrentVersion()
{
    qDebug() << "send signal to mainwindow to getcurrent mcu version";
    emit getVersionSignal();
}
void UpgradeDialog::analysisVersionfromBin()
{
    //analysis version from bin
    int  versionYear = 2017;
    int  versionMonth = 8;
    int  versionDay = 24;
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
             upUpgradeButton_slot();
             return;
        }
        else
        {
            ui->upUpgradeButton->setEnabled(true);
        }

    }
    else
    {
        ui->upStatusText->setText("It's latest version,no need to upgrade");
        ui->upUpgradeButton->setDisabled(true);
        qDebug() <<"no need to upgrade!";
    }
}

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
            //get from p disk

            dstBinFilePath = "E:/github/downloadtest/downloadtest/peerwireclient.cpp";
            /*
            srcBinFilePath = "\\gcn.mediatek.inc\\MSZ_DC\\HSDSA1_SA5\\QT5\\watermelon-war-client\\watermelon_war.exe";
            QFile *srcBinFile = new QFile;
            srcBinFile->setFileName(srcBinFilePath);
            */
            dstbinfile = new QFile;
            dstbinfile->setFileName(dstBinFilePath);

            if(dstbinfile->exists())
            {
                //dstbinfile->remove();
                //QFile::remove(dstBinFileName);
                binfileLen = dstbinfile->size();
                qDebug() << "bin file is exist! filelen: " <<binfileLen;
                isUpgradefileDownloaded = true;
                emit signalDownloadFinished();  //send signal to analysis version from bin file
            }

            //QFile::copy(srcBinFilePath,dstBinFileName);

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
    ui->upProgressBar->setMaximum(100);
    ui->upProgressBar->setValue(0);
    ui->upProgressBar->show();

    QFile binFile(dstBinFilePath);
    if(!binFile.open(QIODevice::ReadOnly))
    {
        ui->upStatusText->setText("binFile is crrupt!\n");
        //QMessageBox::critical(this,"DownLoad Error","Download file " + binPath +"Error");
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
uint32_t crc32;
void UpgradeDialog::addUpgradeFinishPacket()
{
    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);

    uint32_t file_len = binfileLen; //upgrade bin file len

    //fseek(upgrade_file, 0, SEEK_END);
    //file_len = ftell(upgrade_file);

    struct frame_t *frame = (struct frame_t *)buf;

    frame->header = FRAME_HEADER;
    frame->data_len = sizeof(struct frame_t);
    frame->seq_num = seqnum++;
    frame->msg = UPGRADE_FINISH;

    buf[frame->data_len++] = file_len & 0xFF;
    buf[frame->data_len++] = (file_len >> 8) & 0xFF;
    buf[frame->data_len++] = (file_len >> 16) & 0xFF;
    buf[frame->data_len++] = (file_len >> 24) & 0xFF;

    buf[frame->data_len++] = crc32 & 0xFF;
    buf[frame->data_len++] = (crc32 >> 8) & 0xFF;
    buf[frame->data_len++] = (crc32 >> 16) & 0xFF;
    buf[frame->data_len++] = (crc32 >> 24) & 0xFF;

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
    addUpgradeFinishPacket();
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

        emit sendCmdSignal((char*)buf,frame->data_len + 1);
        upgradeCmdList.removeAt(0);
    }
    else
    {
        //ui->upProgressBar->setMaximum(100);
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

typedef uint32_t u32;
//软件CRC32 u32数据计算
u32 upgrade_crc32(u32 *ptr, u32 len)
{
    u32 xbit;
    u32 data;
    u32 CRC32 = 0xFFFFFFFF;
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
/*
void upgrade_calucate_bin_check_sum()
{
    uint32_t buf[20 * 1024];
    uint32_t read_cnt;

    fseek(dstbinfile, 0, SEEK_SET);

    read_cnt = fread(buf, 4, 5120, dstbinfile);
    crc32 = upgrade_crc32(buf, read_cnt);
}
*/
