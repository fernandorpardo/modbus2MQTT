#ifndef _MODBUS_H_
#define _MODBUS_H_

uint16_t CRC16(const uint8_t *data, uint16_t longitud);
float record2float (uint8_t* data);

#endif
// END OF FILE