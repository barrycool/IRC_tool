#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTcpSocket>

#define DEFAULT_BAUDRATE    "115200"
#define DEFAULT_DATABIT     8
#define DEFAULT_CHECKBIT    0
#define DEFAULT_STOPBIT     1

/*--------Lianlian add for cmd send nack and resend--------*/
int seqnum = 0;
uint8_t backupCmdBuffer[BUF_LEN];
int backupCmdBufferLen = 0;
int resendCount = 1;
/*--------Lianlian add for cmd send nack and resend  end--------*/
static bool isInit = 0;
QStringList IR_protocols;// = {"sony"};
QStringList IR_SIRCS_devices[IR_devices_MAX] = {{"BDP0", "soundbar0"}};

QString logstr = NULL;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    output_log("start...",1);
    output_log("setup ui...",1);
    ui->AgingTestSubWindow->showMaximized();
    ui->actionIRWave->setDisabled(true);
    //ui->leStackedWidget->setCurrentIndex(0);
    ui->atStackedWidget->setCurrentIndex(1);
    currentMcuVersion = 0;
    availableMcuVersion = 0;

    upgradethred = new UpgradeThread();
    connect(upgradethred,SIGNAL(finish(bool)),this,SLOT(httpDowloadFinished(bool)));
    connect(upgradethred,SIGNAL(getVersionSignal()),this,SLOT(getCurrentMcuVersion()));
    upgradethred->start();

    settings = new QSettings("Mediatek","Smart_IR");

    use_TCP.setText("use TCP");
    use_TCP.setCheckable(true);
    use_TCP.setChecked(settings->value("use_tcp", 0).toBool());
    connect(&use_TCP, QAction::triggered, this, on_action_use_tcp);
    ui->menuSetting->addAction(&use_TCP);

    portBox = new QComboBox;
    SerialPortListQAction =  ui->mainToolBar->insertWidget(ui->actionOpenUart,portBox);

    if (use_TCP.isChecked())
    {
        SerialPortListQAction->setVisible(false);
    }

    settings->setValue("Tool_Version",VERSION);
    output_log("setting serial port...",1);
    ui->actionPort_Setting->setDisabled(true);//disable uart setting for now
    on_actionOpenUart_triggered();
    QString baudrate = DEFAULT_BAUDRATE;
    portSetting.baudRate = baudrate.toInt();
    portSetting.checkBit = DEFAULT_CHECKBIT;
    portSetting.dataBit = DEFAULT_DATABIT;
    portSetting.stopBit = DEFAULT_STOPBIT;

    this->lw = NULL;

    QString appPath = qApp->applicationDirPath();
    keyMapDirPath = appPath.append("\\KeyMapConfig");

    logstr = "get keyMap Dir Path :";
    logstr.append(keyMapDirPath);
    qDebug() << logstr;
    output_log(logstr,0);

    //for Aging Test SubWindow
    loadInsetIrMapTable();
    connect(ui->atCustomerCombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &set_IR_device);
    connect(ui->atDeviceCombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &set_IR_command_list);
/*
    QWidget *wContainer = new QWidget(ui->atScriptlistWidget);
    QHBoxLayout *hLayout = new QHBoxLayout(wContainer);

    QLabel *isLearningKey = new QLabel(tr("Proto")); //"O/L"
    QLabel *keyName = new QLabel(tr("keyName"));
    QLabel *time = new QLabel(tr("time"));

    hLayout->addWidget(isLearningKey);
    hLayout->addStretch(1);
    hLayout->addWidget(keyName);
    hLayout->addStretch(1);
    hLayout->addWidget(time);
    hLayout->setContentsMargins(5,0,0,5);//关键代码，如果没有很可能显示不出来
    isLearningKey->setFixedWidth(38);
    keyName->setFixedWidth(90);
    time->setFixedWidth(50);
    QListWidgetItem * scriptItem = new QListWidgetItem(ui->atScriptlistWidget);
    scriptItem->setSizeHint(QSize(190,25));
    scriptItem->setToolTip("title");
    scriptItem->setBackgroundColor(Qt::lightGray);
    ui->atScriptlistWidget->setItemWidget(scriptItem,wContainer);
*/
    QString islearning = "Proto";
    QString keyName = "keyName";
    QString time = "time";
    QByteArray ba1 = islearning.toLatin1();
    QByteArray ba2 = keyName.toLatin1();
    QByteArray ba3 = time.toLatin1();
    QString str = QString::asprintf("%-6s%-13s%-6s",ba1.data(),ba2.data(),ba3.data());
    QListWidgetItem *scriptItem = new QListWidgetItem(str,ui->atScriptlistWidget);
    scriptItem->setBackgroundColor(Qt::lightGray);
    scriptItem->setSelected(false);
    ui->atScriptlistWidget->addItem(scriptItem);

    connect(ui->atScriptlistWidget,SIGNAL(dropIntoSignal(QString,int)),this,SLOT(dropSlotforScriptlw(QString,int)));
    connect(ui->atScriptlistWidget, QListWidget::itemDoubleClicked, this, on_itemDoubleClicked);
    connect(ui->atScriptlistWidget, QListWidget::itemClicked, this, on_itemClicked);
    connect(ui->atScriptlistWidget,SIGNAL(internalMoveSiganl()),this,SLOT(update_IR_items_List()));
    connect(ui->atScriptlistWidget,SIGNAL(dragLeaveEventSiganl(int)),this,SLOT(dragLeaveEventSlot(int)));

    connect(&click_timer, &QTimer::timeout, this, &click_timer_timeout);

    ir_button_Slot_connect();

    connect(&sendcmd_timer, &QTimer::timeout, this, &sendcmdTimeout);
    connect(&serial, SIGNAL(readyRead()), this, SLOT(serial_receive_data()));
    sendcmd_timer.setSingleShot(true);
    //cmdSemaphore = new QSemaphore(1);


    connect(ui->PB_reboot_wifi, QPushButton::clicked, this, on_wifi_setting);
    connect(ui->PB_restore_wifi, QPushButton::clicked, this, on_wifi_setting);
    connect(ui->PB_read_wifi_hotpot, QPushButton::clicked, this, on_wifi_setting);
    connect(ui->PB_set_wifi_hotpot, QPushButton::clicked, this, on_wifi_setting);
    connect(ui->PB_AT_test, QPushButton::clicked, this, on_wifi_setting);

    connect(&socket, QTcpSocket::readyRead, this, serial_receive_data);
    connect(&socket, QTcpSocket::stateChanged, this, on_tcp_connect_state);

    fupdiaglog = NULL;
    isInit = 1;
}

MainWindow::~MainWindow()
{
    if (socket.isOpen())
    {
        socket.close();
    }

    if (serial.isOpen())
    {
        close();
    }

    delete ui;
}

void MainWindow::on_tcp_connect_state(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::ConnectedState)
    {
        ui->actionOpenUart->setIcon(QIcon(":/new/icon/resource-icon/ball_green.png"));
    }
    else if (state == QAbstractSocket::UnconnectedState)
    {
        ui->actionOpenUart->setIcon(QIcon(":/new/icon/resource-icon/ball_yellow.png"));
    }
}

void MainWindow::on_action_use_tcp(bool selected)
{
    settings->setValue("use_tcp", selected);
    SerialPortListQAction->setVisible(!selected);

    if (selected)
    {
        if (serial.isOpen())
        {
            serial.close();
            ui->actionOpenUart->setIcon(QIcon(":/new/icon/resource-icon/ball_yellow.png"));
        }
    }
    else
    {
        if (socket.isOpen())
            socket.close();

        on_actionOpenUart_triggered();
    }
}

void MainWindow::on_wifi_setting()
{
    QString cmd;

    QObject * s = sender();
    if (s->objectName() == "PB_reboot_wifi")
    {
        cmd = "AT+RST";
    }
    else if (s->objectName() == "PB_restore_wifi")
    {
        cmd = "AT+RESTORE";
    }
    else if (s->objectName() == "PB_read_wifi_hotpot")
    {
        cmd = "AT+CWSAP_DEF?";
    }
    else if (s->objectName() == "PB_set_wifi_hotpot")
    {
        cmd = QString::asprintf("AT+CWSAP_DEF=\"%s\",\"%s\",%s,4", ui->LE_softAP_name->text().toLatin1().data(),
                                ui->LE_softAP_passwd->text().toLatin1().data(), ui->LE_softAP_channel->text().toLatin1().data());
    }
    else if (s->objectName() == "PB_AT_test")
    {
        cmd = ui->LE_AT_test->text();
    }

    sendwificmd(cmd);
}

void MainWindow::sendwificmd(QString cmd)
{
    uint8_t buf[255];

    cmd += "\r\n";

    struct frame_t *frame = (struct frame_t *)buf;

    frame->header = FRAME_HEADER;
    frame->data_len = sizeof(struct frame_t);
    frame->seq_num = seqnum++;
    frame->msg = SEND_CMD_TO_UART;

    frame->msg_parameter[0] = cmd.size();
    frame->data_len++;

    strcpy((char*)&frame->msg_parameter[1], cmd.toLatin1().data());
    frame->data_len += frame->msg_parameter[0];


    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    sendCmd2MCU(buf, frame->data_len+1);
}
/*
bool isSameIRType(IR_type_t type1,QString type2)
{
    if(type1 >= IR_TYPE_MAX)
    {
        return false;
    }
    else if(type1 == IR_TYPE_SIRCS && type2 == "Sony")
    {
        return true;
    }
    else if(type1 == IR_TYPE_RC5 && type2 == "RC5")
    {
        return true;
    }
    else if(type1 == IR_TYPE_RC6 && type2 == "RC6")
    {
        return true;
    }
    else if(type1 == IR_TYPE_LEARNING && type2 == "L")
    {
        return true;
    }
    else if(type1 == IR_TYPE_NEC && type2 == "NEC")
    {
        return true;
    }
    else
        return false;

}
*/
void MainWindow::on_itemDoubleClicked(QListWidgetItem * item)
{
    click_timer.stop();
    bool isOK=0;
    IR_item_t ir_item;
    int index = ui->atScriptlistWidget->currentRow();
    if((index == 0) || (item->toolTip() == "title"))
    {
        return;
    }
    qDebug() << "before";
    for(int i=0;i<IR_items.size();i++)
    {
        printIrItemInfo(IR_items.at(i));
    }

    QStringList list = item->text().split(QRegExp("\\W+"), QString::SkipEmptyParts);
    QByteArray typeba = list.at(0).toLatin1();
    QByteArray butba = list.at(1).toLatin1();
    QString newdelay = QInputDialog::getText(NULL, "Input Dialog",
                                                       "Reset delay time:(ms)",
                                                       QLineEdit::Normal,
                                                       list.at(2),
                                                       &isOK);

    if (!isOK)
    {
        newdelay = list.at(2);
    }
    else if(newdelay.isEmpty())
    {
        newdelay = "5000";
    }

    uint32_t delaytime = newdelay.toInt();
    QByteArray delayba = newdelay.toLatin1();

    QString str = QString::asprintf("%-6s%-13s%-6s",typeba.data(),butba.data(),delayba.data());
    item->setText(str);
    //update to IR_items list
    ir_item = IR_items.at(index-1);
    ir_item.delay_time = delaytime;
    IR_items.removeAt(index-1);
    IR_items.insert(index-1,ir_item);

    qDebug() << "after";
    for(int i=0;i<IR_items.size();i++)
    {
        printIrItemInfo(IR_items.at(i));
    }
    //update_IR_items_List();
}
void MainWindow::update_IR_items_List()
{
    //clear IR_items first,then add all item in listwidget to IR_items
     QString button_name;
     IR_items.clear();
     qDebug() << "update_IR_items_List";
     if(ui->atScriptlistWidget->count() <= 1)
     {
         qDebug() << "111";
         return;
     }
     for(int j =1;j < ui->atScriptlistWidget->count();j++)
     {
         IR_item_t IR_item;
         QListWidgetItem *item = ui->atScriptlistWidget->item(j);
         QStringList list = item->text().split(QRegExp("\\W+"), QString::SkipEmptyParts);
         if(list.size()<3)
         {
             qDebug() << "222";
             return;

         }
         IR_item.is_valid = 1;
         IR_item.delay_time = list.at(2).toInt();

         button_name = list.at(1);
         qDebug()  << button_name << list.at(2);

         //step1: find the button keyvalue from keymap
         for(int i =0;i<IR_maps.size();i++)
         {
             if(button_name == IR_maps.at(i).name)
             {
                 qDebug() << "found button:"<< IR_maps.at(i).name;

                 IR_item.IR_type = IR_maps.at(i).IR_type;
                 QByteArray ba = IR_maps.at(i).name.toLatin1();
                 char *tmpBuf = ba.data();

                 if(String2IRLearningItem(IR_maps.at(i).keyValue,&IR_item) != true)
                 {
                     qDebug() << "update_IR_items_List fail";
                     output_log("update_IR_items_List fail",0);
                 }

                 switch(IR_item.IR_type)
                 {
                     case IR_TYPE_SIRCS:
                         memcpy(IR_item.IR_CMD.IR_SIRCS.name,tmpBuf,MAX_NAME_LEN);
                         break;
                     case IR_TYPE_NEC:
                         memcpy(IR_item.IR_CMD.IR_NEC.name,tmpBuf,MAX_NAME_LEN);
                         break;
                     case IR_TYPE_RC6:
                         memcpy(IR_item.IR_CMD.IR_RC6.name,tmpBuf,MAX_NAME_LEN);
                         break;
                     case IR_TYPE_RC5:
                         memcpy(IR_item.IR_CMD.IR_RC5.name,tmpBuf,MAX_NAME_LEN);
                         break;
                     /*case IR_TYPE_JVC:
                         memcpy(IR_item.IR_CMD.IR_JVC.name,tmpBuf,MAX_NAME_LEN);
                         break;*/
                     case IR_TYPE_LEARNING:
                         memcpy(IR_item.IR_CMD.IR_learning.name,tmpBuf,MAX_NAME_LEN);
                         break;
                     default:
                         break;
                 }
             }
         }

         printIrItemInfo(IR_item);
         IR_items.append(IR_item);

       }

}
void MainWindow::click_timer_timeout()
{
    click_timer.stop();
    if (ui->atScriptlistWidget->currentRow() != -1 && ui->atScriptlistWidget->currentRow() != 0)
    {
        if(!serial.isOpen())
        {
            return;
        }
        uint8_t buf[255];

        struct frame_t *frame = (struct frame_t *)buf;

        frame->header = FRAME_HEADER;
        frame->data_len = sizeof(struct frame_t);
        frame->seq_num = seqnum++;
        frame->msg = SET_SEND_IDX;

        frame->msg_parameter[0] = (ui->atScriptlistWidget->currentRow() - 1) & 0xFF;
        frame->data_len++;

        buf[frame->data_len] = CRC8Software(buf, frame->data_len);

        sendCmd2MCU(buf, frame->data_len+1);
    }
}
void MainWindow::on_itemClicked(QListWidgetItem * item)
{
    int row = ui->atScriptlistWidget->row(item);
    if(row == 0 )
    {
        item->setSelected(false);
        return;
    }
    click_timer.start(300);

}

