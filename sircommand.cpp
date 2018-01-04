#include "sircommand.h"
int cmd_index = 0;
bool serialIsReady = 0;
bool wifiIsReady = 0;
int gseqnum = 0;

extern QList <IR_item_t> IR_items;
extern QList <IR_map_t> IR_maps;

SirCommand::SirCommand(int transMode,QString portname)
{
    if(transMode == TRANSPORT_BY_SERIAL)
    {
        QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
        foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        {
            if(info.portName() == portname)
            {
                serial.setPort(info);
                serial.close();//close it first
                //Sleep(200);
                break;
            }
        }
        serial.setPortName(portname);

        if(serial.open(QIODevice::ReadWrite))
        {
            serial.setBaudRate(QSerialPort::Baud115200);
            serial.setFlowControl(QSerialPort::NoFlowControl);
            serialIsReady = 1;
        }
        else
        {
            qDebug() << "open " << serial.portName() << " fail!";
        }
        QObject::connect(&serial, QSerialPort::readyRead, this, receive_data);

    }
    else if(transMode == TRANSPORT_BY_WIFI)
    {
        QObject::connect(&socket, QTcpSocket::readyRead, this, receive_data);
        QObject::connect(&socket, QTcpSocket::stateChanged, this, on_tcp_connect);
        socket.connectToHost("192.168.4.1", 60001);
    }

}
void SirCommand::on_tcp_connect(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::ConnectedState)
    {
        wifiIsReady = 1;
    }
    else if (state == QAbstractSocket::UnconnectedState)
    {
        wifiIsReady = 0;
    }
}

void SirCommand::DownloadIR_items()
{
    qDebug() << "Download cmd list to mcu\n";
    clear_cmd_list();
    cmd_index = 0;
}
void SirCommand::clear_cmd_list()
{
    qDebug() << "clear_cmd_list:send CLEAR_CMD_LIST to mcu\n";
    //output_log("send CLEAR_CMD_LIST to mcu",1);
    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);
    struct frame_t *frame = (struct frame_t *)buf;

    frame->data_len = sizeof(struct frame_t);
    frame->header = FRAME_HEADER;
    frame->seq_num = gseqnum++;
    frame->msg = CLEAR_CMD_LIST;

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    sendCmdtoMCU(buf, frame->data_len + 1);
}
void SirCommand::set_cmd_list()
{
    if (cmd_index >= IR_items.size())
    {
        qDebug() << "cmd list send finished";
        startsend();
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
    frame->seq_num = gseqnum++;

    frame->msg_parameter[para_index++] = cmd_index;

    //qDebug() << "IR_items.size = " <<IR_items.size();

    IR_item_t ir_item = IR_items.at(cmd_index);

    memcpy(frame->msg_parameter + para_index, &ir_item, sizeof(IR_item_t));

    para_index += sizeof(IR_item_t);
    frame->data_len += para_index;

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    sendCmdtoMCU(buf,frame->data_len +1);

    cmd_index++;

}
void SirCommand::startsend()
{
    //start test
    uint8_t buf[BUF_LEN];
    memset(buf,0x0,BUF_LEN);
    struct frame_t *frame = (struct frame_t *)buf;
    frame->data_len = sizeof(struct frame_t);
    frame->header = FRAME_HEADER;
    frame->seq_num = gseqnum++;
    frame->msg = START_SEND;//CLEAR_CMD_LIST;
    buf[frame->data_len] = CRC8Software(buf, frame->data_len);
    sendCmdtoMCU(buf, frame->data_len + 1);
}

void SirCommand::sendCmdtoMCU(uint8_t *buf,uint8_t len)
{
    if (wifiIsReady && socket.isOpen())
    {
        socket.write((char *)buf, len);
        /*------add for debug----------*/
        QString log; //= "send packet:len=" +QString::number(len);

        for(uint8_t j = 0; j< len; j++)
        {
            log +=  QString::asprintf("%02X ", buf[j]); //QString(" %1").arg(buf[j]);
        }
        qDebug() << log;
        //output_log(log,0);
        /*------add for debug----------*/
    }
    else if(serialIsReady && serial.isOpen())
    {
        /*------add for debug----------*/
                QString log; //= "send packet:len=" +QString::number(len);

                for(uint8_t j = 0; j< len; j++)
                {
                    log +=  QString::asprintf("%02X ", buf[j]); //QString(" %1").arg(buf[j]);
                }
                qDebug() << log;
        /*------add for debug----------*/
        //serial.setBaudRate(QSerialPort::Baud115200);
        //serial.setParity(QSerialPort::NoParity);
        //serial.setDataBits(QSerialPort::Data8);
        //serial.setStopBits(QSerialPort::OneStop);
        //serial.setFlowControl(QSerialPort::NoFlowControl);

       // serial.clearError();
       // serial.clear();
        serial.write((char *)buf, len); //marked just for test
    }
    else
    {
        qDebug() << "please connect smart IR first!";
    }
}
void SirCommand::Ack(uint8_t seq_num, uint8_t msg_id)
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

    sendCmdtoMCU(buf, frame->data_len+1);

}

