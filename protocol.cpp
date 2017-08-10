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