extern bool isUpgradefileDownloaded;
void MainWindow::httpDowloadFinished(bool flag)
{
    (void)flag;

    output_log("upgrade bin download finished!",1);
    emit enableFreshVersion(true);
    uint32_t availableVersion;
    uint32_t checksum;
    QString appPath = qApp->applicationDirPath();
    QString filename = appPath.remove("/debug").remove("/release").append("/IR_stm32f103C8.bin");
    output_log("binPath:" + filename,1);
    //currentMcuVersion = currentMcuVersion -1; //just for test

    if (filename.size() != 0)
    {
        qDebug () << filename;
        if (check_valid_upgrade_bin_version(filename, availableVersion, checksum))
        {
            QString tmp = QString::number(availableVersion,16);
            availableMcuVersion = tmp.toInt();
            output_log("availableMcuVersion is ",1);
            output_log(QString::number(availableMcuVersion),1);

            isUpgradefileDownloaded = 1;

            if(availableMcuVersion <= currentMcuVersion || availableMcuVersion ==0 || currentMcuVersion == 0)
            {
                logstr ="currentMcuVersion is the latest version,no need to upgrade";
                qDebug() << logstr;
                output_log(logstr,1);
                if(fupdiaglog !=NULL)
                {
                    emit updateVersionSignal(currentMcuVersion,availableMcuVersion);
                }
            }
            else
            {
                logstr = "Newer MCU version is available,Do you want to upgrade to the latest version?";
                QMessageBox::StandardButton reply = QMessageBox::question(this, "Upgrade Available ", logstr, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                if(reply == QMessageBox::Yes)
                {
                     on_actionUpgrade_triggered();
                     return;
                }
                qDebug() << logstr;
                output_log(logstr,1);
            }
        }
        else
        {
            output_log("checksum is error ",1);
        }
    }
    else
    {
        output_log("cannot find bin ",1);
    }
    upgradethred->quit();
}

/*--------Lianlian add for cmd send nack and resend--------*/
void saveCmdAsBackup(uint8_t *buf,uint8_t len)
{
    memcpy(backupCmdBuffer,buf,BUF_LEN);
    backupCmdBufferLen = len;
}

void MainWindow::resendBackupCmd()
{
    resendCount++;
    struct frame_t *frame = (struct frame_t *)backupCmdBuffer;
    frame->seq_num++;
    sendCmd2MCU(backupCmdBuffer,backupCmdBufferLen);
}

void MainWindow::sendCmd2MCU(uint8_t *buf,uint8_t len)
{
    saveCmdAsBackup(buf,len);
    //struct frame_t *frame = (struct frame_t *)buf;

    if (use_TCP.isChecked())
    {
        if (socket.isOpen())
        {
            socket.write((char *)buf, len);
            /*------add for debug----------*/
            QString log; //= "send packet:len=" +QString::number(len);

            for(uint8_t j = 0; j< len; j++)
            {
                log +=  QString::asprintf("%02X ", buf[j]); //QString(" %1").arg(buf[j]);
            }
            qDebug() << log;
            output_log(log,0);
            /*------add for debug----------*/
        }
        else
        {
            QMessageBox::critical(this, "TCP", "please connect smart IR first!");
        }

        return;
    }

    if(!serial.isOpen())
    {
        QMessageBox::critical(this, "serial port", "please connect smart IR first!");
        return;
    }

/*------add for debug----------*/
        QString log; //= "send packet:len=" +QString::number(len);

        for(uint8_t j = 0; j< len; j++)
        {
            log +=  QString::asprintf("%02X ", buf[j]); //QString(" %1").arg(buf[j]);
        }
        qDebug() << log;
        output_log(log,0);
/*------add for debug----------*/

    //cmdSemaphore->acquire();

    //serial.setBaudRate(QSerialPort::Baud115200);
    //serial.setParity(QSerialPort::NoParity);
    //serial.setDataBits(QSerialPort::Data8);
    //serial.setStopBits(QSerialPort::OneStop);
    //serial.setFlowControl(QSerialPort::NoFlowControl);

   // serial.clearError();
   // serial.clear();

    serial.write((char *)buf, len); //marked just for test

    /*if(frame->msg != CMD_ACK && frame->msg != UPGRADE_FINISH && frame->msg != SEND_UPGRADE_PACKET && frame->msg != UPGRADE_START)
    {
        sendcmd_timer.start(2000);
    }*/
}
void MainWindow::sendcmdTimeout()
{
    //cmdSemaphore->release();
    /*struct frame_t *frame = (struct frame_t *)backupCmdBuffer;
    if(frame->msg == UPGRADE_FINISH)
    {
        output_log("send UPGRADE_FINISH,500ms timer is triggered!",1);
        serial.close();
        ui->actionOpenUart->setIcon(QIcon(":/new/icon/resource-icon/ball_yellow.png"));
        //QMessageBox::information(this,"Upgrade Finish","Please download the latest Smart_IR Tool by Download/Download MainTool");
        return;
    }*/
    if(resendCount > FAIL_RETRY_TIMES)
    {
        qDebug("retry for %d times still Timeout,quit!",FAIL_RETRY_TIMES);

        logstr = "cmd send fail";
        output_log(logstr,1);

        resendCount = 0;
        emit cmdFailSignal();
        return;
    }

    qDebug() <<"CMD Timeout,resend :" << resendCount;
    logstr = "cmd was handle tiemout,resend ";
    logstr += QString("%1 ").arg(resendCount);
    output_log(logstr,1);

    resendBackupCmd();

}
void MainWindow::sendAck(uint8_t seq_num, uint8_t msg_id)
{
    uint8_t buf[255];

    struct frame_t *frame = (struct frame_t *)buf;

    frame->header = FRAME_HEADER;
    frame->data_len = sizeof(struct frame_t);
    frame->seq_num = seq_num;
    frame->msg = CMD_ACK;

    frame->msg_parameter[0] = msg_id;
    frame->data_len++;

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    sendCmd2MCU(buf, frame->data_len+1);

}



uint8_t buf[BUF_LEN];
qint64 buf_len = 0;
void MainWindow::serial_receive_data()
{
    QString log;

    if (use_TCP.isChecked())
    {
        buf_len += socket.read((char*)buf + buf_len, BUF_LEN - buf_len);
    }
    else
    {
        buf_len += serial.read((char*)buf + buf_len, BUF_LEN - buf_len);
    }

    struct frame_t *frame = (struct frame_t *)buf;

    //cmdSemaphore->release();

    if (frame->header != FRAME_HEADER)
    {
        buf_len = 0;
        //qDebug() << "header error";
        return;
    }

/*
    if (frame->seq_num != backupframe->seq_num)
    {
        buf_len = 0;
        qDebug() << "seq_num error";
        return;
    }
*/

    if (frame->data_len + 1 > buf_len || buf_len < 4)
    {
       // qDebug() << "length error";
        return;
    }

    uint8_t crc = CRC8Software(buf, frame->data_len);

    if (crc != buf[frame->data_len])
    {
        buf_len = 0;
        qDebug() << "CRC32 err";
        output_log("CRC32 err",0);
        return;
    }

    qDebug() << "receive packet:";
    log = "receive packet:";
    //output_log(log,0);

    for(int j = 0; j< buf_len; j++)
    {
        log += QString::asprintf("%02X ", buf[j]); //QString::number(buf[j],10);
        //log += " ";
    }
    qDebug() << log;
    output_log(log,0);

    if(frame->msg == CMD_NACK)
    {
        sendcmd_timer.stop();
        if (frame->msg_parameter[0] == UPGRADE_START || frame->msg_parameter[0] == SEND_UPGRADE_PACKET || frame->msg_parameter[0] == UPGRADE_FINISH)
        {
            qDebug() <<"CMD_NACK of " << frame->msg_parameter[0];
            emit cmdFailSignal();
            return;
        }
        if (frame->msg_parameter[0] == READ_CMD_LIST)
        {
            qDebug() <<"all commands are read out from mcu";
            output_log("NAK: all commands are read out from mcu",1);
        }
        /*if(resendCount >= FAIL_RETRY_TIMES)
        {
            //qDebug() << "NAK :cmd not handled correctly";
            qDebug("retry for %d times still NACK,quit!",FAIL_RETRY_TIMES);
            output_log("NAK :cmd not handled correctly",1);
            resendCount = 0;
            //emit cmdFailSignal();
            return;
        }*/
        //qDebug() <<"CMD_NACK,resend :" << resendCount;
        //resendBackupCmd();

    }
    else if(frame->msg == CMD_ACK)
    {
        sendcmd_timer.stop();
        resendCount=0;

        if (frame->msg_parameter[0] == CLEAR_CMD_LIST)
        {
            set_cmd_list_handle();
        }
        else if (frame->msg_parameter[0] == SET_CMD_LIST)
        {
            set_cmd_list_handle();
        }
        else if (frame->msg_parameter[0] == UPGRADE_START ||frame->msg_parameter[0] == SEND_UPGRADE_PACKET)
        {
            //qDebug() << "receive ack of " << frame->msg_parameter[0];
            //Sleep(200);
            emit receiveAckSignal(frame->msg_parameter[0]);
        }
        else if (frame->msg_parameter[0] == REAL_TIME_SEND)
        {
            qDebug() << "REAL_TIME_SEND success";
            output_log("REAL_TIME_SEND success",1);
        }
        else if (frame->msg_parameter[0] == START_SEND)
        {
            ui->atStartButton->setText("Stop");
        }
        else if (frame->msg_parameter[0] == PAUSE_SEND)
        {
           ui->atStartButton->setText("Start");
        }
        else if (frame->msg_parameter[0] == UPGRADE_FINISH)
        {
            serial.close();
            ui->actionOpenUart->setIcon(QIcon(":/new/icon/resource-icon/ball_yellow.png"));
        }
    }
    else if (frame->msg == SET_CMD_LIST)
    {
        sendcmd_timer.stop();
        sendAck(frame->seq_num, frame->msg);
        uint8_t index = frame->msg_parameter[0];

        qDebug() << "receive cmd list , index = " << index;
        logstr = "receive cmd list , index = ";
        logstr += QString("%1 ").arg(index);
        output_log(logstr,0);

        IR_item_t ir_item;

        memcpy(&ir_item,frame->msg_parameter +1 ,sizeof(IR_item_t));

        //add to IR_items list
        IR_items.append(ir_item);
        //add to script list widget
        char tmpBuf[MAX_NAME_LEN];
        QString name;
        switch(ir_item.IR_type)
        {
            case IR_TYPE_SIRCS:
                memcpy(tmpBuf,ir_item.IR_CMD.IR_SIRCS.name,MAX_NAME_LEN);
                break;
            case IR_TYPE_NEC:
                memcpy(tmpBuf,ir_item.IR_CMD.IR_NEC.name,MAX_NAME_LEN);
                break;
            case IR_TYPE_RC6:
                memcpy(tmpBuf,ir_item.IR_CMD.IR_RC6.name,MAX_NAME_LEN);
                break;
            case IR_TYPE_RC5:
                memcpy(tmpBuf,ir_item.IR_CMD.IR_RC5.name,MAX_NAME_LEN);
                break;
            /*case IR_TYPE_JVC:
                memcpy(tmpBuf,ir_item.IR_CMD.IR_JVC.name,MAX_NAME_LEN);
                break;*/
            case IR_TYPE_LEARNING:
                memcpy(tmpBuf,ir_item.IR_CMD.IR_learning.name,MAX_NAME_LEN);
                break;
            default:
                break;
        }
        name = QString(QLatin1String(tmpBuf));
        qDebug() << " name = " << name;
        qDebug() << " ir_type = " << ir_item.IR_type;
        logstr = "name = ";
        logstr.append(name);
        output_log(logstr,0);

        atAddItem2ScriptListWidget(ir_item.IR_type,name,ir_item.delay_time);

    }
    else if (frame->msg == MCU_VERSION)
    {
        //cmdSemaphore->release();
        sendcmd_timer.stop();
        //sendAck((void *)buf);
        sendAck(frame->seq_num, frame->msg);
        qDebug() << "receive MCU_VERSION: ";
        IR_MCU_Version_t mcuVersion;

        memcpy(&mcuVersion, frame->msg_parameter, sizeof(IR_MCU_Version_t));
        QString currentVersionStr = QString::asprintf("%02x%02x%02x%02x",
                                                     mcuVersion.year_high, mcuVersion.year_low,
                                                     mcuVersion.month, mcuVersion.day);

        currentMcuVersion = currentVersionStr.toInt();
        output_log("currentMcuVersion is ",1);
        output_log(QString::number(currentMcuVersion),1);

        emit updateVersionSignal(currentMcuVersion,availableMcuVersion);
    }
    else if (frame->msg == REAL_TIME_RECV)
    {
        //sendcmd_timer.stop();
        sendAck(frame->seq_num, frame->msg);

        uint8_t len = frame->msg_parameter[0];
        uint8_t wavedata[len];

        memcpy(wavedata,frame->msg_parameter+1,len);
        QString butname = ui->leButtonText->text();

        ui->leKeyTextEdit->clear();

        if(ui->leDebugModeCheckBox->isChecked() && lw != NULL){
            qDebug() << "debug mode: send to learning wave ui ";
            lw->setWaveData(butname,wavedata,len);
            emit send2learningwave();
        }
        else if(!ui->leDebugModeCheckBox->isChecked())
        {
            qDebug() << "non-debug mode: encode the data ";
            IR_encode(frame->msg_parameter + 1, len);
            //key value to string

            ui->leStartRecordBut->setEnabled(true);
            QString tmpStr = IRLearningItem2String(len);

            ui->leKeyTextEdit->setText(tmpStr);
            ui->leStartRecordBut->setEnabled(true);
        }

    }
    else if (frame->msg == REPORT_SENDING_CMD)
    {
        logstr = "current sending cmd: " + QString::asprintf("%d\n", frame->msg_parameter[0]);
        qDebug() <<logstr;
        output_log(logstr,1);
        ui->atScriptlistWidget->setCurrentRow(frame->msg_parameter[0]+1);
    }
    else if (frame->msg == RECV_CMD_FROM_UART)
    {
        buf[frame->data_len] = 0;

        if (ui->TE_wifi_log->document()->lineCount() >= 200)
        {
            ui->TE_wifi_log->clear();
        }

        QString msg((char*)frame->msg_parameter);
        ui->TE_wifi_log->append(msg);

        QTextCursor TC = ui->TE_wifi_log->textCursor();
        TC.movePosition(QTextCursor::End);
        ui->TE_wifi_log->setTextCursor(TC);

        if (msg.contains("CWSAP_DEF"))
        {
            QStringList sects = msg.split(',');

            if (sects.size() >= 3)
            {
                QString name = sects[0];
                name = name.mid(name.indexOf('\"') + 1, name.lastIndexOf('\"') - name.indexOf('\"') - 1);
                ui->LE_softAP_name->setText(name);

                QString passwd = sects[1];
                passwd = passwd.mid(passwd.indexOf('\"') + 1, passwd.lastIndexOf('\"') - passwd.indexOf('\"') - 1);
                ui->LE_softAP_passwd->setText(passwd);

                QString channel = sects[2];
                ui->LE_softAP_channel->setText(channel);
            }
        }
    }

    buf_len = 0;
}

void MainWindow::printIrItemInfo(IR_item_t ir_item)
{
    QString str = QString::number(ir_item.IR_type,16).append(":");
    switch(ir_item.IR_type)
    {
        case IR_TYPE_SIRCS:
            str.append(ir_item.IR_CMD.IR_SIRCS.name);
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_SIRCS.IR_address,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_SIRCS.IR_ext_3_address,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_SIRCS.IR_ext_5_address,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_SIRCS.IR_command,16));
            break;
        case IR_TYPE_NEC:
            str.append(ir_item.IR_CMD.IR_NEC.name);
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_NEC.IR_header_high,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_NEC.IR_address,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_NEC.IR_address_ext,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_NEC.IR_command,16));
            break;
        case IR_TYPE_RC6:
            str.append(ir_item.IR_CMD.IR_RC6.name);
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_RC6.IR_mode,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_RC6.IR_address,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_RC6.IR_command,16));
            break;
        case IR_TYPE_RC5:
            str.append(ir_item.IR_CMD.IR_RC5.name);
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_RC5.IR_address,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_RC5.IR_command,16));
            break;
        /*case IR_TYPE_JVC:
            str.append(ir_item.IR_CMD.IR_JVC.name);
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_JVC.IR_address,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_JVC.IR_command,16));*/
                break;
        case IR_TYPE_LEARNING:
            str.append(ir_item.IR_CMD.IR_learning.name);
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_learning.bit_data,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_learning.bit_data_ext_16,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_learning.bit_number,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_learning.header_high,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_learning.header_low,16));

            for(int i=0;i < IR_LEARNING_PLUSE_CNT;i++){
                str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_learning.pluse_width[i],16));
            }
            break;
        default:
            break;
    }
    str.append(":" + QString::number(ir_item.delay_time));
    qDebug() << str;
    output_log(str,0);
}
/*--------Lianlian add for cmd send nack and resend --- end--------*/
void MainWindow::portChanged(int index)
{
    qDebug()<<"portChanged to: "<< index;
    if(index == -1)
        return;

    settings->setValue("SeialPortName",portBox->currentText());

    logstr = "portChanged to ";
    logstr.append(portBox->currentText());
    output_log(logstr,1);

    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        if(info.portName() == portBox->currentText())
        {
            serial.setPort(info);
            serial.close();//close it first
            upgradethred->serialSetReady(false);
            Sleep(200);
            break;
        }
    }
    serial.setPortName(portBox->currentText());

    if(serial.open(QIODevice::ReadWrite))
    {
        ui->actionOpenUart->setIcon(QIcon(":/new/icon/resource-icon/ball_green.png"));
        serial.setBaudRate(QSerialPort::Baud115200);
        serial.setFlowControl(QSerialPort::NoFlowControl);
        upgradethred->serialSetReady(true);
    }
    else
    {
        ui->actionOpenUart->setIcon(QIcon(":/new/icon/resource-icon/ball_yellow.png"));
        QMessageBox::information(this,"Warning","Open " + serial.portName()+ "fail:" + serial.error());
    }

}
void MainWindow::on_actionAbout_IRC_triggered()
{
    AboutIrc *diaglog = new AboutIrc;
    diaglog->setWindowTitle("Smart_IR Tool Info");
    //diaglog->show();//非模态
    diaglog->exec();//模态
}

