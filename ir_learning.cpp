#include "ir_learning.h"
#include "protocol.h"
#include <QDebug>
#include <QMessageBox>

IR_learning::IR_learning()
{

}

#define LEARNING_PRECISION 3

#define SIRCS_START_BIT_HIGH 24
#define SIRCS_START_BIT_LOW 6

#define NEC_START_BIT_HIGH  90
#define NEC_START_BIT_LOW  45

#define RC6_START_BIT_HIGH 27
#define RC6_START_BIT_LOW 9

#define RC5_START_BIT_HIGH 9
#define RC5_START_BIT_LOW 9

#define JVC_START_BIT_HIGH  84
#define JVC_START_BIT_LOW  42

void pop_sort(uint8_t cnt, uint8_t *times, uint8_t *bit_length)
{
  uint8_t i, j;
  uint8_t tmp;

  for(i = 0; i < cnt - 1; i++)
  {
    for(j = i; j < cnt - 1; j++)
    {
      if (times[j] < times[j+1])
      {
        tmp = times[j];
        times[j] = times[j+1];
        times[j+1] = tmp;

        tmp = bit_length[j];
        bit_length[j] = bit_length[j+1];
        bit_length[j+1] = tmp;
      }
    }
  }
}

uint8_t IR_TX_WF_buf[128];
uint8_t IR_TX_WF_buf_len;

uint8_t IR_decode_learning(struct IR_learning_t *IR_learning)
{
    uint8_t i, j;
    uint8_t diff_cnt = 0;
    uint8_t coding_length[IR_LEARNING_PLUSE_CNT];
    uint64_t coding_value[IR_LEARNING_PLUSE_CNT];
    uint8_t coding_mask[IR_LEARNING_PLUSE_CNT];
    uint64_t tmp;
    uint8_t bit_sum = 0;

  for(i = 0; i< IR_LEARNING_PLUSE_CNT; i++)
  {
    if (IR_learning->pluse_width[i] == 0xFF)
    {
      break;
    }

    diff_cnt++;
  }

  if (diff_cnt == 2)
  {
      coding_value[0] = 0;
      coding_length[0] = 1;
      coding_mask[0] = 1;

      coding_value[1] = 1;
      coding_length[1] = 1;
      coding_mask[1] = 1;
  }
  else if (diff_cnt == 3)
  {
      coding_value[0] = 0;
      coding_length[0] = 1;
      coding_mask[0] = 1;

      coding_value[1] = 1;
      coding_length[1] = 2;
      coding_mask[1] = 3;

      coding_value[2] = 3;
      coding_length[2] = 2;
      coding_mask[2] = 3;
  }
  else
  {
      return 0;
  }


  IR_TX_WF_buf_len = 0;
  IR_TX_WF_buf[IR_TX_WF_buf_len++] = IR_learning->header_high;
  IR_TX_WF_buf[IR_TX_WF_buf_len++] = IR_learning->header_low;

    for(i = 0; i< IR_learning->bit_number && bit_sum < 16; i++)
    {
      for(j = 0; j< diff_cnt; j++)
      {
          if ((IR_learning->bit_data & coding_mask[j]) == coding_value[j])
          {
              IR_TX_WF_buf[IR_TX_WF_buf_len++] = IR_learning->pluse_width[j];
              IR_learning->bit_data >>= coding_length[j];
              bit_sum += coding_length[j];
              break;
          }
      }

      if (j == diff_cnt)
      {
        return 0;
      }
    }

    tmp = IR_learning->bit_data_ext_16;
    IR_learning->bit_data |= tmp << (64 - bit_sum);

    for(;i< IR_learning->bit_number; i++)
    {
      for(j = 0; j< diff_cnt; j++)
      {
          if ((IR_learning->bit_data & coding_mask[j]) == coding_value[j])
          {
              IR_TX_WF_buf[IR_TX_WF_buf_len++] = IR_learning->pluse_width[j];
              IR_learning->bit_data >>= coding_length[j];
              bit_sum += coding_length[j];
              break;
          }
      }

      if (j == diff_cnt)
      {
        return 0;
      }
    }

  QString s;
  for (i = 0; i < IR_TX_WF_buf_len; i++)
  {
    s += QString::asprintf("%02d ", IR_TX_WF_buf[i]);
  }

  qDebug() << s << endl;

  return 1;
}

