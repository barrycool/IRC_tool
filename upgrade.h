#ifndef UPGRADE_H
#define UPGRADE_H

#include <QSerialPort>
#include <QProgressBar>


class upgrade
{
public:
    upgrade();
};

uint8_t upgrade_init(QSerialPort &port, uint8_t seq_num, QProgressBar *progressBar);
void upgrade_send_packet(QSerialPort &port, uint8_t seq_num, QProgressBar *progressBar);
void upgrade_cancel(QProgressBar *progressBar);

#endif // UPGRADE_H