void MainWindow::on_actionOpenUart_triggered()
{
    if (use_TCP.isChecked())
    {
        if (!socket.isOpen())
        {
            socket.connectToHost("192.168.4.1", 60001);
        }
        else
        {
            socket.close();
        }

        return;
    }


    disconnect(portBox,SIGNAL(currentIndexChanged(int)), this, SLOT(portChanged(int)));
    if (portBox->count()==0)
    {
        on_actionFresh_triggered();
    }

    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        if(info.portName() == portBox->currentText())
        {
            serial.setPort(info);
            break;
        }
    }
    serial.setPortName(portBox->currentText());
    portSetting.port_name = portBox->currentText();
    settings->setValue("SeialPortName",portBox->currentText());

    if(serial.isOpen())
    {
        qDebug() << "serial" << portBox->currentText() <<"is open,close it";
        serial.close();
        ui->actionOpenUart->setIcon(QIcon(":/new/icon/resource-icon/ball_yellow.png"));
        upgradethred->serialSetReady(false);
        logstr = "serialPort:";
        logstr.append(portBox->currentText()).append("is closed");
        output_log(logstr,1);
    }
    else
    {
        qDebug() << "serial" << portBox->currentText() <<"is close,open it";
        if(serial.open(QIODevice::ReadWrite))
        {
            //serial.setBaudRate(ui->baudrateText->text().toInt());
            ui->actionOpenUart->setIcon(QIcon(":/new/icon/resource-icon/ball_green.png"));
            //serial.setPortName(portBox->currentText());
            serial.setBaudRate(QSerialPort::Baud115200);
            serial.setFlowControl(QSerialPort::NoFlowControl);
            upgradethred->serialSetReady(true);
            logstr = "serialPort:";
            logstr.append(portBox->currentText()).append("is opened");
            output_log(logstr,1);
        }
        else
        {
             ui->actionOpenUart->setIcon(QIcon(":/new/icon/resource-icon/ball_yellow.png"));
            if(isInit){
                QMessageBox::information(this,"Warning","Open " + serial.portName()+ " Fail " + serial.error());
            }
            else
            {
                QString str = "open seial ";
                str.append(portBox->currentText()).append(" fail\n");
                qDebug() << str;
                output_log(str,1);
            }

        }
    }
    if(isInit)
        on_actionFresh_triggered();

    connect(portBox,SIGNAL(currentIndexChanged(int)), this, SLOT(portChanged(int)));

}

void MainWindow::on_actionFresh_triggered()
{
    portBox->clear();
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    qDebug() << "fresh serial:how many ports:" << portList.size();

    logstr = "fresh serial:";
    logstr +=QString("%1 ").arg(portList.size());
    logstr.append("serialport is available\n");
    output_log(logstr,0);

    bool exsit = 0;
    if(portList.size() > 0)
    {
        //1:add the serial portBox
        QSerialPort serialtmp;
        QString oldPortName = settings->value("SeialPortName","COM1").toString();
        foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        {
            serialtmp.setPort(info);
            portBox->addItem(serialtmp.portName());
            if(info.portName() == oldPortName)
            {
                exsit = 1;
            }

        }

        //2.set current idex as last setting
        if(exsit)
        {
            portBox->setCurrentText(oldPortName);
        }
        else
        {
            portBox->setCurrentIndex(0);
        }
    }
}

void MainWindow::on_actionUpgrade_triggered()
{
    if(fupdiaglog == NULL)
    {
        //serial.close();
        fupdiaglog = new UpgradeDialog(this,currentMcuVersion,availableMcuVersion);

        fupdiaglog->setWindowTitle("Upgrade");
        this->connect(fupdiaglog,SIGNAL(UpgradeRejected(bool,uint32_t)),this,SLOT(returnfromUpgrade(bool,uint32_t)));
        this->connect(fupdiaglog,SIGNAL(rejected()),this,SLOT(upgradedialog_reject()));
        connect(this,SIGNAL(enableFreshVersion(bool)),fupdiaglog,SLOT(enableFreshVersionButton(bool)));
        //fupdiaglog->show();//非模态
        fupdiaglog->exec();//模态
    }
    else
    {
        emit updateVersionSignal(currentMcuVersion,availableMcuVersion);
    }
}

void MainWindow::upgradedialog_reject()
{
    qDebug() << "upgradedialog_reject";
    if(fupdiaglog != NULL)
    {
        delete fupdiaglog;
        fupdiaglog = NULL;
    }
}
void MainWindow::returnfromUpgrade(bool needCloseSerial,uint32_t availableVersion)
{
    (void)availableVersion;

    qDebug() << "return from upgrade";
    if(needCloseSerial)
    {
        //serial.close();
        //ui->actionOpenUart->setIcon(QIcon(":/new/icon/resource-icon/ball_yellow.png"));
        //QMessageBox::information(this,"Upgrade Warning","You NEED to download the latest Smart_IR Tool by Download/Download MainTool ");
    }

    bool needUpgradeTool = 0;
    if(fupdiaglog != NULL)
    {
        //delete fupdiaglog;
        fupdiaglog = NULL;
    }
/*
 *  int oldVersion = settings->value("Tool_Version",0).toInt();

//need reconsider...............
    if(availableVersion >= 20170907 && oldVersion < 11)
    {
        needUpgradeTool = 1;
    }
//need reconsider...............
*/
    if(needUpgradeTool)
    {
        QString appPath = qApp->applicationDirPath();
        QString fileDir = appPath.append("/Download_files");
        QDir *dir = new QDir(fileDir);
        if(!dir->exists())
        {
            logstr = fileDir + " doesn't exist";
            qDebug() << logstr;
            output_log(logstr,1);

            bool ok = dir->mkdir(fileDir);
            if( ok )
            {
                logstr = "fileDir created success.";
                qDebug() << logstr;
                output_log(logstr,0);
            }
            else
            {
                logstr = "upgrade fail.";
                qDebug() << logstr;
                output_log(logstr,1);
                return;
            }

        }
        QString filePath = appPath.append("/Smart_IR.exe");
        QFile binFile(filePath);
        if(binFile.exists())
        {
            qDebug() << "Smart_IR.exe exsits,delete it first";
            QFile::remove(filePath);
        }

        QString srcBinFilePath = "https://github.com/barrycool/bin/raw/master/Smart_IR.exe";
        QString cmd = "wget -P " + fileDir + " " + srcBinFilePath;
        system(cmd.toLatin1().data());
        QProcess process(this);
        process.startDetached("Updater.exe");
        this->close();
    }
}
void MainWindow::leSetIRDevice(int index)
{
    ui->leDeviceText->clear();
    foreach(const QString device, IR_SIRCS_devices[index])
    {
        ui->leDeviceText->addItem(device);
    }

}
void MainWindow::on_actionLearningKey_triggered()
{
    disconnect(ui->leCustomerText, SIGNAL(currentIndexChanged(int)), this, SLOT(leSetIRDevice(int)));
    ui->leCustomerText->clear();
    foreach(const QString procotol, IR_protocols)
    {
        qDebug() << procotol;
        ui->leCustomerText->addItem(procotol);
    }
    connect(ui->leCustomerText, SIGNAL(currentIndexChanged(int)), this, SLOT(leSetIRDevice(int)));

    //QMessageBox::information(this,"Guide","Please choose a button from a panel first,then press the button on remote control,wait until key value shows on the edidtbox");

    leSetIRDevice(0);

    ui->learningKeyGroup->setDisabled(false);
    ui->learningKeyGroup->show();
    //ui->AgingTestSubWindow->setHidden(true);
    ui->actionIRWave->setEnabled(true);
    ui->LearningKeySubWindow->showMaximized();
}