uint8_t IR_encode_learning(uint8_t waveform[], uint8_t wave_form_cnt, IR_learning_t &IR_learn)
{
    uint8_t i, j;
    uint8_t diff_cnt;
    uint8_t repeate_times[IR_LEARNING_PLUSE_CNT];

    QString s;

    for (i = 0; i < wave_form_cnt; i++)
    {
      s += QString::asprintf("%02d ", waveform[i]);
    }

    qDebug() << s << endl;

    for (i = 0; i < IR_LEARNING_PLUSE_CNT; i++)
    {
      IR_learn.pluse_width[i] = 0xFF;
    }

    IR_learn.header_high = waveform[0];
    IR_learn.header_low = waveform[1];

    diff_cnt = 0;
    for (i = 2; i < wave_form_cnt; i++)
    {
      for (j = 0; j < diff_cnt; j++)
      {
        if (abs(IR_learn.pluse_width[j] - waveform[i]) < LEARNING_PRECISION)
        {
            repeate_times[j]++;
          break;
        }
      }

      if (j == diff_cnt)
      {
        IR_learn.pluse_width[j] = waveform[i];
        repeate_times[j] = 1;
        diff_cnt++;
        if (diff_cnt > size_t(IR_learn.pluse_width))
        {
          return 0;
        }
      }
    }

    pop_sort(diff_cnt, repeate_times, IR_learn.pluse_width);

    uint8_t coding_length[IR_LEARNING_PLUSE_CNT];
    uint64_t coding_value[IR_LEARNING_PLUSE_CNT];

    if (diff_cnt == 2)
    {
        coding_value[0] = 0;
        coding_length[0] = 1;

        coding_value[1] = 1;
        coding_length[1] = 1;
    }
    else if (diff_cnt == 3)
    {
        coding_value[0] = 0;
        coding_length[0] = 1;

        coding_value[1] = 1;
        coding_length[1] = 2;

        coding_value[2] = 3;
        coding_length[2] = 2;
    }
    else
    {
        return 0;
    }

    IR_learn.bit_data = 0;
    IR_learn.bit_data_ext_16 = 0;
    IR_learn.bit_number = 0;

    uint8_t bit_index = 0;

    for (i = 2; i < wave_form_cnt; i++)
    {
        for (j = 0; j < diff_cnt; j++)
        {
            if (abs(IR_learn.pluse_width[j] - waveform[i]) < LEARNING_PRECISION)
            {
                if (bit_index < 64)
                {
                    IR_learn.bit_data |= (coding_value[j] << bit_index);

                    if (bit_index +  coding_length[j] > 64)
                    {
                        IR_learn.bit_data_ext_16 |= coding_value[j] >> (64 - bit_index);
                    }
                }
                else
                {
                    IR_learn.bit_data_ext_16 |= (coding_value[j] << (bit_index - 64));
                }

                bit_index += coding_length[j];

                IR_learn.bit_number++;
                break;
            }
        }

        if (j == diff_cnt)
        {
            return 0;
        }
    }

    //IR_decode_learning(&IR_learn);

    return 1;
}

