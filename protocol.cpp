#include "protocol.h"

protocol::protocol()
{
}
/*
struct IR_SIRCS_t IR_SIRCS_commands[SIRCS_DEV_MAX][SIRCS_CMD_MAX] = {
    //BDP
    {
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x15, .name = "Power"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x16, .name = "Eject"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x42, .name = "Home"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x1A, .name = "Play"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x18, .name = "Stop"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x19, .name = "Pause"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x57, .name = "Prev"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x56, .name = "Next"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x1B, .name = "FastReverse"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x1C, .name = "FastForward"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x2C, .name = "Top"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x29, .name = "Popup"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x39, .name = "Up"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x3A, .name = "Down"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x3B, .name = "Left"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x3C, .name = "Right"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x3D, .name = "Enter"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x43, .name = "Return"},
        {.isLearningKey = 0, .IR_address = 0x1A, .IR_ext_3_address = 0x02, .IR_ext_5_address = 0x1C, .IR_command = 0x3F, .name = "Option"},
    },

    //soundbar
    {
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x01, .IR_ext_5_address = 0x00, .IR_command = 0x15, .name = "Power"},
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x05, .IR_ext_5_address = 0x00, .IR_command = 0x69, .name = "Input"},
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x05, .IR_ext_5_address = 0x00, .IR_command = 0x78, .name = "Up"},
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x05, .IR_ext_5_address = 0x00, .IR_command = 0x79, .name = "Down"},
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x01, .IR_ext_5_address = 0x00, .IR_command = 0x0C, .name = "Enter"},
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x00, .IR_ext_5_address = 0x01, .IR_command = 0x7D, .name = "Back"},
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x00, .IR_ext_5_address = 0x01, .IR_command = 0x3A, .name = "Play"},
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x05, .IR_ext_5_address = 0x00, .IR_command = 0x77, .name = "Menu"},
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x01, .IR_ext_5_address = 0x00, .IR_command = 0x12, .name = "Vol_+"},
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x01, .IR_ext_5_address = 0x00, .IR_command = 0x13, .name = "Vol__"},
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x00, .IR_ext_5_address = 0x01, .IR_command = 0x31, .name = "Next"},
        {.isLearningKey = 0, .IR_address = 0x10, .IR_ext_3_address = 0x00, .IR_ext_5_address = 0x01, .IR_command = 0x30, .name = "Prev"},
    },
};
*/

QList <IR_item_t> IR_items;
QList <IR_map_t> IR_maps;
void saveToIrMaps(QString line)
{
    IR_map_t IR_map;

    QStringList list1 = line.split(',');
    bool ok;
    IR_map.IR_type = list1.at(0).toInt(&ok,16);
    //QByteArray ba = list1.at(1).toLatin1();
    IR_map.name = list1.at(1);// ba.data();
    //qDebug() << "savetoIRmaps: IR_map.name:" << IR_map.name;
    //logstr = "savetoIRmaps: IR_map.name:";
    //logstr.append(IR_map.name);
    //output_log(logstr,0);

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

    //qDebug() << "savetoIRmaps: IR_map.keyValue:" << IR_map.keyValue;
    //logstr = "savetoIRmaps: IR_map.keyValue:";
    //logstr.append(IR_map.keyValue);
    //output_log(logstr,0);

    IR_maps.append(IR_map);

}

void add_to_IR_Items(QString button_name,uint32_t delay)
{
    IR_item_t IR_item;
    IR_item.is_valid = 1;
    IR_item.delay_time = delay;

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
                //output_log("add_to_list fail",0);
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
                /*case IR_TYPE_JVC:
                    memcpy(IR_item.IR_CMD.IR_JVC.name,tmpBuf,MAX_NAME_LEN);
                    break;*/
                case IR_TYPE_LEARNING:
                    memcpy(IR_item.IR_CMD.IR_learning.name,tmpBuf,MAX_NAME_LEN);
                    break;
                default:
                    break;
            }
        }
    }
    IR_items.append(IR_item);
    //printIrItemInfo(IR_item);
}