void MainWindow::on_actionPort_Setting_triggered()
{
    fport = new ComPort_Setting(this,&(this->portSetting));
    this->connect(fport,SIGNAL(sendsignal()),this,SLOT(returnPortSetting()));
    //fport->portSetting = &(this->portSetting);
    fport->setWindowTitle("Port Setting");
    fport->show();//非模态
    //diaglog->exec();//模态
}
void MainWindow::returnPortSetting()
{
    portBox->setCurrentText(this->portSetting.port_name);

    delete fport;
}

void MainWindow::on_actionAgingTest_triggered()
{
    //isAutotestState = 1;
    //isLearingkeyState = 0;
    //if(ui->AgingTestSubWindow-isHidden() && ui->LearningKeySubWindow->isMaximized())
    //{
        //QString oldCus = ui->atCustomerCombox->currentText(); //backup last selection
        //QString oldDev = ui->atDeviceCombox->currentText();//backup

        //loadInsetIrMapTable();
        //ui->atCustomerCombox->setCurrentText(oldCus);
        //ui->atDeviceCombox->setCurrentText(oldDev);
        ui->AgingTestSubWindow->showMaximized();
        ui->actionIRWave->setDisabled(true);

        //loadInsetIrMapTable();
    //}

}

void MainWindow::ir_button_Slot_connect()
{
/*---------------------lianlian add for Upgrade----------------*/
    // QObject::connect(ui->upDownloadButton,SIGNAL(clicked()),this,SLOT(upDownloadButton_slot()));
    //QObject::connect(ui->upCancelButton,SIGNAL(clicked()),this,SLOT(upCancelButton_slot()));
    //QObject::connect(ui->upCheckUpdateButton,SIGNAL(clicked()),this,SLOT(checkForMcuUpgrade()));

/*---------------------lianlian add for LearingKey----------------*/
    //QObject::connect(ui->leCustomizeButton,SIGNAL(clicked()),this,SLOT(leCustomizeButton_slot()));
    //QObject::connect(ui->leReturnButton,SIGNAL(clicked()),this,SLOT(leReturnButton_slot()));

    QObject::connect(ui->lePower,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leHome,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leEject,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leEnter,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));

    QObject::connect(ui->leRed,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leGreen,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leYellow,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leBlue,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));

    QObject::connect(ui->leUp,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leDown,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leRight,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leLeft,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));

    QObject::connect(ui->leChanneldown,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leChannelup,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leVolumdown,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leVolumup,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));

    QObject::connect(ui->leDigital_0,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leDigital_1,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leDigital_2,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leDigital_3,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leDigital_4,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leDigital_5,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leDigital_6,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leDigital_7,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leDigital_8,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leDigital_9,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));

    QObject::connect(ui->lePlay,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->lePause,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leStop,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leFastforward,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leFastReverse,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leNext,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->lePrev,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leAudio,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leAutoMute,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leDisplay,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leOption,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leSubtitle,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leRecall,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leRepeat,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leNetflix,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leYouTube,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->leTopMenu,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));
    QObject::connect(ui->lePopMenu,SIGNAL(clicked()),this,SLOT(leIrPanel_slot()));

    QObject::connect(ui->leStartRecordBut,SIGNAL(clicked()),this,SLOT(leStartRecordButton_slot()));
    QObject::connect(ui->leAddToListButton,SIGNAL(clicked()),this,SLOT(leAddToListButton_slot()));
    QObject::connect(ui->leRemoveFromListBut,SIGNAL(clicked()),this,SLOT(leRemoveFromListButton_slot()));
    QObject::connect(ui->leSaveKeymapBut,SIGNAL(clicked()),this,SLOT(leSaveKeymapButton_slot()));
    QObject::connect(ui->leClearButton,SIGNAL(clicked()),this,SLOT(leClearButton_slot()));
    QObject::connect(ui->leRealTimeTestBut,SIGNAL(clicked()),this,SLOT(leRealTimeTestButton_slot()));

    /*---------------------lianlian add for AgingTest----------------*/
/*IR Panel disabled for now */
        QObject::connect(ui->atPower,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atHome,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atEject,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atEnter,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));

        QObject::connect(ui->atRed,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atGreen,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atYellow,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atBlue,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));

        QObject::connect(ui->atUp,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atDown,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atRight,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atLeft,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));

        QObject::connect(ui->atChanneldown,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atChannelup,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atVolumdown,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atVolumup,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));

        QObject::connect(ui->atDigital_0,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atDigital_1,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atDigital_2,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atDigital_3,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atDigital_4,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atDigital_5,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atDigital_6,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atDigital_7,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atDigital_8,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atDigital_9,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));

        QObject::connect(ui->atPlay,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atPause,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atStop,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->leFastforward,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atFastReverse,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atNext,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atPrev,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atAudio,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atAutoMute,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atDisplay,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atOption,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atSubtitle,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atRecall,SIGNAL(clicked()),this,SLOT(atIrPanel_slot()));
        QObject::connect(ui->atCustomizeButton,SIGNAL(clicked()),this,SLOT(atCustomizeButton_slot()));
        QObject::connect(ui->atReturnButton,SIGNAL(clicked()),this,SLOT(atReturnButton_slot()));
/*IR Panel disabled for now */

        /*---------------------lianlian add for AgingTest----------------*/
        QObject::connect(ui->atLoadscriptBut,SIGNAL(clicked()),this,SLOT(atLoadscriptBut_slot()));
        QObject::connect(ui->atAddButton,SIGNAL(clicked()),this,SLOT(atAddButton_slot()));
        QObject::connect(ui->atRemoveButton,SIGNAL(clicked()),this,SLOT(atRemoveButton_slot()));
        QObject::connect(ui->atSaveButton,SIGNAL(clicked()),this,SLOT(atSaveButton_slot()));
        QObject::connect(ui->atClear,SIGNAL(clicked()),this,SLOT(atClearScriptWidget()));
        QObject::connect(ui->atRealTimeSendButton,SIGNAL(clicked()),this,SLOT(atRealTimeSendButton_slot()));
        QObject::connect(ui->atDownloadButton,SIGNAL(clicked()),this,SLOT(atDownloadButton_slot()));
        QObject::connect(ui->atStartButton,SIGNAL(clicked()),this,SLOT(atStartButton_slot()));
        //QObject::connect(ui->atLoadKeyMapButton,SIGNAL(clicked()),this,SLOT(atLoadKeyMapButton_slot()));
        QObject::connect(ui->atReadPushButton,SIGNAL(clicked()),this,SLOT(atReadPushButton_slot()));


        QObject::connect(ui->atCustomizeKeyListWidget,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(atCustomizeKeyListWidgetItemClicked_slot(QListWidgetItem*)));
        //QObject::connect(ui->atScriptlistWidget,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(atScriptlistWidgetClicked_slot(QListWidgetItem*)));

      /*---------------------lianlian add for log----------------*/
        QObject::connect(ui->logSaveButton,SIGNAL(clicked()),this,SLOT(logSaveButton_slot()));
        QObject::connect(ui->logClearButton,SIGNAL(clicked()),this,SLOT(logClearButton_slot()));
        QObject::connect(ui->logSaveasButton,SIGNAL(clicked()),this,SLOT(logSaveAsButton_slot()));
        QObject::connect(ui->actionSave_log,SIGNAL(triggered(bool)),this,SLOT(logSaveButton_slot()));
        QObject::connect(ui->actionSaveasLog,SIGNAL(triggered(bool)),this,SLOT(logSaveAsButton_slot()));
}

/*---------------------lianlian add for Upgrade----------------*/
void MainWindow::sendCmdforUpgradeSlot(uint8_t *buf,int len)
{
    //qDebug() << "sendCmdforUpgradeSlot";
    /*if(!serial.isOpen())
    {
        logstr = "serial is not open!";
        output_log(logstr,1);
        QMessageBox::StandardButton reply = QMessageBox::warning(this,"Error","Please Open Serial Port First!\n");
        if(reply == QMessageBox::Ok)
        {
            fupdiaglog->reject();
        }

        return;
    }*/
    sendCmd2MCU(buf, len);
}

void MainWindow::getCurrentMcuVersion()
{
    /*if(!serial.isOpen())
    {
        QMessageBox::warning(this,"Error","Please Open Serial Port First!\n");
        qDebug() << "serial is not open,cannot check mcu's version";
        logstr = "serial is not open!";
        output_log(logstr,0);
        return;
    }*/

    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);
    struct frame_t *frame = (struct frame_t *)buf;

    frame->data_len = sizeof(struct frame_t);
    frame->header = FRAME_HEADER;
    frame->msg = READ_MCU_VERSION;
    frame->seq_num = seqnum++;
    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    sendCmd2MCU(buf, frame->data_len + 1);
}

void MainWindow::upCancelButton_slot()
{
    ui->AgingTestSubWindow->showMaximized();
}

/*---------------------lianlian add for AgingTest----------------*/
//Load Key Map

int totalIrProtocols = 0;

void MainWindow::loadInsetIrMapTable()
{
    int irProtocolsCount = 0;
    bool irFinishMap = false;
    IR_protocols.clear();
    for(int i = 0 ; i < IR_devices_MAX ; i++)
    IR_SIRCS_devices[i].clear();
/*
    QString appPath = qApp->applicationDirPath();
    QString insetIrMapTablePath = appPath.append("\\KeyMapConfig");
    qDebug() << insetIrMapTablePath;
*/
    QDir insetIrMapTableDir(keyMapDirPath);
    if(!insetIrMapTableDir.exists())
    {
        logstr = "KeyMapConfig doesn't exist";
        qDebug() << logstr;
        output_log(logstr,1);

        //QMessageBox::information(this,"Error","There's no inset IrMapTable Dir!");
        bool ok = insetIrMapTableDir.mkdir(keyMapDirPath);
        if( ok )
        {
            logstr = "KeyMapConfig created success.";
            qDebug() << logstr;
            output_log(logstr,0);
        }

        return;
    }

    QStringList fileNames = insetIrMapTableDir.entryList();
    if(fileNames.size()==0)
    {
        logstr = "InsetKeyMap files don't exist";
        qDebug() << logstr;
        output_log(logstr,1);
        QMessageBox::information(this,"Error","There's no inset IrMapTable file!");
        return;
    }

    for(int index=0; index<fileNames.size(); index++)
    {
        if(fileNames.at(index)=="." || fileNames.at(index)=="..")
            continue;

        qDebug() << fileNames.at(index);
        QString filename = fileNames.at(index);
        QStringList customDevice = filename.split(QRegExp("[%.]"), QString::SkipEmptyParts);
        /*QStringList fileNamePure = filename.split(".");
        QString filenamePure0 = fileNamePure.at(0);
        QStringList customDevice = filenamePure0.split("_");*/
        QString custom = customDevice.at(0);
        qDebug() << custom;
        QString device = customDevice.at(1);

        logstr = custom + ":" + device;
        output_log(logstr,0);
        logstr ="IR_protocols.count()";
        logstr += QString("%1 ").arg(IR_protocols.count());
        output_log(logstr,0);
        logstr ="irProtocolsCount";
        logstr += QString("%1 ").arg(irProtocolsCount);
        output_log(logstr,0);

        qDebug() << device;
        qDebug() <<"IR_protocols.count()" << IR_protocols.count();
        qDebug() <<"irProtocolsCount" << irProtocolsCount;

        if(IR_protocols.count() == 0)
        {
            IR_protocols<<custom;
            IR_SIRCS_devices[0].clear();
            IR_SIRCS_devices[0]<<device;
            totalIrProtocols = 1;
            irProtocolsCount = 0;
            qDebug() << " IR_protocols.at(0)="<<IR_protocols.at(0);
            qDebug() << " IR_SIRCS_devices[0].at(0)="<<IR_SIRCS_devices[0].at(0);
        }
        else
        {
            irProtocolsCount = 0;
            for(int index=0; index<IR_protocols.count(); index++)
            {
                if(IR_protocols.at(index) == custom)
                {
                    IR_SIRCS_devices[index]<<device;
                    qDebug() << " IR_protocols.at(index)="<<IR_protocols.at(index);
                    qDebug() << " IR_SIRCS_devices[index].at(0)="<<IR_SIRCS_devices[index].at(IR_SIRCS_devices[index].count()-1);
                    irFinishMap = true;
                    break;
                }
                irProtocolsCount++;
            }

            if(irFinishMap == true)
            {
                irFinishMap = false;
                continue;
            }

            if(irProtocolsCount ==IR_devices_MAX)
            {
                QMessageBox::information(this,"Error","IrCustomProtocol exceeds,can't load more");
                return;
            }

            if(irProtocolsCount == IR_protocols.count())
            {
                IR_protocols<<custom;
                IR_SIRCS_devices[irProtocolsCount]<<device;
                totalIrProtocols = irProtocolsCount+1;
                qDebug() << " IR_protocols.at(irProtocolsCount)="<<IR_protocols.at(irProtocolsCount);
                qDebug() << " IR_SIRCS_devices[irProtocolsCount].at(0)="<<IR_SIRCS_devices[irProtocolsCount].at(IR_SIRCS_devices[irProtocolsCount].count()-1);
            }
        }
    }

    //add all protocols to atCustomerCombox
    set_IR_protocol();

    int oldCustomerIdx = settings->value("CustomerIdx",0).toInt();
    set_IR_device(oldCustomerIdx);
}