#define SIRCS_BIT0_HIGH 6
#define SIRCS_BIT0_LOW 6
#define SIRCS_BIT1_HIGH 12
#define SIRCS_BIT1_LOW 6
#define SIRCS_ADDR_LEN 5
#define SIRCS_EXT3_ADDR_LEN 3
#define SIRCS_EXT5_ADDR_LEN 5
#define SIRCS_CMD_LEN 7
uint8_t IR_encode_SIRCS(uint8_t waveform[255], uint8_t wave_form_cnt, IR_SIRCS_t &IR_SIRCS)
{
    uint8_t index = 2;

    IR_SIRCS.IR_command = 0;
    IR_SIRCS.IR_address = 0;
    IR_SIRCS.IR_ext_3_address = 0;
    IR_SIRCS.IR_ext_5_address = 0;

    for(uint8_t i = 0; i < SIRCS_CMD_LEN; i++)
    {
        if (abs(waveform[index] - SIRCS_BIT1_HIGH) < LEARNING_PRECISION)
        {
            IR_SIRCS.IR_command |= (1 << i);
        }

        index += 2;
    }

    for(uint8_t i = 0; i < SIRCS_ADDR_LEN; i++)
    {
        if (abs(waveform[index] - SIRCS_BIT1_HIGH) < LEARNING_PRECISION)
        {
            IR_SIRCS.IR_address |= (1 << i);
        }

        index += 2;
    }

    if (wave_form_cnt > 25)
    {
        for(uint8_t i = 0; i < SIRCS_EXT3_ADDR_LEN; i++)
        {
            if (abs(waveform[index] - SIRCS_BIT1_HIGH) < LEARNING_PRECISION)
            {
                IR_SIRCS.IR_ext_3_address |= (1 << i);
            }

            index += 2;
        }
    }

    if (wave_form_cnt > 31)
    {
        for(uint8_t i = 0; i < SIRCS_EXT5_ADDR_LEN; i++)
        {
            if (abs(waveform[index] - SIRCS_BIT1_HIGH) < LEARNING_PRECISION)
            {
                IR_SIRCS.IR_ext_5_address |= (1 << i);
            }

            index += 2;
        }
    }

    return 1;
}

#define NEC_BIT1_HIGH  6
#define NEC_BIT1_LOW  17
#define NEC_BIT0_HIGH  6
#define NEC_BIT0_LOW  6
#define NEC_ADDR_LEN 8
#define NEC_CMD_LEN 8
uint8_t IR_encode_NEC(uint8_t waveform[255], uint8_t wave_form_cnt, IR_NEC_t &IR_NEC)
{
    uint8_t index = 2;

    (void)wave_form_cnt;

    IR_NEC.IR_header_high = waveform[0];
    IR_NEC.IR_address = 0;
    IR_NEC.IR_address_ext = 0;
    IR_NEC.IR_command = 0;

    for(uint8_t i = 0; i < NEC_ADDR_LEN; i++)
    {
        if (abs(waveform[index + 1] - NEC_BIT1_LOW) < LEARNING_PRECISION)
        {
            IR_NEC.IR_address |= (1 << i);
        }

        index += 2;
    }

    for(uint8_t i = 0; i < NEC_ADDR_LEN; i++)
    {
        if (abs(waveform[index + 1] - NEC_BIT1_LOW) < LEARNING_PRECISION)
        {
            IR_NEC.IR_address_ext |= (1 << i);
        }

        index += 2;
    }

    for(uint8_t i = 0; i < NEC_CMD_LEN; i++)
    {
        if (abs(waveform[index + 1] - NEC_BIT1_LOW) < LEARNING_PRECISION)
        {
            IR_NEC.IR_command |= (1 << i);
        }

        index += 2;
    }

    uint8_t IR_command_revert = 0;
    for(uint8_t i = 0; i < NEC_CMD_LEN; i++)
    {
        if (abs(waveform[index + 1] - NEC_BIT1_LOW) < LEARNING_PRECISION)
        {
            IR_command_revert |= (1 << i);
        }

        index += 2;
    }

    IR_command_revert = ~IR_command_revert;

    if (IR_command_revert != IR_NEC.IR_command)
    {
        return 0;
    }

    return 1;
}

