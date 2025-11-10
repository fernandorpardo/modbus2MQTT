/** ************************************************************************************************
 *	MODBUS common functions
 *  (c) Fernando R (iambobot.com)
 *
 * 	1.0.0 - June 2025 - created
 *
 ** ************************************************************************************************
**/

#include <stdio.h>		// uint16_t 
#include "modbus.h"

// Función que calcula el CRC16 Modbus
// CRC16 Modbus RTU (usando el polinomio 0xA001)
// chatGPT code
uint16_t CRC16(const uint8_t *data, uint16_t longitud) 
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < longitud; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}// CRC16

float record2float (uint8_t* data)
{
	uint16_t DataHigh= data[0]<<8 | data[1];
	uint16_t DataLow=  data[2]<<8 | data[3];					
	uint16_t value[2];
	value[1]= DataHigh;
	value[0]= DataLow;
	return *(float *) value;	
} // record2float



// END OF FILE