void MainWindow::set_IR_protocol()
{
    disconnect(ui->atCustomerCombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &set_IR_device);
    disconnect(ui->atDeviceCombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &set_IR_command_list);
    ui->atDeviceCombox->clear();
    ui->atCustomerCombox->clear();
    foreach(const QString procotol, IR_protocols)
    {
        qDebug() << procotol;
        ui->atCustomerCombox->addItem(procotol);
    }

    connect(ui->atCustomerCombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &set_IR_device);
    connect(ui->atDeviceCombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &set_IR_command_list);

    int oldIdx = settings->value("CustomerIdx",0).toInt();
    ui->atCustomerCombox->setCurrentIndex(oldIdx);
}

void MainWindow::set_IR_device(int index)
{

    disconnect(ui->atDeviceCombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &set_IR_command_list);

    qDebug() << "set_IR_device";
     output_log("set_IR_device",0);
    ui->atDeviceCombox->clear();
    foreach(const QString device, IR_SIRCS_devices[index])
    {
        ui->atDeviceCombox->addItem(device);
    }

    int oldIdx = settings->value("DeviceIdx",0).toInt();
    ui->atDeviceCombox->setCurrentIndex(oldIdx);

    set_IR_command_list();
    connect(ui->atDeviceCombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &set_IR_command_list);
}
int index = 0;
void MainWindow::set_IR_command_list()
{
    qDebug() << "set_IR_command_list";
    output_log("set_IR_command_list",0);
    ui->atCustomizeKeyListWidget->clear();
    QString fileName=ui->atCustomerCombox->currentText()+"%"+ui->atDeviceCombox->currentText()+".ini";
    QString appPath = qApp->applicationDirPath();
    QString insetIrMapTablePath = appPath.append("\\KeyMapConfig\\");
    QString filePath = insetIrMapTablePath+fileName;
    qDebug() << filePath;

    QFile *file = new QFile;
    file->setFileName(filePath);
    if(!file->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "filePath open fail";
        logstr = filePath.append(" open fail");
        output_log(logstr,1);
        return;
    }

    QTextStream in(file);
    QString line = in.readLine();//read custom name
    qDebug() << "custom name" << line;
    line = in.readLine();//read device name
    qDebug() << "device name" << line;
    //should add NEC

    IR_maps.clear();
    index = 0;
    while(!in.atEnd()){
        line = in.readLine();
        if(line.contains(','))
        {
            setToKeyListWidget(line);
            saveToIrMaps(line);
        }
    }

    file->close();
    delete file;
    file = NULL;

    settings->setValue("CustomerIdx",ui->atCustomerCombox->currentIndex());
    settings->setValue("DeviceIdx",ui->atDeviceCombox->currentIndex());
}

void MainWindow::atIrPanel_slot()
{
    QPushButton* btn = dynamic_cast<QPushButton*>(sender());
    ui->atButtonText->setText(btn->toolTip());
    //qDebug() << btn->toolTip() << "is clicked in AgingTest ir panel\n";
}

void MainWindow::setToKeyListWidget(QString line)
{
    QString IR_index = NULL;
    QString buttonName = NULL;
    QString str = NULL;

    QStringList list1 = line.split(',');
    IR_index = QString::number(index);
    index++;
    //QByteArray ba = list1.at(1).toLatin1();
    buttonName = list1.at(1);//ba.data();
    qDebug() << "loadkeymap:buttonName " << buttonName;

    str = IR_index.append("     ").append(buttonName/*QString(QLatin1String(buttonName))*/);
    QListWidgetItem *item = new QListWidgetItem(str,ui->atCustomizeKeyListWidget);
    ui->atCustomizeKeyListWidget->addItem(item);
}
void MainWindow::saveToIrMaps(QString line)
{
    IR_map_t IR_map;

    QStringList list1 = line.split(',');
    bool ok;
    IR_map.IR_type = list1.at(0).toInt(&ok,16);
    //QByteArray ba = list1.at(1).toLatin1();
    IR_map.name = list1.at(1);// ba.data();
    qDebug() << "savetoIRmaps: IR_map.name:" << IR_map.name;
    logstr = "savetoIRmaps: IR_map.name:";
    logstr.append(IR_map.name);
    output_log(logstr,0);

    int len = list1.size() - 2 ;
    QString str = list1.at(0);
    QString tmp;
    for(int i = 0; i< len;i++)
    {
        tmp = list1.at(i+2);
        tmp.remove("0x");
        str.append("-");
        str.append(tmp);
        tmp.clear();
    }
    IR_map.keyValue = str;

    qDebug() << "savetoIRmaps: IR_map.keyValue:" << IR_map.keyValue;
    logstr = "savetoIRmaps: IR_map.keyValue:";
    logstr.append(IR_map.keyValue);
    output_log(logstr,0);

    IR_maps.append(IR_map);

}

/*
 * QString to char *
 * const char *c_str2 = str2.toLatin1().data();
 *
 * char * to QString
 * QString string = QString(QLatin1String(c_str2)) ;

*/

void MainWindow::atReadPushButton_slot()
{
    qDebug() << "atReadPushButton_slot:read current cmd list from mcu\n";
    atClearScriptWidget();
    //IR_items.clear();


    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);
    struct frame_t *frame = (struct frame_t *)buf;

    frame->data_len = sizeof(struct frame_t);
    frame->header = FRAME_HEADER;
    frame->seq_num = seqnum++;
    frame->msg = READ_CMD_LIST;

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    sendCmd2MCU(buf, frame->data_len + 1);

}
void MainWindow::atCustomizeKeyListWidgetItemClicked_slot(QListWidgetItem* item)
{
    QString str = item->text();
    QStringList list1 = str.split("     ");
    //QByteArray ba = list1.at(1).toLatin1();
    QString buttonName = list1.at(1);//ba.data();
    ui->atButtonText->setText(/*QString(QLatin1String(*/buttonName);
}
void MainWindow::atScriptlistWidgetClicked_slot(QListWidgetItem* item)
{
    int currentRow = ui->atScriptlistWidget->row(item);
    //qDebug() << "atScriptlistWidgetClicked_slot: currentRow="<< currentRow;
    if(currentRow == 0)
    {
        return;
    }
    char tmpBuf[MAX_NAME_LEN];

    switch(IR_items.at(currentRow-1).IR_type)
    {
        case IR_TYPE_SIRCS:
            memcpy(tmpBuf,IR_items.at(currentRow-1).IR_CMD.IR_SIRCS.name,MAX_NAME_LEN);
            break;
        case IR_TYPE_NEC:
            memcpy(tmpBuf,IR_items.at(currentRow-1).IR_CMD.IR_NEC.name,MAX_NAME_LEN);
            break;
        case IR_TYPE_RC6:
            memcpy(tmpBuf,IR_items.at(currentRow-1).IR_CMD.IR_RC6.name,MAX_NAME_LEN);
            break;
        case IR_TYPE_RC5:
            memcpy(tmpBuf,IR_items.at(currentRow-1).IR_CMD.IR_RC5.name,MAX_NAME_LEN);
            break;
        /*case IR_TYPE_JVC:
            memcpy(tmpBuf,IR_items.at(currentRow-1).IR_CMD.IR_JVC.name,MAX_NAME_LEN);
            break;*/
        case IR_TYPE_LEARNING:
            memcpy(tmpBuf,IR_items.at(currentRow-1).IR_CMD.IR_learning.name,MAX_NAME_LEN);
            break;
        default:
            break;
    }
    QString buttonName = QString(QLatin1String(tmpBuf));
    ui->atButtonText->setText(/*QString(QLatin1String(*/buttonName);
}
void MainWindow::atCustomizeButton_slot()
{
    ui->atStackedWidget->setCurrentIndex(1);

}

void MainWindow::atReturnButton_slot()
{
    ui->atStackedWidget->setCurrentIndex(0);

}
void MainWindow::dragLeaveEventSlot(int row)
{
    qDebug() << "dragLeaveEventSlot:row = " << row;
    /*
    qDebug() << "before";
    for(int i=0;i<IR_items.size();i++)
    {
        printIrItemInfo(IR_items.at(i));
    }
    */
    if(row >= 1 )
    {
        QListWidgetItem *item = ui->atScriptlistWidget->takeItem(row);
        delete item;
        IR_items.removeAt(row-1);
    }
    /*
    qDebug() << "after";
    for(int i=0;i<IR_items.size();i++)
    {
        printIrItemInfo(IR_items.at(i));
    }
    */
}
void MainWindow::dropSlotforScriptlw(QString btnname,int row)
{

    /*
     * bool isOK;
    QString delay = QInputDialog::getText(NULL, "Input Dialog",
                                               "Set delay time:(ms)",
                                                QLineEdit::Normal,
                                               "5000",
                                               &isOK);
     */
    uint32_t delaytime = 5000;
    qDebug() << "drag into:"<<btnname<<"--"<<delaytime<<"---"<<row;
    ui->atScriptlistWidget->setCurrentRow(row);
    add_to_list(btnname,delaytime);
}

void MainWindow::atAddItem2ScriptListWidget(int ir_type,QString button_name,int delaytime)
{
    QString cmd_item;
    QString islearning;
    QString sdelaytime;

    if (ir_type == IR_TYPE_SIRCS)
    {
        islearning = "Sony";
    }
    else if(ir_type == IR_TYPE_NEC)
    {
        islearning = "NEC";
    }
    else if(ir_type == IR_TYPE_RC5)
    {
        islearning = "RC5";
    }
    else if(ir_type == IR_TYPE_RC6)
    {
        islearning = "RC6";
    }
    /*else if(ir_type == IR_TYPE_JVC)
    {
        islearning = "JVC";
    }*/
    else
    {
        islearning = "L";
    }

    cmd_item = button_name;
    sdelaytime = QString("%1").arg(delaytime);
    /*if (IR_item.is_random)
    {
        cmd_item += "--R";
    }*/
/*
    QWidget *wContainer = new QWidget(ui->atScriptlistWidget);
    QHBoxLayout *hLayout = new QHBoxLayout(wContainer);

    QLabel *isLearningKey = new QLabel(islearning);
    QLabel *keyName = new QLabel(cmd_item);
    QLabel *time = new QLabel(sdelaytime);
    //connect(time,SIGNAL(textEdited(const QString &text)),this,SLOT(textChanged_SLOT(const QString &text)));

    hLayout->addWidget(isLearningKey);
    hLayout->addStretch(1);
    hLayout->addWidget(keyName);
    hLayout->addStretch(1);
    hLayout->addWidget(time);
    hLayout->setContentsMargins(5,0,0,5);//关键代码，如果没有很可能显示不出来
    isLearningKey->setFixedWidth(35);
    keyName->setFixedWidth(105);
    time->setFixedWidth(50);

    QListWidgetItem * scriptItem = new QListWidgetItem(ui->atScriptlistWidget);
    scriptItem->setSizeHint(QSize(190,24));
    ui->atScriptlistWidget->setItemWidget(scriptItem,wContainer);
*/
    //QString str = QString("%4  %10%8").arg(islearning).arg(button_name).arg(sdelaytime);
    QByteArray ba1 = islearning.toLatin1();
    QByteArray ba2 = button_name.toLatin1();
    QByteArray ba3 = sdelaytime.toLatin1();
    QString str = QString::asprintf("%-6s%-13s%-6s",ba1.data(),ba2.data(),ba3.data());
    output_log("add:" +str,1);
    QListWidgetItem *scriptItem = new QListWidgetItem(str,ui->atScriptlistWidget);
    ui->atScriptlistWidget->addItem(scriptItem);
    ui->atScriptlistWidget->setCurrentItem(scriptItem); //make KeyMapList always focus on the last item when a new item is added
}
void MainWindow::textChanged_SLOT(const QString &text)
{
    qDebug() << "textChanged_SLOT :" << text;
    //int index = ui->atScriptlistWidget->currentRow()-1;
    //IR_items.at(index).delay_time = text.toInt();
}
void MainWindow::add_to_list(QString button_name,uint32_t delay)
{
    IR_item_t IR_item;
    IR_item.is_valid = 1;
    IR_item.delay_time = delay;

    //IR_item.IR_type = ui->atCustomerCombox->currentIndex();
    //step1: find the button keyvalue from keymap
    for(int i =0;i<IR_maps.size();i++)
    {
        if(button_name == IR_maps.at(i).name)
        {
            qDebug() << "found button:"<< IR_maps.at(i).name;

            IR_item.IR_type = IR_maps.at(i).IR_type;
            QByteArray ba = IR_maps.at(i).name.toLatin1();
            char *tmpBuf = ba.data();

            if(String2IRLearningItem(IR_maps.at(i).keyValue,&IR_item) != true)
            {
                qDebug() << "add_to_list fail";
                output_log("add_to_list fail",0);
            }

            switch(IR_item.IR_type)
            {
                case IR_TYPE_SIRCS:
                    memcpy(IR_item.IR_CMD.IR_SIRCS.name,tmpBuf,MAX_NAME_LEN);
                    break;
                case IR_TYPE_NEC:
                    memcpy(IR_item.IR_CMD.IR_NEC.name,tmpBuf,MAX_NAME_LEN);
                    break;
                case IR_TYPE_RC6:
                    memcpy(IR_item.IR_CMD.IR_RC6.name,tmpBuf,MAX_NAME_LEN);
                    break;
                case IR_TYPE_RC5:
                    memcpy(IR_item.IR_CMD.IR_RC5.name,tmpBuf,MAX_NAME_LEN);
                    break;
                /*case IR_TYPE_JVC:
                    memcpy(IR_item.IR_CMD.IR_JVC.name,tmpBuf,MAX_NAME_LEN);
                    break;*/
                case IR_TYPE_LEARNING:
                    memcpy(IR_item.IR_CMD.IR_learning.name,tmpBuf,MAX_NAME_LEN);
                    break;
                default:
                    break;
            }
        }
    }

    printIrItemInfo(IR_item);
    //step2: add to IR_item list
    //IR_items.append(IR_item);

    //step3: update widget
    //atAddItem2ScriptListWidget(IR_item.IR_type,button_name,IR_item.delay_time);

    qDebug() << "ui->atScriptlistWidget->currentRow()" << ui->atScriptlistWidget->currentRow() ;
       qDebug() << "ui->atScriptlistWidget->count()" << ui->atScriptlistWidget->count() ;
       int curRow = ui->atScriptlistWidget->currentRow();
       if((ui->atScriptlistWidget->currentRow()==-1)
               /*|| (ui->atScriptlistWidget->currentRow()==0)*/
               || ((ui->atScriptlistWidget->currentRow() + 1) == ui->atScriptlistWidget->count()))
       {
           IR_items.append(IR_item);
           atAddItem2ScriptListWidget(IR_item.IR_type,button_name,IR_item.delay_time);
           ui->atScriptlistWidget->setCurrentRow(0);
       }
       else
       {
           char tmpNameBuf[MAX_NAME_LEN];
           atClearScriptWidgetOnly();
           IR_items.insert(curRow,IR_item);
           for(int i=0; i<IR_items.count(); i++)
           {
               switch(IR_items.at(i).IR_type)
               {
                   case IR_TYPE_SIRCS:
                       memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_SIRCS.name,MAX_NAME_LEN);
                       break;
                   case IR_TYPE_NEC:
                       memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_NEC.name,MAX_NAME_LEN);
                       break;
                   case IR_TYPE_RC6:
                       memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_RC6.name,MAX_NAME_LEN);
                       break;
                   case IR_TYPE_RC5:
                       memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_RC5.name,MAX_NAME_LEN);
                       break;
                   /*case IR_TYPE_JVC:
                       memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_JVC.name,MAX_NAME_LEN);
                       break;*/
                   case IR_TYPE_LEARNING:
                       memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_learning.name,MAX_NAME_LEN);
                       break;
                   default:
                       break;
               }
               atAddItem2ScriptListWidget(IR_items.at(i).IR_type,tmpNameBuf,IR_items.at(i).delay_time);
           }
       }
       ui->atScriptlistWidget->setCurrentRow(curRow+1);
}