#define JVC_BIT1_HIGH  5
#define JVC_BIT1_LOW  16
#define JVC_BIT0_HIGH  5
#define JVC_BIT0_LOW  5
#define JVC_ADDR_LEN 8
#define JVC_CMD_LEN 8
uint8_t IR_encode_JVC(uint8_t waveform[255], uint8_t wave_form_cnt, IR_JVC_t &IR_JVC)
{
    uint8_t index = 2;

    (void)wave_form_cnt;
    IR_JVC.IR_address = 0;
    IR_JVC.IR_command = 0;

    for(uint8_t i = 0; i < JVC_ADDR_LEN; i++)
    {
        if (abs(waveform[index + 1] - JVC_BIT1_LOW) < LEARNING_PRECISION)
        {
            IR_JVC.IR_address |= (1 << i);
        }

        index += 2;
    }

    for(uint8_t i = 0; i < JVC_CMD_LEN; i++)
    {
        if (abs(waveform[index + 1] - JVC_BIT1_LOW) < LEARNING_PRECISION)
        {
            IR_JVC.IR_command |= (1 << i);
        }

        index += 2;
    }

    return 1;
}

#define RC5_DATA_LEN 9
#define RC5_DOUBLE_DATA_LEN 18
#define RC5_TOGGLE_LEN RC5_DATA_LEN
#define RC5_ADDR_LEN 5
#define RC5_CMD_LEN 6
uint8_t IR_encode_RC5(uint8_t waveform[255], uint8_t wave_form_cnt, IR_RC5_t &IR_RC5)
{
    uint8_t step_buf[256];
    uint8_t step_buf_index = 0;

    (void)wave_form_cnt;
    IR_RC5.IR_address = 0;
    IR_RC5.IR_command = 0;

    step_buf[step_buf_index++] = 0;

    for(uint8_t i = 0; i < wave_form_cnt; i++)
    {
        if (i & 1)
        {
            step_buf[step_buf_index++] = 0;

            if (abs(waveform[i] - RC5_DOUBLE_DATA_LEN) < LEARNING_PRECISION)
            {
                step_buf[step_buf_index++] = 0;
            }
        }
        else
        {
            step_buf[step_buf_index++] = 1;

            if (abs(waveform[i] - RC5_DOUBLE_DATA_LEN) < LEARNING_PRECISION)
            {
                step_buf[step_buf_index++] = 1;
            }
        }
    }

    step_buf_index = 2;

    if (step_buf[step_buf_index + 1])
    {
        IR_RC5.IR_command |= 1;
    }

    step_buf_index = 6;

    for(uint8_t i = 0; i < RC5_ADDR_LEN; i++)
    {
        IR_RC5.IR_address <<= 1;
        if (step_buf[step_buf_index + 1])
        {
            IR_RC5.IR_address |= 1;
        }

        step_buf_index += 2;
    }

    for(uint8_t i = 0; i < RC5_CMD_LEN; i++)
    {
        IR_RC5.IR_command <<= 1;
        if (step_buf[step_buf_index + 1])
        {
            IR_RC5.IR_command |= 1;
        }

        step_buf_index += 2;
    }

    return 1;
}