uint8_t gbuf[BUF_LEN];
qint64 gbuf_len = 0;
void SirCommand::receive_data()
{
    QString log;

    if (wifiIsReady)
    {
        gbuf_len += socket.read((char*)gbuf +gbuf_len, BUF_LEN - gbuf_len);
    }
    else if(serialIsReady)
    {
        gbuf_len += serial.read((char*)gbuf + gbuf_len, BUF_LEN - gbuf_len);
    }

    struct frame_t *frame = (struct frame_t *)gbuf;

    //cmdSemaphore->release();

    if (frame->header != FRAME_HEADER)
    {
        gbuf_len = 0;
        //qDebug() << "header error";
        return;
    }

/*
    if (frame->seq_num != backupframe->seq_num)
    {
        gbuf_len = 0;
        qDebug() << "seq_num error";
        return;
    }
*/

    if (frame->data_len + 1 > gbuf_len || gbuf_len < 4)
    {
       // qDebug() << "length error";
        return;
    }

    uint8_t crc = CRC8Software(gbuf, frame->data_len);

    if (crc != gbuf[frame->data_len])
    {
        gbuf_len = 0;
        qDebug() << "CRC32 err";
        //output_log("CRC32 err",0);
        return;
    }

    qDebug() << "receive packet:";

    for(int j = 0; j< gbuf_len; j++)
    {
        log += QString::asprintf("%02X ", gbuf[j]); //QString::number(gbuf[j],10);
        //log += " ";
    }
    qDebug() << log;
    //output_log(log,0);

    if(frame->msg == CMD_NACK)
    {
        //sendcmd_timer.stop();

        if (frame->msg_parameter[0] == READ_CMD_LIST)
        {
            qDebug() <<"all commands are read out from mcu";
            //output_log("NAK: all commands are read out from mcu",1);
        }

    }
    else if(frame->msg == CMD_ACK)
    {

        if (frame->msg_parameter[0] == CLEAR_CMD_LIST)
        {
            set_cmd_list();
        }
        else if (frame->msg_parameter[0] == SET_CMD_LIST)
        {
            set_cmd_list();
        }
        else if (frame->msg_parameter[0] == START_SEND)
        {
            qDebug() << "start send ok!";
        }
        else if (frame->msg_parameter[0] == PAUSE_SEND)
        {

        }
    }
    else if (frame->msg == SET_CMD_LIST)
    {
        //sendcmd_timer.stop();
        Ack(frame->seq_num, frame->msg);
        uint8_t index = frame->msg_parameter[0];

        qDebug() << "receive cmd list , index = " << index;

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

    }
    gbuf_len = 0;
}
void SirCommand::startWithDebugMode(QString protocalFile,QString scriptFile,int loopcnt)
{
    qDebug() << "protocalFile:"<< protocalFile << " scriptFile:" << scriptFile << " loopcnt:"<< loopcnt;
    //set loop count
    if(loopcnt > 0)
    {
        //totalSendCnt = loopcnt;
    }
    //set IR cmd list
    QString appPath = qApp->applicationDirPath();
    QString insetIrMapTablePath = appPath.append("\\KeyMapConfig\\");
    QString filePath = insetIrMapTablePath + protocalFile;
    qDebug() << "set protocolfile：" + filePath;

    //根据文件名分析出 customer 和 device 的QString
    QStringList customDevice = protocalFile.split(QRegExp("[%.]"), QString::SkipEmptyParts);
    QString pcustomer = customDevice.at(0);
    QString pdevice = customDevice.at(1);

    QFile *file = new QFile;
    file->setFileName(filePath);
    if(!file->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << filePath + "open fail";
        return;
    }

    QTextStream pin(file);
    QString line = pin.readLine();//read custom name
    line = pin.readLine();//read device name
    //should add NEC

    IR_maps.clear();
    while(!pin.atEnd()){
        line = pin.readLine();
        if(line.contains(','))
        {
            saveToIrMaps(line);
        }
    }
    file->close();

    //set test script
    //QFile *file = new QFile; //(QtCore)核心模块,需要手动释放
    filePath = insetIrMapTablePath + scriptFile;
    qDebug() << "set script file :" + filePath;
    file->setFileName(filePath);
    bool ok = file->open(QIODevice::ReadOnly);//以只读模式打开
    if(!ok)
    {
       qDebug() << filePath + "open fail!";
       file->close();
       delete file;
       return;
    }
      QTextStream sin(file);  //文件与文本流相关联
      QString scustomer = sin.readLine();//读取第一行;
      QString sdevice = sin.readLine();//读取第二行;
      qDebug() << scustomer <<" : " <<sdevice;

      //判断 protocolfile 和 scriptfile 的customer 和 device是否相同，若不同提示错误信息
      if(scustomer != pcustomer || sdevice!=pdevice)
      {
          qDebug() << "protocolfile is not matched with scriptfile";
          file->close();
          delete file;
          return;
      }

      while (!sin.atEnd()) {
          QString line = sin.readLine();
          if(line.contains("---"))
          {
              QStringList list1 = line.split("---");
              add_to_IR_Items(list1.at(0),list1.at(1).toInt());
          }
      }
      file->close();
      delete file;

    //download to mcu
    DownloadIR_items();
}
