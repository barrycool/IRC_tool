#include "upgrade.h"
#include <QSerialPort>
#include <QMessageBox>
#include "protocol.h"
#include "crc32.h"
#include <QProgressBar>

upgrade::upgrade()
{

}

void upgrade_finish(QSerialPort &port, uint8_t seq_num);
void upgrade_calucate_bin_check_sum();

//FILE *upgrade_file;

//#define PACKET_MAX_SIZE 192
//uint8_t packet_idx;
//uint8_t upgrade_flag;
//uint32_t total_file_length;
//double current_file_length;
/*
uint8_t upgrade_init(QSerialPort &port, uint8_t seq_num, QProgressBar *progressBar)
{
    upgrade_file = fopen("G:\\learn\\barry_code\\stm32\\IR_stm32f103VE\\EWARM\\IR_stm32f103VE\\Exe\\IR_stm32f103VE.bin", "rb");
    if (upgrade_file == NULL)
    {
        QMessageBox::warning(NULL, "upgrade", "open file error");
        return 0;
    }

    current_file_length = 0;
    progressBar->setValue(0);
    packet_idx = 0;
    upgrade_flag = 1;

    fseek(upgrade_file, 0, SEEK_END);
    total_file_length = ftell(upgrade_file);

    upgrade_calucate_bin_check_sum();

    fseek(upgrade_file, 0, SEEK_SET);

    uint8_t buf[255];

    struct frame_t *frame = (struct frame_t *)buf;

    frame->header = FRAME_HEADER;
    frame->data_len = sizeof(struct frame_t);
    frame->seq_num = seq_num;
    frame->msg = UPGRADE_START;

    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    port.write((char*)buf, frame->data_len + 1);

    return 1;
}

void upgrade_send_packet(QSerialPort &port, uint8_t seq_num, QProgressBar *progressBar)
{
    uint8_t buf[255] = {0};
    size_t read_cnt;

    if (!upgrade_flag)
    {
        return;
    }

    struct frame_t *frame = (struct frame_t *)buf;

    frame->header = FRAME_HEADER;
    frame->data_len = sizeof(struct frame_t);
    frame->seq_num = seq_num;
    frame->msg = SEND_UPGRADE_PACKET;

    buf[frame->data_len++] = packet_idx++;

    read_cnt = fread(buf + frame->data_len, 1, PACKET_MAX_SIZE, upgrade_file);
    if (read_cnt == 0)
    {
        progressBar->setValue(100);
        upgrade_finish(port, seq_num);
        return;
    }
    else if (read_cnt < PACKET_MAX_SIZE)
    {
        memset(buf + frame->data_len + read_cnt, 0xFF, PACKET_MAX_SIZE - read_cnt);
    }

    current_file_length += read_cnt;

    progressBar->setValue((current_file_length / total_file_length) * 100);

    frame->data_len += PACKET_MAX_SIZE;
    buf[frame->data_len] = CRC8Software(buf, frame->data_len);

    port.write((char*)buf, frame->data_len + 1);
}

uint32_t crc32;
void upgrade_finish(QSerialPort &port, uint8_t seq_num)
{
    uint8_t buf[255];
    uint32_t file_len;

    fseek(upgrade_file, 0, SEEK_END);
    file_len = ftell(upgrade_file);

    struct frame_t *frame = (struct frame_t *)buf;

    frame->header = FRAME_HEADER;
    frame->data_len = sizeof(struct frame_t);
    frame->seq_num = seq_num;
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

    port.write((char*)buf, frame->data_len + 1);

    if (upgrade_file != NULL)
    {
        fclose(upgrade_file);
        upgrade_file = NULL;
    }
}

void upgrade_cancel(QProgressBar *progressBar)
{
    upgrade_flag = 0;
    progressBar->setValue(0);

    if (upgrade_file != NULL)
    {
        fclose(upgrade_file);
        upgrade_file = NULL;
    }
}
*/