#define RC6_DATA_LEN 5
#define RC6_DOUBEL_DATA_LEN 9
#define RC6_TRIPLE_DATA_LEN 13
#define RC6_TOGGLE_LEN RC6_DOUBEL_DATA_LEN
#define RC6_MODE_LEN 3
#define RC6_ADDR_LEN 8
#define RC6_CMD_LEN 8
uint8_t IR_encode_RC6(uint8_t waveform[255], uint8_t wave_form_cnt, IR_RC6_t &IR_RC6)
{
    uint8_t step_buf[256];
    uint8_t step_buf_index = 0;

    (void)wave_form_cnt;
    IR_RC6.IR_address = 0;
    IR_RC6.IR_command = 0;

    for(uint8_t i = 2; i < wave_form_cnt; i++)
    {
        if (i & 1)
        {
            step_buf[step_buf_index++] = 0;

            if (abs(waveform[i] - RC6_DOUBEL_DATA_LEN) < LEARNING_PRECISION)
            {
                step_buf[step_buf_index++] = 0;
            }
            else if (abs(waveform[i] - RC6_TRIPLE_DATA_LEN) < LEARNING_PRECISION)
            {
                step_buf[step_buf_index++] = 0;
                step_buf[step_buf_index++] = 0;
            }
        }
        else
        {
            step_buf[step_buf_index++] = 1;

            if (abs(waveform[i] - RC6_DOUBEL_DATA_LEN) < LEARNING_PRECISION)
            {
                step_buf[step_buf_index++] = 1;
            }
            else if (abs(waveform[i] - RC6_TRIPLE_DATA_LEN) < LEARNING_PRECISION)
            {
                step_buf[step_buf_index++] = 1;
                step_buf[step_buf_index++] = 1;
            }
        }
    }

    step_buf_index = 2;

    for(uint8_t i = 0; i < RC6_MODE_LEN; i++)
    {
        IR_RC6.IR_mode <<= 1;
        if (step_buf[step_buf_index])
        {
            IR_RC6.IR_mode |= 1;
        }

        step_buf_index += 2;
    }

    step_buf_index += 4;

    for(uint8_t i = 0; i < RC6_ADDR_LEN; i++)
    {
        IR_RC6.IR_address <<= 1;
        if (step_buf[step_buf_index])
        {
            IR_RC6.IR_address |= 1;
        }

        step_buf_index += 2;
    }

    for(uint8_t i = 0; i < RC6_CMD_LEN; i++)
    {
        IR_RC6.IR_command <<= 1;
        if (step_buf[step_buf_index])
        {
            IR_RC6.IR_command |= 1;
        }

        step_buf_index += 2;
    }

    return 1;
}

IR_item_t IR_learning_item;
uint8_t IR_encode(uint8_t waveform[], uint8_t wave_form_cnt)
{
    //qDebug() << "IR_encode,wave_form_cnt= " << wave_form_cnt;
    IR_learning_item.is_valid = 1;

    if(abs(waveform[0] -  SIRCS_START_BIT_HIGH) < LEARNING_PRECISION && abs(waveform[1] -  SIRCS_START_BIT_LOW) < LEARNING_PRECISION &&
          (wave_form_cnt == 25 || wave_form_cnt == 31 || wave_form_cnt == 41))
    {
        //qDebug() << "IR_TYPE_SIRCS";
        IR_learning_item.IR_type = IR_TYPE_SIRCS;
        if (IR_encode_SIRCS(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_SIRCS))
            return 1;
    }
    if (/*abs(waveform[0] -  NEC_START_BIT_HIGH) < LEARNING_PRECISION &&*/ abs(waveform[1] -  NEC_START_BIT_LOW) < LEARNING_PRECISION &&
             wave_form_cnt == 67)
    {
       // qDebug() << "IR_TYPE_NEC";
        IR_learning_item.IR_type = IR_TYPE_NEC;
        if (IR_encode_NEC(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_NEC))
            return 1;
    }
    if (abs(waveform[0] -  RC6_START_BIT_HIGH) < LEARNING_PRECISION && abs(waveform[1] -  RC6_START_BIT_LOW) < LEARNING_PRECISION)
    {
        //qDebug() << "IR_TYPE_RC6";
        IR_learning_item.IR_type = IR_TYPE_RC6;
        if (IR_encode_RC6(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_RC6))
            return 1;
    }
    if ((abs(waveform[0] -  RC5_START_BIT_HIGH) < LEARNING_PRECISION || abs(waveform[0] - 2 * RC5_START_BIT_HIGH) < LEARNING_PRECISION) &&
             abs(waveform[1] -  RC5_START_BIT_LOW) < LEARNING_PRECISION)
    {
        //qDebug() << "IR_TYPE_RC5";
        IR_learning_item.IR_type = IR_TYPE_RC5;
        if (IR_encode_RC5(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_RC5))
            return 1;

    }
    if (abs(waveform[0] -  JVC_START_BIT_HIGH) < LEARNING_PRECISION && abs(waveform[1] -  JVC_START_BIT_LOW) < LEARNING_PRECISION &&
             wave_form_cnt == 35)
    {
        //qDebug() << "IR_TYPE_JVC";
        IR_learning_item.IR_type = IR_TYPE_JVC;
        if (IR_encode_JVC(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_JVC))
            return 1;
    }

    IR_learning_item.IR_type = IR_TYPE_LEARNING;
    if (IR_encode_learning(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_learning))
        return 1;

    //QMessageBox::critical(NULL, "unspoorted IR protocol", "unspoorted IR protocol");

    return 0;
}

