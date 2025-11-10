/** ************************************************************************************************
 *	DDSU666H 
 *  (c) Fernando R (iambobot.com)
 * 	1.0.0 - June 2025 - created
 *
 ** ************************************************************************************************
**/

#include <stdio.h>
#include <string.h>			// memset

#include "config.h"
#include "modbus.h"
#include "DDSU666H.h"

DDSU666H_data_type DDSU666H_data;

/*
SAMPLE DATA sniffed used for development

0B 03 20 06 00 02 2F 60 
0B 03 04 3E 66 9A D4 D7 3B

0B 03 20 00 00 22 CE B9 
0B 03 44 43 67 80 00 3F C3 95 81 00 00 00 00 3E 7A 0F 91 3E 7A 0F 91 00 00 00 00 BE 83 2C A5 BE 83 2C A5 00 00 00 00 3E B5 32 61 3E B5 32 61 00 00 00 00 BF 30 A3 D7 BF 30 A3 D7 BF 30 A3 D7 00 00 00 00 42 47 E1 48 43 BD 0B 03 20 06 00 02 2F 60 0B 03 04 3E 71 8F C5 A9 A3

0B 03 40 00 00 20 51 78 
0B 03 40 C4 05 5A E1 C4 05 5A E1 00 00 00 00 00 00 00 00 00 00 00 00 44 17 3A 3D 44 17 3A 3D 00 00 00 00 00 00 00 00 00 00 00 00 44 8E 4A 8F 44 8E 4A 8F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 35 95
*/

void DDSU666H_data_init(void)
{
	memset(&DDSU666H_data, 0, sizeof(struct DDSU666H_data_s));
}

void DDSU666H_printf(void)
{
	fprintf(stdout, "\nGrid (DDSU666H)");
	fprintf(stdout, "\nVoltage                %3.2f Volts", DDSU666H_data.Voltage);
	fprintf(stdout, "\nCurrent                %3.2f Amps", 	DDSU666H_data.Current);
	fprintf(stdout, "\nActivePower            %3.2f Watts", DDSU666H_data.ActivePower);
	fprintf(stdout, "\nReactivePower          %3.2f Var",	DDSU666H_data.ReactivePower);
	fprintf(stdout, "\nPowerFactor            %3.2f", 	   	DDSU666H_data.PowerFactor);
	fprintf(stdout, "\nActiveInElectricity    %3.2f", 		DDSU666H_data.ActiveInElectricity);
	fprintf(stdout, "\nNegativeActiveEnergy   %3.2f kWh",	DDSU666H_data.NegativeActiveEnergy);
	fprintf(stdout, "\nPositiveActiveEnergy   %3.2f kWh", 	DDSU666H_data.PositiveActiveEnergy);
	fprintf(stdout, "\n");
}

char *DDSU666H_generate_json(char *json, size_t max_sz)
{
	snprintf(json, max_sz, "{\"DDSU666H\":{\"v\":\"%3.2f\",\"c\":\"%3.2f\",\"ap\":\"%3.2f\",\"rp\":\"%3.2f\"}}", 
	DDSU666H_data.Voltage, DDSU666H_data.Current, DDSU666H_data.ActivePower, DDSU666H_data.ReactivePower
	); 
	return json;
} // DDSU666H_generate_json



uint16_t  DDSU666H_reg_request;

int DDSU666H_rxdata_process( uint8_t* data, int rxBytes, int ix0)
{
	int ix= ix0;
	for(; ix<rxBytes && data[ix]!=DDSU666H_MODBUS_ADDRESS; ix++);
	if(ix<rxBytes)
	{
		if(rxBytes - ix >= 8)
		{
			// Function Code
			if(data[ix + 1]== 0x03)
			{
				uint16_t reg= ENDIAN(*(uint16_t*)(data + ix + 2));
				// is a request
				// sent by the INVENTER
				// do nothing
				if((reg>=0x2000 && reg<=0x20FF) || (reg>=0x4000 && reg<=0x400F))
				{
					DDSU666H_reg_request= reg;
					uint16_t ErrorCheck= data[ix + 7]<<8 | data[ix + 6];
					uint16_t crc= CRC16( &data[ix], 8 - 2);
					if(ErrorCheck == crc)
					{
						if(_VERBOSE_ && reg != 0x2006) fprintf(stdout, "\nRequest register= %04X bytes= %04X", reg, ENDIAN(*(uint16_t *)(data+ix+4)));
					}
					return ix + 8;
				}
				// is a response
				// sent by the DDSU666H in response to the INVERTER's request
				// do record data
				else
				{
					uint8_t bytecount= *(data+ix+2);
					if(ix + 3 + bytecount + 2 <=rxBytes)
					{
						uint16_t ErrorCheck= data[ix + 3 + bytecount + 1]<<8 | data[ix + 3 + bytecount];
						uint16_t crc= CRC16( &data[ix], 3 + bytecount);
						if(ErrorCheck == crc)
						{
							if(_VERBOSE_ &&  DDSU666H_reg_request != 0x2006) fprintf(stdout, "\nResponse= %d bytes %d %04X %04X %d", bytecount, bytecount/4, ErrorCheck, crc, rxBytes - (ix + 3 + bytecount + 2) );
							
							float value, v=0, a=0;
							for(int i=0; i<(bytecount/4) && i<32; i++)
							{
								value= record2float( ix + data + 3 + 4*i);
								if( DDSU666H_reg_request == 0x2000)
								{
									uint16_t reg= 0x2000 + 2 * i;
									switch(reg)
									{
										case DDSU666H_REG_VOLTAGE: 			DDSU666H_data.Voltage= v= value; break;
										case DDSU666H_REG_CURRENT: 			DDSU666H_data.Current= a= value; break;
										case DDSU666H_REG_ACTIVE_POWER: 	DDSU666H_data.ActivePower= value * 1000.0; break;
										case DDSU666H_REG_REACTIVE_POWER: 	DDSU666H_data.ReactivePower= value * 1000.0; break;
										case DDSU666H_REG_POWER_FACTOR: 	DDSU666H_data.PowerFactor= value * 1000.0; break;
									}
									if(_VERBOSE_) fprintf(stdout, "\n0x%4X= %3.2f", reg, value); 
								}
								else if( DDSU666H_reg_request == 0x4000) 
								{
									uint16_t reg= 0x4000 + 2 * i;
									switch(reg)
									{
										case DDSU666H_REG_ACTIVE_IN_ELECTRICITY: DDSU666H_data.ActiveInElectricity= value; break;
										case DDSU666H_REG_NEGATIVE_ACTIVE_ENERGY: DDSU666H_data.NegativeActiveEnergy= value; break;
										case DDSU666H_REG_POSITIVE_ACTIVE_ENERGY: DDSU666H_data.PositiveActiveEnergy= value; break;
									}
									if(_VERBOSE_) fprintf(stdout, "\n0x%4X= %3.2f", reg, value);
								}
							}
//							if(_VERBOSE_ &&  DDSU666H_reg_request != 0x2006) fprintf(stdout, "\nRegister %04X",  DDSU666H_reg_request);
							return (ix + 3 + bytecount + 2);
						}
					}				
				}
			}
		}
	}
	return 0;
} // DDSU666H_rxdata_process


// END OF FILE