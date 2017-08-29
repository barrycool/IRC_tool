#include "mainwindow.h"
#include "ui_mainwindow.h"

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

    portBox = new QComboBox;
    ui->mainToolBar->insertWidget(ui->actionOpenUart,portBox);
    settings = new QSettings("Mediatek","IRC_Tool");

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
    output_log(logstr,1);

    //for Aging Test SubWindow
    loadInsetIrMapTable();
    connect(ui->atCustomerCombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &set_IR_device);
    connect(ui->atDeviceCombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &set_IR_command_list);

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
    isLearningKey->setFixedWidth(35);
    keyName->setFixedWidth(105);
    time->setFixedWidth(50);

    QListWidgetItem * scriptItem = new QListWidgetItem(ui->atScriptlistWidget);
    scriptItem->setBackgroundColor(Qt::lightGray);
    ui->atScriptlistWidget->setItemWidget(scriptItem,wContainer);

    ir_button_Slot_connect();
    //connect(&set_cmd_list_timer, &QTimer::timeout, this, &set_cmd_list_handle);

    connect(&sendcmd_timer, &QTimer::timeout, this, &sendcmdTimeout);
    connect(&serial, SIGNAL(readyRead()), this, SLOT(serial_receive_data()));
    sendcmd_timer.setSingleShot(true);
    //cmdSemaphore = new QSemaphore(1);
    isInit = 1;
}