void MainWindow::atAddButton_slot()
{

    if(ui->atCustomizeKeyListWidget->currentRow()== -1)
    {
        qDebug()<< "No key has been chosen!";
        //output_log(logstr,1);
        //return;
        ui->atCustomizeKeyListWidget->setCurrentRow(0);
    }
//step1:add to listWidget
    QString str = ui->atCustomizeKeyListWidget->currentItem()->text();

    QStringList list1 = str.split("     ");
    if(ui->atScriptlistWidget->count() >=20 )
    {
        logstr = "Can not add more then 20 cmds!";
        qDebug() << logstr;
        output_log(logstr,1);
        return;
    }
    add_to_list(list1.at(1),ui->atDelayText->text().toInt());
}

void MainWindow::atRemoveButton_slot()
{
    if (ui->atScriptlistWidget->count() >1)
    {
        int index = ui->atScriptlistWidget->currentRow();

        if (index >= 1)
        {
            ui->atScriptlistWidget->removeItemWidget(ui->atScriptlistWidget->takeItem(index));
            IR_items.removeAt(index-1);
        }
    }
    else
    {
        atClearScriptWidget();
    }
}

void MainWindow::atClearScriptWidget()
{
    //qDebug() << "before cleared:atScriptlistWidget->count() = "<<ui->atScriptlistWidget->count();
    int count = ui->atScriptlistWidget->count();
    if (count > 1)
    {
        //int index = ui->atScriptlistWidget->currentRow();

        for (int i = 1;i < count;i++)
        {
             //qDebug() << "remove index :" << i;
            ui->atScriptlistWidget->removeItemWidget(ui->atScriptlistWidget->takeItem(1));

        }
    }
    IR_items.clear();
}
void MainWindow::atClearScriptWidgetOnly()
{
    //qDebug() << "before cleared:atScriptlistWidget->count() = "<<ui->atScriptlistWidget->count();
    int count = ui->atScriptlistWidget->count();
    if (count > 1)
    {
        //int index = ui->atScriptlistWidget->currentRow();

        for (int i = 1;i < count;i++)
        {
             //qDebug() << "remove index :" << i;
            ui->atScriptlistWidget->removeItemWidget(ui->atScriptlistWidget->takeItem(1));

        }
    }
}
void MainWindow::atLoadscriptBut_slot()
{
    QString fileName = QFileDialog::getOpenFileName(this,"Open File",QDir::currentPath());

    if(fileName.isEmpty())
    {
        //QMessageBox::information(this,"Error","Please select a file");
        qDebug() << "Please select a file\n";

        return;
    }

    QFile *file = new QFile; //(QtCore)核心模块,需要手动释放
    file->setFileName(fileName);
    bool ok = file->open(QIODevice::ReadOnly);//以只读模式打开
    if(ok)
    {
      QTextStream in(file);  //文件与文本流相关联
      QString customer = in.readLine();//读取第一行;
      QString device = in.readLine();//读取第二行;
      qDebug() << customer <<"  " <<device;
      if(ui->atCustomerCombox->currentText()!= customer || ui->atDeviceCombox->currentText()!=device)
      {
          qDebug() << "please select the right Customer and Device";
          QMessageBox::StandardButton reply = QMessageBox::question(this, "ScriptFile not Match KeyMap", "Do you want to Switch to the matched KeyMap?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
          if(reply == QMessageBox::Yes)
          {

              ui->atCustomerCombox->setCurrentText(customer);
              ui->atCustomerCombox->setCurrentText(device);

          }
          else
          {
              qDebug() << "load script fail";
              logstr =  "load script fail :";
              logstr.append(fileName);
              output_log(logstr,1);
              return;
          }
      }
      //ui->atScriptlistWidget->clear();
      atClearScriptWidget();

      while (!in.atEnd()) {
          QString line = in.readLine();

          if(line.contains("---"))
          {
              QStringList list1 = line.split("---");
              add_to_list(list1.at(0),list1.at(1).toInt());
          }
      }
      file->close();
      delete file;
      logstr ="load script file:";
      logstr.append(fileName).append("success.");
      output_log(logstr,1);
    }
    else
    {
        QMessageBox::critical(this,"Error","File Open Error" + file->errorString()); //显示打开文件的错误原因
        return;
    }
}

void MainWindow::atSaveButton_slot()
{
    QString saveFileName = QFileDialog::getSaveFileName(this,"Save File",QDir::currentPath());
    if(saveFileName.isEmpty())
    {
        //QMessageBox::information(this,"Error","Please select a file");
        return;
    }
    QFile *file = new QFile; //(QtCore)核心模块,需要手动释放
    file->setFileName(saveFileName);
    bool ok = file->open(QIODevice::WriteOnly|QIODevice::Text);//以只读模式打开
    if(ok)
    {
      QTextStream out(file);  //文件与文本流相关联
      out << ui->atCustomerCombox->currentText() << "\n";
      out << ui->atDeviceCombox->currentText() << "\n";

      qDebug() << "IR_items.size(): " << IR_items.size();

      for(int i = 0;i < IR_items.size();i++)
      {
            char tmpBuf[MAX_NAME_LEN];
            QString name;
            switch(IR_items.at(i).IR_type)
            {
                case IR_TYPE_SIRCS:
                    memcpy(tmpBuf,IR_items.at(i).IR_CMD.IR_SIRCS.name,MAX_NAME_LEN);
                    break;
                case IR_TYPE_NEC:
                    memcpy(tmpBuf,IR_items.at(i).IR_CMD.IR_NEC.name,MAX_NAME_LEN);
                    break;
                case IR_TYPE_RC6:
                    memcpy(tmpBuf,IR_items.at(i).IR_CMD.IR_RC6.name,MAX_NAME_LEN);
                    break;
                case IR_TYPE_RC5:
                    memcpy(tmpBuf,IR_items.at(i).IR_CMD.IR_RC5.name,MAX_NAME_LEN);
                    break;
                /*case IR_TYPE_JVC:
                    memcpy(tmpBuf,IR_items.at(i).IR_CMD.IR_JVC.name,MAX_NAME_LEN);
                    break;*/
                case IR_TYPE_LEARNING:
                    memcpy(tmpBuf,IR_items.at(i).IR_CMD.IR_learning.name,MAX_NAME_LEN);
                    break;
                default:
                    break;
            }
            name = QString(QLatin1String(tmpBuf));
            uint32_t delaytime = IR_items.at(i).delay_time;
            //qDebug() << "  name " << name;
            //qDebug() << "  delay time " << delaytime;
            out << name << "---" << delaytime <<"\n";
      }
      file->close();
      delete file;
      QString tmplog ="save to file:";
      tmplog.append(saveFileName).append(" success.");
      output_log(tmplog,1);
    }
    else
    {
        QMessageBox::information(this,"Error","File Open Error" + file->errorString()); //显示打开文件的错误原因
        return;
    }
}
void MainWindow::atRealTimeSendButton_slot()
{

    /*if(!serial.isOpen())
    {
        QMessageBox::warning(this,"Send Error","Please Open Serial Port First!\n");
        return;
    }*/

    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);

    struct frame_t *frame = (struct frame_t *)buf;
    IR_item_t ir_item;

    frame->data_len = sizeof(struct frame_t);
    frame->header = FRAME_HEADER;
    frame->seq_num = seqnum++;
    frame->msg = REAL_TIME_SEND;


    ir_item.is_valid = 1;
    ir_item.delay_time = 100;

    QString button = ui->atButtonText->text();
    QByteArray ba = button.toLatin1();
    char *tmpBuf = ba.data();

    for(int i=0;i<IR_maps.size();i++)
    {
        if(button == IR_maps.at(i).name)
        {
            ir_item.IR_type = IR_maps.at(i).IR_type;

            if(String2IRLearningItem(IR_maps.at(i).keyValue,&ir_item) != true)
            {
                qDebug() << "transfer fail";
            }

            switch(ir_item.IR_type)
            {
                case IR_TYPE_SIRCS:
                    memcpy(ir_item.IR_CMD.IR_SIRCS.name,tmpBuf,MAX_NAME_LEN);
                    break;
                case IR_TYPE_NEC:
                    memcpy(ir_item.IR_CMD.IR_NEC.name,tmpBuf,MAX_NAME_LEN);
                    break;
                case IR_TYPE_RC6:
                    memcpy(ir_item.IR_CMD.IR_RC6.name,tmpBuf,MAX_NAME_LEN);
                    break;
                case IR_TYPE_RC5:
                    memcpy(ir_item.IR_CMD.IR_RC5.name,tmpBuf,MAX_NAME_LEN);
                    break;
                /*case IR_TYPE_JVC:
                    memcpy(ir_item.IR_CMD.IR_JVC.name,tmpBuf,MAX_NAME_LEN);
                    break;*/
                case IR_TYPE_LEARNING:
                    memcpy(ir_item.IR_CMD.IR_learning.name,tmpBuf,MAX_NAME_LEN);
                    break;
                default:
                    break;
            }
            break;
        }
    }

    qDebug() << "atRealTimeSendButton_slot:";
    printIrItemInfo(ir_item);
    memcpy(frame->msg_parameter,&ir_item,sizeof(IR_item_t));
    frame->data_len += sizeof(IR_item_t);

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    //qDebug() << "send  button: " << button;
    QString tmplog = "current send:";
    tmplog.append(button);
    output_log(tmplog,1);

    sendCmd2MCU(buf, frame->data_len + 1);

}

void MainWindow::set_cmd_list_handle()
{
    if (cmd_index >= IR_items.size())
    {
        qDebug() << "cmd list send finished";
        output_log("cmd list send finished",1);
        return;
    }
    qDebug() << "send SET_CMD_LIST to mcu,cmd_index = " <<cmd_index;

    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);
    uint8_t para_index = 0;

    struct frame_t *frame = (struct frame_t *)buf;
    frame->data_len = sizeof(frame_t);
    //datalen = frame->data_len;
    frame->header = FRAME_HEADER;
    frame->msg = SET_CMD_LIST;
    frame->seq_num = seqnum++;

    frame->msg_parameter[para_index++] = cmd_index;

    //qDebug() << "IR_items.size = " <<IR_items.size();

    IR_item_t ir_item = IR_items.at(cmd_index);

    memcpy(frame->msg_parameter + para_index, &ir_item, sizeof(IR_item_t));

    para_index += sizeof(IR_item_t);
    frame->data_len += para_index;

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    sendCmd2MCU(buf,frame->data_len +1);

    cmd_index++;

}