QString hexByte2String(uint8_t value)
{
    QString tmp = QString::number(value,16);
    bool ok;
    //接收字符为0x00，变字符串为00
    if (tmp.toInt(&ok,16) == 0)
    {
        tmp = ("00");
    }
    //接收字符为0x04,0x07等，补足0
    if ((tmp.toInt(&ok,16) < 0xf) && (tmp.toInt(&ok,16) > 0))
    {
        tmp = tmp.insert(0, "0");
    }
    return tmp;
}

QString IRLearningItem2String(int nLength)
{
    //int m_nLength = nLength;
    //qDebug() << "IRLearningItem2String IR_type: " <<IR_learning_item.IR_type;

    //uint8_t m_pData[m_nLength];
    //memcpy(m_pData, pData, m_nLength);

    (void)nLength;

    QString m_result = QString::number(IR_learning_item.IR_type).append("-");

    QString m_tmp;
    switch(IR_learning_item.IR_type)
    {
        case IR_TYPE_SIRCS:
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_SIRCS.IR_address)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_SIRCS.IR_ext_3_address)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_SIRCS.IR_ext_5_address)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_SIRCS.IR_command));
            break;
        case IR_TYPE_NEC:
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_NEC.IR_header_high)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_NEC.IR_address)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_NEC.IR_address_ext)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_NEC.IR_command));
            break;
        case IR_TYPE_RC6:
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_RC6.IR_mode)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_RC6.IR_address)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_RC6.IR_command));
            break;
        case IR_TYPE_RC5:
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_RC5.IR_address)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_RC5.IR_command));
            break;
        case IR_TYPE_JVC:
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_JVC.IR_address)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_JVC.IR_command));
            break;
        case IR_TYPE_LEARNING:
            //qDebug() << "IR_learning.bit_data=" << IR_learning_item.IR_CMD.IR_learning.bit_data;
            //qDebug("IR_learning.bit_data=0x%x",IR_learning_item.IR_CMD.IR_learning.bit_data);
            //qDebug() << "SIR_learning.bit_data=" << QString::number(IR_learning_item.IR_CMD.IR_learning.bit_data,16);

            m_tmp.append(QString::number(IR_learning_item.IR_CMD.IR_learning.bit_data,16)).append("-");
            m_tmp.append(QString::number(IR_learning_item.IR_CMD.IR_learning.bit_data_ext_16,16)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_learning.bit_number)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_learning.header_high)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_learning.header_low)).append("-");
            for(int i =0;i<IR_LEARNING_PLUSE_CNT;i++)
            {
                m_tmp.append("-").append(hexByte2String(IR_learning_item.IR_CMD.IR_learning.pluse_width[i]));
            }
            break;
        default:
            break;
    }
    m_result.append(m_tmp);
    //qDebug() << "m_reslut: " <<m_result;
    return m_result;
}

