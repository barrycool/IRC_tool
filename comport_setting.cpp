#include "comport_setting.h"
#include "ui_comport_setting.h"

ComPort_Setting::ComPort_Setting(QWidget *parent,struct portSetting_t *portSetting) :
    QDialog(parent),
    ui(new Ui::ComPort_Setting)
{
    ui->setupUi(this);
    this->portSetting = portSetting;

    qDebug() << "portname:" << this->portSetting->port_name;
    qDebug() << "baudrate:" << this->portSetting->baudRate;
    qDebug() << "checkbit:" << this->portSetting->checkBit;
    qDebug() << "dataBit:" << this->portSetting->dataBit;
    qDebug() << "stopBit:" << this->portSetting->stopBit;

    setPortSettingInfo();
}
ComPort_Setting::~ComPort_Setting()
{
    //this->portSetting->port_name = ui->portBox->currentText();
    //this->portSetting->baudRate = ui->baudrateBox->currentText().toInt();
    //this->portSetting->dataBit = ui->DataBitText->text().toInt();
    //this->portSetting->checkBit = ui->checkBitText->text().toInt();
    //this->portSetting->stopBit = ui->stopBitText->text().toInt();
    delete ui;
}

void ComPort_Setting::setPortSettingInfo()
{
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    //qDebug() << "how many ports:" << portList.size();
    if(portList.size() > 0)
    {
        QSerialPort serialtmp;
        foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        {

            serialtmp.setPort(info);
            if(serialtmp.open(QIODevice::ReadWrite))
            {
                ui->portBox->addItem(serialtmp.portName());
                serialtmp.close();
            }
            //ui->baudrateText->setCurrentIndex(3);
        }
        ui->portBox->setCurrentText(this->portSetting->port_name);
        ui->baudrateBox->setCurrentText(QString::number(this->portSetting->baudRate));
        ui->checkBitText->setText(QString::number(this->portSetting->checkBit));
        ui->DataBitText->setText(QString::number(this->portSetting->dataBit));
        ui->stopBitText->setText(QString::number(this->portSetting->stopBit));

    }
    else
        QMessageBox::information(this,"Warning","No available ComPort!");
}

void ComPort_Setting::on_okButton_clicked()
{
    this->portSetting->port_name = ui->portBox->currentText();
    this->portSetting->baudRate = ui->baudrateBox->currentText().toInt();
    this->portSetting->dataBit = ui->DataBitText->text().toInt();
    this->portSetting->checkBit = ui->checkBitText->text().toInt();
    this->portSetting->stopBit = ui->stopBitText->text().toInt();
    emit sendsignal();

    //this->close();
}
