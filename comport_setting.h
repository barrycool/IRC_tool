#ifndef COMPORT_SETTING_H
#define COMPORT_SETTING_H

#include <QDialog>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDebug>

namespace Ui {
class ComPort_Setting;
}

struct portSetting_t {
  QString port_name;
  uint64_t baudRate;
  uint8_t checkBit;
  uint8_t stopBit;
  uint8_t dataBit;
};

class ComPort_Setting : public QDialog
{
    Q_OBJECT

public:
    explicit ComPort_Setting(QWidget *parent,struct portSetting_t *portSetting);
    ~ComPort_Setting();
    struct portSetting_t *portSetting;

private slots:
    void on_okButton_clicked();

signals:
    void sendsignal();

private:
    Ui::ComPort_Setting *ui;
    void setPortSettingInfo();

};

#endif // COMPORT_SETTING_H