void MainWindow::clear_cmd_list_handle()
{
    qDebug() << "clear_cmd_list_handle:send CLEAR_CMD_LIST to mcu\n";
    output_log("send CLEAR_CMD_LIST to mcu",1);
    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);
    struct frame_t *frame = (struct frame_t *)buf;

    frame->data_len = sizeof(struct frame_t);
    frame->header = FRAME_HEADER;
    frame->seq_num = seqnum++;
    frame->msg = CLEAR_CMD_LIST;

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    sendCmd2MCU(buf, frame->data_len + 1);
}

void MainWindow::atDownloadButton_slot()
{
    qDebug() << "Download cmd list to mcu\n";
    output_log("start Download cmd list to mcu...",1);

    /*if (!serial.isOpen())
    {
        QMessageBox::critical(this, tr("send data error"), "please open serial port first");
        return;
    }*/

    clear_cmd_list_handle();
/*
    if(ui->atSetLoopCntRadioButton->isChecked()){
        currentLoopIndex = 0;
        totalLoopCnt = ui->atTotalLoopCntText->text().toInt();
    }
*/
    cmd_index = 0;

}

void MainWindow::atStartButton_slot()
{
    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);
    struct frame_t *frame = (struct frame_t *)buf;
    frame->data_len = sizeof(struct frame_t);
    frame->header = FRAME_HEADER;
    frame->seq_num = seqnum++;

    if(ui->atStartButton->text() == "Start")
    {
        qDebug() << "send START_SEND";
        output_log("start the loop test.",1);
        frame->msg = START_SEND;//CLEAR_CMD_LIST;
        ui->atStartButton->setText("Pause");
    }
    else if(ui->atStartButton->text() == "Stop")
    {

        qDebug() << "send PAUSE_SEND";
        output_log("pause the loop test.",1);
        frame->msg = PAUSE_SEND;//CLEAR_CMD_LIST;

    }
    buf[frame->data_len] = CRC8Software(buf, frame->data_len);
    sendCmd2MCU(buf, frame->data_len + 1);

}

void MainWindow::output_log(const QString log,int flag)
{
    if(flag)
    {
        ui->atLogText->append(log);
    }

    QDateTime time = QDateTime::currentDateTime();//获取系统现在的时间
    QString str = time.toString("yyyy-MM-dd_hh:mm:ss"); //设置显示格式
    str.append("  ");
    str.append(log);
    ui->logTextEdit->append(str);
}

/*---------------------lianlian add for LearningKey----------------*/

void MainWindow::leReturnButton_slot()
{
    //ui->leStackedWidget->setCurrentIndex(0);

}
void MainWindow::leCustomizeButton_slot()
{
    //ui->leStackedWidget->setCurrentIndex(0);
    /*---just for test----

    uint8_t tmp[10]={0x00,0x11,0x22,0x3C,0x4d,0x5e,0x6f,0x7a,0xb8,0x99};

    QString butname = ui->leButtonText->text();

    QString tmpkey = byteArray2String(tmp);
    ui->leKeyTextEdit->setText(tmpkey);

    if(lw != NULL){
        lw->setWaveData(butname,tmp);
        emit send2learningwave();
    }
    else
        qDebug() <<"learning wave ui is not open";
    ---just for test----*/
}

void MainWindow::leIrPanel_slot()
{
    QPushButton* btn = dynamic_cast<QPushButton*>(sender());
    ui->leButtonText->setText(btn->toolTip());
    qDebug() << btn->toolTip() << "is clicked in learning key ir panel\n";
    //QMessageBox::information(this,"info","Learning Power button is clicked\n");
}

void MainWindow::leRealTimeTestButton_slot()
{

    /*if(!serial.isOpen())
    {
        QMessageBox::warning(this,"Send Error","Please Open Serial Port First!\n");
        return;
    }*/

    IR_item_t ir_item;
    //qDebug() << " sizeof ir_item=  "<<sizeof(IR_item_t);
    memset(&ir_item,0x0,sizeof(IR_item_t));
    QString btnname;
    QString btnkey;

    //real time send current item on listwidget

    if(ui->leKeymaplistWidget->currentRow() == -1)
    {
        btnname = ui->leButtonText->text();
        btnkey = ui->leKeyTextEdit->text();
    }
    else
    {   
        QString str = ui->leKeymaplistWidget->currentItem()->text();
        QStringList list1 = str.split(':');
        bool ok;
        ir_item.IR_type = list1.at(0).toInt(&ok,16);
        btnname = list1.at(1);
        btnkey = list1.at(2);
    }

    QByteArray ba = btnname.toLatin1();
    char *tmpBuf = ba.data();
    //memcpy(ir_item.IR_CMD.IR_learn.name,tmpBuf,MAX_NAME_LEN);

    if(String2IRLearningItem(btnkey,&ir_item)==false)
    {
        qDebug() << " tansfer fail! ";
        output_log("get ir_item fail",1);
        return;
    }

    ir_item.is_valid = 1;
    ir_item.delay_time = 100;

    switch(ir_item.IR_type)
    {
        case IR_TYPE_SIRCS:
            memcpy(ir_item.IR_CMD.IR_SIRCS.name,tmpBuf,MAX_NAME_LEN);
            break;
        case IR_TYPE_NEC:
            memcpy(ir_item.IR_CMD.IR_NEC.name,tmpBuf,MAX_NAME_LEN);
            break;
        case IR_TYPE_RC6:
            memcpy(ir_item.IR_CMD.IR_RC6.name,tmpBuf,MAX_NAME_LEN);
            break;
        case IR_TYPE_RC5:
            memcpy(ir_item.IR_CMD.IR_RC5.name,tmpBuf,MAX_NAME_LEN);
            break;
        /*case IR_TYPE_JVC:
            memcpy(ir_item.IR_CMD.IR_JVC.name,tmpBuf,MAX_NAME_LEN);
            break;*/
        case IR_TYPE_LEARNING:
            memcpy(ir_item.IR_CMD.IR_learning.name,tmpBuf,MAX_NAME_LEN);
            break;
        default:
            break;
    }
    logstr = "leRealTimeTestButton_slot: send button:";
    logstr.append(tmpBuf);
    qDebug() << logstr;
    output_log(logstr,1);
    printIrItemInfo(ir_item);
    //serial.setBaudRate(QSerialPort::Baud115200);
    //serial.setParity(QSerialPort::NoParity);
    //serial.setDataBits(QSerialPort::Data8);
    //serial.setStopBits(QSerialPort::OneStop);
    //serial.setFlowControl(QSerialPort::NoFlowControl);
    //serial.clearError();
    //serial.clear();

    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);

    struct frame_t *frame = (struct frame_t *)buf;

    frame->data_len = sizeof(struct frame_t);
    frame->header = FRAME_HEADER;
    frame->seq_num = seqnum++;
    frame->msg = REAL_TIME_SEND;

    memcpy(frame->msg_parameter,&ir_item,sizeof(IR_item_t));

    frame->data_len += sizeof(IR_item_t);

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    sendCmd2MCU(buf, frame->data_len + 1);

}
void MainWindow::leStartRecordButton_slot()
{
    //ui->leKeyTextEdit->setText("1234ffaadd980037ef56");  //just for test


    if(ui->leButtonText->text().isEmpty())
    {
        QMessageBox::warning(this,"Warning","Please choose a Button or input a Button name\n");
        ui->leButtonText->setFocus();
        return;
    }

    //return ; //just for test

    /*if(!serial.isOpen())
    {
        QMessageBox::warning(this,"Port warning","Please Open Serial Port First!\n");
        return;
    }
    else*/
    {
        //serial.setBaudRate(QSerialPort::Baud115200);
        //serial.setParity(QSerialPort::NoParity);
        //serial.setDataBits(QSerialPort::Data8);
        //serial.setStopBits(QSerialPort::OneStop);
        //serial.setFlowControl(QSerialPort::NoFlowControl);

        //serial.clearError();
        //serial.clear();

        //ui->leStartRecordBut->setDisabled(true);  //marked just for test

        //send cmd
        uint8_t buf[BUF_LEN];

        struct frame_t *frame = (struct frame_t *)buf;

        frame->header = FRAME_HEADER;
        frame->data_len = sizeof(struct frame_t);
        frame->seq_num = seqnum++;
        frame->msg = START_LEARNING;


        buf[frame->data_len] = CRC8Software(buf, frame->data_len);

        sendCmd2MCU(buf,frame->data_len + 1);
/*
       //QMessageBox::information(this,"Guide","Please press the button on your RemoteController");
       QMessageBox *messageBox=new QMessageBox(QMessageBox::Information,"Guide","Please press the button on your RemoteController",QMessageBox::Close,this);
       messageBox->show();
*/
        //open learning wave for debug
        if(ui->leDebugModeCheckBox->isChecked() && this->lw == NULL)
        {
            this->lw = new LearningWave(this);
            lw->setWindowTitle("Learning Wave");
            lw->setWaveData(ui->leButtonText->text(),NULL,0);
            ui->leDebugModeCheckBox->setDisabled(true);
            this->connect(lw,SIGNAL(rejected()),this,SLOT(returnfromLearningWave()));
            lw->show();//非模态
            //lw->exec();//模态
        }
    }
}

void MainWindow::returnfromLearningWave()
{
    qDebug()<< "learning wave menu close!";
    delete lw;
    ui->leDebugModeCheckBox->setDisabled(false);
    ui->leStartRecordBut->setDisabled(false);
    this->lw = NULL;
}

void MainWindow::analysisFinshed(QString key)
{
    //qDebug()<< "MainWindow::analysisFinshed from wave ui";
    ui->leKeyTextEdit->setText(key);
    ui->leStartRecordBut->setEnabled(true);
}

void MainWindow::on_actionIRWave_triggered()
{
    if(ui->leDebugModeCheckBox->isChecked() && this->lw == NULL)
    {
        this->lw = new LearningWave(this);
        lw->setWindowTitle("Learning Wave");
        this->connect(lw,SIGNAL(close()),this,SLOT(returnfromLearningWave()));
        ui->leDebugModeCheckBox->setDisabled(true);
        lw->show();//非模态
        //lw->exec();//模态
    }
    else
        qDebug() <<"only enabled in debug mode";
}
void MainWindow::leAddToListButton_slot()
{
    bool ok = 0;
    IR_map_t learningkey;
    learningkey.name = ui->leButtonText->text();
    learningkey.keyValue = ui->leKeyTextEdit->text();

/* step1 : add to listWidget */
    QString str;
    QStringList list = ui->leKeyTextEdit->text().split("-");
    learningkey.IR_type = list.at(0).toInt(&ok,16);
    str.append(list.at(0)).append(":");
    str.append(learningkey.name).append(":");
    str.append(learningkey.keyValue);

    QListWidgetItem *item = new QListWidgetItem(str,ui->leKeymaplistWidget);
    ui->leKeymaplistWidget->addItem(item);
    ui->leKeymaplistWidget->setCurrentItem(item); //make KeyMapList always focus on the last item when a new item is added

/* step2 : add to learningkey_maps QList */
    learningkey_maps.append(learningkey);
    //qDebug() << "add:leKeymaplistWidget total count is " << ui->leKeymaplistWidget->count();
}

void MainWindow::leClearButton_slot()
{
    ui->leKeymaplistWidget->clear();
    learningkey_maps.clear();
}

