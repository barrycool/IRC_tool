#ifndef CRC32_H
#define CRC32_H

#include <QDialog>

//uint32_t CRC32Software(uint8_t *pData, uint16_t Length );
uint8_t CRC8Software(uint8_t *pData, uint16_t Length );

class crc32
{
public:
    crc32();
};

#endif // CRC32_H
