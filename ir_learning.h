#ifndef IR_LEARNING_H
#define IR_LEARNING_H

#include <QDialog>
#include "protocol.h"

class IR_learning
{
public:
    IR_learning();
};

extern IR_item_t IR_learning_item;
uint8_t IR_encode(uint8_t waveform[255], uint8_t wave_form_cnt);
QString IRLearningItem2String(int nLength);
bool String2IRLearningItem(QString src,IR_item_t *learningItem);

#endif // IR_LEARNING_H
