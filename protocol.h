#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QDialog>
#include <QDebug>

#define FRAME_HEADER        0x55
#define MAX_KEY_LEN         128
#define MAX_NAME_LEN        16
#define UPGRADE_PACKET_SIZE 52
#define BUF_LEN             255
#define IR_MAX_LEVEL_NUMBER     10
#define FAIL_RETRY_TIMES    1
#define IR_devices_MAX  20

enum IR_type_t {
  IR_TYPE_SIRCS,
  IR_TYPE_NEC,
  IR_TYPE_RC6,
  IR_TYPE_RC5,
  IR_TYPE_JVC,
  IR_TYPE_LEARNING,

  IR_TYPE_MAX
};

enum SIRCS_dev{
    SIRCS_BDP,
    SIRCS_SOUNDBAR,

    SIRCS_DEV_MAX
};

struct IR_SIRCS_t {
  uint8_t IR_address;
  uint8_t IR_ext_3_address;
  uint8_t IR_ext_5_address;
  uint8_t IR_command;
  char name[16];
};

struct IR_NEC_t {
  uint8_t IR_header_high;
  uint8_t IR_address;
  uint8_t IR_address_ext;
  uint8_t IR_command;
  char name[16];
};

struct IR_RC6_t {
  uint8_t IR_mode : 3;
  uint8_t IR_address;
  uint8_t IR_command;
  char name[16];
};

struct IR_RC5_t {
  uint8_t IR_address :5;
  uint8_t IR_command : 6;
  char name[16];
};

struct IR_JVC_t {
  uint8_t IR_address;
  uint8_t IR_command;
  char name[16];
};

#define IR_LEARNING_PLUSE_CNT 3
struct IR_learning_t{
  uint64_t bit_data;
  uint16_t bit_data_ext_16;

  uint8_t bit_number;

  uint8_t header_high;
  uint8_t header_low;

  uint8_t pluse_width[IR_LEARNING_PLUSE_CNT];

  char name[16];
};

union IR_CMD_t{
  struct IR_SIRCS_t IR_SIRCS;
  struct IR_NEC_t IR_NEC;
  struct IR_RC6_t IR_RC6;
  struct IR_RC5_t IR_RC5;
  struct IR_JVC_t IR_JVC;
  struct IR_learning_t IR_learning;
};

struct IR_item_t {
  union IR_CMD_t IR_CMD;
  uint8_t IR_type;

  uint8_t is_valid;
  uint32_t delay_time; //ms
};

struct IR_map_t {
  uint8_t IR_type;
  QString keyValue;
  //uint8_t IR_address;
  //uint8_t IR_ext_3_address;
  //uint8_t IR_ext_5_address;
  //uint8_t IR_command;
  QString name;
};

struct IR_learningkey_t {
  uint8_t irType;
  QString keyValue;
  QString name;
};

/*
struct IR_learn_t {
  uint8_t IR_key[IR_MAX_LEVEL_NUMBER];
  char name[MAX_NAME_LEN];
};
*/

enum msg_t {
    CMD_NACK,           //0x00
    CMD_ACK,            //0x01
    REAL_TIME_SEND,     //0x02
    CLEAR_CMD_LIST,     //0x03
    SET_CMD_LIST,       //0x04
    READ_CMD_LIST,      //0x05
    REAL_TIME_RECV,     //0X06  mcu报告接收到一条IR按键消息
    READ_MCU_VERSION,   //0x07
    MCU_VERSION,        //0x08 mcu返回mcu的版本信息,eg:20170417
    UPGRADE_START,      //0x09
    SEND_UPGRADE_PACKET,//0x0a 向MCU发送升级包
    UPGRADE_FINISH,     //0x0b
    PAUSE_SEND,         //0x0c
    START_SEND,         //0x0d
    STOP_LEARNING,      //0x0e
    START_LEARNING,     //0x0f
    //SIRCS_CMD_MAX,
};


struct frame_t {
  uint8_t header;
  uint8_t data_len;
  uint8_t seq_num;
  uint8_t msg;

  uint8_t msg_parameter[0];
};

struct IR_MCU_Version_t {
  uint8_t day;
  uint8_t month;
  uint8_t year_low;
  uint8_t year_high;
};

struct IR_Upgrade_Packet_t {
    uint8_t packet_id1;
    uint8_t packet_id2;
    char data[UPGRADE_PACKET_SIZE];
};


#define SIRCS_CMD_MAX 40
//extern struct IR_SIRCS_t IR_SIRCS_commands[SIRCS_DEV_MAX][SIRCS_CMD_MAX];
extern bool HexString2Bytes(QString src,uint8_t* dst);
extern QString byteArray2String(uint8_t *pData);
class protocol
{
public:
    protocol();
};

#endif // PROTOCOL_H