bool String2IRLearningItem(QString src,IR_item_t *learningItem)
{
    qDebug() << "String2IRLearningItem :src"<< src;
    bool ok = 0;
    int len = 0;
    int irType = 0;
    if (src.isNull() || src.isEmpty()) {
        return false;
    }
    if(learningItem == NULL)
    {
       return false;
    }

    QStringList list1 = src.split('-');
    len = list1.size();
    irType = list1.at(0).toInt(&ok,16);

    learningItem->IR_type = irType;

    for (int i = 0; i < len; i++) {
        //qDebug("len=%d,dst[%d] = 0x%.2x ",len,i,list1.at(i).toInt(&ok,16));  //for debug
    }

    //qDebug() << "SIR_learning.bit_data=" << list1.at(1);
    //qDebug() << "IR_learning.bit_data=" << list1.at(1).toULongLong(&ok1,16);

    switch(irType)
    {
        case IR_TYPE_SIRCS:
            if(len == 5)
            {
                learningItem->IR_CMD.IR_SIRCS.IR_address = list1.at(1).toInt(&ok,16);
                learningItem->IR_CMD.IR_SIRCS.IR_ext_3_address = list1.at(2).toInt(&ok,16);
                learningItem->IR_CMD.IR_SIRCS.IR_ext_5_address = list1.at(3).toInt(&ok,16);
                learningItem->IR_CMD.IR_SIRCS.IR_command = list1.at(4).toInt(&ok,16);
            }
            else
            {
                qDebug() << "format error!";
            }
            break;
        case IR_TYPE_NEC:
            if(len == 5)
            {
                learningItem->IR_CMD.IR_NEC.IR_header_high = list1.at(1).toInt(&ok,16);
                learningItem->IR_CMD.IR_NEC.IR_address = list1.at(2).toInt(&ok,16);
                learningItem->IR_CMD.IR_NEC.IR_address_ext = list1.at(3).toInt(&ok,16);
                learningItem->IR_CMD.IR_NEC.IR_command = list1.at(4).toInt(&ok,16);
            }
            else
            {
                qDebug() << "format error!";
            }
            break;
        case IR_TYPE_RC6:
            if(len == 4)
            {
                learningItem->IR_CMD.IR_RC6.IR_mode = list1.at(1).toInt(&ok,16);
                learningItem->IR_CMD.IR_RC6.IR_address = list1.at(2).toInt(&ok,16);
                learningItem->IR_CMD.IR_RC6.IR_command = list1.at(3).toInt(&ok,16);
            }
            else
            {
                qDebug() << "format error!";
            }
            break;
        case IR_TYPE_RC5:
            if(len == 3)
            {
                learningItem->IR_CMD.IR_RC5.IR_address = list1.at(1).toInt(&ok,16);
                learningItem->IR_CMD.IR_RC5.IR_command = list1.at(2).toInt(&ok,16);
            }
            else
            {
                qDebug() << "format error!";
            }
            break;
        case IR_TYPE_JVC:
            if(len == 3)
            {
                learningItem->IR_CMD.IR_JVC.IR_address = list1.at(1).toInt(&ok,16);
                learningItem->IR_CMD.IR_JVC.IR_command = list1.at(2).toInt(&ok,16);
            }
            else
            {
                qDebug() << "format error!";
            }
            break;
        case IR_TYPE_LEARNING:
            if(len == 10)
            {
                learningItem->IR_CMD.IR_learning.bit_data = list1.at(1).toULongLong(&ok,16);
                learningItem->IR_CMD.IR_learning.bit_data_ext_16 = list1.at(2).toUInt(&ok,16);
                learningItem->IR_CMD.IR_learning.bit_number = list1.at(3).toInt(&ok,16);
                learningItem->IR_CMD.IR_learning.header_high = list1.at(4).toInt(&ok,16);
                learningItem->IR_CMD.IR_learning.header_low= list1.at(5).toInt(&ok,16);
                for(int i =0;i<IR_LEARNING_PLUSE_CNT;i++)
                {
                   learningItem->IR_CMD.IR_learning.pluse_width[i]= list1.at(6+i).toInt(&ok,16);
                }
            }
            else
            {
                qDebug() << "format error!";
            }
            break;
        default:
            break;
    }

    return true;
}