MainWindow::~MainWindow()
{
    delete ui;
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
    struct frame_t *frame = (struct frame_t *)buf;

    if(!serial.isOpen())
    {
        qDebug() << "serial port is not open";
        output_log("serial port is not open",1);
        return;
    }

/*------add for debug----------*/
        qDebug() << "send packet:";
        QString log;
        for(uint8_t j = 0; j< len; j++)
        {
            log += QString("%1 ").arg(buf[j]);
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

    if(frame->msg != CMD_ACK)
    {
        sendcmd_timer.start(2000);
    }

}
void MainWindow::sendcmdTimeout()
{
    //cmdSemaphore->release();

    if(resendCount > FAIL_RETRY_TIMES)
    {
        qDebug("retry for %d times still fail,quit!",FAIL_RETRY_TIMES);

        logstr = "cmd send fail";
        output_log(logstr,1);

        resendCount = 0;
        emit cmdFailSignal();
        return;
    }

    //qDebug() <<"cmd was handle tiemout,resend :" << resendCount;
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

    buf_len += serial.read((char*)buf + buf_len, BUF_LEN - buf_len);

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

    //qDebug() << "receive packet:";
    log = "receive packet:";
    output_log(log,0);

    for(uint8_t j = 0; j< buf_len + 1; j++)
    {
        log += QString("%1 ").arg(buf[j]);
    }
    qDebug() << log;
    output_log(log,0);

    if(frame->msg == CMD_NACK)
    {
        sendcmd_timer.stop();

        if(resendCount >= FAIL_RETRY_TIMES)
        {
            qDebug() << "retry for 5 times still fail,quit!";
            output_log("NAK :cmd not handled correctly",1);
            resendCount = 0;
            emit cmdFailSignal();
            return;
        }
        qDebug() <<"CMD_NACK,resend :" << resendCount;
        resendBackupCmd();

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
        else if (frame->msg_parameter[0] == UPGRADE_START ||frame->msg_parameter[0] == SEND_UPGRADE_PACKET )
        {
            emit receiveAckSignal();
        }
        else if (frame->msg_parameter[0] == UPGRADE_FINISH)
        {
            emit receiveFinshSignal();
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
            case IR_TYPE_JVC:
                memcpy(tmpBuf,ir_item.IR_CMD.IR_JVC.name,MAX_NAME_LEN);
                break;
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

        emit updateVersionSignal(&mcuVersion);
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
        case IR_TYPE_JVC:
            str.append(ir_item.IR_CMD.IR_JVC.name);
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_JVC.IR_address,16));
            str.append(":0x").append(QString::number(ir_item.IR_CMD.IR_JVC.IR_command,16));
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
    qDebug() << str;
    output_log(str,0);
}
/*--------Lianlian add for cmd send nack and resend --- end--------*/
void MainWindow::portChanged(int index)
{
    qDebug()<<"portChanged to: "<< index;

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
    diaglog->setWindowTitle("IRC Tool Info");
    //diaglog->show();//非模态
    diaglog->exec();//模态
}

void MainWindow::on_actionOpenUart_triggered()
{
    disconnect(portBox,SIGNAL(currentIndexChanged(int)), this, SLOT(portChanged(int)));
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
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

    fupdiaglog = new UpgradeDialog(this);

    fupdiaglog->setWindowTitle("Upgrade");
    fupdiaglog->show();//非模态
    //diaglog->exec();//模态
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
        QObject::connect(ui->atScriptlistWidget,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(atScriptlistWidgetClicked_slot(QListWidgetItem*)));

      /*---------------------lianlian add for log----------------*/
        QObject::connect(ui->logSaveButton,SIGNAL(clicked()),this,SLOT(logSaveButton_slot()));
        QObject::connect(ui->logClearButton,SIGNAL(clicked()),this,SLOT(logClearButton_slot()));
        QObject::connect(ui->logSaveasButton,SIGNAL(clicked()),this,SLOT(logSaveAsButton_slot()));
        QObject::connect(ui->actionSave_log,SIGNAL(triggered(bool)),this,SLOT(logSaveButton_slot()));
        QObject::connect(ui->actionSaveasLog,SIGNAL(triggered(bool)),this,SLOT(logSaveAsButton_slot()));
}

/*---------------------lianlian add for Upgrade----------------*/
void MainWindow::sendCmdforUpgradeSlot(char *buf,int len)
{
    sendCmd2MCU((uint8_t *)buf, len);
}

void MainWindow::getCurrentMcuVersion()
{
   // currentMcuVersionYear = 2017;
   // currentMcuVersionMonth = 07;
    //currentMcuVersionDay = 07;
/*
    IR_MCU_Version_t version;
    version.year_high = 20;
    version.year_low = 17;
    version.month = 07;
    version.day = 07;

    emit updateVersionSignal(&version);
    return; //just for test
*/
    if(!serial.isOpen())
    {
        //QMessageBox::warning(this,"Send Error","Please Open Serial Port First!\n");
        qDebug() << "serial is not open,cannot check mcu's version";
        logstr = "serial is not open!";
        output_log(logstr,0);
        return;
    }

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
    QString IR_type = NULL;
    QString buttonName = NULL;
    QString str = NULL;

    QStringList list1 = line.split(',');
    IR_type = list1.at(0);
    //QByteArray ba = list1.at(1).toLatin1();
    buttonName = list1.at(1);//ba.data();
    qDebug() << "loadkeymap:buttonName " << buttonName;

    str = IR_type.append("    ").append(buttonName/*QString(QLatin1String(buttonName))*/);
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
void MainWindow::atLoadKeyMapButton_slot()
{
    QString fileDir = keyMapDirPath;
    QDir *dir = new QDir(fileDir);
    dir->setCurrent(keyMapDirPath);
    //QString fileName = fileDir.append("/").append(ui->atCustomerCombox->currentText()).append("_").append(ui->atDeviceCombox->currentText()).append(".ini");
    QString fileName = "sony_BDP.ini"; //just for test
    //QFile *keyFile = new QFile(fileName);
    QFile keyFile(fileName);  //需手动删除,以免内存泄漏
    //keyFile->setFileName(fileName);
    if(!keyFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "read file " << fileName << "fail!\n";
        QMessageBox::critical(this,"Load Error","Open file " + fileName +"Error");
        return;
    }
    QTextStream in(&keyFile);

    while (!in.atEnd()) {
        QString line = in.readLine();
        if(line.contains(','))
        {
            setToKeyListWidget(line);
            saveToIrMaps(line);
        }

    }
    keyFile.close();
    //delete &keyFile;
    QString tmplog ="load keymap file:";
    tmplog.append(fileName).append("success.");
    output_log(tmplog);

}
*/
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
    IR_items.clear();


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
    QStringList list1 = str.split(QRegularExpression("\\W+"), QString::SkipEmptyParts);
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
        case IR_TYPE_JVC:
            memcpy(tmpBuf,IR_items.at(currentRow-1).IR_CMD.IR_JVC.name,MAX_NAME_LEN);
            break;
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
    else if(ir_type == IR_TYPE_JVC)
    {
        islearning = "JVC";
    }
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

    QWidget *wContainer = new QWidget(ui->atScriptlistWidget);
    QHBoxLayout *hLayout = new QHBoxLayout(wContainer);

    QLabel *isLearningKey = new QLabel(islearning);
    QLabel *keyName = new QLabel(cmd_item);
    QLabel *time = new QLabel(sdelaytime);

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
    ui->atScriptlistWidget->setItemWidget(scriptItem,wContainer);
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
                case IR_TYPE_JVC:
                    memcpy(IR_item.IR_CMD.IR_JVC.name,tmpBuf,MAX_NAME_LEN);
                    break;
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
    IR_items.append(IR_item);

    //step3: update widget
    atAddItem2ScriptListWidget(IR_item.IR_type,button_name,IR_item.delay_time);
}

void MainWindow::atAddButton_slot()
{

//step1:add to listWidget
    QString str = ui->atCustomizeKeyListWidget->currentItem()->text();

    QStringList list1 = str.split(QRegularExpression("\\W+"), QString::SkipEmptyParts);

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
    qDebug() << "before cleared:atScriptlistWidget->count() = "<<ui->atScriptlistWidget->count();
    int count = ui->atScriptlistWidget->count();
    if (count > 1)
    {
        int index = ui->atScriptlistWidget->currentRow();

        for (int i = 1;i < count;i++)
        {
             qDebug() << "remove index :" << i;
            ui->atScriptlistWidget->removeItemWidget(ui->atScriptlistWidget->takeItem(1));

        }
    }
    //IR_items.clear();
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
                case IR_TYPE_JVC:
                    memcpy(tmpBuf,IR_items.at(i).IR_CMD.IR_JVC.name,MAX_NAME_LEN);
                    break;
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

    if(!serial.isOpen())
    {
        QMessageBox::warning(this,"Send Error","Please Open Serial Port First!\n");
        return;
    }

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
                case IR_TYPE_JVC:
                    memcpy(ir_item.IR_CMD.IR_JVC.name,tmpBuf,MAX_NAME_LEN);
                    break;
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
        //set_cmd_list_timer.stop();
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

    if (!serial.isOpen())
    {
        QMessageBox::critical(this, tr("send data error"), "please open serial port first");
        return;
    }

    clear_cmd_list_handle();
/*
    if(ui->atSetLoopCntRadioButton->isChecked()){
        currentLoopIndex = 0;
        totalLoopCnt = ui->atTotalLoopCntText->text().toInt();
    }
*/
    //set_cmd_list_timer.stop();
    cmd_index = 0;
    //set_cmd_list_timer.start(50);
    //set_cmd_list_handle();
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
        //set_cmd_list_timer.stop();
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

    if(!serial.isOpen())
    {
        QMessageBox::warning(this,"Send Error","Please Open Serial Port First!\n");
        return;
    }

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
        case IR_TYPE_JVC:
            memcpy(ir_item.IR_CMD.IR_JVC.name,tmpBuf,MAX_NAME_LEN);
            break;
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

    if(!serial.isOpen())
    {
        QMessageBox::warning(this,"Port warning","Please Open Serial Port First!\n");
        return;
    }
    else
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

    //QString oldCus = ui->atCustomerCombox->currentText(); //backup last selection
   // QString oldDev = ui->atDeviceCombox->currentText();//backup

    loadInsetIrMapTable();
    //ui->atCustomerCombox->setCurrentText(oldCus);
    //ui->atDeviceCombox->setCurrentText(oldDev);

    QMessageBox::information(this,"Save Success","Save Key map to file:" + filename);
    leClearButton_slot();

}
/*---------------------------------------------------*/


void MainWindow::on_actionUser_Manual_triggered()
{
    QDesktopServices::openUrl(QUrl("http://wiki.mediatek.inc/display/HBGSMTeam/BDP+Software+Process+Flow"));
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