void MainWindow::leRemoveFromListButton_slot()
{
    QList <QListWidgetItem*> items ;//注意 items是个Qlist 其中的元素是QListWidgetItem
    items = ui->leKeymaplistWidget->selectedItems();
    if(items.size()==0)
        return;
    else
    {
        for(int i =0; i<items.size(); i++)//遍历所有选中的ITEM
        {
            QListWidgetItem *sel = items[i];
            int r = ui->leKeymaplistWidget->row(sel);
            delete  ui->leKeymaplistWidget->takeItem(r);

        }
        //下面代码可实现删除单选的item
        //    QListWidgetItem *item = ui->SPList->takeItem(ui->SPList->currentRow());
        //    delete item;
    }

}
void MainWindow::leSaveKeymapButton_slot()
{
    if(ui->leCustomerText->currentText().isEmpty())
    {
        QMessageBox::information(this,"warning","Cusomer cannot be empty,Please input a Customer name");
        ui->leCustomerText->setFocus();
        return;
    }
    else if(ui->leDeviceText->currentText().isEmpty())
    {
        QMessageBox::information(this,"warning","Device cannot be empty,Please input a Device name");
        ui->leDeviceText->setFocus();
        return;
    }

    if(ui->leKeymaplistWidget->count() == 0)
    {
       QMessageBox::information(this,"warning","Nothing to save,KeymapList is empty");
       ui->leKeymaplistWidget->setFocus();
       return;
    }

    QDir *dir = new QDir(keyMapDirPath);
    dir->setCurrent(keyMapDirPath);
    QString filename = ui->leCustomerText->currentText().append("%").append(ui->leDeviceText->currentText())./*append("_byLearning").*/append(".ini");
    QFile *keymapfile = new QFile;
    //QFile *tempFile = new QFile;
    keymapfile->setFileName(filename);
    QTextStream out(keymapfile);
    if(keymapfile->exists())
    {
        //文件已存在,追加到文件末尾
        qDebug() << "文件已存在\n";
        if(!keymapfile->open(QIODevice::WriteOnly | QIODevice::Text|QIODevice::Append))
        {
            qDebug() << "打开文件失败\n";
            QMessageBox::critical(this,"Save Fail","Save Key map to file ERROR,Can't open file");
            return ;
        }

    }
    else
    {
        //文件不存在,创建新文件
        qDebug() << "文件不存在,新建文件 " << filename << "\n";

        if(!keymapfile->open(QIODevice::WriteOnly | QIODevice::Text))
        {
            qDebug() << "打开文件失败\n";
            QMessageBox::critical(this,"Save Fail","Save Key map to file ERROR,Can't open file");
            return ;
        }
        //QTextStream out(keymapfile);
        out << ui->leCustomerText->currentText() << "\n";
        out << ui->leDeviceText->currentText() << "\n";
    }
    qDebug() << "save:leKeymaplistWidget total count is " << ui->leKeymaplistWidget->count();
    for(int i = 0;i < ui->leKeymaplistWidget->count();i++)
    {
        QListWidgetItem *item = ui->leKeymaplistWidget->item(i);
        qDebug() << " item.text in row " << i << "is" << item->text();
        QString src = item->text();
        QStringList list1 = src.split(":");
        QString type =list1.at(0); // ir_type
        QString name =list1.at(1); // ir_name
        QString key = list1.at(2); // ir_key
        QString dst = type;
        dst.append(",").append(name);
        QStringList list2 = key.split("-");
        for(int i=1;i < list2.size();i++)
        {
            QString tmp = list2.at(i);
            tmp.insert(0,"0x");
            dst.append(",").append(tmp);
        }
        qDebug() << " transfer to " << dst;
       out << dst << "\n";
    }
    keymapfile->close();
    delete keymapfile;  //需手动删除,以免内存泄漏

    loadInsetIrMapTable();

    QMessageBox::information(this,"Save Success","Save Key map to file:" + filename);
    leClearButton_slot();

}
/*---------------------------------------------------*/


void MainWindow::on_actionUser_Manual_triggered()
{
    QDesktopServices::openUrl(QUrl("http://wiki.mediatek.inc/pages/viewpage.action?pageId=335054467"));
    //openUrl_slot("http://wiki.mediatek.inc/display/HBGSMTeam/BDP+Software+Process+Flow");
}

void MainWindow::on_actionLog_triggered()
{
    ui->logSubWindow->showMaximized();
}
void MainWindow::logClearButton_slot()
{
    ui->logTextEdit->clear();
}

void MainWindow::logSaveAsButton_slot()
{
    QString saveFileName = QFileDialog::getSaveFileName(this,"Save File",QDir::currentPath());
    if(saveFileName.isEmpty())
    {
        //QMessageBox::information(this,"Error","Please select a file");
        return;
    }
    QFile *file = new QFile; //(QtCore)核心模块,需要手动释放
    file->setFileName(saveFileName);
    bool ok = file->open(QIODevice::WriteOnly|QIODevice::Text);//以只读模式打开
    if(ok)
    {
      QTextStream out(file);  //文件与文本流相关联
      out << ui->logTextEdit->toPlainText() << "\n";
    }

    file->close();
    delete file;  //需手动删除,以免内存泄漏
}

void MainWindow::logSaveButton_slot()
{

    QString appPath = qApp->applicationDirPath();
    QString logDirPath = appPath.append("\\log");

    logstr = "logDirPath is ";
    logstr.append(logDirPath);
    qDebug() << logstr;
    output_log(logstr,1);

    QDir *dir = new QDir(logDirPath);
    if(!dir->exists())
    {
        logstr = "logDirPath doesn't exist";
        qDebug() << logstr;
        output_log(logstr,1);

        bool ok = dir->mkdir(logDirPath);
        if( ok )
        {
            logstr = "logDirPath created success.";
            qDebug() << logstr;
            output_log(logstr,0);
        }
        else
        {
            logstr = "save fail.";
            qDebug() << logstr;
            output_log(logstr,1);
            return;
        }

    }
    dir->setCurrent(logDirPath);

    QDateTime time = QDateTime::currentDateTime();//获取系统现在的时间
    QString str = time.toString("yyyyMMdd_hh_mm_ss"); //设置显示格式
    QString filename = "AutoSave_";
            filename.append(str).append(".txt");

    logstr = "save to file: ";
    logstr.append(filename);
    qDebug() << logstr;
    //output_log(logstr,1);

    QFile logfile( filename );
    logfile.open(QIODevice::WriteOnly);
    logfile.close();  //创建文件
    if(logfile.exists())
    {
        qDebug() << "log 文件创建成功";
    }
    else
    {
        qDebug() << "log 文件创建失败";
    }



    bool ok = logfile.open(QIODevice::ReadWrite|QIODevice::Text);//以只读模式打开

    if(ok)
    {
      QTextStream out(&logfile);  //文件与文本流相关联
      out << ui->logTextEdit->toPlainText() << "\n";
    }
    else
    {
        qDebug("save fail!");
        //output_log("save fail!",1);
    }

    logfile.close();
}

void MainWindow::on_atUpMove_clicked()
{
    qDebug() << "ui->atScriptlistWidget->currentRow()" << ui->atScriptlistWidget->currentRow() ;
    qDebug() << "ui->atScriptlistWidget->count()" << ui->atScriptlistWidget->count() ;
    int curRow = ui->atScriptlistWidget->currentRow();
    if (curRow == -1 || curRow == 0 || curRow == 1 || IR_items.count() <= 1)
        return;
    IR_items.swap(curRow-1, curRow-2);

    char tmpNameBuf[MAX_NAME_LEN];
    atClearScriptWidgetOnly();
    for(int i=0; i<IR_items.count(); i++)
    {
        switch(IR_items.at(i).IR_type)
        {
            case IR_TYPE_SIRCS:
                memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_SIRCS.name,MAX_NAME_LEN);
                break;
            case IR_TYPE_NEC:
                memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_NEC.name,MAX_NAME_LEN);
                break;
            case IR_TYPE_RC6:
                memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_RC6.name,MAX_NAME_LEN);
                break;
            case IR_TYPE_RC5:
                memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_RC5.name,MAX_NAME_LEN);
                break;
            /*case IR_TYPE_JVC:
                memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_JVC.name,MAX_NAME_LEN);
                break;*/
            case IR_TYPE_LEARNING:
                memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_learning.name,MAX_NAME_LEN);
                break;
            default:
                break;
        }
        atAddItem2ScriptListWidget(IR_items.at(i).IR_type,tmpNameBuf,IR_items.at(i).delay_time);
    }
    ui->atScriptlistWidget->setCurrentRow(curRow-1);
}

void MainWindow::on_atDownMove_clicked()
{
    qDebug() << "ui->atScriptlistWidget->currentRow()" << ui->atScriptlistWidget->currentRow() ;
        qDebug() << "ui->atScriptlistWidget->count()" << ui->atScriptlistWidget->count() ;
        int curRow = ui->atScriptlistWidget->currentRow();
        if (curRow == -1 || curRow == 0 || IR_items.count() <= 1 || curRow == IR_items.count())
            return;
        IR_items.swap(curRow-1, curRow);

        char tmpNameBuf[MAX_NAME_LEN];
        atClearScriptWidgetOnly();
        for(int i=0; i<IR_items.count(); i++)
        {
            switch(IR_items.at(i).IR_type)
            {
                case IR_TYPE_SIRCS:
                    memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_SIRCS.name,MAX_NAME_LEN);
                    break;
                case IR_TYPE_NEC:
                    memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_NEC.name,MAX_NAME_LEN);
                    break;
                case IR_TYPE_RC6:
                    memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_RC6.name,MAX_NAME_LEN);
                    break;
                case IR_TYPE_RC5:
                    memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_RC5.name,MAX_NAME_LEN);
                    break;
                /*case IR_TYPE_JVC:
                    memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_JVC.name,MAX_NAME_LEN);
                    break;*/
                case IR_TYPE_LEARNING:
                    memcpy(tmpNameBuf,IR_items.at(i).IR_CMD.IR_learning.name,MAX_NAME_LEN);
                    break;
                default:
                    break;
            }
            atAddItem2ScriptListWidget(IR_items.at(i).IR_type,tmpNameBuf,IR_items.at(i).delay_time);
        }
        ui->atScriptlistWidget->setCurrentRow(curRow+1);
}

void MainWindow::on_actionDown_Binary_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/barrycool/bin/tree/master/IR_MCU_upgrade_bin"));
    return;//for now

    QString appPath = qApp->applicationDirPath();
    QString fileDir = appPath.append("/Download_files");
    QDir *dir = new QDir(fileDir);
    if(!dir->exists())
    {
        logstr = fileDir + " doesn't exist";
        qDebug() << logstr;
        output_log(logstr,1);

        bool ok = dir->mkdir(fileDir);
        if( ok )
        {
            logstr = "fileDir created success.";
            qDebug() << logstr;
            output_log(logstr,0);
        }
        else
        {
            logstr = "download fail.";
            qDebug() << logstr;
            output_log(logstr,1);
            return;
        }

    }
    QString filePath = appPath.append("/IR_stm32f103C8.bin");
    QFile binFile(filePath);
    if(binFile.exists())
    {
        qDebug() << "IR_stm32f103C8.bin exsit,delete it first";
        QFile::remove(filePath);
    }

    QString srcBinFilePath = "https://github.com/barrycool/bin/raw/master/IR_MCU_upgrade_bin/IR_stm32f103C8.bin";
    QString cmd = "wget -N -P " + fileDir + " " + srcBinFilePath;
    if(system(cmd.toLatin1().data()))
    {
        output_log("download fail!",1);
    }
    else
    {
        logstr = "download success to " + filePath;
        output_log(logstr,1);
    }
}

void MainWindow::on_actionDownload_MainTool_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/barrycool/bin/tree/master/MainTool"));

    return;//for now

    QString appPath = qApp->applicationDirPath();
    QString fileDir = appPath.append("/Download_files");
    QDir *dir = new QDir(fileDir);
    if(!dir->exists())
    {
        logstr = fileDir + " doesn't exist";
        qDebug() << logstr;
        output_log(logstr,1);

        bool ok = dir->mkdir(fileDir);
        if( ok )
        {
            logstr = "fileDir created success.";
            qDebug() << logstr;
            output_log(logstr,0);
        }
        else
        {
            logstr = "download fail.";
            qDebug() << logstr;
            output_log(logstr,1);
            return;
        }

    }
    QString srcBinFilePath = "https://github.com/barrycool/bin/raw/master/Smart_IR.exe";
    QString cmd = "wget -N -P " + fileDir + " "+ srcBinFilePath;
    if(system(cmd.toLatin1().data()))
    {
        output_log("download fail!",1);
    }
    else
    {
        logstr = "download success to " + fileDir.append("Smart_IR.exe");
        output_log(logstr,1);
    }
}

void MainWindow::on_actionDownload_UserManual_triggered()
{

    QDesktopServices::openUrl(QUrl("https://github.com/barrycool/bin/tree/master/UserManual"));
    return;//for now

    QString appPath = qApp->applicationDirPath();
    QString fileDir = appPath.append("/Download_files");
    QDir *dir = new QDir(fileDir);
    if(!dir->exists())
    {
        logstr = fileDir + " doesn't exist";
        qDebug() << logstr;
        output_log(logstr,1);

        bool ok = dir->mkdir(fileDir);
        if( ok )
        {
            logstr = "fileDir created success.";
            qDebug() << logstr;
            output_log(logstr,0);
        }
        else
        {
            logstr = "download fail.";
            qDebug() << logstr;
            output_log(logstr,1);
            return;
        }

    }
    QString srcBinFilePath = "https://github.com/barrycool/bin/raw/master/Smart%20IR%20user%20manual%20v2.docx";
    QString cmd = "wget -N -P " + fileDir + " "+ srcBinFilePath;
    if(system(cmd.toLatin1().data()))
    {
        output_log("download fail!",1);
    }
    else
    {
        logstr = "download success to " + fileDir.append("Smart%20IR%20user%20manual%20v2.docx");
        output_log(logstr,1);
    }
}

void MainWindow::on_actionDownload_SerialDriver_triggered()
{
    //QDesktopServices::openUrl(QUrl("https://github.com/barrycool/bin/raw/master/SerialDriver/en.stsw-link009.zip"));
    //return;//for now

    QString appPath = qApp->applicationDirPath();
    QString fileDir = appPath.append("/Download_files");
    QDir *dir = new QDir(fileDir);
    if(!dir->exists())
    {
        bool ok = dir->mkdir(fileDir);
        if( ok )
        {
            ;
        }
        else
        {
            logstr = "download fail.";
            qDebug() << logstr;
            output_log(logstr,1);
            return;
        }

    }
    QString srcBinFilePath = "https://github.com/barrycool/bin/raw/master/SerialDriver/en.stsw-link009.zip";
    QString cmd = "wget -N -P " + fileDir + " "+ srcBinFilePath;
    if(system(cmd.toLatin1().data()))
    {
        output_log("download fail!",1);
    }
    else
    {
        logstr = "download success to " + fileDir.append("en.stsw-link009.zip");
        output_log(logstr,1);
    }
}
void MainWindow::on_actionWifiSetting_triggered()
{
    ui->WifiSetting->showMaximized();
}
