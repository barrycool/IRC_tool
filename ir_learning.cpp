#include "ir_learning.h"
#include "protocol.h"

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

uint8_t IR_encode_learning(uint8_t waveform[255], uint8_t wave_form_cnt, IR_learning_t &IR_learn)
{
    uint8_t i, j;
    uint8_t repeate_times[4];
    uint8_t diff_cnt;
    uint8_t bit_index;
    uint8_t code[4];
    uint8_t code_length[4];

    for (i = 0; i < 4; i++)
    {
      IR_learn.bit_counter_set[i] = 0xFF;
    }

    IR_learn.header_high = waveform[0];
    IR_learn.header_low = waveform[1];
    diff_cnt = 0;

    for (i = 2; i < wave_form_cnt; i++)
    {
      for (j = 0; j < diff_cnt; j++)
      {
        if (abs(IR_learn.bit_counter_set[j] - waveform[i]) < 4)
        {
          repeate_times[j]++;
          break;
        }
      }

      if (j == diff_cnt)
      {
        IR_learn.bit_counter_set[diff_cnt++] = waveform[i];
        repeate_times[j] = 1;
        if (diff_cnt > 4)
        {
          return 0;
        }
      }
    }

    if (diff_cnt == 2) // 0 1
    {
      code[0] = 0;
      code_length[0] = 1;

      code[1] = 1;
      code_length[1] = 1;
    }
    else if (diff_cnt == 3) // 0 10 11
    {
      code[0] = 0;
      code_length[0] = 1;

      code[1] = 2;
      code_length[1] = 2;

      code[2] = 3;
      code_length[2] = 2;
    }
    else if (diff_cnt == 4) // 0 10 11
    {
      code[0] = 0;
      code_length[0] = 1;

      code[1] = 2;
      code_length[1] = 2;

      code[2] = 6;
      code_length[2] = 3;

      code[3] = 7;
      code_length[3] = 3;
    }
    else
    {
      return 0;
    }

    pop_sort(diff_cnt, repeate_times, IR_learn.bit_counter_set);

    IR_learn.bit_data = 0;
    IR_learn.bit_number = 0;

    bit_index = 0;

    for (i = 2; i < wave_form_cnt; i++)
    {
      for (j = 0; j < diff_cnt; j++)
      {
        if (abs(IR_learn.bit_counter_set[j] - waveform[i]) < 4)
        {
          IR_learn.bit_data |= (code[j] << bit_index);

          IR_learn.bit_number++;
          bit_index += code_length[j];
          break;
        }
      }

      if (j == diff_cnt)
      {
        return 0;
      }
    }

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
    IR_NEC.IR_address = 0;
    IR_NEC.IR_command = 0;

    for(uint8_t i = 0; i < NEC_ADDR_LEN; i++)
    {
        if (abs(waveform[index + 1] - NEC_BIT1_LOW) < LEARNING_PRECISION)
        {
            IR_NEC.IR_address |= (1 << i);
        }

        index += 2;
    }

    uint8_t IR_address_revert = 0;
    for(uint8_t i = 0; i < NEC_ADDR_LEN; i++)
    {
        if (abs(waveform[index + 1] - NEC_BIT1_LOW) < LEARNING_PRECISION)
        {
            IR_address_revert |= (1 << i);
        }

        index += 2;
    }

    if (IR_address_revert != IR_NEC.IR_address)
    {
        return 0;
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

#define RC5_TOGGLE_LEN 9
#define RC5_DATA_LEN 9
#define RC5_ADDR_LEN 5
#define RC5_CMD_LEN 6
uint8_t IR_encode_RC5(uint8_t waveform[255], uint8_t wave_form_cnt, IR_RC5_t &IR_RC5)
{
    uint8_t step_buf[256];
    uint8_t step_buf_index = 0;

    (void)wave_form_cnt;
    IR_RC5.IR_address = 0;
    IR_RC5.IR_command = 0;

    for(uint8_t i = 0; i < wave_form_cnt; i++)
    {
        if (i & 1)
        {
            step_buf[step_buf_index++] = 0;

            if (abs(waveform[i] - 2 * RC5_DATA_LEN) < LEARNING_PRECISION)
            {
                step_buf[step_buf_index++] = 0;
            }
        }
        else
        {
            step_buf[step_buf_index++] = 1;

            if (abs(waveform[i] - 2 * RC5_DATA_LEN) < LEARNING_PRECISION)
            {
                step_buf[step_buf_index++] = 1;
            }
        }
    }

    step_buf_index = 5;

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
#define RC6_TOGGLE_LEN 9
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

            if (abs(waveform[i] - 2 * RC6_DATA_LEN) < LEARNING_PRECISION)
            {
                step_buf[step_buf_index++] = 0;
            }
            else if (abs(waveform[i] - 3 * RC6_DATA_LEN) < LEARNING_PRECISION)
            {
                step_buf[step_buf_index++] = 0;
            }
        }
        else
        {
            step_buf[step_buf_index++] = 1;

            if (abs(waveform[i] - 2 * RC6_DATA_LEN) < LEARNING_PRECISION)
            {
                step_buf[step_buf_index++] = 1;
            }
            else if (abs(waveform[i] - 3 * RC6_DATA_LEN) < LEARNING_PRECISION)
            {
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
void IR_encode(uint8_t waveform[], uint8_t wave_form_cnt)
{
    qDebug() << "IR_encode,wave_form_cnt= " << wave_form_cnt;
    IR_learning_item.is_valid = 1;

    if(abs(waveform[0] -  SIRCS_START_BIT_HIGH) < 8 && abs(waveform[1] -  SIRCS_START_BIT_LOW) < LEARNING_PRECISION &&
          (wave_form_cnt == 25 || wave_form_cnt == 31 || wave_form_cnt == 39))
    {
        qDebug() << "IR_TYPE_SIRCS";
        IR_learning_item.IR_type = IR_TYPE_SIRCS;
        IR_encode_SIRCS(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_SIRCS);
    }
    else if (abs(waveform[0] -  NEC_START_BIT_HIGH) < LEARNING_PRECISION && abs(waveform[1] -  NEC_START_BIT_LOW) < LEARNING_PRECISION &&
             wave_form_cnt == 65)
    {
        qDebug() << "IR_TYPE_NEC";
        IR_learning_item.IR_type = IR_TYPE_NEC;
        IR_encode_NEC(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_NEC);
    }
    else if (abs(waveform[0] -  RC6_START_BIT_HIGH) < LEARNING_PRECISION && abs(waveform[1] -  RC6_START_BIT_LOW) < LEARNING_PRECISION /*&&
             wave_form_cnt == 25*/)
    {
        qDebug() << "IR_TYPE_RC6";
        IR_learning_item.IR_type = IR_TYPE_RC6;
        IR_encode_RC6(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_RC6);
    }
    else if (abs(waveform[0] -  RC5_START_BIT_HIGH) < LEARNING_PRECISION && abs(waveform[1] -  RC5_START_BIT_LOW) < LEARNING_PRECISION /*&&
             wave_form_cnt == 25*/)
    {
        qDebug() << "IR_TYPE_RC5";
        IR_learning_item.IR_type = IR_TYPE_RC5;
        IR_encode_RC5(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_RC5);

    }
    else if (abs(waveform[0] -  JVC_START_BIT_HIGH) < LEARNING_PRECISION && abs(waveform[1] -  JVC_START_BIT_LOW) < LEARNING_PRECISION /*&&
             wave_form_cnt == 33*/)
    {
        qDebug() << "IR_TYPE_JVC";
        IR_learning_item.IR_type = IR_TYPE_JVC;
        IR_encode_JVC(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_JVC);
    }
    else
    {
        qDebug() << "IR_TYPE_LEARNING";
        IR_learning_item.IR_type = IR_TYPE_LEARNING;
        IR_encode_learning(waveform, wave_form_cnt, IR_learning_item.IR_CMD.IR_learning);
    }

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
    qDebug() << "IRLearningItem2String  .IR_type: " <<IR_learning_item.IR_type;

    //uint8_t m_pData[m_nLength];
    //memcpy(m_pData, pData, m_nLength);

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
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_NEC.IR_address)).append("-");
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
            m_tmp.append(QString::number(IR_learning_item.IR_CMD.IR_learning.bit_data,16)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_learning.bit_number)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_learning.header_high)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_learning.header_low)).append("-");
            m_tmp.append(hexByte2String(IR_learning_item.IR_CMD.IR_learning.unused));
            for(int i =0;i<4;i++)
            {
                m_tmp.append("-").append(hexByte2String(IR_learning_item.IR_CMD.IR_learning.bit_counter_set[i]));
            }
            break;
        default:
            break;
    }
    m_result.append(m_tmp);
    qDebug() << "m_reslut: " <<m_result;
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
        qDebug("len=%d,dst[%d] = 0x%.2x ",len,i,list1.at(i).toInt(&ok,16));  //for debug
    }

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
            if(len == 3)
            {
                learningItem->IR_CMD.IR_NEC.IR_address = list1.at(1).toInt(&ok,16);
                learningItem->IR_CMD.IR_NEC.IR_command = list1.at(2).toInt(&ok,16);
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
                learningItem->IR_CMD.IR_learning.bit_data = list1.at(1).toInt(&ok,16);
                learningItem->IR_CMD.IR_learning.bit_number = list1.at(2).toInt(&ok,16);
                learningItem->IR_CMD.IR_learning.header_high = list1.at(3).toInt(&ok,16);
                learningItem->IR_CMD.IR_learning.header_low= list1.at(4).toInt(&ok,16);
                learningItem->IR_CMD.IR_learning.unused= list1.at(5).toInt(&ok,16);
                for(int i =0;i<4;i++)
                {
                   learningItem->IR_CMD.IR_learning.bit_counter_set[i]= list1.at(6+i).toInt(&ok,16);
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
