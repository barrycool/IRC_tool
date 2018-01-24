#ifndef SIRCOMMAND_H
#define SIRCOMMAND_H

#include <protocol.h>
#include <QSerialPort>
#include <QTcpSocket>
#include <QSerialPortInfo>
#include <QDesktopServices>
#include <QThread>
#include <QApplication>
#include <QFile>
#include <crc32.h>

enum transMode_T {
    TRANSPORT_BY_SERIAL,           //0x00
    TRANSPORT_BY_WIFI,            //0x01
};

class SirCommand : public QObject
{
        Q_OBJECT
public:
    SirCommand(int transMode,QString portname);
    ~SirCommand();
    int startWithDebugMode(QString protocalFile,QString scriptFile,int loopcnt);
    void stopLoop();
    void clear_and_stop();

private slots:
    void on_tcp_connect(QAbstractSocket::SocketState state);
    void receive_data();

private:
    QSerialPort serial;
    QTcpSocket socket;

    void DownloadIR_items();
    void clear_cmd_list();
    void set_cmd_list();
    void sendCmdtoMCU(uint8_t *buf,uint8_t len);
    void Ack(uint8_t seq_num, uint8_t msg_id);
    void startsend();

};

#endif // SIRCOMMAND_H
